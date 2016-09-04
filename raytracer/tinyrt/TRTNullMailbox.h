//=====================================================================================================================
//
//   TRTNullMailbox.h
//
//   Definition of class: TinyRT::NullMailbox
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_NULLMAILBOX_H_
#define _TRT_NULLMAILBOX_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A 'null' mailbox
    ///
    ///  This class is a mailboxing template which implements the mailboxing interface, but which does not actually
    ///   do any mailboxing.  In some cases, the overhead of mailboxing is not worth the reduction in intersection tests
    ///
    ///  This class implements the Mailbox_C concept
    //=====================================================================================================================
    class NullMailbox
    {
    public:

        inline NullMailbox( const void* ) {};

        inline void ClearMailbox() const {};

        template< typename ObjectRef_T >
        inline bool CheckMailbox( const ObjectRef_T& rObject ) const { return false; };
    };

}

#endif // _TRT_NULLMAILBOX_H_
