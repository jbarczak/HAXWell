//=====================================================================================================================
//
//   TRTSSEVec4.h
//
//   Definition of class: TinyRT::SSEVec4
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_SSEVEC4_H_
#define _TRT_SSEVEC4_H_

#include <xmmintrin.h>
#include <emmintrin.h>


namespace TinyRT
{
    class SSEVec4;

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A wrapper class for SIMD operations on integer values
    //=====================================================================================================================
    class SSEVec4I
    {
    public:
        
        static const uint32 WIDTH = 4; ///< Width of the SIMD vector, in floats
        static const uint ALIGN = 16;

        union
        {
            __m128i vec128;
            int32 values[4];
        };

        // constructors
        inline SSEVec4I() {} ;
        inline SSEVec4I( __m128i vec ) : vec128(vec) {}
        inline SSEVec4I( const int32* data ) : vec128(_mm_load_si128( reinterpret_cast<const __m128i*>(data))) {};
        inline SSEVec4I( int32 scalar ) : vec128(_mm_set1_epi32(scalar)) {};

        // copy and assignment
        inline SSEVec4I( const SSEVec4I& init ) : vec128(init.vec128) {};
        inline const SSEVec4I& operator=( const SSEVec4I& lhs ) { vec128 = lhs.vec128; return *this;};

        // conversion to m128 type for direct use in _mm intrinsics
        inline operator __m128i() { return vec128; };
        inline operator const __m128i() const { return vec128; };



        // addition
        inline SSEVec4I operator+( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_add_epi32(vec128, rhs.vec128) ); };
        inline SSEVec4I& operator+=(const SSEVec4I& rhs ) { vec128 = _mm_add_epi32(vec128, rhs.vec128); return *this; };
        
        // subtraction
        inline SSEVec4I operator-( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_sub_epi32(vec128, rhs.vec128) ); };
        inline SSEVec4I& operator-= ( const SSEVec4I& rhs ) { vec128 = _mm_sub_epi32(vec128,rhs.vec128); return *this; };

        // shifts
        inline SSEVec4I operator<<( int i ) const { return SSEVec4I( _mm_slli_epi32( vec128, i ) ); };
        inline SSEVec4I operator<<( const SSEVec4I& r ) const { return SSEVec4I( _mm_sll_epi32( vec128, r.vec128 ) ); };
        inline SSEVec4I operator>>( int i ) const { return SSEVec4I( _mm_srli_epi32( vec128, i ) ); };
        inline SSEVec4I operator>>( const SSEVec4I& r ) const { return SSEVec4I( _mm_srl_epi32( vec128, r.vec128 ) ); };


        // comparison
        // these return 0 or 0xffffffff in each component
        inline SSEVec4I operator< ( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_cmplt_epi32( vec128, rhs.vec128 ) ); };
        inline SSEVec4I operator> ( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_cmpgt_epi32( vec128, rhs.vec128 ) ); };
        inline SSEVec4I operator==( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_cmpeq_epi32 ( vec128, rhs.vec128 ) ); };

        // bitwise operators
        inline SSEVec4I operator|( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_or_si128( vec128, rhs.vec128 ) ); };
        inline SSEVec4I operator&( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_and_si128( vec128, rhs.vec128 ) ); };
        inline SSEVec4I operator^( const SSEVec4I& rhs ) const { return SSEVec4I( _mm_xor_si128( vec128, rhs.vec128 ) ); };
        inline const SSEVec4I& operator|=( const SSEVec4I& rhs ) { vec128 = _mm_or_si128( vec128, rhs.vec128 ); return *this; };
        inline const SSEVec4I& operator&=( const SSEVec4I& rhs ) { vec128 = _mm_and_si128( vec128, rhs.vec128 ); return *this; };
    
        /// Store to float array.  Address must be 16-byte aligned
        inline void Store( int32* pVec4 ) const { _mm_store_si128( reinterpret_cast<__m128i*>(pVec4), vec128 ); };

        static inline SSEVec4I Zero() { return _mm_setzero_si128(); };
       
        /// Returns ~(A) & B
        static inline SSEVec4I AndNot( const SSEVec4I& A, const SSEVec4I& B )
        {
            return SSEVec4I( _mm_andnot_si128( A.vec128, B.vec128 ) );
        };

        // executes a conditional move
        // returns (condition) ? A : B;
        static inline SSEVec4I Select( const SSEVec4I& condition, const SSEVec4I& A, const SSEVec4I& B )
        {
            return ( (A & condition) )  | AndNot( condition, B );
        };


        /// Converts from integer to floating point
        inline SSEVec4 ToFloat() const;

        /// Returns a four-bit mask containing the high bits of each vector component
        static inline int Mask( const SSEVec4I& v )
        {
            return _mm_movemask_ps( _mm_castsi128_ps(v.vec128) );
        };
    };


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Simd vector implementation using SSE
    //=====================================================================================================================
    class SSEVec4
    {
    public:        

        static const uint32 WIDTH = 4; ///< Width of the SIMD vector, in floats
        static const uint ALIGN = 16;

        union
        {
            __m128 vec128;
            float values[4];
        };

        // constructors
        inline SSEVec4() {} ;
        inline SSEVec4( float x, float y, float z, float w ) :  vec128( _mm_setr_ps(x,y,z,w) ){}; // reverse, so SSEVec4(x,y,z,w) == SSEVec4( {x,y,z,w} )
        inline SSEVec4( __m128 vec ) : vec128(vec) {}
        inline SSEVec4( float scalar ) : vec128(_mm_set1_ps(scalar)) {};
        inline SSEVec4( const float* data ) : vec128(_mm_load_ps(data)) 
        {
            // kick and scream if the address isn't aligned
            TRT_ASSERT( ( reinterpret_cast<size_t>(data) & 0xf ) == 0 );
        };

        // copy and assignment
        inline SSEVec4( const SSEVec4& init ) : vec128(init.vec128) {};
        inline const SSEVec4& operator=( const SSEVec4& lhs ) { vec128 = lhs.vec128; return *this;};

        // conversion to m128 type for direct use in _mm intrinsics
        inline operator __m128() { return vec128; };
        inline operator const __m128() const { return vec128; };

        // negation
        inline SSEVec4 operator-() const { return _mm_mul_ps( vec128, SSEVec4(-1.0f) ); }

        // addition
        inline SSEVec4 operator+( const SSEVec4& rhs ) const { return SSEVec4( _mm_add_ps(vec128, rhs.vec128) ); };
        inline SSEVec4& operator+=(const SSEVec4& rhs ) { vec128 = _mm_add_ps(vec128, rhs.vec128); return *this; };
        
        // multiplication
        inline SSEVec4 operator*( const SSEVec4& rhs ) const { return SSEVec4( _mm_mul_ps(vec128, rhs.vec128) ); };
        inline SSEVec4& operator*=( const SSEVec4& rhs ) { vec128 = _mm_mul_ps(vec128,rhs.vec128); return *this; };

        // subtraction
        inline SSEVec4 operator-( const SSEVec4& rhs ) const { return SSEVec4( _mm_sub_ps(vec128, rhs.vec128) ); };
        inline SSEVec4& operator-= ( const SSEVec4& rhs ) { vec128 = _mm_sub_ps(vec128,rhs.vec128); return *this; };

        // division
        inline SSEVec4 operator/( const SSEVec4& rhs ) const { return SSEVec4( _mm_div_ps(vec128, rhs.vec128) ); };
        inline SSEVec4& operator/= ( const SSEVec4& rhs ) { vec128 = _mm_div_ps(vec128,rhs.vec128); return *this; };

        // scalar division
        inline SSEVec4 operator/( float rhs )   const { return SSEVec4( _mm_div_ps(vec128, _mm_load1_ps(&rhs)) ); };
        inline SSEVec4& operator/=( float rhs )  { vec128 = _mm_div_ps(vec128, _mm_load1_ps(&rhs)); return *this; };

        // comparison
        // these return 0 or 0xffffffff in each component
        inline SSEVec4 operator< ( const SSEVec4& rhs ) const { return SSEVec4( _mm_cmplt_ps( vec128, rhs.vec128 ) ); };
        inline SSEVec4 operator> ( const SSEVec4& rhs ) const { return SSEVec4( _mm_cmpgt_ps( vec128, rhs.vec128 ) ); };
        inline SSEVec4 operator<=( const SSEVec4& rhs ) const { return SSEVec4( _mm_cmple_ps( vec128, rhs.vec128 ) ); };
        inline SSEVec4 operator>=( const SSEVec4& rhs ) const { return SSEVec4( _mm_cmpge_ps( vec128, rhs.vec128 ) ); };
        inline SSEVec4 operator==( const SSEVec4& rhs ) const { return SSEVec4( _mm_cmpeq_ps ( vec128, rhs.vec128 ) ); };

        // bitwise operators
        inline SSEVec4 operator|( const SSEVec4& rhs ) const { return SSEVec4( _mm_or_ps( vec128, rhs.vec128 ) ); };
        inline SSEVec4 operator&( const SSEVec4& rhs ) const { return SSEVec4( _mm_and_ps( vec128, rhs.vec128 ) ); };
        inline SSEVec4 operator^( const SSEVec4& rhs ) const { return SSEVec4( _mm_xor_ps( vec128, rhs.vec128 ) ); };
        inline const SSEVec4& operator|=( const SSEVec4& rhs ) { vec128 = _mm_or_ps( vec128, rhs.vec128 ); return *this; };
        inline const SSEVec4& operator&=( const SSEVec4& rhs ) { vec128 = _mm_and_ps( vec128, rhs.vec128 ); return *this; };
    
        // loads/stores

        /// Store to float array.  Address must be 16-byte aligned
        inline void Store( float* pVec4 ) const { _mm_store_ps( pVec4, vec128 ); };

        /// Store to float array with SIMD vector offset
        inline void Store( float* pVec4, uint32 nVecOffs ) const { Store( pVec4 + nVecOffs*WIDTH ); };


        // swizzling
        
        /// Replicates the 'Nth' channel into all channels of the vector
        template<int n> inline SSEVec4 Replicate() const { return _mm_shuffle_ps( vec128, vec128, _MM_SHUFFLE(n,n,n,n) ); };
        
        /// Shuffles the vector channels
        template<int n1,int n2, int n3, int n4>
        inline SSEVec4 Swizzle() const { return _mm_shuffle_ps( vec128, vec128, _MM_SHUFFLE(n1,n2,n3,n4)); };

        
        /// Returns a zero vector
        static inline SSEVec4 Zero() { return _mm_setzero_ps(); };
    
        /// Returns a four-bit mask containing the high bits of each vector component
        static inline int Mask( const SSEVec4& rVec ) { return _mm_movemask_ps( rVec.vec128 ); };

        /// Tests whether the masks for all components are set
        static inline bool All( const SSEVec4& rVec ) { return Mask( rVec ) == 0x0f; };

        /// Tests whether the masks for ANY components are set
        static inline bool Any( const SSEVec4& rVec ) { return Mask(rVec) != 0; };
      
        // RCP with newton-raphson iteration.  Implementation stolen from Intel's fvec.h header
        static inline SSEVec4 Rcp( const SSEVec4& a ) 
        { 
            __m128 Ra0 = _mm_rcp_ps(a.vec128);
	        return SSEVec4(_mm_sub_ps(_mm_add_ps(Ra0, Ra0), _mm_mul_ps(_mm_mul_ps(Ra0, a.vec128), Ra0)));
        };

        // RSQ with newton-raphson iteration.
        static inline SSEVec4 Rsq( const SSEVec4& a )
        {
	        SSEVec4 Ra0 = _mm_rsqrt_ps(a.vec128);
	        return (SSEVec4(0.5f) * Ra0) * (SSEVec4(3.0f) - (a  * Ra0) * Ra0);
        };

        /// Quick, approximate reciprocal square root (no newton-raphson)
        static inline SSEVec4 Rsq_Fast( const SSEVec4& rVec ) { return SSEVec4( _mm_rsqrt_ps(rVec.vec128) ); };


        static inline SSEVec4 Sqrt( const SSEVec4& v ) { return SSEVec4( _mm_sqrt_ps(v.vec128) ); };
        static inline SSEVec4 Min( const SSEVec4& v1, const SSEVec4& v2 ) { return SSEVec4( _mm_min_ps(v1.vec128, v2.vec128) ); };
        static inline SSEVec4 Max( const SSEVec4& v1, const SSEVec4& v2 ) { return SSEVec4( _mm_max_ps(v1.vec128, v2.vec128) ); };

        /// Vectorized ABS()
        static inline SSEVec4 Abs( const SSEVec4& v )
        {
            static TRT_SIMDALIGN const uint32 mask[4] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
            return _mm_and_ps( SSEVec4( (float*) mask ), v.vec128 );
        };
        
        /// Returns ~(A) & B
        static inline SSEVec4 AndNot( const SSEVec4& A, const SSEVec4& B ) { return SSEVec4( _mm_andnot_ps( A.vec128, B.vec128 ) ); };

        /// Executes a conditional move.  For each component, returns (condition) ? A : B;
        static inline SSEVec4 Select( const SSEVec4& condition, const SSEVec4& A, const SSEVec4& B )
        {
            return ( (A & condition) )  | AndNot( condition, B );
        };

        /// Executes a conditional move.  For each component, returns (!condition) ? A : B;
        static inline SSEVec4 SelectNot( const SSEVec4& condition, const SSEVec4& A, const SSEVec4& B )
        {
            return ( (B & condition) )  | AndNot( condition, A );
        };
        
        // horizontal operations

        /// Performs a horizontal add of the vector components and returns the result
        inline float HAdd( ) const
        {
            float ret;
            __m128 vsum1 = _mm_add_ps( vec128, _mm_movehl_ps( vec128, vec128 ) ); // 0123 + 2323
            __m128 vsum2 = _mm_add_ss( vsum1, _mm_shuffle_ps( vsum1, vsum1, _MM_SHUFFLE(1,1,1,1) ) ); // 0+2 + 1 + 3
            _mm_store_ss( &ret, vsum2 );
            return ret;
        };

        /// Performs a horizontal min of the vector components and returns the result
        inline float HMin( ) const {
            float ret;
            __m128 vsum1 = _mm_min_ps( vec128, _mm_movehl_ps( vec128, vec128 ) ); // 0123 + 2323
            __m128 vsum2 = _mm_min_ss( vsum1, _mm_shuffle_ps( vsum1, vsum1, _MM_SHUFFLE(1,1,1,1) ) ); // 0+2 + 1 + 3
            _mm_store_ss( &ret, vsum2 );
            return ret;
        };

        /// Performs a horizontal max of the vector components and returns the result
        inline float HMax( ) const {
            float ret;
            __m128 vsum1 = _mm_max_ps( vec128, _mm_movehl_ps( vec128, vec128 ) ); // 0123 + 2323
            __m128 vsum2 = _mm_max_ss( vsum1, _mm_shuffle_ps( vsum1, vsum1, _MM_SHUFFLE(1,1,1,1) ) ); // 0+2 + 1 + 3
            _mm_store_ss( &ret, vsum2 );
            return ret;
       
        }

        /// Conversion to integer
        inline SSEVec4I ToInt( ) const { return SSEVec4I( _mm_cvttps_epi32( vec128 ) ); };

        /// Reinterpret as integer
        inline SSEVec4I AsInt( ) const { return SSEVec4I( reinterpret_cast<const int32*>( this ) ); };
    };


    inline SSEVec4 SSEVec4I::ToFloat() const { return SSEVec4( _mm_cvtepi32_ps( vec128 ) ); };

}

#endif // _TRT_SSEVEC4_H_
