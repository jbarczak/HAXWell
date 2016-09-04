//=====================================================================================================================
//
//   TRTLeafPolicy..h
//
//   Leaf policies, for controlling tree construction
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTLEAFPOLICY_H_
#define _TRTLEAFPOLICY_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A leaf policy with no effect
    //=====================================================================================================================
    class NullLeafPolicy
    {
    public:

        inline float AdjustLeafCost( uint nRecursionDepth, uint nObjectCount, float fLeafCost ) const { return fLeafCost; };

    };
    

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A leaf policy which enforces a maximum object count per leaf 
    ///
    ///  Care must be taken when forcing splits in KD trees, as an infinite split cost can lead to infinite recursion.
    ///   To prevent this, the 'MaxCountAndDepthPolicy' should be used instead
    ///
    //=====================================================================================================================
    template< int DEFAULT >
    class MaxCountLeafPolicy
    {
    public:

        inline MaxCountLeafPolicy() : MaxObjectsPerLeaf(DEFAULT) {};

        inline float AdjustLeafCost( uint nRecursionDepth, uint nObjectCount, float fLeafCost ) const 
        { 
            if( nObjectCount > MaxObjectsPerLeaf )
                return std::numeric_limits<float>::infinity();  // force a split
            else
                return fLeafCost;
        }

        uint MaxObjectsPerLeaf;
    };

   
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A leaf policy which enforces a maximum recursion depth
    //=====================================================================================================================
    template< int DEFAULT >
    class MaxDepthLeafPolicy
    {
    public:

        inline MaxDepthLeafPolicy() : MaxRecursionDepth(DEFAULT) {};

        inline float AdjustLeafCost( uint nRecursionDepth, uint nObjectCount, float fLeafCost ) const 
        { 
            if( nRecursionDepth == MaxRecursionDepth )
                return 0;  // force a leaf
            else
                return fLeafCost;
        }

        uint MaxRecursionDepth;
    };

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A leaf policy which applies two leaf policies successively. 
    ///   The 'Parent' leaf policy is applied first, followed by the 'Child' policy
    //=====================================================================================================================
    template< class Parent, class Child >
    class CompoundLeafPolicy : public Parent, Child
    {
    public:

        inline float AdjustLeafCost( uint nRecursionDepth, uint nObjectCount, float fLeafCost ) const
        {
            return Child::AdjustLeafCost( nRecursionDepth, nObjectCount, Parent::AdjustLeafCost( nRecursionDepth, nObjectCount, fLeafCost ) );
        }
    };

 
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A pairing of the MaxCountLeafPolicy and MaxDepthLeafPolicy
    //=====================================================================================================================
    template< int DEFAULT_COUNT, int DEFAULT_DEPTH >
    class MaxCountAndDepthLeafPolicy : public CompoundLeafPolicy<MaxCountLeafPolicy<DEFAULT_COUNT>, MaxDepthLeafPolicy<DEFAULT_DEPTH> >
    {
    };

    
}

#endif // _TRTLEAFPOLICY_H_
