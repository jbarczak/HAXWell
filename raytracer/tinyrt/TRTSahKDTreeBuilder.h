//=====================================================================================================================
//
//   TRTSahKDTreeBuilder..h
//
//   Definition of class: TinyRT::SahKDTreeBuilder
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTSAHKDTREEBUILDER_H_
#define _TRTSAHKDTREEBUILDER_H_

#include "TRTCostMetric.h"

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief SAH-based KD tree construction
    /// 
    ///  The Sah KD tree builder implements standard perfect-split KD-tree construction, as described 
    ///   by Wald and Havran "On building fast kd-Trees for Ray Tracing, and on doing that in O(N log N)" (RT'06)
    ///
    ///  To use this class, you must provide an object set, as well as a clipper object which is used to subdivide
    ///   primitives which straddle KD split planes.  TRT provides a simple BoxClipper which clips only the AABBs of the
    ///   objects.  For triangle meshes, there is also a TriangleClipper.
    ///
    ///  You may also wish to use a specialized cost function.  The cost function should return the cost of a ray-object intersection
    ///   test, relative to the cost of a node traversal
    ///
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    /// \param Clipper_T Must implement the Clipper_C concept
    /// \param CostFunction_T Must implement the CostFUnction_C concept.
    /// \param LeafPolicy_T Must implement the LeafPolicy_C concept.
    ///
    //=====================================================================================================================
    template< class ObjectSet_T, class Clipper_T, typename CostFunction_T = ConstantCost<typename ObjectSet_T::obj_id>, class LeafPolicy_T = NullLeafPolicy >
    class SahKDTreeBuilder : public LeafPolicy_T
    {
    public:

        typedef typename ObjectSet_T::obj_id obj_id;

        /// \param fISectCost   Cost of an object intersection test, relative to a node traversal
        inline SahKDTreeBuilder( const CostFunction_T& rCostFunc ) :  m_costFunc(rCostFunc) {};

        /// Constructs a KD tree for the specified object set, returning its maximum depth
        template< class KDTree_T >
        inline uint BuildTree( ObjectSet_T* pObjects, KDTree_T* pTree );
    
    private:

        enum EventTypes // ORDER MATTERS!
        {
            END,        ///< End of object AABB
            IN_PLANE,   ///< Planar object at this plane location        
            START,      ///< Start of object AABB
        };

        enum Side
        {
            LEFT,
            RIGHT,
            BOTH
        };

        /// Object information needed during tree construction
        struct ObjectInfo
        {
            AxisAlignedBox bbox;    ///< Bounding box of that portion of the object which intersects the split region
            obj_id nObject;         ///< ID of object in object set
            Side   eSide;           ///< Temporary variable used during classification.  Indicates which side of the split object is on
            ObjectInfo* pNext;      ///< Next object in the list
        };

        /// A linked list of object information
        struct ObjectList
        {
            obj_id nObjects;
            ObjectInfo* pHead;
        };

        /// A split plane candidate
        struct SplitEvent
        {
            float  fPosition;
            SplitEvent* pNext;   ///< Next event in event list
            ObjectInfo* pObj;    ///< Object which introduced this event
            uint8  nAxis;
            uint8 nEventType;   
        };

        /// A linked list of candidate split planes
        struct SplitList
        {
            SplitEvent* pHead;
        };

        /// Information about a selected split plane
        struct SplitSelection
        {
            float fPosition;
            uint nAxis;
            Side eSide;             ///< Which side to put 'in-plane' objects on (left or right)
            Side eSideWithObjects;  ///< Which side contains objects (left, right, or both)
        };

        /// Functor for sorting split plane candidates
        class SortSplits
        {
        public:
            inline bool operator()( const SplitEvent& a, const SplitEvent& b )
            {
                if( a.fPosition < b.fPosition ) 
                    return true;
                else if( a.fPosition > b.fPosition ) 
                    return false;
                else 
                    return a.nEventType < b.nEventType;
            };
        };

        /// Creates initial split candidates and object info structures
        void CreateLists( ObjectSet_T* pObjects, SplitList pEventLists[3], ObjectList& rObjectList, AxisAlignedBox& rootAABB );

        template< class KDTree_T >
        uint BuildTreeRecurse( uint nRecursionDepth,
                               const AxisAlignedBox& rRootBB, 
                               ObjectSet_T* pObjects, 
                               SplitList eventLists[3],
                               ObjectList& rObjectList,
                               KDTree_T* pTree,
                               typename KDTree_T::NodeHandle hNode );


        /// Choose a split plane given a list of split events
        bool SelectSplit( uint nRecursionDepth, const AxisAlignedBox& rRootBB, const ObjectList& rObjects, SplitList eventLists[3], SplitSelection& rSplitOut );


        /// Groups objects in relation to a split plane
        void ClassifyObjects( const ObjectList& rList, const SplitSelection& rSplit, const SplitList& rSplitEvents );
        
        /// Sifts objects into lists based on their classification (left,right,both)
        void PartitionObjects( const ObjectList& rList, ObjectList pObjectListsOut[3] );

        /// Divides split candidates into left and right sub-lists based on their object (splits in both trees are excluded from both lists)
        void PartitionSplitEvents( SplitList pEventList[3], SplitList pLeftEvents[3], SplitList pRightEvents[3] );
    

        /// Subdivides objects which straddle a split plane, and creates new split candidates
        void ClipStraddlingObjects( const ObjectSet_T* pObjects,
                                    const SplitSelection& rSplit, 
                                    ObjectList pObjectsByClass[3], // LEFT, RIGHT, BOTH
                                    SplitList  pEventListsLeft[3], 
                                    SplitList  pEventListsRight[3] );

        /// Creates a set of split candidates for a list of objects
        void CreateSplitCandidates( const ObjectList& rList, SplitEvent* pSplitsOut, SplitList pSplitLists[3] );

        /// Combines two sorted split lists into a new one (the 'a' list is merged into the 'b' list)
        void MergeSplitLists( SplitList& a, SplitList& b );
        
        /// Sorts a split list
        void SortSplitList( SplitList& a );

        /// Adds a full set of split lists to the free list
        void ReclaimNodes( SplitList pLists[3], ObjectList& rObjectList );

        /// Frees all temporary memory
        void FreeTemporaryMemory( );



        /// A utility class for linked list memory management
        template< class T >
        class LinkedListHelper
        {
        public:

            LinkedListHelper() : m_pFreeList(0) {};

            /// Allocates a fresh, contiguous array of objects.  Does not initialize
            T* AllocateArray( uint nCount )
            {
                T* pMem = new T[nCount];
                m_allocs.push_back( pMem );
                return pMem;
            }

            /// Allocates a linked list of the specified length, reusing nodes if available  
            T* AllocateList( uint nCount )
            {
                if( m_pFreeList )
                {
                    // allocate from free list first
                    T* pHead = m_pFreeList;
                            
                    // skip along until we run out of free list, or until we've allocated all we need
                    nCount--;
                    while( m_pFreeList->pNext && nCount )
                    {
                        m_pFreeList = m_pFreeList->pNext;
                        nCount--;
                    }
                
                    // allocate final node from free list
                    T* pTail = m_pFreeList;
                    m_pFreeList = m_pFreeList->pNext;

                    if( !nCount )
                    {
                        pTail->pNext = NULL;
                        return pHead; // successfully allocated everything from free list
                    }

                    // not enough nodes in free list....
                    // allocate fresh array for remaining nodes
                    T* pArray = AllocateArray( nCount );
                    pTail->pNext = pArray;
                    for( uint i=0; i<nCount; i++ )
                        pArray[i].pNext = &pArray[i+1];
                    
                    pArray[nCount-1].pNext = NULL;
                    return pHead;
                    
                }
                else
                {
                    // empty free list.  Allocate a fresh array
                    T* pArray = AllocateArray( nCount );
                    for( uint i=0; i<nCount; i++ )
                        pArray[i].pNext = &pArray[i+1];
                    
                    pArray[nCount-1].pNext = NULL;
                    return pArray;
                }     

            }

            /// Adds a single node to the free list
            void ReleaseNode( T* pNode )
            {
                pNode->pNext = m_pFreeList;
                m_pFreeList = pNode;
            }

            /// Adds an entire chain of nodes to the free list
            void ReleaseList( T* pListHead )
            {
                // chase pointers until we find the end of the list
                T* pListTail = pListHead;
                while( pListTail->pNext )
                    pListTail = pListTail->pNext;

                // splice this list onto the free list
                pListTail->pNext = m_pFreeList;
                m_pFreeList = pListHead;
            }

            void Deallocate() 
            {
                // release all allocations ever made for this type of list node
                for( typename std::vector<T*>::iterator it = m_allocs.begin(); it != m_allocs.end(); ++it )
                    delete *it;
                m_allocs.clear();

                m_pFreeList = NULL;
            }

        private:

            std::vector<T*> m_allocs;
            T* m_pFreeList;
        };


        LinkedListHelper<SplitEvent> m_splitListHelper;
        LinkedListHelper<ObjectInfo> m_objectListHelper;

        CostFunction_T m_costFunc;
    };
    
}

#include "TRTSahKDTreeBuilder.inl"

#endif // _TRTSAHKDTREEBUILDER_H_
