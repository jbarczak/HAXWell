//=====================================================================================================================
//
//   TRTTriangleRayHit.h
//
//   Definition of class: TinyRT::TriangleRayHit
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_TRIANGLERAYHIT_H_
#define _TRT_TRIANGLERAYHIT_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Information about a ray-triangle intersection point
    //=====================================================================================================================
    struct TriangleRayHit
    {
        uint32 nTriIdx;     ///< Index of the triangle which was hit
        Vec2f  vUVCoords;   ///< Barycentric coordinates at the intersection point
    };

}

#endif // _TRT_TRIANGLERAYHIT_H_
