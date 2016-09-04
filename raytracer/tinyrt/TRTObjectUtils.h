//=====================================================================================================================
//
//   TRTObjectUtils..h
//
//   TRT object set utility functions
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTOBJECTUTILS_H_
#define _TRTOBJECTUTILS_H_

#include "TRTScopedArray.h"

namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Utility function which reorders elements in an array, using a shallow copy  
    /// \param pObjects     The array to be reordered
    /// \param nCount       Size of the array
    /// \param pObjectRemap Array containing the object from the old ordering that belongs at position i
    //=====================================================================================================================
    template< class T, class Index_T >
    void RemapArray( T* pObjects, size_t nCount, const Index_T* pObjectRemap )
    {
        ScopedArray<uint8> pCopy( new uint8[sizeof(T)*nCount] );
        memcpy( pCopy, pObjects, sizeof(T)*nCount );

        // scatter objects into existing array
        T* pT = reinterpret_cast<T*>( &pCopy[0] );
        for( size_t i=0; i<nCount; i++ )
            pObjects[i] = pT[ pObjectRemap[i] ];
    }
    
}

#endif // _TRTOBJECTUTILS_H_
