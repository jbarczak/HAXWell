//=====================================================================================================================
//
//   TRTAABBTree.inl
//
//   Implementation of class: TinyRT::AABBTree
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
    //=====================================================================================================================
    template< class ObjectSet_T >
    inline AABBTree<ObjectSet_T>::AABBTree( ) :
        m_pNodes( NULL ),
        m_nNodesInUse( 0 )   // the root node is considered 'in use' 
    {
    };


    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// This method is called by tree builders at the start of tree construction.  This method signals that a new tree
    ///   is being built, and calling it causes any pointers to pre-existing nodes to be invalidated
    ///
    /// \param rBox         The root bounding box of the entire scene
    /// \param nMaxNodes    The number of nodes that the tree should allocate
    /// \return A pointer to the root node of the tree
    //=====================================================================================================================
    template< typename ObjectSet_T >
    inline typename AABBTree<ObjectSet_T>::Node* AABBTree<ObjectSet_T>::Initialize( const AxisAlignedBox& rBox, uint32 nMaxNodes )
    {
        // TODO:  Don't reallocate if we already have enough
        if( m_pNodes ) 
        {
            delete[] m_pNodes;
            m_pNodes = NULL;
        }

        m_pNodes = new Node[ nMaxNodes ];
        m_nNodesInUse = 1;
        return m_pNodes;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< typename ObjectSet_T >
    inline std::pair< typename AABBTree<ObjectSet_T>::Node*,
                      typename AABBTree<ObjectSet_T>::Node* > AABBTree<ObjectSet_T>::MakeInnerNode( Node* n, uint32 nAxis )
    {       
        
        uint32 nChild = m_nNodesInUse;
        m_nNodesInUse += 2;
        n->MakeInnerNode( nChild, nAxis );
        
        return std::pair<Node*,Node*>( m_pNodes + nChild, m_pNodes + nChild + 1 );
    };

    //=====================================================================================================================
    /// Constructs a tree in-place
    /// \param pObjects     The set of objects to build the tree around.  
    /// \param rBuilder     An AABB builder to use to construct the tree
    //=====================================================================================================================
    template< typename ObjectSet_T >
    template< typename AABBTreeBuilder_T >
    void AABBTree<ObjectSet_T>::Build( ObjectSet_T* pObjects, AABBTreeBuilder_T& rBuilder )
    {
        m_nStackDepth = rBuilder.BuildTree( pObjects, this );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< typename ObjectSet_T >
    void AABBTree<ObjectSet_T>::GetMemoryUsage( size_t& rnBytesUsed, size_t& rnBytesAllocated ) const
    {
        rnBytesUsed = m_nNodesInUse*sizeof(Node);
        rnBytesAllocated = m_nNodesInUse*sizeof(Node);
    }


    //=====================================================================================================================
    //
    //           Protected Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================
}
