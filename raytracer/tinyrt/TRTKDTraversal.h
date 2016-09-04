//=====================================================================================================================
//
//   TRTKDTraversal.h
//
//   Definition of class: TinyRT::KDTraversal
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTKDTRAVERSAL_H_
#define _TRTKDTRAVERSAL_H_

namespace TinyRT
{
    template< class KDTree_T >
    struct KDStackEntry
    {
        typename KDTree_T::ConstNodeHandle pNode;
        float fTMin;
        float fTMax;
    };

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Searches for the first intersection between a ray and an object in a KD-Tree.  
    /// \param Mailbox_T        Must implement the Mailbox_C concept
    /// \param KDTree_T         Must implement the ObjectKDTree_C concept
    /// \param ObjectSet_T      Must implement the ObjectSet_C concept
    /// \param HitInfo_T        Must implement the HitInfo_C concept
    /// \param Ray_T            Must implement the Ray_C concept
    //=====================================================================================================================
    template< class Mailbox_T, typename KDTree_T, typename ObjectSet_T, typename HitInfo_T, typename Ray_T >
    void RaycastKDTree( const KDTree_T* pTree, const ObjectSet_T* pObjects, Ray_T& rRay, HitInfo_T& rHitInfo, typename KDTree_T::ConstNodeHandle pRoot, ScratchMemory& rScratch )
    {
        Mailbox_T mailbox( pObjects );

        typedef KDStackEntry<KDTree_T> StackEntry;

        ScratchArray<StackEntry> pStackArray( rScratch, pTree->GetStackDepth() );
        StackEntry* pStack = pStackArray;
        StackEntry* pStackBottom = pStack;

        const AxisAlignedBox& rBox = pTree->GetBoundingBox();
        float fTMin, fTMax;
        if( !RayAABBTest( rBox.Min(), rBox.Max(), rRay, fTMin, fTMax ) )
            return;

        float fRayMin = rRay.MinDistance();
        float fRayMax = rRay.MaxDistance();
        if( fTMin < fRayMin )
            fTMin = fRayMin;
        if( fTMax > fRayMax )
            fTMax = fRayMax;

        const Vec3f& rRayOrigin = rRay.Origin();
        const Vec3f& rRayDirectionInv = rRay.InvDirection();

        // precompute the node traversal order on each axis. A 1 bit means go right first.
        uint nNodeOrder;
        nNodeOrder  = rRayDirectionInv[0] > 0 ? 0 : 1;
        nNodeOrder |= rRayDirectionInv[1] > 0 ? 0 : 2;
        nNodeOrder |= rRayDirectionInv[2] > 0 ? 0 : 4;
        

        typename KDTree_T::ConstNodeHandle pNode = pTree->GetRoot();
        while( 1 )
        {
            if( pTree->IsNodeLeaf( pNode ) )
            {
                // intersect all objects in this leaf node, then proceed with next node from stack
                typename KDTree_T::LeafIterator itBegin, itEnd;
                pTree->GetNodeObjectList( pNode, itBegin, itEnd );

                while( itBegin != itEnd )
                {
                    typename KDTree_T::obj_id nObject = *itBegin;
                    if( !mailbox.CheckMailbox( nObject ) )
                        pObjects->RayIntersect( rRay, rHitInfo, nObject ); 
                    
                    ++itBegin;
                }

            }
            else
            {
                int axis     = pTree->GetNodeSplitAxis( pNode );
                float fSplit = pTree->GetNodeSplitPosition( pNode );
                float fO     = rRayOrigin[axis] ;
                float fD     = rRayDirectionInv[axis];
                float fTHit  = ( fSplit - fO ) * fD;

                uint nFirst = (nNodeOrder>>axis) & 1;
                uint nSecond = nFirst ^ 1;
                typename KDTree_T::ConstNodeHandle pNear = pTree->GetChild( pNode, nFirst );
                typename KDTree_T::ConstNodeHandle pFar  = pTree->GetChild( pNode, nSecond );
                    
                if( fTHit > fTMax )
                {
                    // hit near only
                    pNode = pNear;
                    continue;
                }
                else if( fTHit < fTMin )
                {
                    // hit far only
                    pNode = pFar;
                    continue;
                }
                else
                {
                    // hit both
                    pStack->pNode = pFar;
                    pStack->fTMin = fTHit;
                    pStack->fTMax = fTMax;
                    pStack++;

                    pNode = pNear;
                    fTMax = fTHit;
                    continue;
                }         
            }

            // continue popping the stack until we locate a node that is nearer than the ray depth limit
            do
            {
                if( pStack == pStackBottom )
                    return;      // stack is empty, we have fallen out of the tree
            
                pStack--;
            } while( !rRay.IsDistanceValid( pStack->fTMin ) );

            // visit the next node from the stack
            pNode = pStack->pNode;
            fTMin = pStack->fTMin;
            fTMax = pStack->fTMax;

        } // end of infinite traversal loop

    }
}
#endif // _TRTKDTRAVERSAL_H_
