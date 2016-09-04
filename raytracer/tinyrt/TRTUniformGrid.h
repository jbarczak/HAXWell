//=====================================================================================================================
//
//   TRTUniformGrid.h
//
//   Definition of class: TinyRT::UniformGrid
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_UNIFORMGRID_H_
#define _TRT_UNIFORMGRID_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A uniform grid data structure
    ///
    ///  This class implements the UniformGrid_C concept
    ///
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    //=====================================================================================================================
    template< class ObjectSet_T >
    class UniformGrid
    {
    public:

        typedef typename ObjectSet_T::obj_id obj_count;
        typedef typename ObjectSet_T::obj_id obj_id;
        typedef const obj_id* CellIterator;

        typedef uint32 UnsignedCellIndex;
        typedef int32 SignedCellIndex;

        /// Returns the bounding box of the grid
        inline const AxisAlignedBox& GetBoundingBox() const { return m_boundingBox; };

        /// Returns the number of grid cells along each axis
        inline const Vec3<uint32>& GetCellCounts() const { return m_cellCounts; };

        /// Returns the number of objects in a cell 
        inline size_t GetCellObjectCount( const Vec3<uint32>& rCell ) const { 
            const size_t* pCell = &m_cellOffsets[AddressCell(rCell)];        
            return pCell[1] - pCell[0]; 
        };


        /// Retrieves iterators for the list of objects in a particular cell
        inline void GetCellObjectList( const Vec3<uint32>& rCell, CellIterator& hCellStart, CellIterator& hCellEnd ) const { 
        
            // check the bit array first, before accessing the cell offset table
            // In big grids, most cells tend to be empty, so this results in less cache pollution
            size_t nCellAddr = AddressCell(rCell);
            if( m_cellMasks[nCellAddr/8] & (1<< (nCellAddr%8)) )
            {
                const size_t* pCellOffs = &m_cellOffsets[nCellAddr];
                hCellStart = m_objectList + pCellOffs[0];
                hCellEnd = m_objectList + pCellOffs[1];
                return;
            }
        
            hCellStart = 0;
            hCellEnd = 0;
        };

        /// Rebuilds the grid from an object set
        void Build( ObjectSet_T* pObjects, float fLambda );
        
    private:


        /// Computes the address of a cell given its 3D cell coordinates
        inline size_t AddressCell( const Vec3<uint32>& rCell ) const { 
            TRT_ASSERT( rCell.x < m_cellCounts.x && rCell.y < m_cellCounts.y && rCell.z < m_cellCounts.z );
            size_t nCell = rCell.z*m_nWH + rCell.y*m_cellCounts.x + rCell.x; 
            return nCell;
        };

        // The grid is represented by storing each cell's offset into the object list.  The object count
        //  for cell i is offset[i+1] - offset[i].  The cell offset array contains an extra (bogus) entry to avoid
        //  the need to check boundary conditions

        ScopedArray< size_t > m_cellOffsets;   ///< Offset of cell i's object references in the object list.  
        ScopedArray< obj_id > m_objectList;    ///< Global list of object references

        ScopedArray< uint8 > m_cellMasks;   ///< Bit array indicating, for each cell, whether it actually has objects in it

        AxisAlignedBox m_boundingBox;

        Vec3<uint32> m_cellCounts;  ///< Number of cells along each axis
        uint32 m_nWH;               ///< cellCounts.x*cellCounts.y
    };

}

#include "TRTUniformGrid.inl"

#endif // _TRT_UNIFORMGRID_H_
