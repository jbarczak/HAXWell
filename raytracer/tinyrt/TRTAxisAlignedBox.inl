//=====================================================================================================================
//
//   TRTAxisAlignedBox.inl
//
//   Implementation of class: TinyRT::AxisAlignedBox
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include <algorithm>

namespace TinyRT
{

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// Computes the bounding box of a set of points, and makes the box equal to it
    /// \param pPoints  Array of points
    /// \param nPoints  Number of points in the array.
    //=====================================================================================================================
    inline void AxisAlignedBox::ComputeFromPoints( const Vec3f* pPoints, size_t nPoints )
    {
        TRT_ASSERT( nPoints > 0 );
        m_min = *pPoints;
        m_max = *pPoints;

        const Vec3f* pLast = pPoints + nPoints;
        for( ++pPoints;  pPoints != pLast; ++pPoints )
            Expand( *pPoints );       
    }

    //=====================================================================================================================
    //=====================================================================================================================
    inline void AxisAlignedBox::Expand( const Vec3f& rPoint )
    {
        m_min.x = TinyRT::Min( m_min.x, rPoint.x );
        m_min.y = TinyRT::Min( m_min.y, rPoint.y );
        m_min.z = TinyRT::Min( m_min.z, rPoint.z );

        m_max.x = TinyRT::Max( m_max.x, rPoint.x );
        m_max.y = TinyRT::Max( m_max.y, rPoint.y );
        m_max.z = TinyRT::Max( m_max.z, rPoint.z );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    inline void AxisAlignedBox::Merge( const AxisAlignedBox& rBox )
    {
        m_min.x = TinyRT::Min( m_min.x, rBox.Min().x );
        m_min.y = TinyRT::Min( m_min.y, rBox.Min().y );
        m_min.z = TinyRT::Min( m_min.z, rBox.Min().z );
        m_max.x = TinyRT::Max( m_max.x, rBox.Max().x );
        m_max.y = TinyRT::Max( m_max.y, rBox.Max().y );
        m_max.z = TinyRT::Max( m_max.z, rBox.Max().z );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    inline bool AxisAlignedBox::Contains( const AxisAlignedBox& rBox ) const
    {
        if( m_min.x > rBox.m_min.x ) return false;
        if( m_min.y > rBox.m_min.y ) return false;
        if( m_min.z > rBox.m_min.z ) return false;
        if( m_max.x < rBox.m_max.x ) return false;
        if( m_max.y < rBox.m_max.y ) return false;
        if( m_max.z < rBox.m_max.z ) return false;
        return true;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    inline bool AxisAlignedBox::Contains( const Vec3f& P ) const 
    {
        if( P.x < m_min.x ) return false;
        if( P.y < m_min.y ) return false;
        if( P.z < m_min.z ) return false;
        if( P.x > m_max.x ) return false;
        if( P.y > m_max.y ) return false;
        if( P.z > m_max.z ) return false;
        return true;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    inline bool AxisAlignedBox::Intersects( const AxisAlignedBox& rBox ) const
    {
        if( m_min.x > rBox.m_max.x ) return false;
        if( m_min.y > rBox.m_max.y ) return false;
        if( m_min.z > rBox.m_max.z ) return false;
        if( m_max.x < rBox.m_min.x ) return false;
        if( m_max.y < rBox.m_min.y ) return false;
        if( m_max.z < rBox.m_min.z ) return false;
        return true;
    }

    //=====================================================================================================================
    /// This method assumes that the given cutting plane actually intersects the box
    /// \param nAxis        The axis to cut on (0,1,2 for X,Y,or Z)
    /// \param fLocation    Location to cut at
    /// \param rLeft        Set to the left side AABB
    /// \param rRight       Set to the right side AABB
    //=====================================================================================================================
    inline void AxisAlignedBox::Cut( uint nAxis, float fLocation, AxisAlignedBox& rLeft, AxisAlignedBox& rRight ) const
    {
        rLeft.Min() = m_min;
        rRight.Max() = m_max;

        switch( nAxis )
        {
        case 0:
            rLeft.Max() = Vec3f( fLocation, m_max.y, m_max.z );
            rRight.Min() = Vec3f( fLocation, m_min.y, m_min.z );
            break;
            
        case 1:
            rLeft.Max() = Vec3f( m_max.x, fLocation, m_max.z );
            rRight.Min() = Vec3f( m_min.x, fLocation, m_min.z );
            break;
        case 2:
            rLeft.Max() = Vec3f( m_max.x, m_max.y, fLocation );
            rRight.Min() = Vec3f( m_min.x, m_min.y, fLocation );
            break;
        }
    }


    //=====================================================================================================================
    /// This method assumes that the given cutting plane actually intersects the box
    /// \param nAxis        The axis to cut on (0,1,2 for X,Y,or Z)
    /// \param fLocation    Location to cut at
    /// \param rLeft        Set to the left side AABB
    //=====================================================================================================================
    inline void AxisAlignedBox::CutLeft( uint nAxis, float fLocation, AxisAlignedBox& rLeft ) const
    {
        rLeft.Min() = m_min;
        
        switch( nAxis )
        {
        case 0:
            rLeft.Max() = Vec3f( fLocation, m_max.y, m_max.z );
            break;
            
        case 1:
            rLeft.Max() = Vec3f( m_max.x, fLocation, m_max.z );
            break;
        case 2:
            rLeft.Max() = Vec3f( m_max.x, m_max.y, fLocation );
            break;
        }
    }

    //=====================================================================================================================
    /// This method assumes that the given cutting plane actually intersects the box
    /// \param nAxis        The axis to cut on (0,1,2 for X,Y,or Z)
    /// \param fLocation    Location to cut at
    /// \param rRight       Set to the right side AABB
    //=====================================================================================================================
    inline void AxisAlignedBox::CutRight( uint nAxis, float fLocation, AxisAlignedBox& rRight ) const
    {
        rRight.Max() = m_max;

        switch( nAxis )
        {
        case 0:
            rRight.Min() = Vec3f( fLocation, m_min.y, m_min.z );
            break;
            
        case 1:
            rRight.Min() = Vec3f( m_min.x, fLocation, m_min.z );
            break;
        case 2:
            rRight.Min() = Vec3f( m_min.x, m_min.y, fLocation );
            break;
        }
    }

    //=====================================================================================================================
    //=====================================================================================================================            
    void AxisAlignedBox::Intersect( const AxisAlignedBox& rOther )
    {
        m_min = Max3( m_min, rOther.Min() );
        m_max = Min3( m_max, rOther.Max() );
    }
}
