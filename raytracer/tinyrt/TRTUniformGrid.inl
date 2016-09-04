//=====================================================================================================================
//
//   TRTUniformGrid.inl
//
//   Implementation of class: TinyRT::UniformGrid
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include "TRTUniformGrid.h"

namespace TinyRT
{

    //=====================================================================================================================
    //
    //         Constructors/Destructors
    //
    //=====================================================================================================================

   

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// \param pObjects The object set for this grid
    /// \param fLambda  Parameter that loosely controls the number of objects per cell.  
    ///                   Lower values mean more objects per cell
    //=====================================================================================================================
    template< class ObjectSet_T >
    void UniformGrid<ObjectSet_T>::Build( ObjectSet_T* pObjects, float fLambda )
    {
        
        // get the global bounding box of the object set
        pObjects->GetAABB( m_boundingBox );

        obj_id nObjects = pObjects->GetObjectCount();
        Vec3f vDiagonal = Normalize3( m_boundingBox.Max() - m_boundingBox.Min() );

        // choose the cell count to be proportional to the number of objects and the box dimensions, 
        // as described in the coherent grid paper by Wald et al. (Siggraph '06)
        
        float fTargetCells = pow( nObjects*fLambda, 0.3333333f );
        m_cellCounts.x  = static_cast<uint32>( Max( fTargetCells*vDiagonal.x, 1.0f ) );
        m_cellCounts.y = static_cast<uint32>( Max( fTargetCells*vDiagonal.y, 1.0f ) );
        m_cellCounts.z = static_cast<uint32>( Max( fTargetCells*vDiagonal.z, 1.0f ) );

        // force cell count to be a multiple of 8 (force multiples of 2 in each dimension)
        m_cellCounts.x += m_cellCounts.x & 1;
        m_cellCounts.y += m_cellCounts.y & 1;
        m_cellCounts.z += m_cellCounts.z & 1;
        
        m_nWH = m_cellCounts.x*m_cellCounts.y;

        // ----------------------------------------------------------------------------------------------------------------
        // step one.  For each object, collect a list of cell references, count the number of objects referencing each cell
        // ----------------------------------------------------------------------------------------------------------------

        uint32 nCells = m_cellCounts.x*m_cellCounts.y*m_cellCounts.z;

        ScopedArray< size_t > objectRefCounts( new size_t[nObjects] );  // number of cell references for each object
        ScopedArray< size_t > cellObjectCounts( new size_t[nCells] ); // number of object references for each cell
        std::vector< size_t > cellRefs;         // array of cell references, sorted by the object that is referencing the cells
        
        memset( objectRefCounts, 0, sizeof(size_t)*nObjects );
        memset( cellObjectCounts, 0, sizeof(size_t)*nCells );

        Vec3f vCellCounts = Vec3f( (float) m_cellCounts.x, (float) m_cellCounts.y, (float) m_cellCounts.z );
        Vec3f vScaleFactor = vCellCounts / ( m_boundingBox.Max() - m_boundingBox.Min() );

        for( obj_id i=0; i<nObjects; i++ )
        {
            AxisAlignedBox objectBB;
            pObjects->GetObjectAABB( i, objectBB );

            // convert bbox into cell coordinates
            Vec3f vBoxMin = (objectBB.Min() - m_boundingBox.Min()) * vScaleFactor;
            Vec3f vBoxMax = (objectBB.Max() - m_boundingBox.Min()) * vScaleFactor;

            Vec3<uint32> vMinInt = Vec3<uint32>( (uint32) vBoxMin.x, (uint32) vBoxMin.y, (uint32) vBoxMin.z );
            Vec3<uint32> vMaxInt = Vec3<uint32>( (uint32) vBoxMax.x, (uint32) vBoxMax.y, (uint32) vBoxMax.z );
            vMinInt = Min3( vMinInt, m_cellCounts - Vec3<uint32>(1,1,1) );
            vMaxInt = Min3( vMaxInt, m_cellCounts - Vec3<uint32>(1,1,1) );

            objectRefCounts[i] = (1 + vMaxInt.x - vMinInt.x ) * ( 1 + vMaxInt.y - vMinInt.y) * (1 + vMaxInt.z - vMinInt.z);
            TRT_ASSERT( objectRefCounts[i] > 0 );

            // insert object into each cell it touches
            Vec3<uint32> cellIndices;
            for( cellIndices.x = vMinInt.x; cellIndices.x <= vMaxInt.x; cellIndices.x++ )
            {
                for( cellIndices.y = vMinInt.y; cellIndices.y <= vMaxInt.y; cellIndices.y++ )
                {
                    for( cellIndices.z =vMinInt.z; cellIndices.z <= vMaxInt.z; cellIndices.z++ )
                    {
                        size_t nCell = AddressCell( cellIndices );
                        cellRefs.push_back( nCell );

                        cellObjectCounts[nCell]++;
                    }
                }
            }
        }


        // ----------------------------------------------------------------------------------------------------------------
        // step two.  Compute a prefix sum on the object counts, to get the offsets of each cell in the object reference array
        // ----------------------------------------------------------------------------------------------------------------
    
        // we add a redundant entry to avoid having to check boundary conditions when we compute per-cell object counts
        //  we want to be able to compute the object count in cell i as cellOffsets[i+1] - cellOffsets[i]

        size_t nCellOffsets = nCells + 1;
        m_cellOffsets.reallocate( nCellOffsets );
        m_cellOffsets[0] = 0;
        
        for( size_t i=1; i<nCellOffsets; i++ )
        {
            m_cellOffsets[i] = m_cellOffsets[i-1] + cellObjectCounts[i-1];
        }

        TRT_ASSERT( m_cellOffsets[nCellOffsets-1] == cellRefs.size() );
        TRT_ASSERT( nCells % 8 == 0 );

        // build occupied cells mask
        m_cellMasks.reallocate( nCells/8 );
        memset( m_cellMasks, 0, nCells/8 );
        for( size_t i=0; i<nCells; i++ )
        {
            if( cellObjectCounts[i] > 0 )
                m_cellMasks[ i/8 ] |= ( 1 << (i%8) );
        }

        // ----------------------------------------------------------------------------------------------------------------
        // step three.  Sweep the cell references, and insert each object into all cells it appears in
        // ----------------------------------------------------------------------------------------------------------------

        // Allocate the object reference array        
        m_objectList.reallocate( m_cellOffsets[nCellOffsets-1] );
        
        // sweep the cell reference array, object by object, and fill all cells with the objects they contain
        std::vector<size_t>::iterator itRefs = cellRefs.begin(); 
        for( size_t* pObject = objectRefCounts; pObject != objectRefCounts + nObjects; ++pObject )
        {
            obj_id nObjID = static_cast<obj_id>( pObject - objectRefCounts );
            TRT_ASSERT( nObjID < nObjects );

            std::vector<size_t>::iterator itLast = itRefs + (*pObject); // one past last reference for this object
            while( itRefs != itLast )
            {
                size_t nCell = (*itRefs++);
                m_objectList[ m_cellOffsets[nCell] + (--cellObjectCounts[nCell]) ] = nObjID;
            }
        }


    }

    //=====================================================================================================================
    //
    //           Protected Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================
}
