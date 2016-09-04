//=====================================================================================================================
//
//   TRTPointKDTree..inl
//
//   Definition of class: TinyRT::PointKDTree
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2010 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include "TRTPointKDTree.h"

namespace TinyRT
{

    
    //=====================================================================================================================
    //
    //         Constructors/Destructors
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    //=====================================================================================================================
    PointKDTree::PointKDTree() : m_nNodes(0), m_pNodes(0), m_nStackDepth(0), m_nNodesInUse(0)
    {
    }

    //=====================================================================================================================
    //=====================================================================================================================
    PointKDTree::~PointKDTree()
    {
        AlignedFree(m_pNodes);
    }

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    /// Constructs a KDTree for a set of points.  The previous tree is invalidated
    /// \param pPoints  Array of points for which to build the tree.  The points will be re-ordered to correspond
    ///                    to the ordering in the tree leaves
    /// \param nPoints  Number of points in the array
    /// \param rBuilder Tree builder to use for tree construction
    //=====================================================================================================================
    template< class Point_T, class PointKDTreeBuilder_T >
    void PointKDTree::Build( Point_T* pPoints, uint nPoints, PointKDTreeBuilder_T& rBuilder )
    {
        m_nStackDepth = rBuilder.Build( pPoints, nPoints, *this );
    }


    //=====================================================================================================================
    /// \param rRootAABB    AABB of the point set for which the tree is being built
    /// \param nMaxNodes    Expected number of nodes that will be created
    /// \return A handle to the root node
    //=====================================================================================================================
    PointKDTree::NodeHandle PointKDTree::Initialize( const AxisAlignedBox& rRootAABB, uint nMaxNodes )
    {
        // size up to the expected number of nodes
        if( m_nNodes < nMaxNodes )
        {
            AlignedFree( m_pNodes );
            m_pNodes = (Node*) AlignedMalloc( nMaxNodes*sizeof(Node), 8 );
            m_nNodes = nMaxNodes;
        }

        m_nStackDepth = 0;
        m_nNodesInUse = 1;
        m_aabb = rRootAABB;
        return 0;
    }

    //=====================================================================================================================
    /// \param  hNode       Node to be made inner
    /// \param fSplitPlane  Split plane location
    /// \param nSplitAxis   Split plane axis
    /// \return The node's children
    //=====================================================================================================================
    inline std::pair<PointKDTree::NodeHandle,PointKDTree::NodeHandle> 
        PointKDTree::MakeInnerNode( NodeHandle hNode, float fSplitPlane, uint nSplitAxis )
    {
        // grow the array as needed
        if( m_nNodesInUse > m_nNodes-2 )
        {
            m_nNodes *= 2;
            Node* pNodes = reinterpret_cast<Node*>(AlignedMalloc( m_nNodes*sizeof(Node), 8 ));
            memcpy( pNodes, m_pNodes, m_nNodesInUse*sizeof(Node) );
            AlignedFree(m_pNodes);
            m_pNodes = pNodes;
        }

        Node* pNode = &m_pNodes[hNode];
        uint nKids = m_nNodesInUse;
        m_nNodesInUse += 2;

        uint nByteOffset = (nKids-hNode)* sizeof(Node) ;
        TRT_ASSERT( !(nByteOffset & (INNER_MASK|AXIS_MASK)) );  // or it will blow up

        pNode->nAxisAndChildren = INNER_MASK | nByteOffset | nSplitAxis;
        pNode->fSplit = fSplitPlane;
        return std::pair<NodeHandle,NodeHandle>( nKids, nKids+1 );
    }

    //=====================================================================================================================
    /// \param n            Node to turn into a leaf
    /// \param nFirstObj    Index of first point in the leaf
    /// \param nObjCount    Number of points in the leaf
    //=====================================================================================================================
    inline void PointKDTree::MakeLeafNode( NodeHandle n, uint32 nFirstObj, uint32 nObjCount ) 
    {
        Node* pn = &m_pNodes[n];
        pn->nFirstPoint = nFirstObj;
        pn->nPoints = nObjCount;
        TRT_ASSERT( !(nObjCount & INNER_MASK) ); // or it will blow up...        
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


