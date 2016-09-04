//=====================================================================================================================
//
//   Rand.h
//
//   Miscellaneous generators of randomness
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2010 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================


#ifndef _RAND_H_
#define _RAND_H_

#include <stdlib.h>

namespace Simpleton
{
    inline float Rand(  ) { return rand() / (float)RAND_MAX; }

    inline float Rand( float a, float b )
    {
        float t = Rand();
        return t*b + (1-t)*a;
    }
    
}

#endif // _RAND_H_
