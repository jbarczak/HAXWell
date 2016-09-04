//=====================================================================================================================
//
//   TRTSahAABBTreeBuilder.h
//
//   Definition of class: TinyRT::SahAABBTreeBuilder
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_SAHAABBTREEBUILDER_H_
#define _TRT_SAHAABBTREEBUILDER_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief An AABBTree builder which uses the surface area heuristic
    ///
    ///  This builder class may be used to construct either an AABBTree or QuadAABBTree.
    ///  To use this class, an object set(ObjectSet_C) and a cost function (CostFunction_C) are needed.
    ///
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    /// \param CostFunction_T Must implement the CostFunction_C concept. 
    ///                         The cost function should return the cost of a ray-object intersection test, 
    ///                         relative to the cost of a node traversal
    /// \param LeafPolicy_T Must implement the LeafPolicy_C concept
    //=====================================================================================================================
    template< class ObjectSet_T, class CostFunction_T = ConstantCost<typename ObjectSet_T::obj_id>, class LeafPolicy_T = NullLeafPolicy >
    class SahAABBTreeBuilder : public LeafPolicy_T
    {
    public:

        typedef ObjectSet_T ObjectSet;
        typedef typename ObjectSet::obj_id   obj_id;

        inline SahAABBTreeBuilder( const CostFunction_T& rCost );

        /// Builds an AABB tree
        template< class AABBTree_T >
        uint32 BuildTree( ObjectSet* pObjects, AABBTree_T* pTree );

        /// Builds a Quad-AABB tree
        template< class QAABBTree_T >
        uint32 BuildQuadAABBTree( ObjectSet* pObjects, QAABBTree_T* pTree );


    private:

        struct Object
        {
            AxisAlignedBox box;
            obj_id nID;
            obj_id nSortIndices[3]; ///< Position of this object in a sorted ordering along each axis
        };


        /// Functor for sorting object structures along an axis
        template< int axis >
        class SortObjects
        {
        public:

            inline bool operator()( const Object* a, const Object* b ) const
            {
                return ( (a->box.Min()[axis]+a->box.Max()[axis]) < 
                         (b->box.Min()[axis]+b->box.Max()[axis]) );
            };
        };

        /// Functor for partitioning sorted object lists along an axis
        class PartitionObjects
        {
        public:

            inline PartitionObjects( uint32 nAxis, size_t nIndex ) : m_nAxis(nAxis), m_nIndex(nIndex) {};

            inline bool operator()( Object*& a ) const
            {
                return ( a->nSortIndices[m_nAxis] <= m_nIndex );
            };

        private:
            uint32 m_nAxis;
            size_t m_nIndex;
        };

        /// Functor which partitions objects on an axis, and which simultaneously computes the AABB of the left side
        class PartitionAndComputeBox
        {
        public:

            inline PartitionAndComputeBox( uint32 nAxis, size_t nIndex, AxisAlignedBox* pBoxOut ) 
                : m_pBoxOut(pBoxOut), m_nIndex(nIndex), m_nAxis(nAxis)
            {};

            inline bool operator()( Object*& a ) const
            {
                if( a->nSortIndices[m_nAxis] <= m_nIndex )
                {
                    // if this object goes on the left, merge in its AABB
                    m_pBoxOut->Merge( a->box );
                    return true;
                }
                return false;
            };

        private:

            AxisAlignedBox* m_pBoxOut;
            size_t m_nIndex;
            uint32 m_nAxis;
        };


        /// Performs preprocessing on the object set to build the data structures needed for tree construction
        void SetupObjectInfo( ObjectSet* pObjects, std::vector<Object>& rObjects, std::vector<Object*> objectPtrs[3], AxisAlignedBox& rGlobalBB );

        int SplitObjects( uint nRecursionDepth,
                          Object** objectsByAxis[3],
                          const AxisAlignedBox& rBox,
                          obj_id nObjects,
                          Object** objectsRight[3],
                          obj_id&  nObjectsLeft,
                          obj_id&  nObjectsRight,
                          AxisAlignedBox& rLeftBox,
                          AxisAlignedBox& rRightBox );

        /// Recursive method which implements the tree build
        template< typename AABBTree_T >
        uint32 BuildRecurse( uint nRecursionDepth, 
                             Object** objectsByAxis[3],
                             obj_id nObjects,
                             AABBTree_T* pTree,
                             typename AABBTree_T::NodeHandle pNode,
                             const AxisAlignedBox& rBox,
                             obj_id  nFirstObject );

        template< typename QAABBTree_T >
        uint32 BuildQAABBRecurse_Even( uint nRecursionDepth, 
                                    Object** objectsByAxis[3],
                                  obj_id nObjects,
                                  QAABBTree_T* pTree,
                                  typename QAABBTree_T::NodeHandle pNode,
                                  uint32 nChild,
                                  const AxisAlignedBox& rBox,
                                  obj_id  nFirstObject);

        template< typename QAABBTree_T >
        uint32 BuildQAABBRecurse_Odd( uint nRecursionDepth, 
                                    Object** objectsByAxis[3],
                                  obj_id nObjects,
                                  QAABBTree_T* pTree,
                                  typename QAABBTree_T::NodeHandle pNode,
                                  uint32 nChild,
                                  const AxisAlignedBox& rBox,
                                  obj_id  nFirstObject,
                                  uint32& nSplitAxisOut );


        CostFunction_T m_costFunc;
    };
}

#include "TRTSahAABBTreeBuilder.inl"

#endif // _TRT_SAHAABBTREEBUILDER_H_
