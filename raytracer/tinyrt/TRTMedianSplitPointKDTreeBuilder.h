//=====================================================================================================================
//
//   TRTMedianSplitPointKDTreeBuilder..h
//
//   Definition of class: TinyRT::MedianSplitPointKDTreeBuilder
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTMEDIANSPLITPOINTKDTREEBUILDER_H_
#define _TRTMEDIANSPLITPOINTKDTREEBUILDER_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Constructs point KD-trees using spatial median splits
    /// \param Point_T       Must implement the Point_C concept
    /// \param PointKDTree_T Must implement the PointKDTree_C concept
    /// \sa Point_C
    /// \sa PointKDTree_C
    /// \sa PointKDTree
    //=====================================================================================================================
    template< class Point_T, class PointKDTree_T  >
    class MedianSplitPointKDTreeBuilder 
    {
    public:
    
        uint LeafThreshold; ///< Minimum number of points allowed in a leaf node.  If point count drops below this, create a leaf

        inline MedianSplitPointKDTreeBuilder() : LeafThreshold(1) {};

        /// Constructs a KDTree for an array of points. The points will be reordered in the array
        uint Build( Point_T* pPoints, uint nPoints, PointKDTree_T& rTree );

    protected:

        /// Recursive helper method for tree construction
        uint BuildRecurse(  Point_T* pPointArrayHead,
                             Point_T* pPoints, 
                             uint nPoints, 
                             const AxisAlignedBox& rBBox,
                             typename PointKDTree_T::NodeHandle hRoot,
                             PointKDTree_T& rTree );

    };
    
}

#include "TRTMedianSplitPointKDTreeBuilder.inl"

#endif // _TRTMEDIANSPLITPOINTKDTREEBUILDER_H_
