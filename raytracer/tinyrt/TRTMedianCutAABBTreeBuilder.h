//=====================================================================================================================
//
//   TRTMedianCutAABBTreeBuilder.h
//
//   Definition of class: TinyRT::MedianCutAABBTreeBuilder
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_MEDIANCUTAABBTREEBUILDER_H_
#define _TRT_MEDIANCUTAABBTREEBUILDER_H_

#include <memory>

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief An AABB tree builder implementation which uses median splits
    ///
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    //=====================================================================================================================
    template< class ObjectSet_T >
    class MedianCutAABBTreeBuilder 
    {
    public:

        typedef  ObjectSet_T ObjectSet;
        typedef typename ObjectSet::obj_id   obj_id;

        inline MedianCutAABBTreeBuilder( uint32 nMaxLeafObjects=1 );
    
        //=====================================================================================================================
        /// \param pObjects     Object set for which the tree is constructed
        /// \param pTree        The tree to be constructed.  May NOT be NULL
        /// \return The maximum depth of the constructed tree (0 is the depth of the root)
        //=====================================================================================================================
        template< class AABBTree_T >
        uint32 BuildTree( ObjectSet* pObjects, AABBTree_T* pTree );

    
    private:


        struct Object
        {
            AxisAlignedBox box;
            obj_id nID;
        };


        /// Functor for sorting object structures along an axis
        template< int axis >
        class ObjectSorter
        {
        public:

            inline bool operator()( const Object* a, const Object* b ) const
            {
                return ( a->box.Min()[axis] < b->box.Min()[axis] );
            };
        };


        template< typename AABBTree_T >
        uint32 MedianCut( Object** pObjects,
                          obj_id nObjects,
                          AABBTree_T* pTree,
                          typename AABBTree_T::NodeHandle pNode, 
                          const AxisAlignedBox* pRootBox,
                          obj_id* pObjectsOut,
                          obj_id nFirstObject,
                          uint32 nDepth  );

        uint32 m_nMaxLeafObjects;   ///< Maximum number of objects allowed in a leaf node
    };

}

#include "TRTMedianCutAABBTreeBuilder.inl"

#endif // _TRT_MEDIANCUTAABBTREEBUILDER_H_
