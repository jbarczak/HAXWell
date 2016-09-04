//=====================================================================================================================
//
//   TRTSphereIntersect.h
//
//   Ray/sphere intersection
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2014 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_SPHEREINTERSECT_H_
#define _TRT_SPHEREINTERSECT_H_

namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \param C             Sphere center
    /// \param r             Sphere radius
    /// \param D             Ray direction
    /// \param O             Ray origin
    /// \param rDistancesOut Receives the intersection distances if a hit is found
    ///
    /// \return True if a hit was found, false otherwise
    //=====================================================================================================================
    TRT_FORCEINLINE bool RaySphereTest( const Vec3f& D, const Vec3f& O, const Vec3f& C, float r, Vec2f& rDistancesOut )
    {
        Vec3f EC = O - C;
        float a = Dot3(D,D);
        float b = 2*Dot3(D,EC);
        float c = Dot3(EC,EC)-r*r;

        float disc = b*b - 4*a*c;
        if( disc <= 0 )
            return false;

        disc = sqrtf(disc);
        float fScale = 0.5f/a;
        rDistancesOut.x = (-b - disc) * fScale;
        rDistancesOut.y = (disc - b)  * fScale;

        return true;        
    }

}

#endif
