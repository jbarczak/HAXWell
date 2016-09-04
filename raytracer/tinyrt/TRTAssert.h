//=====================================================================================================================
//
//   TRTAssert.h
//
//   Definition of class: TinyRT::Assert
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_ASSERT_H_
#define _TRT_ASSERT_H_

#include <assert.h>

#ifndef TRT_ASSERT
/// Library clients may #define TRT_ASSERT to substitute their own assertion mechanism 
#define TRT_ASSERT(x) assert(x); 
#endif

namespace TinyRT
{
    template< bool b >  struct StaticAssertFailure;
    template<> struct StaticAssertFailure<true> { };

    template<int s >
    struct StaticAssertTest
    {
    };
}

/// Compile-time assertion macro
#define TRT_STATIC_ASSERT(x) typedef TinyRT::StaticAssertTest< sizeof( TinyRT::StaticAssertFailure< (bool)(x) >) >  StaticAssert;

#endif // _TRT_ASSERT_H_
