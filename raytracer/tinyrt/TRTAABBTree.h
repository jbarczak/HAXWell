//=====================================================================================================================
//
//   TRTAABBTree.h
//
//   Definition of class: TinyRT::AABBTree
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_AABBTREE_H_
#define _TRT_AABBTREE_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A basic axis-aligned bounding box tree
    ///
    ///  An AABBTree is a binary BVH with axis aligned bounding boxes as its bounding volumes.
    ///   This class implements the AABBTree_C concept.
    ///
    /// \param ObjectSet_T must implement the ObjectSet_C concept
    //=====================================================================================================================
    template< class ObjectSet_T >
    class AABBTree
    {
    public:

        typedef ObjectSet_T ObjectSet;
        typedef typename ObjectSet_T::obj_id obj_id;

        class Node
        {
        public:
            
            inline bool IsLeaf() const { return m_nObjectCount >= 3; };
            inline uint32 GetLeftChildIndex() const {  TRT_ASSERT( !IsLeaf() ); return m_nFirstChild; };
            inline uint32 GetRightChildIndex() const { TRT_ASSERT( !IsLeaf() ); return m_nFirstChild+1; };

            /// Turns this node into an inner node
            /// \param nFirstChildIdx   Index of the node's first child
            /// \param nAxis            Split axis on which the objects were subdivided
            inline void MakeInnerNode( uint32 nFirstChildIdx, uint32 nAxis ) {
                m_nFirstChild = nFirstChildIdx; 
                m_nObjectCount = nAxis;
            };

            inline const AxisAlignedBox& GetAABB() const { return m_box; };
            inline void SetAABB( const AxisAlignedBox& rBox ) { m_box = rBox; };
            
            /// Returns the number of objects stored in this node (leaves only)
            inline uint32 GetObjectCount() const { return m_nObjectCount-3; };

            /// Returns the axis on which objects were split for this node during tree construction (inner nodes only)
            inline uint32 GetSplitAxis() const { 
                TRT_ASSERT( !IsLeaf() );
                return m_nObjectCount; 
            };

            /// Returns the range of objects stored in this leaf (leaves only)
            inline void GetObjectRange( obj_id& rStart, obj_id& rEnd ) const { 
                TRT_ASSERT( IsLeaf() );
                rStart = m_nFirstObject;
                rEnd   = rStart + (m_nObjectCount-3); 
            };

            /// Turns this node into a leaf containing the specified range of objects
            inline void MakeLeaf( obj_id nFirst, obj_id nCount ) {
                m_nFirstObject = nFirst;
                m_nObjectCount = nCount+3;
            };

        private:

            AxisAlignedBox m_box;
            union
            {
                uint32 m_nFirstChild;   ///< Index of the left child (if an inner node).  Right child is always allocated next to it
                obj_id m_nFirstObject;  ///< First object in this node (if a leaf)
            };
            uint32 m_nObjectCount;      ///< Number of objects in this node, plus 3 (if < 3, it is an inner node, and the value is the split axis)
        };

        typedef Node* NodeHandle;
        typedef const Node* ConstNodeHandle;

        inline AABBTree();

        inline ~AABBTree() { delete[] m_pNodes; }
       
        inline const Node* GetLeftChild( const Node* n ) const  { return m_pNodes + n->GetLeftChildIndex() ; }
        inline const Node* GetRightChild( const Node* n ) const { return m_pNodes + n->GetRightChildIndex() ; };
        inline const Node* GetRoot() const { return m_pNodes; };
        inline Node* GetLeftChild( Node* n ) { return m_pNodes + n->GetLeftChildIndex(); };
        inline Node* GetRightChild( Node* n ) { return m_pNodes + n->GetRightChildIndex(); };
        inline Node* GetRoot() { return m_pNodes; };
        
        inline size_t GetChildCount( ConstNodeHandle n ) const { return IsNodeLeaf(n) ? 0 : 2; };
        inline ConstNodeHandle GetChild( ConstNodeHandle n, size_t i ) const { return m_pNodes + n->GetLeftChildIndex()+i; };

        inline bool IsNodeLeaf( const Node* n ) const { return n->IsLeaf(); };

        inline uint32 GetNodeSplitAxis( const Node* n ) const { return n->GetSplitAxis(); };
        
        inline void GetNodeObjectRange( const Node* n, obj_id& rFirst, obj_id& rLast ) const { n->GetObjectRange( rFirst, rLast ); };
        
        inline uint32 GetNodeObjectCount( const Node* n ) const { return n->GetObjectCount(); };

        inline uint32 GetStackDepth() const { return m_nStackDepth; };

        inline const AxisAlignedBox& GetNodeBoundingVolume( const Node* n ) const { return n->GetAABB(); };

        inline void SetNodeAABB( Node* n, const AxisAlignedBox& rBox ) { n->SetAABB( rBox ); };
        
        /// Turns the specified node into a leaf, containing the specified objects
        inline void MakeLeafNode( Node* n, obj_id nFirstObj, obj_id nObjCount ) { n->MakeLeaf( nFirstObj, nObjCount ); };

        inline Node* Initialize( const AxisAlignedBox& rBox, uint32 nMaxNodes ) ;
        
        inline std::pair<Node*,Node*> MakeInnerNode( Node* n, uint32 nSplitAxis ) ;
        
        /// Performs a ray intersection test with a node's bounding volume
        template< class Ray_T >
        inline bool RayNodeTest( const Node* n, const Ray_T& rRay, float& rTOut ) const
        {
            return RayAABBTest( n->GetAABB().Min(), n->GetAABB().Max(), rRay, rTOut );
        }

        /// Performs a ray intersection test with a node's bounding volume
        template< class Ray_T >
        inline bool RayNodeTest( const Node* n, const Ray_T& rRay ) const
        {
            return RayAABBTest( n->GetAABB().Min(), n->GetAABB().Max(), rRay );
        }

        /// Returns the number of nodes in the tree
        inline uint32 GetNodeCount() const { return m_nNodesInUse; };

        /// Returns the memory consumption of the data structure, as well as the amount allocated
        inline void GetMemoryUsage( size_t& rnBytesUsed, size_t& rnBytesAllocated ) const;

        /// Constructs a new AABB tree
        template< class AABBTreeBuilder_T >
        void Build( ObjectSet_T* pObjects, AABBTreeBuilder_T& rBuilder );

    private:

        Node*  m_pNodes;
        uint32 m_nNodesInUse;
        uint32 m_nStackDepth;
    };

}

#include "TRTAABBTree.inl"

#endif // _TRT_AABBTREE_H_

