//=====================================================================================================================
//
//   TRTQuadAABBTree.inl
//
//   Implementation of class: TinyRT::QuadAABBTree
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include "TRTQuadAABBTree.h"

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
    QuadAABBTree<ObjectSet_T>::QuadAABBTree( )
    : m_pNodes(0), m_nNodeArraySize(1), m_nNodesInUse(1),
      m_pLeafObjects(0), m_nLeafArraySize(1), m_nLeafsInUse(1)
    {
        // allocate a sentinal leaf to point empty leaf pointers at
        m_pLeafObjects = new LeafObjects[1];
        m_pLeafObjects->nFirstObj=0;
        m_pLeafObjects->nLastObj=0;

        // allocate the root node
        m_pNodes = reinterpret_cast<Node*>( AlignedMalloc( sizeof(Node), SimdVec4f::ALIGN ));


    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    QuadAABBTree<ObjectSet_T>::~QuadAABBTree( )
    {
        if( m_pNodes )
            AlignedFree( m_pNodes );
        if( m_pLeafObjects )
            delete[] m_pLeafObjects;
    }


    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

    template< class ObjectSet_T >
    template< class QAABBBuilder_T >
    void QuadAABBTree<ObjectSet_T>::Build( ObjectSet_T* pObjects, QAABBBuilder_T& rBuilder )
    {
        m_nStackDepth = rBuilder.BuildQuadAABBTree( pObjects, this );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    void QuadAABBTree<ObjectSet_T>::GetNodeObjectRange( NodeHandle n, obj_id& rFirst, obj_id& rLast ) const
    {
        const LeafObjects* pLeaf = LookupLeaf( n );
        rFirst = pLeaf->nFirstObj;
        rLast = pLeaf->nLastObj;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    typename QuadAABBTree<ObjectSet_T>::NodeHandle QuadAABBTree<ObjectSet_T>::SubdivideChild( NodeHandle nNode, uint32 nChild )
    {
        NodeHandle nChildNode = BuyNode();
        Node* pNode = LookupNode( nNode );
        pNode->m_intersectMask |= ( 1 << nChild ); // not an empty leaf
        pNode->m_children[nChild] = nChildNode;
        return pNode->m_children[nChild];
    }

    //=====================================================================================================================
    /// Sets the axes for three split planes which divide the children of an MBVH node.  
    /// \param nNode    Node to set
    /// \param nA0      Axis dividing the left two children from the right two
    /// \param nA1      Axis dividing the first two children
    /// \param nA2      Axis dividing the last two children
    //=====================================================================================================================
    template< class ObjectSet_T >
    void QuadAABBTree<ObjectSet_T>::SetSplitAxes( NodeHandle nNode, uint32 nA0, uint32 nA1, uint32 nA2 )
    {
        Node* pNode = LookupNode( nNode );
      
        uint32 nAxisMask0 = (1<<nA0);
        uint32 nAxisMask1 = (1<<nA1);
        uint32 nAxisMask2 = (1<<nA2);

        // based on the split planes, compute traversal orderings for each possible octant
        // The octant ID is formed from the sign bits of the ray directions (Z,Y,X)
        // Based on the octant number and split indices, we can derive the traversal order and encode it
        //  in one byte per octant (from MSB to LSB)
        //
        // This trick is taken from the RT'08 paper "Multi-Bounding Volume Hierarchies"
        for( int i=0; i<8; i++ )
        {
            // octant numbers range from 0-7.  Bit k in the octant number indicates the sign bit of the ray direction
            //  on axis k for that octant.  For example, octant 2 (010) is Z+,Y-,X+.  octant 5 (101) is Z-,Y+,X-

            uint32 order[4] = { 0,1,2,3 };
            if( nAxisMask1 & i )
            {
                std::swap( order[0], order[1] );
            }
            if( nAxisMask2 & i )
            {
                std::swap( order[2], order[3] );
            }
            
            if( nAxisMask0 & i )
            {
                std::swap( order[0], order[2] ); 
                std::swap( order[1], order[3] );
            }

            pNode->m_traversalOrder[i] = (order[0] << 6) | (order[1] << 4) | (order[2] << 2) | order[3];
        }
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    void QuadAABBTree<ObjectSet_T>::CreateLeafChild( NodeHandle nNode, uint32 nChildIdx, obj_id nFirstObject, obj_id nObjects )
    {
        Node* pNode = LookupNode( nNode );
        pNode->m_children[nChildIdx] = BuyLeaf();
        pNode->m_intersectMask |= ( 1 << nChildIdx );  // not an empty leaf 
        
        LeafObjects* pLeaf = LookupLeaf( pNode->m_children[nChildIdx] );
        pLeaf->nFirstObj = nFirstObject;
        pLeaf->nLastObj = nFirstObject + nObjects;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    void QuadAABBTree<ObjectSet_T>::CreateEmptyLeafChild( NodeHandle nNode, uint32 nChildIdx )
    {
        Node* pNode = LookupNode( nNode );
        pNode->m_children[nChildIdx] = EMPTY_LEAF;
        pNode->m_intersectMask &= ~(1 << nChildIdx); // empty leaf
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    void QuadAABBTree<ObjectSet_T>::SetChildAABB( NodeHandle nNode, uint32 nChildIdx, const AxisAlignedBox& rBox )
    {
        Node* pNode = LookupNode( nNode );
        for(int i=0; i<3; i++ )
        {
            pNode->m_bbox[2*i].values[nChildIdx] = rBox.Min()[i];
            pNode->m_bbox[2*i+1].values[nChildIdx] = rBox.Max()[i];
        }
    }

    //=====================================================================================================================
    /// \param nNode    The node to be tested
    /// \param rRay     The ray
    /// \param pStack   The traversal stack.  Each time a hit is found, it is placed on the stack, and the stack is incremented
    /// \param vSIMDRay    Pre-swizzled ray information.  See RayQuadAABBTest
    /// \param nDirSigns    Elements 0-2 contain 16 if the corresponding ray direction component is negative, 0 otherwise.
    ///                        Element 3 contains a mask formed as follows:  signs[2]<<2 | signs[1]<<1 | signs[0].  
    ///                        This encodes the octant index of the ray
    /// \return The node stack
    //=====================================================================================================================
    template< class ObjectSet_T >
    template< class Ray_T >
    TRT_FORCEINLINE
    typename QuadAABBTree<ObjectSet_T>::ConstNodeHandle* QuadAABBTree<ObjectSet_T>::RayIntersectChildren( ConstNodeHandle nNode, 
                                                                                                        const SimdVec4f vSIMDRay[6],                                                                                                   
                                                                                                        const Ray_T& rRay, 
                                                                                                       ConstNodeHandle* pStack, 
                                                                                                       const int nDirSigns[4] ) const
    {
        Node* pNode = LookupNode( nNode );
        
        // test ray against child node AABBs
        int nHit = RayQuadAABBTest( pNode->m_bbox, vSIMDRay, rRay, nDirSigns );
        
        // exclude empty leaves
        nHit = nHit & pNode->m_intersectMask; 

        if( !nHit )
            return pStack;     // missed everything, bail out

        // push each child that was hit, in reverse order
        // The traversal order is pre-computed and packed into an 8-byte form.  See 'SetSplitAxes'

        int nOctant = nDirSigns[3];
        int nOrder = pNode->m_traversalOrder[nOctant];
          
        for(int i=0; i<4; i++ )
        {
            uint nChild = nOrder & 3;    // nOrder % 4
            *pStack = pNode->m_children[nChild];
            pStack += (( nHit >> nChild ) & 1); // conditionally increment the stack. For incoherent rays, this is faster than branching
            nOrder >>= 2;
        }

        return pStack;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class ObjectSet_T >
    inline void QuadAABBTree<ObjectSet_T>::GetMemoryUsage( size_t& rnBytesUsed, size_t& rnBytesAllocated ) const
    {
        rnBytesUsed = m_nNodesInUse*sizeof(Node) + m_nLeafsInUse*sizeof(LeafObjects);
        rnBytesAllocated = m_nNodeArraySize*sizeof(Node) + m_nLeafArraySize*sizeof(LeafObjects);
    }

    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================


}
