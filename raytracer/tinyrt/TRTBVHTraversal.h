//=====================================================================================================================
//
//   TRTBVHTraversal.h
//
//   Single-ray traversal through BVH data structures
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_BVHTRAVERSAL_H_
#define _TRT_BVHTRAVERSAL_H_

#include "TRTScratchMemory.h"

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Searches for the first intersection between a ray and an object in a BVH.  
    /// \param BVH_T        Must implement the BVH_C concept
    /// \param ObjectSet_T  Must implement the ObjectSet_C concept
    /// \param HitInfo_T    Must implement the HitInfo_C concept
    /// \param Ray_T        Must implement the Ray_C concept
    //=====================================================================================================================
    template< typename BVH_T, typename ObjectSet_T, typename HitInfo_T, typename Ray_T >
    void RaycastBVH( const BVH_T* pBVH, const ObjectSet_T* pObjects, Ray_T& rRay, HitInfo_T& rHitInfo, typename BVH_T::ConstNodeHandle pRoot, ScratchMemory& rScratch )
    {
        typedef typename BVH_T::obj_id obj_id;
        typedef typename BVH_T::ConstNodeHandle NodeHandle;
        ScratchArray< NodeHandle > stack( rScratch, pBVH->GetStackDepth() );
        NodeHandle* pStack = stack;
        
        NodeHandle* pStackBottom = pStack++;
        *pStackBottom = pRoot;
        
        while( pStack != pStackBottom )
        {
            pStack--;
            NodeHandle pNode = *pStack;

            while( pBVH->RayNodeTest( pNode, rRay ) )
            {
                if( pBVH->IsNodeLeaf( pNode ) )
                {
                    // intersect all objects in this leaf node, then proceed with next node from stack
                    obj_id rFirstObj;
                    obj_id rLastObj;
                    pBVH->GetNodeObjectRange( pNode, rFirstObj, rLastObj );
                    pObjects->RayIntersect( rRay, rHitInfo, rFirstObj, rLastObj );
                    break;
                }
                else
                {
                    // inner node: Visit node's children
                    NodeHandle pLeft  = pBVH->GetLeftChild( pNode );
                    NodeHandle pRight = pBVH->GetRightChild( pNode );

                    uint32 nAxis = pBVH->GetNodeSplitAxis( pNode );
                    if( rRay.Direction()[nAxis] < 0 )
                    {
                        *pStack = pLeft;
                        pNode = pRight;
                    }
                    else
                    {
                        *pStack = pRight;
                        pNode = pLeft;
                    }
                    pStack++;
                    
                }
            }
        }

        /*
        // alternative implementation which does NOT make use of stored split planes
        struct StackEntry
        {
            const BVH_T::Node* pNode;
            float t;
        };

        // TODO:  Should we use a fancier mechanism for allocating this stack space??
        StackEntry* pStack = (StackEntry*) _alloca( pBVH->GetStackDepth()*sizeof(StackEntry) );
        StackEntry* pStackBottom = pStack;

        const BVH_T::Node* pNode = pRoot;
        while( 1 )
        {
            if( pBVH->IsNodeLeaf( pNode ) )
            {
                // intersect all objects in this leaf node, then proceed with next node from stack
                BVH_T::obj_id rFirstObj;
                BVH_T::obj_id rLastObj;
                pBVH->GetNodeObjectRange( pNode, rFirstObj, rLastObj );
                const BVH_T::ObjectSet& rISect = pBVH->GetObjectSet();

                while( rFirstObj != rLastObj )
                {
                    rISect.RayIntersect( rRay, rHitInfo, rFirstObj );
                    rFirstObj++;
                }
            }
            else
            {
                const BVH_T::Node* pLeft  = pBVH->GetLeftChild( pNode );
                const BVH_T::Node* pRight = pBVH->GetRightChild( pNode );

                // visit both of this node's children
                float fTLeft, fTRight;
                bool bHitLeft  = pBVH->RayNodeTest( pLeft, rRay, fTLeft );
                bool bHitRight = pBVH->RayNodeTest( pRight, rRay, fTRight );

                if( bHitLeft && bHitRight )
                {
                    // hit both children.  Push the further of the two to the stack, and visit the nearer
                    if( fTRight > fTLeft )
                    {
                        pStack->pNode = pRight;
                        pStack->t = Max( rRay.MinDistance(), fTRight );
                        pNode = pLeft;
                    }
                    else
                    {
                        pStack->pNode = pLeft;
                        pStack->t = Max( rRay.MinDistance(), fTLeft );
                        pNode = pRight;
                    }

                    pStack++;
                    continue;
                }
                else if( bHitLeft )
                {
                    // hit only one child.  Visit that one
                    pNode = pLeft;
                    continue;
                }
                else if( bHitRight )
                {
                    // hit only right child.  Visit that one next
                    pNode = pRight;
                    continue;
                }

                // if we reach this point, we missed both child nodes
                // continue traversal with the last node from the stack
            }

            // continue popping the stack until we locate a node that is nearer than the ray depth limit
            do
            {
                // stack is empty, we have fallen out of the tree
                if( pStack == pStackBottom )
                    return;

                pStack--;
            } while( !rRay.IsDistanceValid( pStack->t ) );


            // visit the next node from the stack
            pNode = pStack->pNode;

        } // end of infinite traversal loop
        */
    }
}

#endif // _TRT_BVHTRAVERSAL_H_
