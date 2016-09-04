
//=====================================================================================================================
//
//   TRTSampling.h
//
//   Sampling routines
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2014 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

namespace TinyRT
{
    
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Computes a cosine-weighted direction on the hemisphere from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  Value is 1 at the pole
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    /// \return  The direction is expressed in tangent space, where Y is up
    //=====================================================================================================================
    inline Vec3f CosineWeightedDirection( float s, float t )
    {
        float fCosTheta = sqrtf( s ); 
        float fSinTheta = sqrtf( 1.0f - s );
        float phi = (t*TRT_TWO_PI) - (TRT_PI);
        float y = fCosTheta;
        float x = cos(phi)*fSinTheta;
        float z = sin(phi)*fSinTheta;
        return Vec3f(x,y,z);        
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Generates positions on a unit disk from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  0 at the center
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    /// \return  The direction is expressed in tangent space, where Y is up
    //=====================================================================================================================
    inline Vec2f UniformSampleDisk( float s, float t )
    {
        float a = TRT_TWO_PI*t;
        float r = sqrtf(s);
        return Vec2f( r * cos(a), r*sin(a) );
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Computes a uniform location on the sphere from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  Value is 0.5 at the equator
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    //=====================================================================================================================
    inline Vec3f UniformSampleSphere( float s, float t )
    {
        float fCosTheta = 1.0f - 2.0f*s;
        float fSinTheta = 2.0f * sqrtf( s*(s-1) );
        float phi = t*TRT_TWO_PI - TRT_PI;
        float y = fCosTheta;
        float x = cos(phi)*fSinTheta;
        float z = sin(phi)*fSinTheta;
        return Vec3f(x,y,z);  
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Computes a cosine-weighted direction on the hemisphere from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  Value is 1 at the pole
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    /// \return  The direction is expressed in tangent space, where Y is up
    //=====================================================================================================================
    inline Vec3f UniformSampleHemisphere( float s, float t )
    {
        float fCosTheta = s;
        float fSinTheta = sqrtf( 1.0f - s*s );
        float phi = t*TRT_TWO_PI - TRT_PI;
        float y = fCosTheta;
        float x = cos(phi)*fSinTheta;
        float z = sin(phi)*fSinTheta;
        return Vec3f(x,y,z);  
    }

}