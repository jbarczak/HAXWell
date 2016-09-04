//=====================================================================================================================
//
//   TRTBoxClipper..h
//
//   Definition of class: TinyRT::BoxClipper
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTBOXCLIPPER_H_
#define _TRTBOXCLIPPER_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A clipper implementation which simply clips object AABBs 
    ///
    ///  This class is meant to function as a default Clipper implementation for KD tree construction.  
    ///   Clipping object AABBs is a simple, functional solution, but it will result in low quality trees
    ///    and should only be used as a fallback.
    ///
    ///  This class implements the Clipper_C concept.
    /// \param ObjectSet_T Must implement the ObjectSet_C concept
    //=====================================================================================================================
    template< class ObjectSet_T >
    class BoxClipper
    {
    public:
            
        typedef typename ObjectSet_T::obj_id obj_id;
    
        /// \brief Clips an object to an axis-aligned plane, and computes the bounding boxes of the two halves
        /// This method, given an object and a subset of its AABB, computes the AABBs of the portions of the objects on either
        ///  side of the split plane.  The resulting AABBs are always subsets of the input AABB, to account for repeated
        ///   clipping of the same object by multiple planes.  
        ///
        /// \param pMesh         Object set whose objects are being clipped
        /// \param nObjectID     Id of the object being clipped
        /// \param rOldBB        The axis-aligned box of the portion of the object being clipped.  This box must intersect the clipping plane
        /// \param fPosition     Location of axis aligned split plane
        /// \param nAxis         Index of the axis to which the plane is aligned
        /// \param rLeftSideOut  Receives the AABB of the back (lower) portion 
        /// \param rRightSideOut Receives the AABB of the front (upper) portion
        static inline void ClipObjectToAxisAlignedPlane( const ObjectSet_T* pMesh, 
                                                         obj_id nObjectID, 
                                                         const AxisAlignedBox& rOldBB, 
                                                         float fPosition, 
                                                         uint nAxis,
                                                         AxisAlignedBox& rLeftSideOut, 
                                                         AxisAlignedBox& rRightSideOut )
        {
            // if this fails, it indicates a bug in the KD tree builder....
            TRT_ASSERT( rOldBB.Min()[nAxis] < fPosition && rOldBB.Max()[nAxis] > fPosition );
            rOldBB.Cut( nAxis, fPosition, rLeftSideOut, rRightSideOut );
        }
    };
    
}

#endif // _TRTBOXCLIPPER_H_
