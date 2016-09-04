//=====================================================================================================================
//
//   TRTScopedArray..h
//
//   Definition of class: TinyRT::ScopedArray
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTSCOPEDARRAY_H_
#define _TRTSCOPEDARRAY_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A basic wrapper for Raw arrays, which provides resizing and automatic destruction
    //=====================================================================================================================
    template< class T >
    class ScopedArray
    {
    public:
    
        /// Creates an empty array
        inline ScopedArray( ) : m_pA(0) {};

        /// Takes ownership of the specified array
        inline ScopedArray( T* pArray ) : m_pA( pArray ) {}

        inline ~ScopedArray() { delete[] m_pA; };

        /// Conversion to raw pointer
        inline operator T*() { return m_pA; };
        
        /// Conversion to raw const pointer
        inline operator const T*() const { return m_pA; };

        /// Reallocates the array and copies
        inline void resize( size_t n, size_t nOldSize )
        {
            T* p = new T[n];
            if( m_pA )
                memcpy( p, m_pA, sizeof(T)* Min(n,nOldSize) );
            delete[] m_pA;
            m_pA = p;
        };

        /// Reallocates the array without copying
        inline void reallocate( size_t n )
        {
            delete[] m_pA;
            m_pA = new T[n];
        };

    private:

        /// Disallow shallow copies
        inline ScopedArray( const ScopedArray<T>& rhs ) {};
        inline ScopedArray& operator=( const ScopedArray<T>& rhs ) {};
        
        T* m_pA;
    };
    
}

#endif // _TRTSCOPEDARRAY_H_
