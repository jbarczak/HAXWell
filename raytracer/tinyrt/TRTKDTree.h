//=====================================================================================================================
//
//   TRTKDTree.h
//
//   Definition of class: TinyRT::KDTree
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTKDTREE_H_
#define _TRTKDTREE_H_


namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A basic KDTree implementation
    ///
    /// This class implements the ObjectKDTree_C concept
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    //=====================================================================================================================
    template< class ObjectSet_T >
    class KDTree
    {
    public:
                
        typedef ObjectSet_T ObjectSet;
        typedef typename ObjectSet_T::obj_id obj_id;
        typedef const obj_id* LeafIterator; ///< Type that acts as an iterator over objects in a leaf

        /// A node in a KD-tree
        class Node
        {
        public:
            
            inline float GetSplitPosition() const { return m_fSplitPlane; };
            inline uint GetSplitAxis() const { return m_nSplitAxis; };
            inline bool IsLeaf() const { return m_nSplitAxis == 3; };
            
            inline uint GetObjectArraySize() const { return m_nObjSize; };
            inline uint GetFirstObjectOffset() const { return m_nByteOffset;};
            inline uint GetChildrenOffset() const { return m_nByteOffset; };
          
            inline void MakeInnerNode( uint nLeftChildOffs, float fSplit, uint nSplitAxis )
            {
                m_fSplitPlane = fSplit;
                m_nSplitAxis = nSplitAxis;
                m_nByteOffset = nLeftChildOffs;
            }

            inline void MakeLeafNode( uint nFirstObjectOffs, uint nObjCount )
            {
                m_nObjSize = nObjCount*sizeof(obj_id);
                m_nByteOffset = nFirstObjectOffs;
                m_nSplitAxis = 3;
            };


        private:
            union 
            {
                float  m_fSplitPlane;
                uint32 m_nObjSize; 
            };


            unsigned m_nByteOffset : 30;    ///< Address of first child (if an inner node), or first object index (if leaf)
            unsigned m_nSplitAxis : 2;      ///< Split axis (a value of 3 indicates a leaf)
        };

        typedef uint NodeHandle;        ///< KDTree node handles are byte offsets from the start of the array
        typedef uint ConstNodeHandle;   ///< KDTree node handles are byte offsets from the start of the array

        inline KDTree( );

        /// Returns the root node of the tree
        inline ConstNodeHandle GetRoot() const   { return 0; };

        /// Returns the i'th child of a particular (non-leaf) node.  0 is the left
        inline ConstNodeHandle GetChild( ConstNodeHandle r, size_t i ) const { 
            return NodeFromHandle(r)->GetChildrenOffset() + i*sizeof(Node); };

        /// Returns the i'th child of a particular (non-leaf) node.  0 is the left
        inline ConstNodeHandle GetLeftChild( ConstNodeHandle r ) const { return NodeFromHandle(r)->GetChildrenOffset(); };

        /// Returns the i'th child of a particular (non-leaf) node.  0 is the left
        inline ConstNodeHandle GetRightChild( ConstNodeHandle r ) const { return NodeFromHandle(r)->GetChildrenOffset() + sizeof(Node); };

        /// Returns the number of children of a particular node (always 0 or 2 for a KD tree)
        inline size_t GetChildCount( ConstNodeHandle r ) const  { return ( NodeFromHandle(r)->IsLeaf() ? 0 : 2 ) ; };
        
        /// Tests whether or not the given node is a leaf
        inline bool IsNodeLeaf( ConstNodeHandle r ) const { return NodeFromHandle(r)->IsLeaf(); };
        
        /// Returns the number of objects in a particular node
        inline size_t GetNodeObjectCount( ConstNodeHandle r ) const { return NodeFromHandle(r)->GetObjectArraySize()/sizeof(obj_id); };
        

        /// Returns the root bounding box of the tree
        inline const AxisAlignedBox& GetBoundingBox() const { return m_aabb; };

        /// Returns the maximum depth of any node in the tree.  This is used to allocate stack space during traversal
        inline size_t GetStackDepth() const { return m_nStackDepth; };    
    
    
        /// Returns iterators over a leaf node's object list (may not be called on inner nodes)
        inline void GetNodeObjectList( ConstNodeHandle n, LeafIterator& rBegin, LeafIterator& rEnd ) const { 
            const Node* pN = NodeFromHandle(n);
            const uint8* pFirst = &m_array[pN->GetFirstObjectOffset()];
            const uint8* pLast = pFirst + pN->GetObjectArraySize();
            rBegin = reinterpret_cast<const obj_id*>( pFirst );
            rEnd   = reinterpret_cast<const obj_id*>( pLast );
        };

        /// Returns the split axis (0,1,2 for a particular node)
        inline uint GetNodeSplitAxis( ConstNodeHandle r ) const { 
            return NodeFromHandle(r)->GetSplitAxis(); 
        };

        /// Returns the split plane location for a particular node
        inline float GetNodeSplitPosition( ConstNodeHandle r ) const { 
            return NodeFromHandle(r)->GetSplitPosition(); 
        };

        /// Clears existing nodes in the tree and creates a new, single-node tree with the specified bounding box, returns the root node
        inline NodeHandle Initialize( const AxisAlignedBox& rRootAABB );

        /// Subdivides a particular node, creating two empty leaf children for it
        inline std::pair<NodeHandle,NodeHandle> MakeInnerNode( NodeHandle hNode, float fSplitPlane, uint nSplitAxis );

        /// Turns the specified node into a leaf containing the given set of object references.  Returns an object ref array which the caller must fill
        inline obj_id* MakeLeafNode( NodeHandle n, obj_id nObjCount ) ;

        /// Constructs a new KD tree
        template< class KDTreeBuilder_T >
        inline void Build( ObjectSet_T* pObjects, KDTreeBuilder_T& rBuilder );

        /// Returns the memory usage of the tree
        inline void GetMemoryUsage( size_t& rnBytesUsed, size_t& rnBytesAllocated ) const
        {
            rnBytesUsed = sizeof(KDTree) + m_nArrayInUse;
            rnBytesAllocated = sizeof(KDTree) + m_nArraySize;
        }

        /// Returns the number of nodes
        inline size_t GetNodeCount() const { return m_nNodesInUse; };

        /// Returns the number of object references
        inline size_t GetObjectRefCount() const { return m_nObjectRefsInUse; };


    private:

        /// Helper method to hide the cast
        inline const Node* NodeFromHandle( ConstNodeHandle h ) const { 
            TRT_ASSERT( h < m_nArrayInUse );
            return reinterpret_cast<const Node*>( &m_array[h] ); 
        };


        /// Helper method to hide the cast
        inline Node* NodeFromHandle( NodeHandle h ) { 
            TRT_ASSERT( h < m_nArrayInUse );
            return reinterpret_cast<Node*>( &m_array[h] ); 
        };

        AxisAlignedBox m_aabb;
        size_t m_nStackDepth;
        size_t m_nNodesInUse;
        size_t m_nObjectRefsInUse;

        ScopedArray<uint8> m_array; ///< Contains nodes and object indices, packed together.  Object index lists are padded to node size
        size_t m_nArrayInUse;
        size_t m_nArraySize;
    };
}

#include "TRTKDTree.inl"

#endif // _TRTKDTREE_H_
