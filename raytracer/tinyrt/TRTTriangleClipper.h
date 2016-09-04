//=====================================================================================================================
//
//   TRTTriangleClipper..h
//
//   Definition of class: TinyRT::TriangleClipper
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTTRIANGLECLIPPER_H_
#define _TRTTRIANGLECLIPPER_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A clipper class which clips triangles in a mesh
    ///
    /// Implements the Clipper_C concept
    /// \param Mesh_T Must implement the Mesh_C concept
    //=====================================================================================================================
    template< class Mesh_T >
    class TriangleClipper
    {
    public:
        typedef typename Mesh_T::face_id face_id;
    
        /// \brief Clips an object to an axis-aligned plane, and computes the bounding boxes of the two halves
        /// This method, given an object and a subset of its AABB, computes the AABBs of the portions of the objects on either
        ///  side of the split plane.  The resulting AABBs are always subsets of the input AABB, to account for repeated
        ///   clipping of the same object by multiple planes.  
        ///
        /// \param pMesh         Mesh whose triangles are being clipped
        /// \param nObjectID     Id of the object being clipped
        /// \param rOldBB        The axis-aligned box of the portion of the object being clipped.  This box must intersect the clipping plane
        /// \param fPosition     Location of axis aligned split plane
        /// \param nAxis         Index of the axis to which the plane is aligned
        /// \param rLeftSideOut  Receives the AABB of the back (lower) portion 
        /// \param rRightSideOut Receives the AABB of the front (upper) portion
        static inline void ClipObjectToAxisAlignedPlane( const Mesh_T* pMesh, 
                                                         face_id nObjectID, 
                                                         const AxisAlignedBox& rOldBB, 
                                                         float fPosition, 
                                                         uint nAxis,
                                                         AxisAlignedBox& rLeftSideOut, 
                                                         AxisAlignedBox& rRightSideOut );
    };
    
}

#include "TRTTriangleClipper.inl"

#endif // _TRTTRIANGLECLIPPER_H_
