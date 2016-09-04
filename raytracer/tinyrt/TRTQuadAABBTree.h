//=====================================================================================================================
//
//   TRTQuadAABBTree.h
//
//   Definition of class: TinyRT::QuadAABBTree
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_QUADAABBTREE_H_
#define _TRT_QUADAABBTREE_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A 'QBVH' AABB tree, which stores four children per node and allows SIMD single-ray traversal
    ///
    /// This class implements the QuadAABBTree_C concept.
    ///
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    //=====================================================================================================================
    template< class ObjectSet_T >
    class QuadAABBTree
    {
    public:

        typedef ObjectSet_T ObjectSet;
        typedef typename ObjectSet_T::obj_id     obj_id;
        
        typedef uint32 NodeHandle;
        typedef uint32 ConstNodeHandle;

    public:

        static const uint32 BRANCH_FACTOR = 4;

        QuadAABBTree( );

        inline ~QuadAABBTree();


        /// Called at the start of tree construction.  Returns a reference to the QBVH root
        /// QBVH trees always have a single QBVH node as their root
        inline NodeHandle Initialize( const AxisAlignedBox& rBox ) { m_nNodesInUse=1; return 0; };

        /// Returns the maximum depth of the tree
        inline uint32 GetStackDepth() const { return m_nStackDepth; };

        /// Returns a reference to the root node
        inline NodeHandle GetRoot() const { return 0; };

        /// Tests whether or not a node is a leaf
        inline bool IsNodeLeaf( NodeHandle n ) const { return n >= 0x80000000; };

        /// Returns the range of objects stored in a leaf node
        inline void GetNodeObjectRange( NodeHandle n, obj_id& rFirst, obj_id& rLast ) const;

        /// Returns the number of objects stored in a leaf node
        inline obj_id GetNodeObjectCount( NodeHandle n ) const {
            obj_id last, first;
            GetNodeObjectRange( n, first, last );
            return last-first;
        };

        /// Returns the number of children of a node
        inline size_t GetChildCount( NodeHandle n ) const { return IsNodeLeaf( n ) ? 0 : 4; };

        /// Returns the 'N'th child of a node
        inline NodeHandle GetChild( NodeHandle n, size_t i ) const {
            const Node* pN = LookupNode( n );
            return pN->m_children[i];
        };

        /// Subdivides a child node into four children, and returns a reference to the subdivided child
        inline NodeHandle SubdivideChild( NodeHandle nNode, uint32 nChild );

        /// Sets the split axes that are used for ordered traversal of a particular node
        inline void SetSplitAxes( NodeHandle nNode, uint32 nA0, uint32 nA1, uint32 nA2 );
    
        /// Turns a child of a QBVH node into a leaf
        inline void CreateLeafChild( NodeHandle nNode, uint32 nChildIdx, obj_id nFirstObject, obj_id nObjects ); 

        /// Turns a child of a QBVH node into an empty leaf
        inline void CreateEmptyLeafChild( NodeHandle pNode, uint32 nChildIdx );
   
        /// Sets the stored AABB for one of a node's children
        inline void SetChildAABB( NodeHandle nNode, uint32 nChildIdx, const AxisAlignedBox& rBox );
   
        /// Performs a ray intersection test against the children of a node, pushing any hit nodes onto the given stack
        template< class Ray_T >
        TRT_FORCEINLINE NodeHandle* RayIntersectChildren( NodeHandle nNode, const SimdVec4f vSIMDRay[6], const Ray_T& rRay, NodeHandle* pStack, const int nDirSigns[4] ) const;


        /// Returns the memory consumption of the data structure, as well as the amount allocated
        inline void GetMemoryUsage( size_t& rnBytesUsed, size_t& rnBytesAllocated ) const;

        /// Constructs a tree for an object set
        template< class QAABBBuilder_T >
        inline void Build( ObjectSet_T* pObjects, QAABBBuilder_T& rBuilder );



        /// Returns a mask where each bit is 0 if the corresponding child is an empty leaf node, and 1 otherwise (LSB to MSB)
        inline uint GetEmptyLeafMask( NodeHandle nNode ) const { 
            TRT_ASSERT( !IsNodeLeaf(nNode) );
            return LookupNode(nNode)->m_intersectMask; 
        };

        /// \brief Returns an value indicating the order of node traversals.  
        /// The return value is a set of two-bit child indices, with the lowest order bits giving the index of the LAST child to traverse
        inline uint8 GetChildTraversalOrder( NodeHandle nNode, uint nRayOctant ) const {
            TRT_ASSERT( !IsNodeLeaf(nNode) );
            return LookupNode(nNode)->m_traversalOrder[nRayOctant];
        };
    
        inline float* GetChildAABBs( NodeHandle nNode ) const {
            TRT_ASSERT( !IsNodeLeaf(nNode) );
            return LookupNode(nNode)->m_bbox[0].values;
        };

        /// \brief Retrieves the AABB of a child of a node
        /// \param nNode        Must be an inner node
        /// \param nChildIdx    Index of the child node
        /// \param rBoxOut      Receives the bounding box
        inline void GetChildAABB( NodeHandle nNode, uint nChildIdx, AxisAlignedBox& rBoxOut ) const {
            TRT_ASSERT( nChildIdx < BRANCH_FACTOR );

            Node* pN = LookupNode(nNode);
            float* pBBox = reinterpret_cast<float*>( &pN->m_bbox[0] );
            rBoxOut.Min()[0] = pBBox[nChildIdx]; pBBox += 4;
            rBoxOut.Max()[0] = pBBox[nChildIdx]; pBBox += 4;
            rBoxOut.Min()[1] = pBBox[nChildIdx]; pBBox += 4;
            rBoxOut.Max()[1] = pBBox[nChildIdx]; pBBox += 4;
            rBoxOut.Min()[2] = pBBox[nChildIdx]; pBBox += 4;
            rBoxOut.Max()[2] = pBBox[nChildIdx]; pBBox += 4;
        }

    private:

        static const uint32 EMPTY_LEAF = 0x80000000;

        /// Inner node data structure
        struct Node
        {
            SimdVec4f m_bbox[6];        ///< x (min/max) y(min/max) z(min/max)    
            NodeHandle m_children[4];      ///< Node references.  The high bit indicates whether they point to inner nodes or leaves 
            uint8 m_traversalOrder[8];  ///< Precomputed traversal ordering for each possible ray octant
            uint32 m_intersectMask;     ///< Mask which is 0 for empty leaf children, 1 otherwise.  Used to avoid visiting empty leaves
        };

        /// Leaf information
        struct LeafObjects
        {
            obj_id nFirstObj;
            obj_id nLastObj;
        };


        /// Allocates a QBVH node
        inline NodeHandle BuyNode()
        {
            if( m_nNodesInUse == m_nNodeArraySize )
            {
                // reallocate
                Node* pNewNodes = reinterpret_cast<Node*>( AlignedMalloc( sizeof(Node)*m_nNodeArraySize*2, SimdVec4f::ALIGN ) );
                memcpy( pNewNodes, m_pNodes, sizeof(Node)*m_nNodesInUse );
                AlignedFree( m_pNodes );
                m_pNodes = pNewNodes;
                m_nNodeArraySize *= 2;
            }

            return m_nNodesInUse++;
        }

        /// Allocates leaf information
        inline NodeHandle BuyLeaf()
        {
            if( m_nLeafsInUse == m_nLeafArraySize )
            {
                LeafObjects* pNewLeafs = new LeafObjects[m_nLeafArraySize*2];
                memcpy( pNewLeafs, m_pLeafObjects, sizeof(LeafObjects)*m_nLeafsInUse );
                delete[] m_pLeafObjects;
                m_pLeafObjects = pNewLeafs;
                m_nLeafArraySize*=2;
            }

            m_nLeafsInUse++;
            uint32 n = m_nLeafsInUse-1;
            return ( n | 0x80000000 );
        };

        /// Obtains a QBVH node pointer
        inline Node* LookupNode( NodeHandle nNode ) const
        {
            TRT_ASSERT( !IsNodeLeaf( nNode ) );
            return m_pNodes + nNode;
        }

        /// Obtains a leaf pointer
        inline LeafObjects* LookupLeaf( NodeHandle nNode ) 
        {
            TRT_ASSERT( IsNodeLeaf( nNode ) );
            return &m_pLeafObjects[nNode & 0x7fffffff ];
        }
        inline const LeafObjects* LookupLeaf( NodeHandle nNode ) const 
        {
            TRT_ASSERT( IsNodeLeaf( nNode ) );
            return &m_pLeafObjects[nNode & 0x7fffffff ];
        }

        LeafObjects* m_pLeafObjects;
        uint32 m_nLeafArraySize;
        uint32 m_nLeafsInUse;

        Node* m_pNodes;
        uint32 m_nNodeArraySize;
        uint32 m_nNodesInUse;

        uint32 m_nStackDepth;
    };
}


#include "TRTQuadAABBTree.inl"

#endif // _TRT_QUADAABBTREE_H_
