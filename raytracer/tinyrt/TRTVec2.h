//=====================================================================================================================
//
//   TRTVec2.h
//
//   Definition of class: TinyRT::Vec2
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_VEC2_H_
#define _TRT_VEC2_H_

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
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

        /// Indexing
//        inline const T& operator[]( int i ) const { return ((const T*)this)[i]; };
//        inline T& operator[] ( int i ) { return ((T*)this)[i]; };

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

    typedef Vec2<float>  Vec2f;

}


#endif // _TRT_VEC2_H_

