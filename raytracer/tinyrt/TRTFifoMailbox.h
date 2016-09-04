//=====================================================================================================================
//
//   TRTFifoMailbox.h
//
//   Definition of class: TinyRT::FifoMailbox
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_FIFOMAILBOX_H_
#define _TRT_FIFOMAILBOX_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A mailboxing scheme which uses "inverse mailboxing" with a FIFO cache
    ///
    ///  This class implements the Mailbox_C concept
    //=====================================================================================================================
    template< class ObjectRef_T, size_t SIZE = 8 >
    class FifoMailbox
    {
    public:

        inline FifoMailbox( const void* ) : m_nCount(0) {}

        //=====================================================================================================================
        //=====================================================================================================================
        inline void ClearMailbox() { m_nCount = 0; };

        //=====================================================================================================================
        //=====================================================================================================================
        inline bool CheckMailbox( const ObjectRef_T& rObject ) 
        {
            if( m_nCount >= SIZE )
            {
                // search entire cache
                if( CheckEntireCache<SIZE>( rObject ) )
                    return true;
            }
            else
            {
                // search part of the cache
                for( size_t i=0; i< m_nCount; i++ )
                {
                    if( m_cache[i] == rObject )
                    {
                        // cache hit.  No change to cache
                        return true;
                    }
                }
            }
            
            // cache miss.  Insert new object at end of cache (wrap around as needed)
            m_cache[m_nCount % SIZE] = rObject;
            m_nCount++;
            return false;
        };


    private:

        //=====================================================================================================================
        /// This method searches the entire cache for a reference to a particular object. 
        //=====================================================================================================================
#ifdef __GNUC__
        template< int n >
        inline bool CheckEntireCache( const ObjectRef_T& rObject ) const
        {
            for( uint i=0; i<n; i++ )
            {
                if( m_cache[SIZE-n] == rObject )
                    return true;
            }
            return false;
        }
#else
        template< int n >
        inline bool CheckEntireCache( const ObjectRef_T& rObject ) const
        {
            return ( m_cache[SIZE-n] == rObject ) || CheckEntireCache<n-1>( rObject );
        };

        template<>
        inline bool CheckEntireCache<1>(  const ObjectRef_T& rObject ) const
        {
            return m_cache[SIZE-1] == rObject;
        }
#endif

        ObjectRef_T m_cache[SIZE];
        size_t m_nCount;
    };

}

#endif // _TRT_FIFOMAILBOX_H_
