//=====================================================================================================================
//
//   TRTRay.h
//
//   Definition of class: TinyRT::Ray
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_RAY_H_
#define _TRT_RAY_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A ray, possessing an endpoint and a direction
    ///
    ///  This class assumes that the ray's valid region begins at the origin and ends at some fixed value.
    ///
    ///  The end of the ray's valid region may be changed during ray casting.
    ///
    ///  This class implements the Ray_C concept
    //=====================================================================================================================
    class Ray
    {
    public:

        /// Constructs a ray, setting the max distance to FLT_MAX
        inline Ray( const Vec3f& rOrigin, const Vec3f& rDirection ) 
            : m_origin( rOrigin ), m_direction( rDirection ), 
              m_invDirection( Vec3f( 1.0f /rDirection.x, 1.0f / rDirection.y, 1.0f / rDirection.z ) ),
              m_fTMax( std::numeric_limits<float>::max() )
        {
        }

        /// Returns the origin of the ray
        inline const Vec3f& Origin() const { return m_origin; };
        
        /// Returns the direction of the ray
        inline const Vec3f& Direction() const { return m_direction; };
        
        /// Returns the reciprocal direction of the ray
        inline const Vec3f& InvDirection() const { return m_invDirection; };

        /// Returns the start of the ray's valid interval (always 0)
        inline float MinDistance() const { return 0.0f; };

        /// Returns the end of the ray's valid interval
        inline float MaxDistance() const { return m_fTMax; };

        /// Sets the end of the ray's valid interval
        inline void SetMaxDistance( float t ) { m_fTMax = t; };

        /// Tests whether a 'T-value' is within the ray's valid interval
        inline bool IsDistanceValid( float t ) const { return ( t >= 0.0f && t < m_fTMax ); };

        /// Tests whether a range of T values is within the ray's valid interval
        inline bool IsIntervalValid( float fTMin, float fTMax ) const { return fTMax >= 0.0f && fTMin < m_fTMax ; };

        /// Vectorized version of 'IsIntervalValid'
        inline SimdVec4f AreIntervalsValid( const SimdVec4f& rMin, const SimdVec4f& rMax ) const { return rMax >= SimdVec4f::Zero() & rMin < SimdVec4f( m_fTMax ); };

        /// Vectorized version of 'AreDistancesValid'
        inline SimdVec4f AreDistancesValid( const SimdVec4f& rT ) const { return rT >= SimdVec4f::Zero() & rT < SimdVec4f( m_fTMax ); };
        
        /// Modifies the ray origin
        inline void SetOrigin( const Vec3f& rOrigin ) { m_origin = rOrigin; };
        
    private:

        Vec3f m_origin;         ///< The ray origin
        Vec3f m_direction;      ///< The ray direction
        Vec3f m_invDirection;   ///< Reciprocal of the ray direction

        float m_fTMax;
    };

}

#endif // _TRT_RAY_H_
