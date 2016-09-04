//=====================================================================================================================
//
//   TRTPerspectiveCamera.h
//
//   Definition of class: TinyRT::PerspectiveCamera
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_PERSPECTIVECAMERA_H_
#define _TRT_PERSPECTIVECAMERA_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A simple pinhole camera, with methods for computing ray directions
    //=====================================================================================================================
    class PerspectiveCamera
    {
    public:

        /// \param rOrigin      The position of the camera
        /// \param rDirection   The viewing direction of the camera
        /// \param rUp          An 'up' vector which is used to derive the camera basis vectors
        /// \param fFOV         The angle between the top and bottom edges of the image plane (in degrees)
        /// \param fAspect      The aspect ratio of the image (width/height)
        /// \param bRightHanded If true, the camera uses a right handed coordinate system
        inline PerspectiveCamera( const Vec3f& rOrigin, const Vec3f& rDirection, const Vec3f& rUp, float fFOV, float fAspect, bool bRightHanded=false )
            : m_vPosition( rOrigin ), m_vDirection( Normalize3( rDirection ) )
        {
            m_vRight = ( bRightHanded ) ? Cross3( m_vDirection, rUp ) : Cross3(rUp, m_vDirection);
            m_vUp = ( bRightHanded ) ? Cross3( m_vRight, m_vDirection ) : Cross3(m_vDirection, m_vRight);

            // distance from center of viewing plane to edges
            // assuming a hither distance of 1, it is:  tan(fov/2.0)
            float fPlaneWidth = tan( (fFOV * (3.1415926f/180.0f))/2.0f );
            
            // normalize basis vectors and rescale them according to FOV and aspect ratio
            m_vRight *= (fPlaneWidth*fAspect) / Length3( m_vRight );
            m_vUp *= (fPlaneWidth) / Length3( m_vUp );
        }

        /// Returns the position of the camera
        inline const Vec3f& GetPosition() const { return m_vPosition; };
        
        /// \brief Returns the viewing direction of the camera.  
        /// This is the unit vector passing through the center of the image plane
        inline const Vec3f& GetDirection() const { return m_vDirection; };
        
        /// \brief Returns a vector aligned to the horizontal axis of the image plane (and pointing right)
        ///  The length of this vector depends on the camera field of view and aspect ratio, and is equal to half the width of the image plane
        inline const Vec3f& GetXBasis() const { return m_vRight; };
        
        /// \brief Returns a vector aligned to the vertical axis of the image plane (and pointing up)
        /// The length of this vector depends on the camera field of view, and is equal to half the height of the image plane 
        ///  This is not always the same as the 'up' vector used to construct the camera
        inline const Vec3f& GetYBasis() const { return m_vUp; };

        /// \brief Computes the direction of a ray passing through a point on the image plane
        /// \param rPosNDC  Sample location in NDC space.  NDC space ranges from 0-1 on each axis over the length of the screen,
        ///                      with 0,0 at the top-left)
        inline Vec3f GetRayDirectionNDC( const Vec2f& rPosNDC ) const 
        {
            return m_vDirection + (m_vRight*(rPosNDC.x*2.0f - 1.0f)) - (m_vUp*(rPosNDC.y*2.0f - 1.0f));
        };

    private:

        Vec3f m_vPosition;
        Vec3f m_vDirection;
        Vec3f m_vRight;
        Vec3f m_vUp;
    };

}

#endif // _TRT_PERSPECTIVECAMERA_H_
