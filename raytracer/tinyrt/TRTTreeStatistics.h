//=====================================================================================================================
//
//   TRTTreeStatistics.h
//
//   Definition of class: TinyRT::TreeStatistics
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_TREESTATISTICS_H_
#define _TRT_TREESTATISTICS_H_

#include <limits>

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A structure containing statistics about the distribution of nodes and objects in a tree structure
    //=====================================================================================================================
    struct TreeStatistics
    {
        size_t nMinLeafDepth;
        size_t nMaxLeafDepth;
        size_t nAvgLeafDepth;
        
        size_t nNodes;
        size_t nLeafs;
        size_t nEmptyLeafs;

        size_t nMinObjectsPerLeaf;
        size_t nMaxObjectsPerLeaf;
        size_t nAvgObjectsPerLeaf;
        size_t nAvgObjectsPerNonEmptyLeaf;
        size_t nTotalObjects; ///< Total number of objects referenced across all nodes
    };


    //=====================================================================================================================
    /// Computes a set of statistics for a tree
    /// \param Tree_T Must implement the Tree_C concept
    //=====================================================================================================================
    template< class Tree_T >
    void GetTreeStatistics( TreeStatistics& rStats, const Tree_T* pTree )
    {
        rStats.nMinLeafDepth = std::numeric_limits<size_t>::max();
        rStats.nMaxLeafDepth = 0;
        rStats.nAvgLeafDepth = 0;
        
        rStats.nMinObjectsPerLeaf = std::numeric_limits<size_t>::max();
        rStats.nMaxObjectsPerLeaf = 0;
        rStats.nAvgObjectsPerLeaf = 0;
        rStats.nLeafs = 0;
        rStats.nEmptyLeafs = 0;
        rStats.nNodes = 0;
    
        typedef std::pair< typename Tree_T::ConstNodeHandle, size_t > StackItem;

        std::vector< StackItem > stack;
        stack.push_back( StackItem( pTree->GetRoot(), 0 ) );

        while( !stack.empty() )
        {
            StackItem node = stack.back();
            stack.pop_back();

            rStats.nNodes++;
         
            if( pTree->IsNodeLeaf( node.first ) )
            {
                rStats.nLeafs++;
                rStats.nMinLeafDepth = Min( rStats.nMinLeafDepth, node.second );
                rStats.nMaxLeafDepth = Max( rStats.nMaxLeafDepth, node.second );
                rStats.nAvgLeafDepth += node.second;
                
                size_t nObjects = pTree->GetNodeObjectCount( node.first );
                rStats.nMinObjectsPerLeaf = Min( rStats.nMinObjectsPerLeaf, nObjects );
                rStats.nMaxObjectsPerLeaf = Max( rStats.nMaxObjectsPerLeaf, nObjects );
                rStats.nAvgObjectsPerLeaf += nObjects;

                if( nObjects == 0 )
                    rStats.nEmptyLeafs++;
            }
            else
            {
                for( size_t i=0; i<pTree->GetChildCount( node.first ); i++ )
                    stack.push_back( StackItem( pTree->GetChild( node.first, i ), node.second+1 ) );
            }
        }

        rStats.nAvgObjectsPerNonEmptyLeaf = rStats.nAvgObjectsPerLeaf / (rStats.nLeafs - rStats.nEmptyLeafs);
        rStats.nTotalObjects = rStats.nAvgObjectsPerLeaf;
        rStats.nAvgLeafDepth /= rStats.nLeafs;
        rStats.nAvgObjectsPerLeaf /= rStats.nLeafs;
    }

}

#endif // _TRT_TREESTATISTICS_H_
