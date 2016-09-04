//=====================================================================================================================
//
//   TRTCostMetric.h
//
//   Utilities for computing raytracing cost estimates on various data structures
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_COSTMETRIC_H_
#define _TRT_COSTMETRIC_H_


namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Cost function which returns a constant cost per object
    /// This class implements the CostFunction_C concept.
    //=====================================================================================================================
    template< class obj_id >
    class ConstantCost
    {
    public:

        inline ConstantCost( float c ) : m_fConstantCost(c) {};

        inline float operator()( obj_id i ) const { return m_fConstantCost; };

    private:
        float m_fConstantCost;
    };


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Calculates the SAH cost of an AABB tree, using a cost functor
    /// \param rCostFunc        Function giving the cost of an intersection test, relative to the cost of a BVH node visit
    /// \param pTree            The AABB tree whose cost is desired
    /// \param pNode            The node whose subtree cost is being calculated
    /// \param AABBTree_T       Must implement the AABBTree_C concept
    /// \param CostFunction_T   Must implement the CostFunction_C concept
    //=====================================================================================================================
    template< typename AABBTree_T, typename CostFunction_T >
    float GetAABBTreeSAHCost( const CostFunction_T& rCostFunc, const AABBTree_T* pTree, const typename AABBTree_T::ConstNodeHandle pNode )
    {
        if( pTree->IsNodeLeaf( pNode ) )
        {
            typename AABBTree_T::obj_id nFirst, nLast;
            pTree->GetNodeObjectRange( pNode, nFirst, nLast );
            
            float fCost = 0;
            while( nFirst != nLast )
            {
                fCost += rCostFunc(nFirst);
                nFirst++;
            }

            return fCost;
        }
        else
        {
            AxisAlignedBox box = pTree->GetNodeBoundingVolume( pNode );
            Vec3f vSize = box.Max() - box.Min();
            float fArea = vSize.x*( vSize.y + vSize.z ) + vSize.y*vSize.z;

            typedef typename AABBTree_T::ConstNodeHandle NodeHandle;
            NodeHandle pLeft = pTree->GetLeftChild( pNode );
            NodeHandle pRight = pTree->GetRightChild( pNode );
            AxisAlignedBox leftBox  = pTree->GetNodeBoundingVolume( pLeft );
            AxisAlignedBox rightBox = pTree->GetNodeBoundingVolume( pRight );
            Vec3f vLeftSize = leftBox.Max() - leftBox.Min();
            Vec3f vRightSize = rightBox.Max() - rightBox.Min();
            float fLeftArea  = vLeftSize.x*( vLeftSize.y + vLeftSize.z ) + vLeftSize.y*vLeftSize.z;
            float fRightArea = vRightSize.x*( vRightSize.y + vRightSize.z ) + vRightSize.y*vRightSize.z;

            float fLeftCost  = GetAABBTreeSAHCost( rCostFunc, pTree, pLeft );
            float fRightCost = GetAABBTreeSAHCost( rCostFunc, pTree, pRight );
            return 2.0f + (( fLeftArea*fLeftCost + fRightArea*fRightCost ) / (fArea+0.000001f));
        }
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Calculates the SAH cost of an AABB tree, assuming a fixed cost per object
    /// \param fFixedCost   The cost of an intersection test, relative to the cost of a BVH node visit
    /// \param pTree        The AABB tree whose cost is desired
    /// \param pNode        The node whose subtree cost is being calculated
    /// \param AABBTree_T Must implement the AABBTree_C concept
    //=====================================================================================================================
    template< typename AABBTree_T >
    float GetAABBTreeSAHCost( float fFixedCost, const AABBTree_T* pTree, const typename AABBTree_T::ConstNodeHandle pNode )
    {
        return GetAABBTreeSAHCost( ConstantCost<typename AABBTree_T::obj_id>(fFixedCost), pTree, pNode );
    }


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Calculates the SAH cost of a KD tree, using a cost functor
    /// \param rCostFunc   Function giving the cost of an object intersection test, relative to a KD traversal operation
    /// \param pTree        The tree whose cost is to be computed
    /// \param pNode        Root of the subtree whose cost is to be computed
    /// \param rRootBox     AABB of the subtree whose cost is to be computed
    /// \param KDTree_T Must implement the KDTree_C concept
    /// \param CostFunction_T Must implement the CostFunction_C concept
    //=====================================================================================================================
    template< typename KDTree_T, typename CostFunction_T >
    float GetKDTreeSAHCost( const CostFunction_T& rCostFunc, const KDTree_T* pTree, const typename KDTree_T::ConstNodeHandle pNode, const AxisAlignedBox& rRootBox )
    {
        if( pTree->IsNodeLeaf( pNode ) )
        {
            typename KDTree_T::LeafIterator itStart;
            typename KDTree_T::LeafIterator itEnd;
            pTree->GetNodeObjectList( pNode, itStart, itEnd );

            float fCost = 0;
            while( itStart != itEnd )
            {
                fCost += rCostFunc( *itStart );
                ++itStart;
            }

            return fCost;
        }
        else
        {
            uint nAxis = pTree->GetNodeSplitAxis( pNode );
            float fPlanePos = pTree->GetNodeSplitPosition( pNode );

            AxisAlignedBox leftBox, rightBox;
            rRootBox.Cut( nAxis, fPlanePos, leftBox, rightBox );
            
            // surface area of a box is:  2xy + 2yz + 2xz
            //  If we factor out the dimension that is changing (X, say), we have:
            //    2X(Y+Z) + 2YZ.  
            // (Or, 2X*B + 2A).  Note that the factors of two cancel out, and we can pre-multiply by the other stuff
            uint nY = (nAxis+1) % 3;
            uint nZ = (nAxis+2) % 3;
            Vec3f vBBSize = rRootBox.Max() - rRootBox.Min();
            float fAlpha = ( vBBSize[nY] * vBBSize[nZ] ) ;
            float fBeta  = ( vBBSize[nY] + vBBSize[nZ] ) ;
            float fInvRootArea = 1.0f / (fAlpha + vBBSize[nAxis]*fBeta);
            
            float fXL = fPlanePos - rRootBox.Min()[nAxis];
            float fXR = rRootBox.Max()[nAxis] - fPlanePos;
            float fPL = ( fXL*fBeta + fAlpha )*fInvRootArea;
            float fPR = ( fXR*fBeta + fAlpha )*fInvRootArea;

            return 1.0f + fPL * GetKDTreeSAHCost( rCostFunc, pTree, pTree->GetLeftChild( pNode ), leftBox ) 
                        + fPR * GetKDTreeSAHCost( rCostFunc, pTree, pTree->GetRightChild( pNode ), rightBox );
        }
    }


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Calculates the SAH cost of a KD tree, assuming a fixed cost per object
    /// \param fFixedCost   The cost of an object intersection test, relative to a KD traversal operation
    /// \param pTree        The tree whose cost is to be computed
    /// \param pNode        Root of the subtree whose cost is to be computed
    /// \param rRootBox     AABB of the subtree whose cost is to be computed
    /// \param KDTree_T Must implement the KDTree_C concept
    //=====================================================================================================================
    template< typename KDTree_T >
    float GetKDTreeSAHCost( float fFixedCost, const KDTree_T* pTree, const typename KDTree_T::ConstNodeHandle pNode, const AxisAlignedBox& rRootBox )
    {
        return GetKDTreeSAHCost( ConstantCost<typename KDTree_T::obj_id>( fFixedCost ), pTree, pNode, rRootBox );
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Calculates the SAH cost of a uniform grid, assuming a fixed cost per object
    /// \param fFixedCost   The cost of an object intersection test, relative to a grid graversal operation
    /// \param pGrid        The grid whose cost is to be computed
    /// \param UniformGrid_T Must implement the UniformGrid_C concept
    //=====================================================================================================================
    template< typename UniformGrid_T >
    float GetUniformGridSAHCost( float fFixedCost, const UniformGrid_T* pGrid )
    {
        const Vec3<uint32>& rCellCounts = pGrid->GetCellCounts();
        
        // compute the probability of hitting any particular grid cell, given that we hit the root
        Vec3f vBoxSizes = pGrid->GetBoundingBox().Max() - pGrid->GetBoundingBox().Min();
        float fRootArea = ( vBoxSizes.x * ( vBoxSizes.y + vBoxSizes.z ) + vBoxSizes.y*vBoxSizes.z );
        vBoxSizes.x /= rCellCounts.x;
        vBoxSizes.y /= rCellCounts.y;
        vBoxSizes.z /= rCellCounts.z;
        float fPCell = ( vBoxSizes.x * ( vBoxSizes.y + vBoxSizes.z ) + vBoxSizes.y*vBoxSizes.z ) / fRootArea;
        
        // the cost metric for a grid is sum( PCell*(object_cost + cell_cost) )
        //  We define the object cost to be relative to the cell cost, and we can factor out the term 'PCell'
        float fCost = 0;
        for( uint32 x=0; x<rCellCounts.x; x++ )
        {
            for( uint32 y=0; y<rCellCounts.y; y++ )
            {
                for( uint32 z=0; z < rCellCounts.z; z++ )
                {
                    size_t nObjects = pGrid->GetCellObjectCount( Vec3<uint32>(x,y,z) );
                    fCost += 1.0f + fFixedCost*nObjects;
                }
            }
        }

        return fCost * fPCell;
    }

}

#endif // _TRT_COSTMETRIC_H_
