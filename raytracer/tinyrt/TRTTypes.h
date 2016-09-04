//=====================================================================================================================
//
//   TRTTypes.h
//
//   Type definitions
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_TYPES_H_
#define _TRT_TYPES_H_


#include "TRTVec2.h"
#include "TRTVec3.h"

namespace TinyRT
{
    // define aliases for integral types of specific sizes
    typedef unsigned char   uint8;
    typedef unsigned short  uint16;
    typedef unsigned int    uint32;
    typedef char    int8;
    typedef short   int16;
    typedef int     int32;
    typedef void*   Handle;

    typedef unsigned int uint;

    typedef Vec3<float> Vec3f;
    typedef Vec2<float> Vec2f;
}


#endif // _TRT_TYPES_H_
