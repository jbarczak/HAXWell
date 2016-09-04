//=====================================================================================================================
//
//   TRTMedianCutAABBTreeBuilder.inl
//
//   Implementation of class: TinyRT::MedianCutAABBTreeBuilder
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
    //
    //         Constructors/Destructors
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    /// \param nMaxObjects  The maximum number of objects allowed in a leaf node. 
    ///                       Objects will be partitioned until this limit is reached
    //=====================================================================================================================
    template< class ObjectSet_T >
    MedianCutAABBTreeBuilder< ObjectSet_T >::MedianCutAABBTreeBuilder( uint32 nMaxObjects )
    : m_nMaxLeafObjects( nMaxObjects )
    {
        TRT_ASSERT( nMaxObjects >= 0 );
    }
   

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// \param pObjects     Object set for which the tree is constructed
    /// \param pTree        The tree to be constructed.  May NOT be NULL
    /// \return The maximum depth of the constructed tree (0 is the depth of the root)
    //=====================================================================================================================
    template< class ObjectSet_T >
    template< class AABBTree_T >
    uint32 MedianCutAABBTreeBuilder< ObjectSet_T >::BuildTree( ObjectSet* pObjects, AABBTree_T* pTree )
    {
        obj_id nObjects = pObjects->GetObjectCount();
        
        // compute object bounding boxes, global bounding box
        //  assemble object structures
        ScopedArray<Object>  objects( new Object[nObjects]  );
        ScopedArray<Object*> objectPtrs( new Object*[nObjects] );
        ScopedArray<obj_id>  objectRemap( new obj_id[nObjects] );

        AxisAlignedBox globalBox;
        pObjects->GetObjectAABB( 0, globalBox );
        objects[0].box = globalBox;
        objects[0].nID = 0;
        objectPtrs[0] = &objects[0];

        for( obj_id i=1; i< nObjects; i++ )
        {
            pObjects->GetObjectAABB( i, objects[i].box );
            objects[i].nID = i;
            globalBox.Merge( objects[i].box );
            objectPtrs[i] = &objects[i];
        }
        
        // initialize the tree
        typename AABBTree_T::NodeHandle pRoot = pTree->Initialize( globalBox, 2*nObjects - 1 );

        // build the tree
        uint32 nMaxDepth = MedianCut( &objectPtrs[0], nObjects, pTree, pRoot, &globalBox, &objectRemap[0], 0, 0 );

        // rearrange the objects so they are in tree order
        pObjects->RemapObjects( &objectRemap[0] );
       
        return nMaxDepth;
    }

    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    template< class AABBTree_T >
    uint32 MedianCutAABBTreeBuilder< ObjectSet_T >::MedianCut( Object** pObjects,
                                                              obj_id nObjects,
                                                              AABBTree_T* pTree,
                                                              typename AABBTree_T::NodeHandle pNode, 
                                                              const AxisAlignedBox* pRootBox,
                                                              obj_id* pObjectRemap,
                                                              obj_id nFirstObject,
                                                              uint32 nDepth  )
    {
        typedef typename AABBTree_T::NodeHandle NodeHandle;

        if( nObjects <= m_nMaxLeafObjects )
        {
            TRT_ASSERT( nObjects >= 1 );
            
            // create a leaf node
            pTree->MakeLeafNode( pNode, nFirstObject, nObjects );
            pTree->SetNodeAABB( pNode, *pRootBox );
            
            // output object IDs for objects in this leaf node
            for( uint32 i=0; i<nObjects; i++ )
            {
                *pObjectRemap = pObjects[i]->nID;
                pObjectRemap++;
            }

            return nDepth;
        }
        else
        {
            // choose the long axis
            uint32 nAxis=0;
            Vec3f vBoxSize = pRootBox->Max() - pRootBox->Min();
            if( vBoxSize[1] > vBoxSize[0] )
                nAxis = 1;
            if( vBoxSize[2] > vBoxSize[nAxis] )
                nAxis = 2;

            // sort the objects along this axis
            ObjectSorter<0> sortx; ObjectSorter<1> sorty; ObjectSorter<2> sortz;
            switch( nAxis )
            {
            case 0: std::sort( pObjects, pObjects + nObjects, sortx ); break;
            case 1: std::sort( pObjects, pObjects + nObjects, sorty ); break;
            case 2: std::sort( pObjects, pObjects + nObjects, sortz ); break;
            };

            // split the object list in half
            Object** pLeftObjects = pObjects;
            Object** pRightObjects = pObjects + nObjects/2;
            obj_id nLeftObjects  = nObjects/2;
            obj_id nRightObjects = nObjects - (nObjects/2);
            
            // compute bounding boxes for each side
            AxisAlignedBox leftBox  = pLeftObjects[0]->box;
            AxisAlignedBox rightBox = pRightObjects[0]->box;
            for( uint32 i=1; i<nLeftObjects; i++ )
                leftBox.Merge( pLeftObjects[i]->box );
            for( uint32 i=1; i<nRightObjects; i++ )
                rightBox.Merge( pRightObjects[i]->box );
            
            // build the subtrees
            std::pair<NodeHandle,NodeHandle> children = pTree->MakeInnerNode( pNode, nAxis );
            pTree->SetNodeAABB( pNode, *pRootBox );

            uint32 nLeftDepth = MedianCut( pLeftObjects, nLeftObjects, pTree, children.first, &leftBox, 
                                           pObjectRemap, nFirstObject, nDepth+1 );

            uint32 nRightDepth = MedianCut( pRightObjects, nRightObjects, pTree, children.second, &rightBox, 
                                            pObjectRemap + nLeftObjects, nFirstObject + nLeftObjects, nDepth+1 );
                    
            return Max( nLeftDepth, nRightDepth );
        }
    }


}
