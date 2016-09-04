//=====================================================================================================================
//
//   TRTSimd.h
//
//   SIMD vector types for TinyRT
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_SIMD_H_
#define _TRT_SIMD_H_

#ifdef __GNUC__
    #define TRT_SIMDALIGN   // GCC keeps the stack 16-byte aligned,  and it just spits warnings if we use __declspec(align())
#else
    #define TRT_SIMDALIGN __declspec(align(16))
#endif

#include "TRTSSEVec4.h"

#define TRT_SIMD_ALIGNMENT 16

namespace TinyRT
{
    typedef SSEVec4  SimdVec4f;   ///< Typedef corresponding to a four-component SIMD vector type
    typedef SSEVec4  SimdVecf;    ///< Typedef corresponding to a platform-independent (variable-width) SIMD vector type
    typedef SSEVec4I SimdVec4i;   ///< Typedef corresponding to a four-compponent SIMD integer type
    typedef SSEVec4I SimdVeci;    ///< Typedef corresponding to a platform-independent (variable-width) SIMD integer type

}

#endif // _TRT_SIMD_H_
