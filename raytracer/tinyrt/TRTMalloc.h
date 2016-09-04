//=====================================================================================================================
//
//   TRTMalloc.h
//
//   TinyRT memory allocation routines
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTMALLOC_H_
#define _TRTMALLOC_H_


namespace TinyRT
{
    //=====================================================================================================================
    /// Allocates memory aligned to a particular byte offset
    /// \param nSize    Size of the desired block, in bytes
    /// \param nAlign   Desired alignment
    //=====================================================================================================================
    inline void* AlignedMalloc( size_t nSize, uint8 nAlign )
    {
        uint8* pMemory = reinterpret_cast<uint8*>( malloc( nSize + nAlign ) );

        // compute misalignment of returned memory
        uintptr_t nMisAlignment = ( reinterpret_cast<uintptr_t>(pMemory) ) & (nAlign-1);
        uintptr_t nAdjust = nAlign - nMisAlignment;

        // move forwards to the next aligned address
        uint8* pAdjusted = pMemory + nAdjust;
        
        // store the offset in the byte prior to the returned address, so we can free the correct address later
        pAdjusted[-1] = static_cast<uint8>( nAdjust );
        return pAdjusted;

    }

    //=====================================================================================================================
    /// Frees memory allocated with 'AlignedMalloc'
    //=====================================================================================================================
    inline void AlignedFree( void* pMemory )
    {
        if( pMemory )
        {
            uint8* pBytes = reinterpret_cast<uint8*>( pMemory );
            free( pBytes - pBytes[-1] );
        }
    }

    
}

#endif // _TRTMALLOC_H_
