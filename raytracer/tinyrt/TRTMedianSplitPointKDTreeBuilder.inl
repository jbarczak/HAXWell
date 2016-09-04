//=====================================================================================================================
//
//   TRTMedianSplitPointKDTreeBuilder..inl
//
//   Definition of class: TinyRT::MedianSplitPointKDTreeBuilder
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2010 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include "TRTMedianSplitPointKDTreeBuilder.h"

namespace TinyRT
{

    
    //=====================================================================================================================
    //
    //         Constructors/Destructors
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================
    
    //=====================================================================================================================
    /// \param pPoints  Points over which to build the tree.  The contents of the array will be reordered 
    ///                    during tree construction to match the ordering of the tree leaves.
    /// \param nPoints  Number of points
    /// \param rTree    Tree that is to be built
    /// \return The maximum depth of any leaf in the tree
    //=====================================================================================================================
    template< class Point_T, class PointKDTree_T >
    uint MedianSplitPointKDTreeBuilder<Point_T,PointKDTree_T>::Build( Point_T* pPoints, uint nPoints, PointKDTree_T& rTree )
    {
        LeafThreshold = Max(LeafThreshold,(uint)2); // or we'll divide by zero below...

        // compute AABB of the points
        AxisAlignedBox box( pPoints[0].GetPosition(), pPoints[0].GetPosition() );
        for( uint i=1; i<nPoints; i++ )
            box.Expand( pPoints[i].GetPosition() );

        // estimate number of leaves created (not guaranteed to be correct, but generally close)
        uint nExpectedLeafs = 1 + (nPoints/(LeafThreshold/2));

        // set up the tree
        typename PointKDTree_T::NodeHandle hRoot = rTree.Initialize( box, 2*nExpectedLeafs-1 );

        // call recursive build
        return BuildRecurse( pPoints, pPoints, nPoints, box, hRoot, rTree );
    }


    //=====================================================================================================================
    //
    //           Protected Methods
    //
    //=====================================================================================================================
    

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Point_T, class PointKDTree_T >
    uint MedianSplitPointKDTreeBuilder<Point_T,PointKDTree_T>::BuildRecurse( Point_T* pPointArrayHead,
                                                                             Point_T* pPoints, 
                                                                             uint nPoints, 
                                                                             const AxisAlignedBox& rBBox,
                                                                             typename PointKDTree_T::NodeHandle hRoot,
                                                                             PointKDTree_T& rTree )
    {
        // select longest AABB axis
        uint nSplitAxis=0;
        Vec3f vDiag = rBBox.Max() - rBBox.Min();
        if (vDiag.y > vDiag.x )
            nSplitAxis = 1;
        if( vDiag.z > vDiag[nSplitAxis] )
            nSplitAxis = 2;

        // create leaf if we're below the object threshold, or if we have a large number of overlapping points
        if( vDiag[nSplitAxis] < TRT_EPSILON || nPoints <= LeafThreshold )
        {
            rTree.MakeLeafNode( hRoot, 
                                static_cast<typename PointKDTree_T::PointID>( pPoints - pPointArrayHead ), 
                                static_cast<typename PointKDTree_T::PointCount>(nPoints) );
            return 0;
        }

        // partition points based on spatial median of longest axis
        float fSplitPos = rBBox.Min()[nSplitAxis] + vDiag[nSplitAxis]*0.5f;
        Point_T* pStart = pPoints;
        Point_T* pEnd   = pPoints + nPoints;

        AxisAlignedBox bbleft( Vec3f( FLT_MAX, FLT_MAX, FLT_MAX ),
                               Vec3f( -FLT_MAX,-FLT_MAX,-FLT_MAX ) );

        AxisAlignedBox bbright( Vec3f( FLT_MAX, FLT_MAX, FLT_MAX ),
                                Vec3f( -FLT_MAX,-FLT_MAX,-FLT_MAX ) );
          
        while( pStart != pEnd )
        {
            Vec3f vPos = pStart->GetPosition();
            
            if( vPos[nSplitAxis] > fSplitPos ) 
            {
                // belongs at end
                bbright.Expand( vPos );
                --pEnd;
                std::swap( *pStart, *pEnd );
            }
            else 
            {
                // belongs at beginning
                bbleft.Expand( vPos );
                ++pStart;
            }
        }

        std::pair< typename PointKDTree_T::NodeHandle,
                   typename PointKDTree_T::NodeHandle > kids = rTree.MakeInnerNode( hRoot, fSplitPos, nSplitAxis );

        uint nPointsLeft  = static_cast<uint>( pEnd - pPoints );
        uint nPointsRight = nPoints - nPointsLeft;
        TRT_ASSERT( nPointsLeft > 0 && nPointsRight > 0 );

        uint nLeftDepth = BuildRecurse( pPointArrayHead, pPoints, nPointsLeft, bbleft, kids.first, rTree );
        uint nRightDepth = BuildRecurse( pPointArrayHead, pEnd, nPointsRight, bbright, kids.second, rTree );
        return Max( nLeftDepth, nRightDepth ) + 1;
    }
}

