//=====================================================================================================================
//
//   TRTGridTraversal.h
//
//   Uniform grid traversal
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Structure which maintains the state of a DDA through a 3D grid
    ///
    /// The template parameters control how many bits are used for cell indices.  Unsigned_T and Signed_T must
    ///  be the corresponding unsigned and signed integer types for a particular bit width
    //=====================================================================================================================
    template< typename Unsigned_T, typename Signed_T >
    struct DDAState
    {
        Vec3<Unsigned_T> vCellIndices;      ///< Current position in the grid
        Vec3<Unsigned_T> vCellCounts;       ///< Number of cells in the grid
        Vec3<Signed_T>  vStepSigns;         ///< Directions to step in (1 if positive, -1 if negative)
        Vec3f        vTNext;                ///< Distance to next cell boundary
        Vec3f        vDeltaT;               ///< Change in T per single-cell step in X,Y,Z
    };

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Sets up a DDA through a 3D grid.  
    ///
    /// \param rRay     The ray to be traversed through the grid
    /// \param pGrid    A grid through which the ray is to be traversed
    /// \param rState   DDA state structure which is initialized for traversal of the ray through the grid
    /// 
    /// \param Ray_T         Must implement the Ray_C concept
    /// \param UniformGrid_T Must implement the UniformGrid_C concept
    /// \param DDA_T         Must be an instance of TinyRT::DDAState
    //=====================================================================================================================
    template< typename Ray_T,
              typename UniformGrid_T,
              typename DDA_T >
    TRT_FORCEINLINE bool DDAInit( const Ray_T& rRay, const UniformGrid_T* pGrid, DDA_T& rState )
    {
        const AxisAlignedBox& rBox = pGrid->GetBoundingBox();

        float fTMin;
        if( !RayAABBTest( rBox.Min(), rBox.Max(), rRay, fTMin ) )
            return false;

        fTMin = Max( fTMin, rRay.MinDistance() );
        rState.vCellCounts = pGrid->GetCellCounts();
        Vec3f vBoxSize = rBox.Max() - rBox.Min();
        Vec3f vStartPos = rRay.Origin() + (rRay.Direction()*fTMin); // location at which ray begins traversal through grid
        
        for( int i=0; i<3; i++ )
        {
            float fInvDir = rRay.InvDirection()[i];
            rState.vDeltaT[i] = fabs( fInvDir ) * ( vBoxSize[i] / rState.vCellCounts[i] );
       
            // compute start position expressed in fractional cell coordinates
            //  (0-N, where N is the number of cells.  A value of N only occurs at the upper bound of the grid)
            float fCellCount = (float) rState.vCellCounts[i];
            float fCellCoord = ((vStartPos[i] - rBox.Min()[i]) / vBoxSize[i]) * fCellCount;
            
            fCellCoord = Max( 0.0f, fCellCoord );
            fCellCoord = Min( fCellCoord, fCellCount );

            // compute index of initial cell
            float fCellIndex = floor( Min( fCellCoord, fCellCount-1 ) );
            rState.vCellIndices[i] = static_cast<uint32>( fCellIndex );

            float fCellFrac = fCellCoord - fCellIndex; // fractional position within our initial cell (0-1)
         
            // compute distance to next cell boundary on this axis
            if( fInvDir > 0 )
            {
                rState.vTNext[i] = fTMin + ((1.0f-fCellFrac) * rState.vDeltaT[i]);
                rState.vStepSigns[i] = 1;
            }
            else
            {
                rState.vTNext[i] = fTMin + (fCellFrac * rState.vDeltaT[i]);
                rState.vStepSigns[i] = -1;
            }

        }

        return true;
    }

    
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Steps to the next cell in a uniform grid
    /// \param rState    A DDA state structure built by 'TinyRT::DDAInit'
    /// \param rRay      The ray used in the call to 'TinyRT::DDAInit'
    /// \return True if the ray continued to the next cell.  False if the ray has left the grid, or if the 
    ///             next cell is beyond the ray's valid disatance
    ///
    /// \param DDA_T            An instance of the TinyRT::DDAState template
    /// \param Ray_T            Must implement the Ray_C concept
    //=====================================================================================================================
    template< typename DDA_T, typename Ray_T >
    TRT_FORCEINLINE bool DDAStep( DDA_T& rState, const Ray_T& rRay )
    {
        if( rState.vTNext[1] < rState.vTNext[0] )
        {
            if( rState.vTNext[2] < rState.vTNext[1] )
            { 
                if( rRay.MaxDistance() < rState.vTNext[2] )
                    return false;

                // step in Z
                rState.vTNext[2] += rState.vDeltaT[2];
                rState.vCellIndices[2] += rState.vStepSigns[2];

                // if stepping negatively, overflow will make us stop...
                return rState.vCellIndices[2] < rState.vCellCounts[2];
            }
            else
            {
                if( rRay.MaxDistance() < rState.vTNext[1] )
                    return false;

                // step in Y
                rState.vTNext[1] += rState.vDeltaT[1];
                rState.vCellIndices[1] += rState.vStepSigns[1];

                // if stepping negatively, overflow will make us stop...
                return rState.vCellIndices[1] < rState.vCellCounts[1];
            }
        }
        else
        {
            if( rState.vTNext[2] < rState.vTNext[0] )
            {
                if( rRay.MaxDistance() < rState.vTNext[2] )
                    return false;

                // step in Z
                rState.vTNext[2] += rState.vDeltaT[2];
                rState.vCellIndices[2] += rState.vStepSigns[2];

                // if stepping negatively, overflow will make us stop...
                return rState.vCellIndices[2] < rState.vCellCounts[2];
            }
            else
            {
                if( rRay.MaxDistance() < rState.vTNext[0] )
                    return false;
                
                // step in X
                rState.vTNext[0] += rState.vDeltaT[0];
                rState.vCellIndices[0] += rState.vStepSigns[0];

                // if stepping negatively, overflow will make us stop...
                return  rState.vCellIndices[0] < rState.vCellCounts[0];
            }
        }
    }


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Searches for the first intersection between a ray and an object in a uniform grid
    /// \param pGrid    The grid to be traversed
    /// \param pObjects The object set used to create the grid (or an equivalent one)
    /// \param rHitInfo Receives intersection information
    ///
    /// \param Mailbox_T        Must implement the Mailbox_C concept
    /// \param UniformGrid_T    Must implement the UniformGrid_C concept
    /// \param ObjectSet_T      Must implement the ObjectSet_C concept
    /// \param HitInfo_T        Must implement the HitInfo_C concept
    /// \param Ray_T            Must implement the Ray_C concept
    //=====================================================================================================================
    template< 
        typename Mailbox_T,
        typename UniformGrid_T,
        typename ObjectSet_T,
        typename HitInfo_T,
        typename Ray_T
    >
    void RaycastUniformGrid( const UniformGrid_T* pGrid, const ObjectSet_T* pObjects, Ray_T& rRay, HitInfo_T& rHitInfo )
    {
        Mailbox_T mailbox(pObjects);
        const ObjectSet_T& rObjects = *pObjects;
           
        // DDA setup
        DDAState<typename UniformGrid_T::UnsignedCellIndex,
                 typename UniformGrid_T::SignedCellIndex > ddaState;
        if( !DDAInit( rRay, pGrid, ddaState ) )
            return; // missed grid completely

        while(1)
        {
            // Does this cell have objects?  If it does, intersect them
            typename UniformGrid_T::CellIterator itBegin, itEnd;
            pGrid->GetCellObjectList( ddaState.vCellIndices, itBegin, itEnd );

            while( itBegin != itEnd )
            {
                typename UniformGrid_T::obj_id nObject = *itBegin;
                if( !mailbox.CheckMailbox( nObject ) )
                    rObjects.RayIntersect( rRay, rHitInfo, nObject ); 
                
                ++itBegin;
            }
           
            // advance to next cell
            if( !DDAStep( ddaState, rRay ) )
                return; // fell out of grid, or traversed past ray depth limit.  In either case, bail out
         }
    }

}
