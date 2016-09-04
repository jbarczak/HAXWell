//=====================================================================================================================
//
//   TRTKDTree.inl
//
//   Implementation of class: TinyRT::KDTree
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
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
    
    template< class ObjectSet_T >
    KDTree<ObjectSet_T>::KDTree() :
        m_nNodesInUse(0), 
        m_nObjectRefsInUse(0), 
        m_nArraySize(0),
        m_nArrayInUse(0)
    {
    }

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    typename KDTree<ObjectSet_T>::NodeHandle KDTree<ObjectSet_T>::Initialize( const AxisAlignedBox& rRootAABB )
    {
        // prime the array on first use
        if( m_nArraySize < sizeof(Node) )
        {
            m_nArraySize = 64*1024;
            m_array.reallocate( m_nArraySize );
        }

        // set up an empty root node
        reinterpret_cast<Node*>( &m_array[0] )->MakeLeafNode(0,0);
        m_nArrayInUse = sizeof(Node);
        m_nNodesInUse = 1;
        m_nObjectRefsInUse = 0;
        m_aabb = rRootAABB;
        return 0;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    std::pair< typename KDTree<ObjectSet_T>::NodeHandle, typename KDTree<ObjectSet_T>::NodeHandle > 
        KDTree<ObjectSet_T>::MakeInnerNode( NodeHandle hNode, float fSplitPlane, uint nSplitAxis )
    {
        uint nLeftOffs = m_nArrayInUse; // store offset of left child

        // buy more nodes
        m_nNodesInUse += 2;
        m_nArrayInUse += 2*sizeof(Node);
        if( m_nArraySize < m_nArrayInUse )
        {
            uint nNewSize = Max( m_nArraySize*2, m_nArrayInUse );
            m_array.resize( nNewSize, m_nArraySize );
            m_nArraySize = nNewSize;
        }

        Node* pRoot = NodeFromHandle( hNode );
        pRoot->MakeInnerNode( nLeftOffs, fSplitPlane, nSplitAxis );

        return std::pair< NodeHandle, NodeHandle > (  nLeftOffs,  nLeftOffs + sizeof(Node) );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    typename KDTree<ObjectSet_T>::obj_id* KDTree<ObjectSet_T>::MakeLeafNode( typename KDTree<ObjectSet_T>::NodeHandle n, 
                                                                             typename KDTree<ObjectSet_T>::obj_id nObjCount )
    {
        uint nIndexOffs = m_nArrayInUse; // store offset of first index 

        // buy more object refs
        m_nObjectRefsInUse += nObjCount;

        uint nSize = nObjCount*sizeof(obj_id);
        nSize = (nSize + (sizeof(Node)-1)) & ~(sizeof(Node)-1); // round up to node size to prevent cacheline splitting of nodes
        m_nArrayInUse += nSize;

        if( m_nArraySize < m_nArrayInUse )
        {
            uint nNewSize = Max( m_nArraySize*2, m_nArrayInUse );
            m_array.resize( nNewSize, m_nArraySize );
            m_nArraySize = nNewSize;
        }

        NodeFromHandle(n)->MakeLeafNode( nIndexOffs, nObjCount );
        return reinterpret_cast<obj_id*>( &m_array[nIndexOffs] );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    template< class KDTreeBuilder_T >
    void KDTree<ObjectSet_T>::Build( ObjectSet_T* pObjects, KDTreeBuilder_T& rBuilder )
    {
        m_nStackDepth = rBuilder.BuildTree( pObjects, this );
    }
}

