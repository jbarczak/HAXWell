//=====================================================================================================================
//
//   TRTSimdFifoMailbox.h
//
//   Definition of class: TinyRT::SimdFifoMailbox
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_SIMDFIFOMAILBOX_H_
#define _TRT_SIMDFIFOMAILBOX_H_

#include <emmintrin.h>

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A SIMD-optimized FIFO mailboxing scheme
    ///
    ///  This mailbox implementation uses a SIMD-optimized mailbox test on 32-bit integer object IDs.
    ///  The template parameter 'SIZE' must be a multiple of the SIMD width.
    ///
    ///  The implementation assumes that the value 0xffffffff is an unused object index
    ///   (when the cache is cleared, all its entries are cleared to 0xffffffff)
    ///
    ///  This class implements the Mailbox_C concept
    ///
    //=====================================================================================================================
    template< size_t SIZE = 8 >
    class SimdFifoMailbox
    {
    public:

        //=====================================================================================================================
        //=====================================================================================================================
        inline SimdFifoMailbox( const void* ) { ClearMailbox(); };

        //=====================================================================================================================
        //=====================================================================================================================
        inline void ClearMailbox()
        {
            __m128i vClear = _mm_set1_epi32(0xffffffff);
            for( int i=0; i<SIZE/4; i++ ) 
                m_cache_simd[i] = vClear;
        }

        //=====================================================================================================================
        //=====================================================================================================================
        inline bool CheckMailbox( int nID ) 
        {    
            // search entire cache
            __m128i vTst = _mm_set1_epi32( nID );
            if( _mm_movemask_epi8( ReadCache<SIZE/4>( vTst ) ) != 0 )
                return true;
        
            m_cache[m_nCount % SIZE] = nID;
            m_nCount++;
            return false;
        }

    private:

#ifdef __GNUC__ 

        inline __m128i ReadCache( __m128i& vTst )
        {
            __m128i v = _mm_cmpeq_epi32( m_cache_simd[0], vTst );
            for( int i=1; i<SIZE/4; i++ )
                v = _mm_or_si128( v, _mm_cmpeq_epi32( m_cache_simd[i], vTst ));
            return v;
        }

#else
        template< int nSize >
        inline __m128i ReadCache( __m128i& vTst )
        {
            return _mm_or_si128( _mm_cmpeq_epi32( m_cache_simd[nSize-1], vTst ), ReadCache<nSize-1>(vTst) ) ;
        }

        template<>
        inline __m128i ReadCache<1>(__m128i& vTst )
        {
            return _mm_cmpeq_epi32( m_cache_simd[0], vTst );
        }
#endif

        union
        {
            __m128i m_cache_simd[SIZE/4];
            int m_cache[SIZE];
        };

        size_t m_nCount;
    };

}

#endif // _TRT_SIMDFIFOMAILBOX_H_
