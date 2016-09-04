//=====================================================================================================================
//
//   TRTSahAABBTreeBuilder.inl
//
//   Implementation of class: TinyRT::SahAABBTreeBuilder
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


    template< typename ObjectSet_T, typename CostFunction_T, typename LeafPolicy_T >
    SahAABBTreeBuilder<ObjectSet_T, CostFunction_T,LeafPolicy_T>::SahAABBTreeBuilder( const CostFunction_T& rCost  ) 
        : m_costFunc(rCost)
    {
    }

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

  
    //=====================================================================================================================
    /// \param pObjects     Object set for which the tree is constructed
    /// \param pTree        The tree to be constructed.  
    /// \param rCostFunc    Per-object cost function.  The return value is the cost of intersection testing a given object,
    ///                       relative to the cost of a node traversal
    /// \return The maximum depth of the constructed tree (0 is the depth of the root)
    //=====================================================================================================================
    template< typename ObjectSet_T, typename CostFunction_T, typename LeafPolicy_T >
    template< typename AABBTree_T >
    uint32 SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::BuildTree( ObjectSet* pObjects, AABBTree_T* pTree )
    {
        typedef typename AABBTree_T::NodeHandle NodeHandle;
     
        obj_id nObjects = pObjects->GetObjectCount();
       
        // object information
        std::vector< Object >  objects;
        std::vector< Object* > objectPtrs[3];
        AxisAlignedBox globalBox;
        SetupObjectInfo( pObjects, objects, objectPtrs, globalBox );
        
        // initialize tree
        NodeHandle pRoot = pTree->Initialize( globalBox, 2*nObjects - 1 );

        // build the tree
        Object** objectsByAxis[3] = { &(objectPtrs[0][0]), &(objectPtrs[1][0]), &(objectPtrs[2][0]) };
        uint32 nDepth = BuildRecurse( 0, objectsByAxis, nObjects, pTree, pRoot, globalBox, 0 );

        // free up some memory.  We only need one set of pointers now 
        objectPtrs[0].clear();
        objectPtrs[1].clear();

        // construct an object remapping table
        ScopedArray<obj_id> objectRemap( new obj_id[nObjects] );
        for( size_t i=0; i<nObjects; i++ )
        {
            objectRemap[i] = objectPtrs[2][i]->nID;
        }

        // don't need these anymore, so free up some memory
        objects.clear();
        objectPtrs[2].clear();

        // put the objects in the right order
        pObjects->RemapObjects( &objectRemap[0] );
        return nDepth;
    }


    template< typename ObjectSet_T, typename CostFunction_T, typename LeafPolicy_T  >
    template< typename QAABBTree_T >
    uint32 SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::BuildQuadAABBTree( ObjectSet* pObjects, QAABBTree_T* pTree )
    {
        typedef typename QAABBTree_T::NodeHandle NodeHandle;
        
        obj_id nObjects = pObjects->GetObjectCount();
       
        // object information
        std::vector< Object >  objects;
        std::vector< Object* > objectPtrs[3];
        AxisAlignedBox globalBox;
        SetupObjectInfo( pObjects, objects, objectPtrs, globalBox );
        
        // initialize tree
        NodeHandle pRoot = pTree->Initialize( globalBox );

        // build the tree
        
        Object** objectsByAxis[3] = { &(objectPtrs[0][0]), &(objectPtrs[1][0]), &(objectPtrs[2][0]) };

        obj_id nObjectsLeft;
        obj_id nObjectsRight;
        AxisAlignedBox leftBox;
        AxisAlignedBox rightBox;
        Object** objectsRight[3];
        int nAxis0 = SplitObjects( 0, objectsByAxis, globalBox, nObjects, objectsRight, nObjectsLeft, nObjectsRight, leftBox, rightBox );
        
        uint32 nDepth = 1;
        if( nAxis0 == -1 )
        {
            // this means its better not to split at all, but to just create a flat list
            pTree->SetChildAABB( pRoot, 0, globalBox );
            pTree->CreateLeafChild( pRoot, 0, 0, (obj_id) nObjects );
            pTree->CreateEmptyLeafChild( pRoot, 1 );
            pTree->CreateEmptyLeafChild( pRoot, 2 );
            pTree->CreateEmptyLeafChild( pRoot, 3 );
            return 1;
        }
        else
        {
            uint32 nAxis1, nAxis2;
            uint32 nDepthLeft = BuildQAABBRecurse_Odd( 0,objectsByAxis, nObjectsLeft, pTree, pRoot, 0, leftBox, 0, nAxis1 );
            uint32 nDepthRight = BuildQAABBRecurse_Odd( 0,objectsRight, nObjectsRight, pTree,pRoot, 2, rightBox, nObjectsLeft, nAxis2 );
            pTree->SetSplitAxes( pRoot, nAxis0, nAxis1, nAxis2 );

            uint32 nDepth = 1 + Max( nDepthLeft, nDepthRight );
            
            // free up some memory.  We only need one set of pointers now 
            objectPtrs[0].clear();
            objectPtrs[1].clear();

            // construct an object remapping table
            std::vector<obj_id> objectRemap;
            objectRemap.resize( nObjects );
            for( size_t i=0; i<nObjects; i++ )
            {
                objectRemap[i] = objectPtrs[2][i]->nID;
            }

            // don't need these anymore, so free up some memory
            objects.clear();
            objectPtrs[2].clear();

            // put the objects in the right order
            pObjects->RemapObjects( &objectRemap[0] );
            return nDepth;
        }        
    }

    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// Preprocesses the objects in the object set to prepare for tree construction
    /// \param pObjects     The object set
    /// \param objects      Array which is filled with object info structures
    /// \param objectPtrs   Arrays which are filled with pointers into the array 'objects', sorted on each of the three axes
    /// \param rGlobalBB    Receives the bounding box of all objects in the object set
    //=====================================================================================================================
    template< class ObjectSet_T, class CostFunction_T, typename LeafPolicy_T >
    void SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::SetupObjectInfo( ObjectSet* pObjects, std::vector<Object>& objects, 
                                                                           std::vector<Object*> objectPtrs[3], AxisAlignedBox& rGlobalBB )
    {
        obj_id nObjects = pObjects->GetObjectCount();
       
        // object information
        objects.resize( nObjects );
        objectPtrs[0].resize( nObjects );
        objectPtrs[1].resize( nObjects );
        objectPtrs[2].resize( nObjects );

        // initialize the first object's information
        pObjects->GetObjectAABB( 0, objects[0].box );
        objects[0].nID = 0;
        objectPtrs[0][0] = &objects[0];
        objectPtrs[1][0] = &objects[0];
        objectPtrs[2][0] = &objects[0];

        rGlobalBB = objects[0].box;

        // process subsequent objects
        for( obj_id i=1; i<nObjects; i++ )
        {
            pObjects->GetObjectAABB( i, objects[i].box );
            objects[i].nID = i;
            objectPtrs[0][i] = &objects[i];
            objectPtrs[1][i] = &objects[i];
            objectPtrs[2][i] = &objects[i];
        
            // compute global AABB as we go
            rGlobalBB.Merge( objects[i].box );
        }

        // sort the pointer lists
        SortObjects<0> x; SortObjects<1> y; SortObjects<2> z;
        std::sort( objectPtrs[0].begin(), objectPtrs[0].end(), x );
        std::sort( objectPtrs[1].begin(), objectPtrs[1].end(), y );
        std::sort( objectPtrs[2].begin(), objectPtrs[2].end(), z );
        
        // fill in the sort indices in the object structures
        typename std::vector<Object*>::iterator itX = objectPtrs[0].begin();
        typename std::vector<Object*>::iterator itY = objectPtrs[1].begin();
        typename std::vector<Object*>::iterator itZ = objectPtrs[2].begin();
        for( obj_id i=0; i<nObjects; i++ )
        {
            (*itX)->nSortIndices[0] = i;
            (*itY)->nSortIndices[1] = i;
            (*itZ)->nSortIndices[2] = i;
            ++itX;
            ++itY;
            ++itZ;
        }
    }


 
    //=====================================================================================================================
    /// \param objectsByAxis    Lists of objects, sorted along each axis.  These will be partitioned if a split is chosen
    /// \param nRecursionDepth  Current depth in the tree
    /// \param rBox             Root AABB of subtree being constructed
    /// \param nObjects         Number of objects
    /// \param objectsRight     Array which receives pointers to the objects on the right side, sorted along each axis
    /// \param rnObjectsLeft     Receives the number of objects on the left side
    /// \param rnObjectsRight    Receives the number of objects on the right side
    /// \param rLeftBox         Receives the left side AABB
    /// \param rRightBox        Receives the right side AABB
    /// \return The axis on which the objects are split.  -1 if it is decided not to split
    //=====================================================================================================================
    template< class ObjectSet_T, class CostFunction_T, class LeafPolicy_T >
    int SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::SplitObjects( uint nRecursionDepth, 
                                                                       Object** objectsByAxis[3],
                                                                       const AxisAlignedBox& rBox,
                                                                       obj_id nObjects,
                                                                       Object** objectsRight[3],
                                                                       obj_id&  rnObjectsLeft,
                                                                       obj_id&  rnObjectsRight,
                                                                       AxisAlignedBox& rLeftBox,
                                                                       AxisAlignedBox& rRightBox )
    {
        if( nObjects == 1 )
            return -1; // do not split a single object

        TRT_ASSERT( nObjects > 1 );
        
        Vec3f vBoxSizes = rBox.Max() - rBox.Min();
        float fRootArea = ( vBoxSizes.x * ( vBoxSizes.y + vBoxSizes.z ) + vBoxSizes.y*vBoxSizes.z );
        float fInvRootArea = 1.0f / fRootArea;

        // split information, initialized to the cost of creating a leaf
        float fBestCost =0;
        int nSplitAxis = -1;
        obj_id nSplitId = 0;
        AxisAlignedBox splitRightBox;

        // left subtree costs resulting from putting all objects > i on the right side 
        //   - first entry is the cost of having only the leftmost object on the left side
        //   - last entry is junk
        std::vector<float> leftCosts; 
        leftCosts.resize( nObjects );


        for( int axis=0; axis<3; axis++ )
        {
            float fTotalCost = 0;

            // sweep left, compute left subtree costs
            AxisAlignedBox leftBox = AxisAlignedBox( Vec3f( std::numeric_limits<float>::max() ), 
                                                     Vec3f( -std::numeric_limits<float>::max() ) );
            for( obj_id i=0; i<nObjects; i++ )
            {
                leftBox.Merge( objectsByAxis[axis][i]->box );                   
                
                Vec3f vLeftSize = leftBox.Max() - leftBox.Min();
                float fLeftArea = ( vLeftSize.x * ( vLeftSize.y + vLeftSize.z ) + vLeftSize.y*vLeftSize.z );
                fTotalCost +=  m_costFunc( objectsByAxis[axis][i]->nID );
                leftCosts[i] = fLeftArea*fTotalCost;
            }

            // sweep right, compute subtree costs, and select a split
            AxisAlignedBox rightBox = objectsByAxis[axis][nObjects-1]->box;
            
            
            // we now have the sum of the object ISect costs.  Use that as the 'no-split' cost
            if( axis == 0 )
                fBestCost = LeafPolicy_T::AdjustLeafCost(nRecursionDepth,nObjects,fTotalCost); 

            fTotalCost = 0;

            for( obj_id nObjectsRight = 1; nObjectsRight < nObjects; nObjectsRight++ )
            {
                obj_id i = (nObjects - nObjectsRight) - 1; 
                Object** it = objectsByAxis[axis] + i;
                
                Vec3f vRightSize = rightBox.Max() - rightBox.Min();
                float fRightArea = ( vRightSize.x * ( vRightSize.y + vRightSize.z ) + vRightSize.y*vRightSize.z );

                fTotalCost += m_costFunc( (*it)->nID );
                float fCost = 2.0f + ( leftCosts[i] + (fRightArea*fTotalCost)) * fInvRootArea ;
                
                // if this split is better than the previous one, save it
                if( fCost < fBestCost )
                {
                    fBestCost = fCost;
                    nSplitAxis = axis;
                    nSplitId = (*it)->nSortIndices[axis];
                    splitRightBox = rightBox;        
                }
        
                rightBox.Merge( (*it)->box ); 
            }
        }

        // we have now figured out what to do (split or not split)
        if( nSplitAxis != -1 )
        {
            // split
            rLeftBox = AxisAlignedBox( Vec3f( std::numeric_limits<float>::max() ), 
                                       Vec3f( -std::numeric_limits<float>::max() ) );
                
            PartitionObjects partF( nSplitAxis, nSplitId );
            PartitionAndComputeBox partBoxF( nSplitAxis, nSplitId, &rLeftBox );
            
            // partition the three sorted object lists, and get pointers to the first object on the right side 
            //  The first partitioning pass also computes the AABB of the left side. 
            /// The AABB of the right side was computed earlier during split selection
            
            objectsRight[0] = std::stable_partition( objectsByAxis[0], objectsByAxis[0] + nObjects, partBoxF );
            objectsRight[1] = std::stable_partition( objectsByAxis[1], objectsByAxis[1] + nObjects, partF );
            objectsRight[2] = std::stable_partition( objectsByAxis[2], objectsByAxis[2] + nObjects, partF );

            rnObjectsLeft = static_cast<obj_id>( objectsRight[0] - objectsByAxis[0] );
            rnObjectsRight = nObjects - rnObjectsLeft;

            rRightBox = splitRightBox;
        }

        return nSplitAxis;
    }


    //=====================================================================================================================
    /// \param objectsByAxis    Lists of objects, sorted along each axis
    /// \param pTree            The tree being constructed
    /// \param pNode            The root of the subtree being constructed
    /// \param rBox             Bounding box of the objects in this subtree
    /// \param pObjectRemap     Array which is filled with object ids in tree order
    /// \param nFirstObject     Index of the first object in this subtree (in the reordered object set)
    //=====================================================================================================================
    template< class ObjectSet_T, typename CostFunction_T, typename LeafPolicy_T >
    template< class AABBTree_T >
    uint32 SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::BuildRecurse(  uint nRecursionDepth,
                                                                          Object** objectsByAxis[3], 
                                                                          obj_id nObjects, 
                                                                          AABBTree_T* pTree, 
                                                                          typename AABBTree_T::NodeHandle pNode,
                                                                          const AxisAlignedBox& rBox, obj_id nFirstObject )
    {
        typedef typename AABBTree_T::NodeHandle NodeHandle;
                                                          
        pTree->SetNodeAABB( pNode, rBox );

        if( nObjects == 1 )
        {
            // only one object, make a leaf
            pTree->MakeLeafNode( pNode, nFirstObject, (obj_id) nObjects );
            return 1;
        }


        AxisAlignedBox leftBox;
        AxisAlignedBox rightBox;
        obj_id nObjectsLeft;
        obj_id nObjectsRight;
        Object** objectsRight[3];
       
        int nSplitAxis = SplitObjects( nRecursionDepth, objectsByAxis, rBox, nObjects, objectsRight, nObjectsLeft, nObjectsRight, leftBox, rightBox );
        if( nSplitAxis != -1 )
        {
            // objects were split.  Make an inner node and keep going
            std::pair<NodeHandle,NodeHandle> nodes = pTree->MakeInnerNode( pNode, nSplitAxis );
            NodeHandle pLeft = nodes.first;
            NodeHandle pRight = nodes.second;

            uint32 nDepthLeft = BuildRecurse( nRecursionDepth+1, objectsByAxis, nObjectsLeft, pTree, pLeft, leftBox, nFirstObject );
            uint32 nDepthRight = BuildRecurse( nRecursionDepth+1, objectsRight, nObjectsRight, pTree, pRight, rightBox, nFirstObject + nObjectsLeft );

            return 1 + Max( nDepthLeft, nDepthRight );
        }
        else
        {
            // no split, make a leaf
            pTree->MakeLeafNode( pNode, nFirstObject, (obj_id) nObjects );
            return 1;
        }
    }



    template< class ObjectSet_T, typename CostFunction_T, typename LeafPolicy_T >
    template< class QAABBTree_T >
    uint32 SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::BuildQAABBRecurse_Even( uint nRecursionDepth,
                                                                                    Object** objectsByAxis[3], obj_id nObjects, QAABBTree_T* pTree, 
                                                                                    typename QAABBTree_T::NodeHandle pNode,
                                                                                    uint32 nChild,
                                                                                    const AxisAlignedBox& rBox, obj_id nFirstObject )
    {
        typedef typename QAABBTree_T::NodeHandle NodeHandle;
        
        // if objects == 1, then make a leaf 
        if( nObjects == 1 )
        {
            // make a leaf 
            pTree->SetChildAABB( pNode, nChild, rBox );
            pTree->CreateLeafChild( pNode, nChild, nFirstObject, (obj_id) nObjects );
            return 1;
        }

        // primary split
        AxisAlignedBox leftBox;
        AxisAlignedBox rightBox;
        obj_id nObjectsLeft;
        obj_id nObjectsRight;
        Object** objectsRight[3];
        int nAxis0 =  SplitObjects( nRecursionDepth, objectsByAxis, rBox, nObjects, objectsRight, nObjectsLeft, nObjectsRight, leftBox, rightBox );
        if( nAxis0 != -1 )
        {
            // it makes sense to subdivide the objects, split this child node and build subtrees recursively
            NodeHandle pLeaf = pTree->SubdivideChild( pNode, nChild );

            uint32 nAxis1, nAxis2;
            uint32 nDepthLeft = BuildQAABBRecurse_Odd( nRecursionDepth, objectsByAxis, nObjectsLeft, pTree, pLeaf, 0, leftBox, nFirstObject, nAxis1 );
            uint32 nDepthRight = BuildQAABBRecurse_Odd( nRecursionDepth, objectsRight, nObjectsRight, pTree, pLeaf, 2, rightBox, nFirstObject + nObjectsLeft, nAxis2 );
            
            pTree->SetSplitAxes( pLeaf, nAxis0, nAxis1, nAxis2 );

            return 1 + Max( nDepthLeft, nDepthRight );
        }
        else
        {
            // it doesn't make sense to subdivide the objects, so make a leaf
            pTree->SetChildAABB( pNode, nChild, rBox );
            pTree->CreateLeafChild( pNode, nChild, nFirstObject, nObjects );
            return 1;
        }
    }

    template< class ObjectSet_T, typename CostFunction_T, typename LeafPolicy_T >
    template< class QAABBTree_T >
    uint32 SahAABBTreeBuilder<ObjectSet_T,CostFunction_T,LeafPolicy_T>::BuildQAABBRecurse_Odd( uint nRecursionDepth,
                                                                                   Object** objectsByAxis[3], 
                                                                                   obj_id nObjects, 
                                                                                   QAABBTree_T* pTree, 
                                                                                   typename QAABBTree_T::NodeHandle pNode,
                                                                                   uint32 nChild,
                                                                                   const AxisAlignedBox& rBox, obj_id nFirstObject, uint32& rSplitAxis )
    {
        typedef typename QAABBTree_T::NodeHandle NodeHandle;
       
        // if objects == 1, then make a leaf on the left, and an empty node on the right 
        if( nObjects == 1 )
        {
            pTree->SetChildAABB( pNode, nChild, rBox );
            pTree->CreateLeafChild( pNode, nChild, nFirstObject, nObjects );
            pTree->CreateEmptyLeafChild( pNode, nChild+1 );       
            rSplitAxis=0;
            return 0;
        }

        // split
        AxisAlignedBox leftBox;
        AxisAlignedBox rightBox;
        obj_id nObjectsLeft;
        obj_id nObjectsRight;
        Object** objectsRight[3];
    
        int nAxis =  SplitObjects( nRecursionDepth, objectsByAxis, rBox, nObjects, objectsRight, nObjectsLeft, nObjectsRight, leftBox, rightBox );
        if( nAxis != -1 )
        {
            // recursively build on either side of the node
            pTree->SetChildAABB( pNode, nChild, rBox );
            pTree->SetChildAABB( pNode, nChild+1, rBox );
            
            uint32 nDepth1 = BuildQAABBRecurse_Even( nRecursionDepth+1, objectsByAxis, nObjectsLeft, pTree, pNode, nChild, leftBox, nFirstObject );
            uint32 nDepth2 = BuildQAABBRecurse_Even( nRecursionDepth+1, objectsRight, nObjectsRight, pTree, pNode, nChild+1, rightBox, nFirstObject+nObjectsLeft );
            rSplitAxis = nAxis;
            return Max(nDepth1,nDepth2);
        }
        else
        {
            // make a leaf on the left, empty node on right
            pTree->SetChildAABB( pNode, nChild, rBox );
            pTree->CreateLeafChild( pNode, nChild, nFirstObject, nObjects );
            pTree->CreateEmptyLeafChild( pNode, nChild+1 );
            rSplitAxis=0;
            return 0;
        }       
    }


}
