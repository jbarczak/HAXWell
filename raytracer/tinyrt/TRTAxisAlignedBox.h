//=====================================================================================================================
//
//   TRTAxisAlignedBox.h
//
//   Definition of class: TinyRT::AxisAlignedBox
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_AXISALIGNEDBOX_H_
#define _TRT_AXISALIGNEDBOX_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A utility class for representing axis-aligned bounding boxes
    ///
    ///  The bounding box is stored as two points, which define its minimum and maximum extents on each axis
    //=====================================================================================================================
    class AxisAlignedBox
    {
    public:

        /// Constructs an uninitialized box
        inline AxisAlignedBox( ) {};

        /// Computes the bounding box of a set of points
        inline AxisAlignedBox( const Vec3f* pPoints, size_t nPoints ) { ComputeFromPoints( pPoints, nPoints ); };
        
        /// Constructs the box given two endpoints
        inline AxisAlignedBox( const Vec3f& rMin, const Vec3f& rMax ) : m_min(rMin), m_max(rMax) {};

        inline bool operator==( const AxisAlignedBox& r ) const { return m_min == r.m_min && m_max == r.m_max; };
        
        /// Returns a reference to the minimum point
        inline const Vec3f& Min() const { return m_min; };
        
        /// Returns a reference to the maximum point
        inline const Vec3f& Max() const { return m_max; };
        
        /// Returns a reference to the minimum point.  The caller may modify it
        inline Vec3f& Min() { return m_min; };

        /// Returns a reference to the maximum point.  The caller may modify it
        inline Vec3f& Max() { return m_max; };

        /// Computes the center of the box
        inline Vec3f Center() const { return (m_min+m_max)*0.5f; };

        /// Makes this box equal to the AABB of a set of points
        inline void ComputeFromPoints( const Vec3f* pPoints, size_t nPoints );

        /// Expands this box to include the specified point
        inline void Expand( const Vec3f& rPoint );

        /// Expands this box to include the specified box
        inline void Merge( const AxisAlignedBox& rBox );

        /// Checks whether the argument box is fully contained in the calling box
        inline bool Contains( const AxisAlignedBox& rBox ) const;

        /// Tests whether a point is contained in the box (inclusive.  Points on the edges are counted)
        inline bool Contains( const Vec3f& P ) const;
        
        /// Checks whether the intersection of two boxes is non-empty
        inline bool Intersects( const AxisAlignedBox& rBox ) const;
        
        /// Cuts an AABB using an axis-aligned split plane
        inline void Cut( uint nAxis, float fLocation, AxisAlignedBox& rLeft, AxisAlignedBox& rRight ) const;

        /// Cuts an AABB using an axis-aligned split plane, returning the lower half
        inline void CutLeft( uint nAxis, float fLocation, AxisAlignedBox& rLeft ) const;

        /// Cuts an AABB using an axis-aligned split plane, returning the upper half
        inline void CutRight( uint nAxis, float fLocation, AxisAlignedBox& rRight ) const;

        /// Sets this box to the intersection of this box with another
        inline void Intersect( const AxisAlignedBox& rBox );
     
        /// Tests whether the box's Min point is <= its Max point
        inline bool IsValid() const { return m_min.x <= m_max.x && m_min.y <= m_max.y && m_min.z <= m_max.z; };

    private:
        Vec3f m_min;
        Vec3f m_max;
    };

}

#include "TRTAxisAlignedBox.inl"

#endif // _TRT_AXISALIGNEDBOX_H_
