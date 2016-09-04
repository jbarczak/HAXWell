//=====================================================================================================================
//
//   TRTEpsilonRay.h
//
//   Definition of class: TinyRT::EpsilonRay
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_EPSILONRAY_H_
#define _TRT_EPSILONRAY_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A ray implementation which uses TRT_EPSILON its minimum distance
    ///
    ///  Note that, although this class derives from TinyRT::Ray, the two classes are not designed to behave
    ///   polymorphically.  TinyRT uses templates for polymorphism.  Inheritance is used here only as a
    ///   convenience.
    ///
    ///  This class implements the Ray_C concept
    //=====================================================================================================================
    class EpsilonRay : public Ray
    {
    public:

        /// Constructs an epsilon ray with the specified origin and direction, setting the max distance to FLT_MAX
        inline EpsilonRay( const Vec3f& rOrigin, const Vec3f& rDirection )
            : Ray( rOrigin, rDirection )
        {
        };

        /// Returns the start of the ray's valid interval (always TRT_EPSILON)
        inline float MinDistance() const { return TRT_EPSILON; };

        /// Tests whether a 'T-value' is within the ray's valid interval
        inline bool IsDistanceValid( float t ) const { return ( t >= MinDistance() && t < MaxDistance() ); };

        /// Tests whether a range of T values is within the ray's valid interval
        inline bool IsIntervalValid( float fTMin, float fTMax ) const { return fTMax >= MinDistance() && fTMin < MaxDistance() ; };

        /// Vectorized version of 'IsIntervalValid'
        inline SimdVec4f AreIntervalsValid( const SimdVec4f& rMin, const SimdVec4f& rMax ) const { 
            return rMax >= SimdVec4f(MinDistance()) & rMin < SimdVec4f( MaxDistance() ); 
        };

        /// Vectorized version of 'AreDistancesValid'
        inline SimdVecf AreDistancesValid( const SimdVecf& rT ) const { return rT >= SimdVecf( MinDistance() ) & rT < SimdVecf( MaxDistance() ); };


    };

}

#endif // _TRT_EPSILONRAY_H_
