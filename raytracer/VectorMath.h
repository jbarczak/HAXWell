//=====================================================================================================================
//
//   VectorMath.h
//
//   Assorted Vector math
//
//   The lazy man's utility library
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _VECTOR_MATH_H_
#define _VECTOR_MATH_H_

#include <math.h>


namespace Simpleton
{
    static const float PI_f     = 3.14159265358979323846f;
    static const float TWOPI_F  = 2.0f * 3.14159265358979323846f;

#ifdef __GNUC__
    inline float sqrtf( float f ) { return sqrt(f); };
#endif

    //=====================================================================================================================
    /// \brief A template class for two-component vectors.
    ///
    ///   The template argument must be a numeric type
    ///
    //=====================================================================================================================
    template<class T>
    class Vec2
    {
    public:

        T x;
        T y;

        // *****************************************
        //     Constructors
        // *****************************************

        /// Default constructor
        inline Vec2 ( ) {};

        /// Value constructor
        inline Vec2 ( const T& vx, const T& vy ) : x(vx), y(vy) {};
        
        /// Copy constructor
        inline Vec2 ( const Vec2<T>& val ) : x(val.x), y(val.y) {};
    
        /// Single value constructor.  Sets all components to the given value
        inline Vec2 ( const T& v ) : x(v), y(v) {};


        // *****************************************
        //     Conversions/Assignment/Indexing
        // *****************************************

        /// cast to T*
        inline operator const T*() const { return (const T*)this; };

        /// cast to T*
        inline operator T*() { return (T*)this; };

        /// Assignment
        inline const Vec2<T>& operator=( const Vec2<T>& rhs )  { x = rhs.x; y = rhs.y; return *this; };

        // *****************************************
        //    Comparison
        // *****************************************

        /// Equality comparison
        inline bool operator==( const Vec2<T>& rhs ) const { return ( x == rhs.x && y == rhs.y ); };
        
        /// Inequality comparision
        inline bool operator!=( const Vec2<T>& rhs ) const { return ( x != rhs.x || y != rhs.y ); };

        // *****************************************
        //    Arithmetic
        // *****************************************

        /// Addition
        inline const Vec2<T> operator+( const Vec2<T>& rhs ) const { return Vec2<T>( x + rhs.x, y + rhs.y); };

        /// Subtraction
        inline const Vec2<T> operator-( const Vec2<T>& rhs ) const { return Vec2<T>( x - rhs.x, y - rhs.y );};

        /// Divide by vector
        inline const Vec2<T> operator/( const Vec2<T>& rhs ) const { return Vec2<T>( x / rhs.x, y / rhs.y ); };

        /// Multiply by scalar
        inline const Vec2<T> operator*( const T& v ) const { return Vec2<T>( x*v, y*v ); };

        /// Divide by scalar
        inline const Vec2<T> operator/( const T& v ) const { return Vec2<T>( x/v, y/v ); };

        /// Addition in-place
        inline Vec2<T>& operator+= ( const Vec2<T>& rhs ) { x += rhs.x; y += rhs.y; return *this; };
    
        /// Subtract in-place
        inline Vec2<T>& operator-= ( const Vec2<T>& rhs ) { x -= rhs.x; y -= rhs.y; return *this; };

        /// Scalar multiply in-place
        inline Vec2<T>& operator*= ( const T& v ) { x *= v; y *= v; return *this; };

        /// Scalar divide in-place
        inline Vec2<T>& operator/= ( const T& v ) { x /= v; y /= v; return *this; };

        /// Vector multiply in place
        inline Vec2<T>& operator*= ( const Vec2<T>& v ) { x *= v.x; y *= v.y; return *this; };

        /// Vector divide in-place
        inline Vec2<T>& operator/= ( const Vec2<T>& v ) {  x /= v.x; y /= v.y; return *this; };
    };


     /// \brief A template class for three-component vectors.
    template<class T>
    class Vec3
    {
    public:

        T x;
        T y;
        T z;

        // *****************************************
        //     Constructors
        // *****************************************

        /// Default constructor
        inline Vec3 ( ) {};

        /// Value constructor
        inline Vec3 ( const T& vx, const T& vy, const T& vz ) : x(vx), y(vy), z(vz) {};
        
        /// Copy constructor
        inline Vec3 ( const Vec3<T>& val ) : x(val.x), y(val.y), z(val.z) {};
    
        /// Single value constructor.  Sets all components to the given value
        inline Vec3 ( const T& v ) : x(v), y(v), z(v) {};

        /// Array constructor.  Assumes a 3-component array
        inline Vec3 ( const T* v ) : x(v[0]), y(v[1]), z(v[2]) {};

        // *****************************************
        //     Conversions/Assignment/Indexing
        // *****************************************

        /// cast to T*
        inline operator const T*() const { return (const T*)this; };

        /// cast to T*
        inline operator T*() { return (T*)this; };

        /// Assignment
        inline const Vec3<T>& operator=( const Vec3<T>& rhs )  { x = rhs.x; y = rhs.y; z = rhs.z; return *this; };

        // *****************************************
        //    Comparison
        // *****************************************

        /// Equality comparison
        inline bool operator==( const Vec3<T>& rhs ) const { return ( x == rhs.x && y == rhs.y && z == rhs.z); };
        
        /// Inequality comparision
        inline bool operator!=( const Vec3<T>& rhs ) const { return ( x != rhs.x || y != rhs.y || z != rhs.z ); };

        // *****************************************
        //    Arithmetic
        // *****************************************

        /// Negation
        inline const Vec3<T> operator-() const { return Vec3<T>( -x, -y, -z ); };

        /// Addition
        inline const Vec3<T> operator+( const Vec3<T>& rhs ) const { return Vec3<T>( x + rhs.x, y + rhs.y, z + rhs.z ); };

        /// Subtraction
        inline const Vec3<T> operator-( const Vec3<T>& rhs ) const { return Vec3<T>( x - rhs.x, y - rhs.y, z - rhs.z );};

        /// Multiply by scalar
        inline const Vec3<T> operator*( const T& v ) const { return Vec3<T>( x*v, y*v, z*v ); };

        /// Multiply by vector
        inline const Vec3<T> operator*( const Vec3<T>& v ) const { return Vec3<T>( v.x*x, v.y*y, v.z*z ); };

        /// Divide by scalar
        inline const Vec3<T> operator/( const T& v ) const { return Vec3<T>( x/v, y/v, z/v ); };

        /// Divide by vector
        const Vec3<T> operator/( const Vec3<T>& rhs ) const { return Vec3<T>( x/rhs.x, y/rhs.y, z/rhs.z ); };

        /// Addition in-place
        inline Vec3<T>& operator+= ( const Vec3<T>& rhs ) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; };
    
        /// Subtract in-place
        inline Vec3<T>& operator-= ( const Vec3<T>& rhs ) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; };

        /// Scalar multiply in-place
        inline Vec3<T>& operator*= ( const T& v ) { x *= v; y *= v; z *= v; return *this; };

        /// Scalar divide in-place
        inline Vec3<T>& operator/= ( const T& v ) { x /= v; y /= v; z /= v; return *this; };

        /// Vector multiply in place
        inline Vec3<T>& operator*= ( const Vec3<T>& v ) { x *= v.x; y *= v.y; z *= v.z; return *this; };

        /// Vector divide in-place
        inline Vec3<T>& operator/= ( const Vec3<T>& v ) {  x /= v.x; y /= v.y; z /= v.z; return *this; };

    };

    /// Multiply by scalar on left
    template <class T>
    inline Vec3<T> operator*( float f, const Vec3<T>& rVec ) { return rVec*f; };

    typedef Vec2<float>  Vec2f;
    typedef Vec3<float>  Vec3f;

    // ---------------------------------------------------------------------------------------------------
    // Length
    // ---------------------------------------------------------------------------------------------------

    //=====================================================================================================================
    //=====================================================================================================================
    template<class Scalar> inline Scalar Length3( const Scalar* v1 )  
    { 
        return sqrtf( v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2] );
    };

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3f_T >
    inline float Length3( const Vec3f_T& a )
    {
        return sqrtf( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
    };


    //=====================================================================================================================
    //=====================================================================================================================
    template<class Scalar> inline Scalar Length3Sq( const Scalar* v1 )  
    { 
        return ( v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2] );
    };

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3f_T >
    inline float Length3Sq( const Vec3f_T& a )
    {
        return ( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
    };

    // ---------------------------------------------------------------------------------------------------
    // Dot product
    // ---------------------------------------------------------------------------------------------------


    //=====================================================================================================================
    //=====================================================================================================================
    template<class Scalar> inline Scalar Dot3( const Scalar* v1, const Scalar* v2 ) 
    { 
        return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    };


    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar >
    inline Scalar Dot3( const Vec3<Scalar>& a, const Vec3<Scalar>& b )
    {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };

    
    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar >
    inline Scalar Dot3( const Vec3<Scalar>& a, const Scalar* b )
    {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar >
    inline Scalar Dot3( const Scalar* a, const Vec3<Scalar>& b )
    {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };

    // ---------------------------------------------------------------------------------------------------
    //  Normalization
    // ---------------------------------------------------------------------------------------------------


    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3_T >
    inline Vec3_T Normalize3( const Vec3_T& rV )
    {
        return rV * ( 1.0f / Length3(rV) );
    };

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar >
    inline void Normalize3( const Scalar v[3], Scalar vOut[3] )
    {
        Scalar vNrm = 1.0f / Length3(v);
        vOut[0] = v*vNrm;
        vOut[1] = v*vNrm;
        vOut[2] = v*vNrm;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar >
    inline void Normalize3( Scalar v[3] )
    {
        Scalar vNrm = 1.0f / Length3(v);
        v[0] *= vNrm;
        v[1] *= vNrm;
        v[2] *= vNrm;
    }

    // ---------------------------------------------------------------------------------------------------
    //   Cross Product
    // ---------------------------------------------------------------------------------------------------


    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3_T_1, class Vec3_T_2 >
    inline Vec3_T_1 Cross3( const Vec3_T_1& v1, const Vec3_T_2& v2 )
    {
        return Vec3_T_1( (v1.y*v2.z) - (v1.z*v2.y),
                         (v1.z*v2.x) - (v1.x*v2.z),
                         (v1.x*v2.y) - (v1.y*v2.x) );
    };


    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar >
    inline void Cross3( const Scalar v1[3], const Scalar v2[3], Scalar vOut[3] )
    {
        vOut[0] =  (v1[1]*v2[2]) - (v1[2]*v2[1]);
        vOut[1] =  (v1[2]*v2[0]) - (v1[0]*v2[2]);
        vOut[2] =  (v1[0]*v2[1]) - (v1[1]*v2[0]);
    };

    // ---------------------------------------------------------------------------------------------------
    //   Interpolation
    // ---------------------------------------------------------------------------------------------------

    template< class Scalar1, class Scalar2, class Scalar3 >
    inline Scalar1 Lerp( const Scalar1& a, const Scalar2& b, const Scalar3& t )
    {
        return a + (b-a)*t;
    }

    template< class Vec3_T_1, class Vec3_T_2, class Scalar >
    inline Vec3_T_1 Lerp3( const Vec3_T_1& v1, const Vec3_T_2& v2, const Scalar& t )
    {
        return Vec3_T_1( Lerp( v1[0], v2[0], t ),
                         Lerp( v1[1], v2[1], t ),
                         Lerp( v1[2], v2[2], t ) );
    };

    template< class Vec3_T_1, class Vec3_T_2, class Scalar >
    inline void Lerp3( const Vec3_T_1& v1, const Vec3_T_2& v2, const Scalar& t, Vec3_T_1& rOut )
    {
        rOut[0] = Lerp( v1[0], v2[0], t );
        rOut[1] = Lerp( v1[1], v2[1], t );
        rOut[2] = Lerp( v1[2], v2[2], t );
    };

    // ---------------------------------------------------------------------------------------------------
    //   Clamping
    // ---------------------------------------------------------------------------------------------------

    template< class Scalar_T >
    inline Scalar_T Clamp( Scalar_T x, Scalar_T xMin, Scalar_T xMax )
    {
        if( x <= xMin ) return xMin;
        if( x >= xMax ) return xMax;
        return x;
    }

    template< class Scalar_T >
    inline Scalar_T Clamp_01( Scalar_T x )
    {
        return Clamp( x, Scalar_T(0), Scalar_T(1) );
    };

    template< class Vec3_T, class Scalar_T >
    inline void Clamp3( Vec3_T& v, Scalar_T xMin, Scalar_T xMax )
    {
        v[0] = Clamp(v[0],xMin,xMax);
        v[1] = Clamp(v[1],xMin,xMax);
        v[2] = Clamp(v[2],xMin,xMax);
    }

    
    template< class Vec2_T >
    inline Vec2_T Clamp2_01( const Vec2_T& v )
    {
        return Vec2_T( Clamp_01(v[0]), Clamp_01(v[1]) );
    }

    template< class Vec3_T >
    inline Vec3_T Clamp3_01( const Vec3_T& v )
    {
        return Vec3_T( Clamp_01(v[0]), Clamp_01(v[1]), Clamp_01(v[2]));
    }
    template< class Vec3_T >
    inline void Clamp3_01( const Vec3_T& v, Vec3_T& vout )
    {
        vout[0] = Clamp_01(v[0]);
        vout[1] = Clamp_01(v[1]);
        vout[2] = Clamp_01(v[2]);
    }

    // ---------------------------------------------------------------------------------------------------
    //   Minima/Maxima
    // ---------------------------------------------------------------------------------------------------

    // We define our own min/max over std::min/max, because std's use of references, confuses the VC++ compiler

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar_T >
    inline Scalar_T Min( Scalar_T a, Scalar_T b )
    {
        return a < b ? a : b;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Scalar_T >
    inline Scalar_T Max( Scalar_T a, Scalar_T b )
    {
        return a > b ? a : b;
    }


    //=====================================================================================================================
    /// Computes the component-wise minima of a pair of vectors
    //=====================================================================================================================
    template< class Vec3_T_1, class Vec3_T_2 >
    inline Vec3_T_1 Min3( const Vec3_T_1& a, const Vec3_T_2& b )
    {
        return Vec3_T_1( Min(a.x,b.x), Min(a.y,b.y), Min(a.z,b.z) );
    }

    //=====================================================================================================================
    /// Computes the component-wise maxima of a pair of vectors
    //=====================================================================================================================
    template< class Vec3_T_1, class Vec3_T_2 >
    inline Vec3_T_1 Max3( const Vec3_T_1& a, const Vec3_T_2& b  )
    {
        return Vec3_T_1( Max(a.x,b.x), Max(a.y,b.y), Max(a.z,b.z) );
    }

    //=====================================================================================================================
    /// Computes the min and max of a pair of vectors.
    /// \param a    Vector whose components are set to min(a,b)
    /// \parma b    Vector whose components are set to max(a,b)
    //=====================================================================================================================
    template< class Vec3_T_1, class Vec3_T_2 >
    inline void MinMax3( Vec3_T_1& a, Vec3_T_2& b )
    {
        if( a[0] > b[0] ) std::swap( a[0], b[0] );
        if( a[1] > b[1] ) std::swap( a[1], b[1] );
        if( a[2] > b[2] ) std::swap( a[2], b[2] );
    }


    // ---------------------------------------------------------------------------------------------------
    //   Assorted vector operations
    // ---------------------------------------------------------------------------------------------------

    //=====================================================================================================================
    /// Constructs a pair of tangent vectors for a given normal vector
    //=====================================================================================================================
    inline void BuildTangentFrame( const Vec3f& N, Vec3f& T, Vec3f& B )
    {
        // technique and code shamelessly stolen from pbrt
        if( fabs( N.x ) > fabs( N.y ) )
        {
            float fInvLen = 1.0f / sqrtf( N.x*N.x + N.z*N.z );
            T = Vec3f( -N.z *fInvLen, 0, N.x*fInvLen );
        }
        else
        {
            float fInvLen = 1.0f / sqrtf( N.y*N.y + N.z*N.z );
            T = Vec3f( 0, N.z*fInvLen, -N.y*fInvLen );
        }
        B = Cross3( N, T );
    }

    //=====================================================================================================================
    /// Computes a cosine-weighted direction on the hemisphere from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  Value is 1 at the pole
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    /// \return  The direction is expressed in tangent space, where Y is up
    //=====================================================================================================================
    inline Vec3f CosineWeightedDirection( float s, float t )
    {
        float fCosTheta = sqrtf( s ); 
        float fSinTheta = sqrtf( 1.0f - s );
        float phi = (t*TWOPI_F) - (PI_f);
        float y = fCosTheta;
        float x = cos(phi)*fSinTheta;
        float z = sin(phi)*fSinTheta;
        return Vec3f(x,y,z);        
    }

    //=====================================================================================================================
    /// Computes a uniform location on the sphere from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  Value is 0.5 at the equator
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    //=====================================================================================================================
    inline Vec3f UniformSampleSphere( float s, float t )
    {
        float fCosTheta = 1.0f - 2.0f*s;
        float fSinTheta = 2.0f * sqrtf( s*(s-1) );
        float phi = t*TWOPI_F - PI_f;
        float y = fCosTheta;
        float x = cos(phi)*fSinTheta;
        float z = sin(phi)*fSinTheta;
        return Vec3f(x,y,z);  
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Computes a cosine-weighted direction on the hemisphere from a pair of random numbers
    /// \param s The polar coordinate, a random value between 0 and 1.  Value is 1 at the pole
    /// \param t The azimuthal coordinate, a random value between 0 and 1
    /// \return  The direction is expressed in tangent space, where Y is up
    //=====================================================================================================================
    inline Vec3f UniformSampleHemisphere( float s, float t )
    {
        float fCosTheta = s;
        float fSinTheta = sqrtf( 1.0f - s*s );
        float phi = t*TWOPI_F - PI_f;
        float y = fCosTheta;
        float x = cos(phi)*fSinTheta;
        float z = sin(phi)*fSinTheta;
        return Vec3f(x,y,z);  
    }

    //=====================================================================================================================
    /// Converts between spherical coordinates (in radians) and cartesian coordinates
    /// \param fTheta   Polar coordinate (0 at Y+)
    /// \param fPhi     Azimuthal coordinate
    //=====================================================================================================================
    inline Vec3f SphericalToCartesian( float fTheta, float fPhi )
    {
        float fCosTheta = cos(fTheta);
        float fSinTheta = sin(fTheta);
        float fCosPhi = cos(fPhi);
        float fSinPhi = sin(fPhi);
        return Vec3f( fCosPhi*fSinTheta, fCosTheta, fSinPhi*fSinTheta );
    }


}

#endif // _TRT_MATH_H_
