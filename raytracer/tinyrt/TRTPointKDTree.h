//=====================================================================================================================
//
//   TRTPhotonKDTree..h
//
//   Definition of class: TinyRT::PhotonKDTree
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTPHOTONKDTREE_H_
#define _TRTPHOTONKDTREE_H_


namespace TinyRT
{


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A KDTree over point data
    ///
    ///  Implements the PointKDTree_C concept
    /// \sa PointKDTree_C
    /// \sa KNNSearchKDTree
    /// \sa MedianSplitPointKDTreeBuilder
    //=====================================================================================================================
    class PointKDTree
    {
    private:
        struct Node;

    public:

        typedef uint32 PointID;                 ///< Type used for a point index
        typedef uint32 PointCount;              ///< Type used for the point count in a leaf node
        typedef uint   NodeHandle;              ///< Non-const handle to a tree node
        typedef const Node* ConstNodeHandle;    ///< Const handle to a tree node

        inline PointKDTree( );

        inline ~PointKDTree( );

        /// Returns the AABB of the nodes in the tree
        inline const AxisAlignedBox& GetBoundingBox() const { return m_aabb; };

        /// Returns the left (less-than-equal) child of an inner node in this tree
        inline ConstNodeHandle GetLeftChild( ConstNodeHandle p ) const  {return &GetNodeChildren(p)[0];};

        /// Returns the right (greater-than) child of an inner node in this tree
        inline ConstNodeHandle GetRightChild( ConstNodeHandle p ) const { return &GetNodeChildren(p)[1]; };

        /// Returns the maximum depth of any node in the tree.  This is used to allocate stack space during traversal
        inline size_t GetStackDepth() const { return m_nStackDepth; };    
    
        /// Returns the point range for a leaf node
        inline void GetNodeObjectRange( ConstNodeHandle p, PointID& start, PointCount& count ) const {
            start = p->nFirstPoint;
            count = p->nPoints;
        };

        /// Returns the split axis (0,1,2 for a particular node)
        inline uint GetNodeSplitAxis( ConstNodeHandle p ) const { return p->nAxisAndChildren&AXIS_MASK; };

        /// Returns the split plane location for a particular node
        inline float GetNodeSplitPosition( ConstNodeHandle p ) const { return p->fSplit; };
        
        /// Tests whether or not a particular node is a leaf
        inline bool IsNodeLeaf( ConstNodeHandle p ) const { return !(p->nAxisAndChildren & INNER_MASK); };

        /// Returns a handle to the tree root
        inline ConstNodeHandle GetRoot() const { return m_pNodes; };
        
        /// \brief Returns a handle to one of a node's children.  
        /// May not be called on an inner node
        inline ConstNodeHandle GetChild( ConstNodeHandle n, size_t i ) const { return &GetNodeChildren(n)[i]; };
        
        /// \brief Returns the number of children of a node (generally a constant, but not necessarily).  
        inline size_t GetChildCount( ConstNodeHandle n ) const { return IsNodeLeaf(n) ? 0 : 2; };
        
        /// \brief Determines the number of object stored in the given node.  
        /// May not be called on an inner node
        inline int GetNodeObjectCount( ConstNodeHandle n ) const { return n->nPoints; };

        /// Constructs a tree for a point set, discarding any previous tree
        template< class Point_T, class PointKDTreeBuilder_T >
        inline void Build( Point_T* pPoints, uint nPoints, PointKDTreeBuilder_T& rBuilder );

        /// Clears existing nodes in the tree and creates a new, single-node tree with the specified bounding box, returns the root node
        inline NodeHandle Initialize( const AxisAlignedBox& rRootAABB, uint nMaxNodes );

        /// Subdivides a particular node, creating two empty leaf children for it
        inline std::pair<NodeHandle,NodeHandle> MakeInnerNode( NodeHandle hNode, float fSplitPlane, uint nSplitAxis );

        /// Turns the specified node into a leaf containing a range of object references
        inline void MakeLeafNode( NodeHandle n, uint32 nFirstObj, uint32 nObjCount ) ;

        /// Returns the memory usage of the tree
        inline void GetMemoryUsage( size_t& nMemUsed, size_t& nMemAllocated ) const
        {
            nMemUsed = m_nNodesInUse*sizeof(Node) + sizeof(PointKDTree);
            nMemAllocated = m_nNodes*sizeof(Node) + sizeof(PointKDTree);
        }

    private:

        
        struct Node
        {
            union
            {
                float fSplit;               ///< Split location (inner nodes)
                uint32 nFirstPoint;         ///< Index of the first point in the node (for leaves)
            };
            union
            {
                uint32 nAxisAndChildren;    ///< Split axis (bits 0,1).  High bit iS 1 forinner nodes.  Remaining bits are byte offset to children (8-byte aligned) 
                uint32 nPoints;             ///< Point count (leaves)
            };
        };


        enum
        {
            INNER_MASK = (1<<31),
            AXIS_MASK = 3,
            CHILDREN_MASK = ~(INNER_MASK | AXIS_MASK)
        };

        /// Retrieves the two children of a node
        inline const Node* GetNodeChildren( const Node* p) const { 
            TRT_ASSERT( !IsNodeLeaf(p) );
            return reinterpret_cast<const Node*>( reinterpret_cast<const uint8*>( p ) + (p->nAxisAndChildren & CHILDREN_MASK) );
        };

        /// Retrieves the two children of a node
        inline Node* GetNodeChildren( Node* p ) {
            TRT_ASSERT( !IsNodeLeaf(p) );
            return reinterpret_cast<Node*>( reinterpret_cast<uint8*>(p) + (p->nAxisAndChildren & CHILDREN_MASK) );
        };

        AxisAlignedBox m_aabb;  ///< AABB of the points in the tree
        uint m_nStackDepth;     ///< Amount of stack space needed to iteratively traverse the tree (in "stack entries")
        Node* m_pNodes;         ///< Node array.  Must be 8-byte aligned
        uint m_nNodes;          ///< Size of the node array
        uint m_nNodesInUse;     ///< Number of nodes in use in the current tree

    };
    
}

#include "TRTPointKDTree.inl"

#endif // _TRTPHOTONKDTREE_H_
