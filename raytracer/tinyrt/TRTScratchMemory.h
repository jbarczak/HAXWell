//=====================================================================================================================
//
//   TRTScratchMemory..h
//
//   Definition of class: TinyRT::ScratchMemory
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTSCRATCHMEMORY_H_
#define _TRTSCRATCHMEMORY_H_


namespace TinyRT
{
    class ScratchMemory;

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A memory pool used to allocate temporary storage during raytracing
    ///
    ///  This is an internal class used by 'ScratchMemory' and 'ScratchArray'
    /// \sa ScratchMemory
    /// \sa ScratchArray
    //=====================================================================================================================
    class ScratchPool
    {
    public:

        /// Allocates memory
        uint8* Alloc( size_t nSize ) 
        {
            nSize = (TRT_SIMD_ALIGNMENT + nSize) & ~(TRT_SIMD_ALIGNMENT); // round allocation size up to next multiple of SIMD align

            // no space.  fail
            if( ((m_pNextAddr-m_pBaseAddr) + nSize) > m_nSize )
                return NULL;
            
            // allocate
            uint8* pAllocation = m_pNextAddr;
            m_pNextAddr += nSize;

            // add a pool referene for each allocation
            m_nRefCount++;

            return pAllocation;
        }

        /// Releases an allocation.  Allocations MUST be freed in LIFO order
        void Free( uint8* pAllocation )
        {
            // allocations MUST be freed in LIFO order
            m_pNextAddr = pAllocation;
            Release();
        }

        /// Releases a reference to the pool
        void Release()
        {
            m_nRefCount--;
            if( m_nRefCount == 0 )
            {
                TinyRT::AlignedFree( m_pBaseAddr );
                delete this;
            }
        }

        inline size_t GetSize() const { return m_nSize; };

    private:

        friend class ScratchMemory;

        ScratchPool( uint8* pMemory, size_t nSize )
            : m_pBaseAddr( pMemory ), m_pNextAddr( pMemory ), m_nSize( nSize ), m_nRefCount(1)
        {
        };


        uint8* m_pBaseAddr;
        uint8* m_pNextAddr;
        size_t m_nSize;
        size_t m_nRefCount;
    };


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A small, fast memory pool used for allocating temporary storage during raytracing
    ///
    ///  The ScratchMemory class implements a simple stack-based allocator with automatic reallocation.
    ///  Memory allocated from the scratch pool must be released in LIFO order in order to ensure correctness.
    ///  Using the scoped 'ScratchArray' template is an effective way to guarantee this.
    /// 
    ///  Scratch memory allocations are not thread safe.  If multiple threads are in use, each thread should use its own
    ///   ScratchMemory instance.
    ///   
    //=====================================================================================================================
    class ScratchMemory
    {
    public:

        ScratchMemory( ) { CreatePool( 1024 ); };

        ~ScratchMemory( ) { m_pStackPool->Release(); };

        /// \brief Allocates memory from the scratch pool
        /// \param nCount   Number of objects of type T to allocate
        /// \return A pair containing the allocated memory, plus a pointer to the memory pool from whence it came
        ///            The allocated memory must be released to the pool.  Allocations must be released in LIFO order
        template< class T > 
        inline std::pair<uint8*,ScratchPool*> Allocate( size_t nCount )
        {
            size_t nBytes = nCount*sizeof(T);
            uint8* pMem = m_pStackPool->Alloc( nBytes );
            if( !pMem )
            {
                // grow the pool and try again
                size_t nNewSize = m_pStackPool->GetSize()*2;
                size_t nSize = ( nBytes > nNewSize ) ? nBytes : nNewSize; 
                m_pStackPool->Release();

                CreatePool( nSize );
                pMem = m_pStackPool->Alloc( nBytes );
            }
            return std::pair<uint8*,ScratchPool*>( pMem, m_pStackPool );
        }

    private:

        void CreatePool( size_t nSize )
        {
            uint8* pPool = reinterpret_cast<uint8*>( TinyRT::AlignedMalloc( nSize, TRT_SIMD_ALIGNMENT ) ); 
            m_pStackPool = new ScratchPool( pPool, nSize );
        }

        ScratchPool* m_pStackPool;
    };



    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A simple scoped array wrapper which uses TinyRT::ScratchMemory for allocation
    ///
    ///  The ScratchArray constructor allocates an array from a scratch memory pool, and its destructor releases that memory.
    ///    Scratch arrays are meant to be constructed on the stack to hold temporary storage during raytracing operations.
    ///    By using scratch arrays in this way, the allocations are guaranteed to be released in LIFO order.
    ///
    /// \sa ScratchMemory
    ///
    //=====================================================================================================================
    template< class T >
    class ScratchArray
    {
    public:

        friend class ScratchMemory;

        /// Allocates an array of the specified size from the given 'ScratchMemory' instance
        ScratchArray( ScratchMemory& rMemory, size_t nSize ) 
        {
            std::pair<uint8*,ScratchPool*> mem = rMemory.Allocate<T>( nSize );
            m_pPool = mem.second;
            m_pSpace = reinterpret_cast<T*>( mem.first );
        }

        ~ScratchArray() { m_pPool->Free( reinterpret_cast<uint8*>( m_pSpace ) ); };

        inline operator T*() { return m_pSpace; };
        inline operator const T*() const { return m_pSpace; };

    private:

        // disallow copy and assignment
        ScratchArray( const ScratchArray<T>& a ) {};
        ScratchArray& operator=( const ScratchArray<T>& a ) { return *this; };

        T* m_pSpace;
        ScratchPool* m_pPool;
    };



}

#endif // _TRTSCRATCHMEMORY_H_
