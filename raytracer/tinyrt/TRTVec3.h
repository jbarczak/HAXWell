//=====================================================================================================================
//
//   TRTVec3.h
//
//   Definition of class: TinyRT::Vec3
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_VEC3_H_
#define _TRT_VEC3_H_

namespace TinyRT
{

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


};



#endif // _TRT_VEC3_H_

