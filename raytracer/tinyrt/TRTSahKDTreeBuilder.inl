//=====================================================================================================================
//
//   TRTSahKDTreeBuilder.inl
//
//   Implementation of class: TinyRT::SahKDTreeBuilder
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include "TRTSahKDTreeBuilder.h"

namespace TinyRT
{
 
    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T >
    template< class KDTree_T >
    uint SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::BuildTree( ObjectSet_T* pObjects, KDTree_T* pTree )
    {
        SplitList splitLists[3];
        ObjectList objectList;
        AxisAlignedBox rootAABB;
        CreateLists( pObjects, splitLists, objectList, rootAABB );

        typename KDTree_T::NodeHandle hRoot = pTree->Initialize( rootAABB );

        uint nDepth = BuildTreeRecurse( 0, rootAABB, pObjects, splitLists, objectList, pTree, pTree->GetRoot() );

        FreeTemporaryMemory( );
        return nDepth;
    }
    
    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// Creates initial object info. structures, and initial lists of split plane candidates
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::CreateLists( ObjectSet_T* pObjects, 
                                                                               SplitList pEventLists[3],
                                                                               ObjectList& rObjectList,
                                                                               AxisAlignedBox& rootAABB )
    {
   
        typename ObjectSet_T::obj_id nObjects = pObjects->GetObjectCount();

        // allocate list nodes
        SplitEvent* pEvents = m_splitListHelper.AllocateArray( 6*nObjects );
        ObjectInfo* pObjectInfo = m_objectListHelper.AllocateArray( nObjects );        

        // create object info list
        rootAABB.Min() = Vec3f( FLT_MAX,FLT_MAX,FLT_MAX );
        rootAABB.Max() = Vec3f( -FLT_MAX, -FLT_MAX, -FLT_MAX );

        for( typename ObjectSet_T::obj_id i=0; i< nObjects; i++ )
        {
            pObjects->GetObjectAABB( i, pObjectInfo[i].bbox );
            pObjectInfo[i].nObject = i;
            pObjectInfo[i].pNext = &pObjectInfo[i+1];
            rootAABB.Merge( pObjectInfo[i].bbox );
        }
        
        pObjectInfo[nObjects-1].pNext = NULL;
        rObjectList.nObjects = nObjects;
        rObjectList.pHead = pObjectInfo;

        // create candidate split planes

        // assign split events axis by axis
        for( uint axis=0; axis<3; axis++ )
        {
            uint nEvents=0;
            SplitEvent* pFirstEvent = pEvents;
            
            ObjectInfo* pObj= rObjectList.pHead;
            while( pObj != NULL )
            {
                const AxisAlignedBox& rBox = pObj->bbox;
                if( rBox.Max()[axis] == rBox.Min()[axis] )
                {
                    // object is flat on this axis, create only an 'in-plane' event
                    pEvents->fPosition = rBox.Min()[axis];
                    pEvents->nAxis = axis;
                    pEvents->nEventType = IN_PLANE;
                    pEvents->pObj = pObj;
                    pEvents++;
                }
                else
                {
                    // object is not flat... create start and end events
                    pEvents->fPosition = rBox.Max()[axis];
                    pEvents->nAxis = axis;
                    pEvents->nEventType = END;
                    pEvents->pObj = pObj;
                    pEvents++;
                    
                    pEvents->fPosition = rBox.Min()[axis];
                    pEvents->nAxis = axis;
                    pEvents->nEventType = START;
                    pEvents->pObj = pObj;
                    pEvents++;
                }
                pObj = pObj->pNext;
            }

            std::sort( pFirstEvent, pEvents, SortSplits() );
            pEventLists[axis].pHead = pFirstEvent;

            while( pFirstEvent != pEvents )
            {
                pFirstEvent->pNext = pFirstEvent+1;
                pFirstEvent++;
            }

            pEvents[-1].pNext = NULL;
        }        
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T >
    template< class KDTree_T >
    uint SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::BuildTreeRecurse( uint nRecursionDepth,
                                                                                   const AxisAlignedBox& rRootBB, 
                                                                                  ObjectSet_T* pObjects, 
                                                                                  SplitList pEventLists[3], 
                                                                                  ObjectList& rObjectList,
                                                                                  KDTree_T* pTree,
                                                                                  typename KDTree_T::NodeHandle hNode )
    {
        typedef typename KDTree_T::NodeHandle NodeHandle;

        SplitSelection splitSelection;
        if( !SelectSplit( nRecursionDepth, rRootBB, rObjectList, pEventLists, splitSelection ) )
        {
            // we've decided to create a leaf, so do it
            obj_id* pObjectList = pTree->MakeLeafNode( hNode, rObjectList.nObjects );
            
            ObjectInfo* pObj = rObjectList.pHead;
            while( pObj != NULL )
            {
                *pObjectList = pObj->nObject;
                pObjectList++;
                pObj = pObj->pNext;
            }

            // recycle list nodes
            ReclaimNodes( pEventLists, rObjectList );
            return 1;
        }
        else
        {
            // we've decided to split...........

            if( splitSelection.eSideWithObjects == BOTH )
            {
                // there are objects on both sides of the split plane

                // Classify all objects as left, right, or straddling the split plane
                ClassifyObjects( rObjectList, splitSelection, pEventLists[ splitSelection.nAxis] );

                // partition objects by their classifications
                ObjectList objectsByClass[3]; // LEFT, RIGHT, BOTH
                PartitionObjects( rObjectList, objectsByClass );

                // make sure that split selection and classification agree on the object distribution
                TRT_ASSERT( objectsByClass[LEFT].nObjects != rObjectList.nObjects && objectsByClass[RIGHT].nObjects != rObjectList.nObjects );
                
                // make sure that each object is in precisely one object list
                TRT_ASSERT( objectsByClass[LEFT].nObjects + objectsByClass[RIGHT].nObjects + objectsByClass[BOTH].nObjects == rObjectList.nObjects );

                // Partition the existing events based on object placement.  
                //  Events for left side objects on left, right side on right.   Events for both-side objects are discarded
                SplitList pEventListsLeft[3];
                SplitList pEventListsRight[3];
                PartitionSplitEvents( pEventLists, pEventListsLeft, pEventListsRight );

                // prevent double-freeing of list nodes during recursion.  After partitioning, we must remove nodes from the input lists
                //  Otherwise there will be multiple lists on the stack pointing into the same nodes
                rObjectList.pHead = NULL; 
                for( int i=0; i<3; i++ )
                    pEventLists[i].pHead = NULL;

                
                // Clip straddling objects to the split plane, creating new object structures and split events
                ClipStraddlingObjects( pObjects, splitSelection, objectsByClass, pEventListsLeft, pEventListsRight  );

                // make this node an inner node
                std::pair<NodeHandle, NodeHandle> kids =
                    pTree->MakeInnerNode( hNode, splitSelection.fPosition, splitSelection.nAxis );

                // cut the AABB, and recurse
                AxisAlignedBox leftBox;
                AxisAlignedBox rightBox;
                rRootBB.Cut( splitSelection.nAxis, splitSelection.fPosition, leftBox, rightBox );

                // recursively build the subtrees
                uint nLeftDepth = BuildTreeRecurse( nRecursionDepth+1,leftBox, pObjects, pEventListsLeft, objectsByClass[LEFT], pTree, kids.first );                
                uint nRightDepth = BuildTreeRecurse( nRecursionDepth+1,rightBox, pObjects, pEventListsRight, objectsByClass[RIGHT], pTree, kids.second );
                
                return 1 + Max( nLeftDepth, nRightDepth );
            }
            else if( splitSelection.eSideWithObjects == LEFT )
            {
                // right child is an empty leaf.  Construct on left
                std::pair<NodeHandle, NodeHandle> kids =
                    pTree->MakeInnerNode( hNode, splitSelection.fPosition, splitSelection.nAxis );

                pTree->MakeLeafNode( kids.second, 0 );

                AxisAlignedBox leftBox;
                rRootBB.CutLeft( splitSelection.nAxis, splitSelection.fPosition, leftBox );
                
                return 1 + BuildTreeRecurse( nRecursionDepth+1,leftBox, pObjects, pEventLists, rObjectList, pTree, kids.first );
            }
            else //( splitSelection.eSideWithObjects == RIGHT )
            {
                // left child is an empty leaf.  Construct on right
                std::pair<NodeHandle, NodeHandle> kids =
                    pTree->MakeInnerNode( hNode, splitSelection.fPosition, splitSelection.nAxis );

                pTree->MakeLeafNode( kids.first, 0 );

                AxisAlignedBox rightBox;
                rRootBB.CutRight( splitSelection.nAxis, splitSelection.fPosition, rightBox );

                return 1 + BuildTreeRecurse( nRecursionDepth+1, rightBox, pObjects, pEventLists, rObjectList, pTree, kids.second );
            }
        }
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T >
    bool SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::SelectSplit( uint nRecursionDepth,
                                                                              const AxisAlignedBox& rRootBB, 
                                                                              const ObjectList& rObjects,
                                                                              SplitList eventLists[3], 
                                                                              SplitSelection& rSplitOut )
    {
        uint nObjects = rObjects.nObjects;
        if( nObjects == 0 )
            return false;
   
        Vec3f vBBSize = rRootBB.Max() - rRootBB.Min();
        float fInvRootArea = 1.0f / ( vBBSize.x*( vBBSize.y + vBBSize.z ) + vBBSize.y*vBBSize.z );

        float fTotalCost = 0;
        ObjectInfo* pObj = rObjects.pHead;
        while( pObj )
        {
            fTotalCost += m_costFunc( pObj->nObject );
            pObj = pObj->pNext;
        }

        float fBestCost = LeafPolicy_T::AdjustLeafCost( nRecursionDepth, nObjects, fTotalCost );
        bool bHaveSplit = false;
        
        for( uint i=0; i<3; i++ )
        {
            float fLeftCost  = 0;
            float fRightCost = fTotalCost;

            // surface area of a box is:  2xy + 2yz + 2xz
            //  If we factor out the dimension that is changing (X, say), we have:
            //    2X(Y+Z) + 2YZ.  
            // (Or, 2X*B + 2A).  Note that the factors of two cancel out, and we can pre-multiply by the other stuff
            uint nY = (i+1)%3;
            uint nZ = (i+2)%3;

            float fAlpha = ( vBBSize[nY] * vBBSize[nZ] ) * fInvRootArea ;
            float fBeta  = ( vBBSize[nY] + vBBSize[nZ] ) * fInvRootArea ;

            uint nObjectsLeft = 0;
            uint nObjectsRight = nObjects;

            // cycle through potential split planes
            SplitEvent* pSplit = eventLists[i].pHead;
            while( pSplit != NULL )
            {
                // figure out how the object distribution changes at this split plane location
                uint nCountThisPlane[3] = { 0,0,0 };    // object counts for END,START,IN_PLANE events
                float fCostThisPlane[3] = { 0,0,0 };    // net object cost for END,START,IN_PLANE events
                float fPlanePos = pSplit->fPosition;

                TRT_ASSERT( rRootBB.Min()[i] <= fPlanePos && rRootBB.Max()[i] >= fPlanePos );

                do
                {
                    fCostThisPlane[pSplit->nEventType] += m_costFunc( pSplit->pObj->nObject );
                    nCountThisPlane[pSplit->nEventType]++;
                    pSplit = pSplit->pNext;

                } while( pSplit != NULL && pSplit->fPosition == fPlanePos );

                // move onto this plane.  END events shift their objects to left side.  IN_PLANE objects are in limbo
                //  Note that START events do not take effect until after we have passed this plane location
                fRightCost -= ( fCostThisPlane[IN_PLANE] + fCostThisPlane[END] );
                nObjectsRight -= nCountThisPlane[END] + nCountThisPlane[IN_PLANE];

                // evaluate SAH (nLeft, nInThisPlane, nRight )
                float fXL = fPlanePos - rRootBB.Min()[i];
                float fXR = rRootBB.Max()[i] - fPlanePos;
                float fPL = ( fXL*fBeta + fAlpha );
                float fPR = ( fXR*fBeta + fAlpha );

                float fSAHLeft  = 1.0f + fPL*( fLeftCost + fCostThisPlane[IN_PLANE] ) + fPR*( fRightCost );
                float fSAHRight = 1.0f + fPL*( fLeftCost ) + fPR*( fRightCost + fCostThisPlane[IN_PLANE] );

                // choose a side to put the 'in-plane' objects on
                Side eSide = (fSAHLeft < fSAHRight) ? LEFT : RIGHT;
                float fCost = (fSAHLeft < fSAHRight) ? fSAHLeft : fSAHRight;

                // keep this split if it is best so far
                if( fCost < fBestCost )
                {
                    bHaveSplit = true;
                    rSplitOut.nAxis = i;
                    rSplitOut.eSide = eSide;        // side on which to put 'IN_PLANE' objects
                    rSplitOut.fPosition = fPlanePos;
                    
                    uint nLeft  = (eSide == LEFT) ? nObjectsLeft + nCountThisPlane[IN_PLANE] : nObjectsLeft;
                    uint nRight = (eSide == RIGHT) ? nObjectsRight + nCountThisPlane[IN_PLANE] : nObjectsRight;
                    if( nLeft && nRight )
                        rSplitOut.eSideWithObjects = BOTH;
                    else if( nLeft )
                        rSplitOut.eSideWithObjects = LEFT;
                    else
                        rSplitOut.eSideWithObjects = RIGHT;

                    fBestCost = fCost;
                }

                // move past this plane
                fLeftCost += ( fCostThisPlane[IN_PLANE] + fCostThisPlane[START] );
                nObjectsLeft += (nCountThisPlane[IN_PLANE] + nCountThisPlane[START]);
            }
        }

        return bHaveSplit;
    }

    //=====================================================================================================================
    /// Classifies objects as left, right, or straddling a split plane
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::ClassifyObjects( const ObjectList& rList, 
                                                                                  const SplitSelection& rSplit, 
                                                                                  const SplitList& rSplitEvents )
    {
        // tentatively mark all objects as 'BOTH sides'
        ObjectInfo* pObj = rList.pHead;
        while( pObj != NULL )
        {
            pObj->eSide = BOTH;
            pObj = pObj->pNext;
        }

        // sweep the split event list to identify 'left' and 'right' objects
        SplitEvent* pEvent = rSplitEvents.pHead;
        while( pEvent != NULL )
        {
            // TODO:  This logic can be simplified...
            if( pEvent->nEventType == END && pEvent->fPosition <= rSplit.fPosition )
                pEvent->pObj->eSide = LEFT;
            else if( pEvent->nEventType == START && pEvent->fPosition >= rSplit.fPosition )
                pEvent->pObj->eSide = RIGHT;
            else if( pEvent->nEventType == IN_PLANE )
            {
                if( pEvent->fPosition < rSplit.fPosition || ( pEvent->fPosition == rSplit.fPosition && rSplit.eSide == LEFT ) )
                    pEvent->pObj->eSide = LEFT;
                else if( pEvent->fPosition > rSplit.fPosition || ( pEvent->fPosition == rSplit.fPosition && rSplit.eSide == RIGHT ) )
                    pEvent->pObj->eSide = RIGHT;
                //else
                //    both
            }
            //else both
                

            pEvent = pEvent->pNext;
        }

    }

    //=====================================================================================================================
    /// Sifts objects into lists by classification
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::PartitionObjects( const ObjectList& rList, ObjectList pObjectListsOut[3] )
    {
        // initialize lists
        ObjectInfo* pTails[3];
        for( uint i=0; i<3; i++ )
        {
            pTails[i] = 0;
            pObjectListsOut[i].nObjects = 0;
            pObjectListsOut[i].pHead = 0;
        }

        // sift the objects
        ObjectInfo* pObj = rList.pHead;
        while( pObj != NULL )
        {
            if( pTails[ pObj->eSide ] )
                pTails[ pObj->eSide ]->pNext = pObj;
            else
                pObjectListsOut[ pObj->eSide ].pHead = pObj; // first entry in this list.  Set the head
            
            pTails[ pObj->eSide ] = pObj;
            pObjectListsOut[ pObj->eSide ].nObjects++;

            pObj = pObj->pNext;
        }

        // NULL-terminate the lists
        for( uint i=0; i<3; i++ )
        {
            if( pTails[i] )
                pTails[i]->pNext = NULL;
        }
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::PartitionSplitEvents( SplitList pEventList[3], 
                                                                                       SplitList pLeftEvents[3], 
                                                                                       SplitList pRightEvents[3] )
    {
        // divide split events based on the classifications of their objects. Events for 'left' and 'right' objects are
        // passed through to the output lists.  Events for 'straddle' objects are dropped

        for( uint i=0; i<3; i++ )
        {
            SplitEvent* pSplit = pEventList[i].pHead;
            SplitEvent* pLeftTail = NULL;
            SplitEvent* pRightTail = NULL;
            pLeftEvents[i].pHead = NULL;
            pRightEvents[i].pHead = NULL;

            while( pSplit != NULL )
            {
                if( pSplit->pObj->eSide == LEFT )
                {
                    // push event onto left list
                    if( pLeftTail )
                        pLeftTail->pNext = pSplit;
                    else
                        pLeftEvents[i].pHead = pSplit;

                    pLeftTail = pSplit;
                    pSplit = pSplit->pNext;
                }
                else if( pSplit->pObj->eSide == RIGHT )
                {
                    // push event onto right list
                    if( pRightTail )
                        pRightTail->pNext = pSplit;
                    else
                        pRightEvents[i].pHead = pSplit;

                    pRightTail = pSplit;
                    pSplit = pSplit->pNext;
                }
                else
                {
                    // object is on both sides.  Drop this split node
                    SplitEvent* pFreeNode = pSplit;
                    pSplit = pSplit->pNext;

                    m_splitListHelper.ReleaseNode( pFreeNode );
                }
            }

            if( pLeftTail )
                pLeftTail->pNext = NULL;
            if( pRightTail )
                pRightTail->pNext = NULL;
        }

    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::ClipStraddlingObjects( const ObjectSet_T* pObjectSet, 
                                                                                      const SplitSelection& rSplit,
                                                                                      ObjectList  pObjectsByClass[3], // LEFT, RIGHT, BOTH
                                                                                      SplitList   pEventListsLeft[3],
                                                                                      SplitList   pEventListsRight[3] )
    {
        uint nSplitObjects = pObjectsByClass[BOTH].nObjects;
        if( nSplitObjects == 0 )
            return;

        SplitEvent* pNewSplitsLeft  = m_splitListHelper.AllocateList( 6*nSplitObjects );
        SplitEvent* pNewSplitsRight = m_splitListHelper.AllocateList( 6*nSplitObjects );
        ObjectInfo* pSplitObjects   = m_objectListHelper.AllocateList( nSplitObjects );

        // for each object in the straddling list, clip into two pieces and append the pieces to the left and right lists
        ObjectInfo* pLEnd = NULL; 
        ObjectInfo* pREnd = NULL;
        ObjectInfo* pL = pObjectsByClass[BOTH].pHead; // we re-cycle the old 'BOTH' list for the left side, and make a copy for the right
        ObjectInfo* pR = pSplitObjects;
        for( size_t i=0; i<nSplitObjects; i++ )
        {
            // compute clipped AABBs
            AxisAlignedBox oldBox = pL->bbox;
            Clipper_T::ClipObjectToAxisAlignedPlane( pObjectSet, pL->nObject, oldBox, rSplit.fPosition, rSplit.nAxis, pL->bbox, pR->bbox );

            // verify that the clipper implementation is correct
            TRT_ASSERT( pL->bbox.Max()[rSplit.nAxis] >= rSplit.fPosition && pL->bbox.Min()[rSplit.nAxis] <= rSplit.fPosition );
            TRT_ASSERT( pL->bbox.IsValid() && pR->bbox.IsValid() );
            TRT_ASSERT( oldBox.Contains( pL->bbox ) && oldBox.Contains( pR->bbox ) );
                        
            pR->nObject = pL->nObject; // copy object reference from old 'BOTH' list into new 'RIGHT' list

            pLEnd = pL;      // keep a chase pointer so we can concatenate onto the lists at the end
            pREnd = pR;
            pL = pL->pNext;
            pR = pR->pNext;
        }

        // create split events for each of the new AABB fragments
        ObjectList newObjectsLeft;
        newObjectsLeft.pHead = pObjectsByClass[BOTH].pHead;
        newObjectsLeft.nObjects = nSplitObjects;

        ObjectList newObjectsRight;
        newObjectsRight.pHead = pSplitObjects;
        newObjectsRight.nObjects = nSplitObjects;

        SplitList leftSplitLists[3];
        SplitList rightSplitLists[3];
        CreateSplitCandidates( newObjectsLeft, pNewSplitsLeft, leftSplitLists );
        CreateSplitCandidates( newObjectsRight, pNewSplitsRight, rightSplitLists );

        // merge new split events in with existing ones
        for( int i=0; i<3; i++ )
        {
            MergeSplitLists( leftSplitLists[i], pEventListsLeft[i] );
            MergeSplitLists( rightSplitLists[i], pEventListsRight[i] );
        }


        // append new object info structures to the left and right object lists. 
        pLEnd->pNext = pObjectsByClass[LEFT].pHead;
        pObjectsByClass[LEFT].pHead = newObjectsLeft.pHead;
        pObjectsByClass[LEFT].nObjects += nSplitObjects;

        pREnd->pNext = pObjectsByClass[RIGHT].pHead;
        pObjectsByClass[RIGHT].pHead = newObjectsRight.pHead;
        pObjectsByClass[RIGHT].nObjects += nSplitObjects;        
    }


    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::CreateSplitCandidates( const ObjectList& rObjectList, 
                                                                         SplitEvent* pEvents, 
                                                                         SplitList pSplitLists[3] )
    {
        // assign split events axis by axis
        for( uint axis=0; axis<3; axis++ )
        {
            uint nEvents=0;
            SplitEvent* pFirstEvent = pEvents;
            
            ObjectInfo* pObj= rObjectList.pHead;
            while( pObj != NULL )
            {
                const AxisAlignedBox& rBox = pObj->bbox;
                if( rBox.Max()[axis] == rBox.Min()[axis] )
                {
                    // object is flat on this axis, create only an 'in-plane' event
                    pEvents->fPosition = rBox.Min()[axis];
                    pEvents->nAxis = axis;
                    pEvents->nEventType = IN_PLANE;
                    pEvents->pObj = pObj;
                }
                else
                {
                    // object is not flat... create start and end events
                    pEvents->fPosition = rBox.Min()[axis];
                    pEvents->nAxis = axis;
                    pEvents->nEventType = START;
                    pEvents->pObj = pObj;
                    pEvents = pEvents->pNext;
                    
                    pEvents->fPosition = rBox.Max()[axis];
                    pEvents->nAxis = axis;
                    pEvents->nEventType = END;
                    pEvents->pObj = pObj;
                }
                pObj = pObj->pNext;
                if( pObj != NULL )
                    pEvents = pEvents->pNext;
            }

            // cut this list and sort it
            SplitEvent* pTail = pEvents;
            pEvents = pEvents->pNext;
            pTail->pNext = NULL;
            pSplitLists[axis].pHead = pFirstEvent;

            SortSplitList( pSplitLists[axis] );
        }
    }

    
    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::MergeSplitLists( SplitList& a, SplitList& b )
    {
        // easy special cases first
        if( a.pHead == NULL )
            return;
        
        if( b.pHead == NULL )
        {
            b = a;
            return;
        }

        SortSplits sorter;

        SplitEvent* pA = a.pHead;
        SplitEvent* pB = b.pHead;
        SplitEvent* pTail;
        if( sorter( *pA, *pB ) )
        {
            pTail = pA;
            pA = pA->pNext;
        }
        else
        {
            pTail = pB;
            pB = pB->pNext;
        }

        // merge lists in sorted fashion
        b.pHead = pTail;
        while( pA && pB )
        {
            if( sorter( *pA, *pB ) )
            {
                pTail->pNext = pA;
                pTail = pA;
                pA = pA->pNext;
            }
            else
            {
                pTail->pNext = pB;
                pTail = pB;
                pB = pB->pNext;
            }
        }

        // exactly one list is empty.  Concatenate the remains of the other one
        pTail->pNext = reinterpret_cast<SplitEvent*>( reinterpret_cast<size_t>( pA ) | reinterpret_cast<size_t>( pB ) );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::SortSplitList( SplitList& l )
    {
        SortSplits order;

        // easy corner cases
        if( !l.pHead )
            return;
        if( !l.pHead->pNext )
            return;

        // find ordered segment
        SplitEvent* pSegmentHead = l.pHead;
        SplitEvent* pSegmentTail = l.pHead;
        while( pSegmentTail->pNext && order( *pSegmentTail, *pSegmentTail->pNext ) )
            pSegmentTail = pSegmentTail->pNext;

        // sort the rest
        SplitList remainder;
        remainder.pHead = pSegmentTail->pNext;
        SortSplitList( remainder );
        
        // clip off the first sorted segment
        pSegmentTail->pNext = NULL;

        // now merge the pieces
        MergeSplitLists( remainder, l );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::FreeTemporaryMemory()
    {
        m_splitListHelper.Deallocate();
        m_objectListHelper.Deallocate();
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, class CostFunction_T, class LeafPolicy_T  >
    void SahKDTreeBuilder<ObjectSet_T,Clipper_T,CostFunction_T,LeafPolicy_T>::ReclaimNodes( SplitList pSplitLists[3], 
                                                                                            ObjectList& rObjectList )
    {
        for( int i=0; i<3; i++ )
        {
            TRT_ASSERT( pSplitLists[i].pHead );        
            m_splitListHelper.ReleaseList( pSplitLists[i].pHead );
        }

        TRT_ASSERT( rObjectList.pHead );
        m_objectListHelper.ReleaseList( rObjectList.pHead );
    }


}

