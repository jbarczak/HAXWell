//=====================================================================================================================
//
//   TRTDirectMapMailbox.h
//
//   Definition of class: TinyRT::DirectMapMailbox
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_DIRECTMAPMAILBOX_H_
#define _TRT_DIRECTMAPMAILBOX_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A mailbox which uses "inverse mailboxing" with a direct-mapped cache
    ///
    ///  This class implements the Mailbox_C concept
    //=====================================================================================================================
    template< class ObjectRef_T, size_t SIZE=8 >
    class DirectMapMailbox
    {
    public:
        inline DirectMapMailbox( const void* ) 
        { 
            // we clear the cache by filling its buckets with values which cannot possibly hash to those buckets. 
            // This prevents false hits when the cache is uninitialized
            for( int i=0; i<SIZE; i++ )
                m_cache[i] = i+1;
        };

        //=====================================================================================================================
        //=====================================================================================================================
        inline bool CheckMailbox( const ObjectRef_T& nID ) 
        {
            size_t nSlot = ((size_t)(nID)) % SIZE;
            
            if( m_cache[nSlot] == nID )
                return true;
        
            m_cache[nSlot] = nID;
            return false;
        }

    private:


        ObjectRef_T m_cache[SIZE];
    };

}

#endif // _TRT_DIRECTMAPMAILBOX_H_
