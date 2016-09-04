//=====================================================================================================================
//
//   TRTBoxIntersect.h
//
//   Ray-box intersection routines.
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_BOXINTERSECT_H_
#define _TRT_BOXINTERSECT_H_


namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Ray-box intersection test which returns entry distance.  
    /// \param rBBMin   The lower-left corner of the AABB
    /// \param rBBMax   The upper-right corner of the AABB
    /// \param rRay     The ray to be tested
    /// \param rTMin    If a hit is found, this receives the 'T' value at which the ray enters the box
    ///                     This will be negative if the ray origin is in the box
    /// \return True if the ray hits the box.  False otherwise.  Hits are only returned if the hit lies inside
    ///               of the ray's "valid region"
    //=====================================================================================================================
    template< class Ray_T >
    TRT_FORCEINLINE bool RayAABBTest( const Vec3f& rBBMin, const Vec3f& rBBMax, const Ray_T& rRay, float& rTMinOut )
    {
        const Vec3f& rOrigin = rRay.Origin();
        const Vec3f& rInvDir = rRay.InvDirection();

        float fTMin, fTMax;
        if( rInvDir.x < 0 )
        {
            fTMin = ( rBBMax.x - rOrigin.x ) * rInvDir.x;
            fTMax = ( rBBMin.x - rOrigin.x ) * rInvDir.x;
        }
        else
        {
            fTMin = ( rBBMin.x - rOrigin.x ) * rInvDir.x;
            fTMax = ( rBBMax.x - rOrigin.x ) * rInvDir.x;
        }

        if( rInvDir.y < 0 )
        {
            fTMin = Max( fTMin, ( rBBMax.y - rOrigin.y ) * rInvDir.y );
            fTMax = Min( fTMax, ( rBBMin.y - rOrigin.y ) * rInvDir.y );
        }
        else
        {
            fTMin = Max( fTMin, ( rBBMin.y - rOrigin.y ) * rInvDir.y );
            fTMax = Min( fTMax, ( rBBMax.y - rOrigin.y ) * rInvDir.y );
        }

        if( rInvDir.z < 0 )
        {
            fTMin = Max( fTMin, ( rBBMax.z - rOrigin.z ) * rInvDir.z );
            fTMax = Min( fTMax, ( rBBMin.z - rOrigin.z ) * rInvDir.z );
        }
        else
        {
            fTMin = Max( fTMin, ( rBBMin.z - rOrigin.z ) * rInvDir.z );
            fTMax = Min( fTMax, ( rBBMax.z - rOrigin.z ) * rInvDir.z );
        }

        if( fTMax < fTMin || !rRay.IsIntervalValid( fTMin, fTMax ) )
            return false;

        rTMinOut = fTMin;  // return hit distance
        return true;
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Ray-box intersection test which returns entry and exit distances  
    /// \param rBBMin   The lower-left corner of the AABB
    /// \param rBBMax   The upper-right corner of the AABB
    /// \param rRay     The ray to be tested
    /// \param rTMin    If a hit is found, this receives the 'T' value at which the ray enters the box 
    ///                     This will be negative if the ray origin is in the box
    /// \param rTMax    If a hit is found, this receives the 'T' value at which the ray exits the box
    /// \return True if the ray hits the box.  False otherwise.  Hits are only returned if the hit lies inside
    ///               of the ray's "valid region"
    //=====================================================================================================================
    template< class Ray_T >
    TRT_FORCEINLINE bool RayAABBTest( const Vec3f& rBBMin, const Vec3f& rBBMax, const Ray_T& rRay, float& rTMinOut, float& rTMaxOut )
    {
        const Vec3f& rOrigin = rRay.Origin();
        const Vec3f& rInvDir = rRay.InvDirection();

        float fTMin, fTMax;
        if( rInvDir.x < 0 )
        {
            fTMin = ( rBBMax.x - rOrigin.x ) * rInvDir.x;
            fTMax = ( rBBMin.x - rOrigin.x ) * rInvDir.x;
        }
        else
        {
            fTMin = ( rBBMin.x - rOrigin.x ) * rInvDir.x;
            fTMax = ( rBBMax.x - rOrigin.x ) * rInvDir.x;
        }

        if( rInvDir.y < 0 )
        {
            fTMin = Max( fTMin, ( rBBMax.y - rOrigin.y ) * rInvDir.y );
            fTMax = Min( fTMax, ( rBBMin.y - rOrigin.y ) * rInvDir.y );
        }
        else
        {
            fTMin = Max( fTMin, ( rBBMin.y - rOrigin.y ) * rInvDir.y );
            fTMax = Min( fTMax, ( rBBMax.y - rOrigin.y ) * rInvDir.y );
        }

        if( rInvDir.z < 0 )
        {
            fTMin = Max( fTMin, ( rBBMax.z - rOrigin.z ) * rInvDir.z );
            fTMax = Min( fTMax, ( rBBMin.z - rOrigin.z ) * rInvDir.z );
        }
        else
        {
            fTMin = Max( fTMin, ( rBBMin.z - rOrigin.z ) * rInvDir.z );
            fTMax = Min( fTMax, ( rBBMax.z - rOrigin.z ) * rInvDir.z );
        }

        if( fTMax < fTMin || !rRay.IsIntervalValid( fTMin, fTMax ) )
            return false;

        rTMinOut = fTMin;  // return hit interval
        rTMaxOut = fTMax;   
        return true;
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// Ray-box intersection test which returns NO hit distance
    /// \param rBBMin   The lower-left corner of the AABB
    /// \param rBBMax   The upper-right corner of the AABB
    /// \param rRay     The ray to be tested
    /// \param rTMin    If a hit is found, this receives the 'T' value at which the ray enters the box (it may be negative)
    /// \return True if the ray hits the box.  False otherwise.  Hits are only returned if the hit lies inside
    ///               of the ray's "valid region"
    //=====================================================================================================================
    template< class Ray_T >
    TRT_FORCEINLINE bool RayAABBTest( const Vec3f& rBBMin, const Vec3f& rBBMax, const Ray_T& rRay )
    {
        const Vec3f& rOrigin = rRay.Origin();
        const Vec3f& rInvDir = rRay.InvDirection();

        float fTMin, fTMax;
        if( rInvDir.x < 0 )
        {
            fTMin = ( rBBMax.x - rOrigin.x ) * rInvDir.x;
            fTMax = ( rBBMin.x - rOrigin.x ) * rInvDir.x;
        }
        else
        {
            fTMin = ( rBBMin.x - rOrigin.x ) * rInvDir.x;
            fTMax = ( rBBMax.x - rOrigin.x ) * rInvDir.x;
        }

        if( rInvDir.y < 0 )
        {
            fTMin = Max( fTMin, ( rBBMax.y - rOrigin.y ) * rInvDir.y );
            fTMax = Min( fTMax, ( rBBMin.y - rOrigin.y ) * rInvDir.y );
        }
        else
        {
            fTMin = Max( fTMin, ( rBBMin.y - rOrigin.y ) * rInvDir.y );
            fTMax = Min( fTMax, ( rBBMax.y - rOrigin.y ) * rInvDir.y );
        }

        if( rInvDir.z < 0 )
        {
            fTMin = Max( fTMin, ( rBBMax.z - rOrigin.z ) * rInvDir.z );
            fTMax = Min( fTMax, ( rBBMin.z - rOrigin.z ) * rInvDir.z );
        }
        else
        {
            fTMin = Max( fTMin, ( rBBMin.z - rOrigin.z ) * rInvDir.z );
            fTMax = Min( fTMax, ( rBBMax.z - rOrigin.z ) * rInvDir.z );
        }

        if( fTMax < fTMin || !rRay.IsIntervalValid( fTMin, fTMax ) )
            return false;

        return true;
    }


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Performs an intersection test between a ray and a vectorized set of AABBs
    /// \param vAABB    Array containing sets of four AABB slabs.  The order is:  XMin,XMax, YMin,YMax, ZMin,ZMax
    /// \param rRay     The ray to be tested
    /// \return A four-bit mask indicating which AABBs were hit by the ray
    //=====================================================================================================================
    template< class Ray_T >
    TRT_FORCEINLINE int RayQuadAABBTest( const SimdVec4f vAABB[6], const Ray_T& rRay )
    {
        const Vec3f& rOrigin = rRay.Origin();
        const Vec3f& rInvDir = rRay.InvDirection();

        // intersect with X aligned slabs 
        SimdVec4f vDirInvX = SimdVec4f( rInvDir.x );
        SimdVec4f vTXIn  = (vAABB[0] - SimdVec4f( rOrigin[0] )) * vDirInvX;
        SimdVec4f vTXOut = (vAABB[1] - SimdVec4f( rOrigin[0] )) * vDirInvX;

        // account for negative directions (make sure tMin is < tmax)
        SimdVec4f vTMin = SimdVec4f::Min( vTXIn, vTXOut );
        SimdVec4f vTMax = SimdVec4f::Max( vTXIn, vTXOut );

        // Y aligned slabs
        SimdVec4f vDirInvY = SimdVec4f( rInvDir.y );
        SimdVec4f vTYIn  = (vAABB[2] - SimdVec4f( rOrigin[1] )) * vDirInvY;
        SimdVec4f vTYOut = (vAABB[3] - SimdVec4f( rOrigin[1] )) * vDirInvY;
        vTMin = SimdVec4f::Max( SimdVec4f::Min( vTYIn, vTYOut ), vTMin ); // we want to grab the largest min and smallest max
        vTMax = SimdVec4f::Min( SimdVec4f::Max( vTYIn, vTYOut ), vTMax );

        // Z aligned slabs
        SimdVec4f vDirInvZ = SimdVec4f( rInvDir.z );
        SimdVec4f vTZIn  = (vAABB[4] - SimdVec4f( rOrigin[2] )) * vDirInvZ;
        SimdVec4f vTZOut = (vAABB[5] - SimdVec4f( rOrigin[2] )) * vDirInvZ;
        vTMin = SimdVec4f::Max( SimdVec4f::Min( vTZIn, vTZOut ), vTMin );
        vTMax = SimdVec4f::Min( SimdVec4f::Max( vTZIn, vTZOut ), vTMax );

        // did we hit any of them? If so, return now
        return ( SimdVecf::Mask( (vTMin <= vTMax) & rRay.AreIntervalsValid( vTMin, vTMax ) ) );
    }

    
    //=====================================================================================================================
    /// \ingroup TinyRT
    ///
    /// \brief Performs an intersection test between a ray and a vectorized set of AABBs.  
    ///
    /// This version uses precomputed arrays of values to optimize the calculation
    ///  
    /// \param vAABB        Array containing sets of four AABB slabs.  The order is:  XMin,XMax, YMin,YMax, ZMin,ZMax
    /// \param vSIMDRay     Array containing pre-swizzled ray information.  Elements 0 and 1 contain the x coordinates of
    ///                         the reciprocal direction and origin, respectively.  Remaining elements contain the Y and Z coordinates
    ///                          in the same order
    ///
    /// \param rRay         The ray to be tested
    /// \param nDirSigns    Array containing 16 if the corresponding ray direction component is negative, zero otherwise
    /// \return A four-bit mask indicating which AABBs were hit by the ray
    //=====================================================================================================================
    template< class Ray_T >
    TRT_FORCEINLINE int RayQuadAABBTest( const SimdVec4f vAABB[6], const SimdVec4f vSIMDRay[6], const Ray_T& rRay, const int nDirSigns[3] )
    {
        // storing byte addresses for the slabs instead of just sign-bits removes some address arithmetic 
        const char* pBoxes = reinterpret_cast< const char* >( vAABB );
        SimdVec4f vXMin = SimdVec4f( (float*) (pBoxes + nDirSigns[0]) );
        SimdVec4f vXMax = SimdVec4f( (float*) (pBoxes + (1*sizeof(SimdVec4f) - nDirSigns[0])));
        SimdVec4f vYMin = SimdVec4f( (float*) (pBoxes + (2*sizeof(SimdVec4f) + nDirSigns[1])));
        SimdVec4f vYMax = SimdVec4f( (float*) (pBoxes + (3*sizeof(SimdVec4f) - nDirSigns[1])));
        SimdVec4f vZMin = SimdVec4f( (float*) (pBoxes + (4*sizeof(SimdVec4f) + nDirSigns[2])));
        SimdVec4f vZMax = SimdVec4f( (float*) (pBoxes + (5*sizeof(SimdVec4f) - nDirSigns[2])));
        
        // intersect with X aligned slabs 
        SimdVec4f vDirInvX = vSIMDRay[0];
        SimdVec4f vTMin  = (vXMin - vSIMDRay[1] ) * vDirInvX;
        SimdVec4f vTMax  = (vXMax - vSIMDRay[1] ) * vDirInvX;

        // Y aligned slabs
        SimdVec4f vDirInvY = vSIMDRay[2];
        SimdVec4f vTYIn  = (vYMin - vSIMDRay[3] ) * vDirInvY;
        SimdVec4f vTYOut = (vYMax - vSIMDRay[3] ) * vDirInvY;
        vTMin = SimdVec4f::Max( vTYIn, vTMin ); // we want to grab the largest min and smallest max
        vTMax = SimdVec4f::Min( vTYOut, vTMax );

        // Z aligned slabs
        SimdVec4f vDirInvZ = vSIMDRay[4];
        SimdVec4f vTZIn  = (vZMin - vSIMDRay[5] ) * vDirInvZ;
        SimdVec4f vTZOut = (vZMax - vSIMDRay[5] ) * vDirInvZ;
        vTMin = SimdVec4f::Max( vTZIn, vTMin );
        vTMax = SimdVec4f::Min( vTZOut, vTMax );

        return ( SimdVecf::Mask( (vTMin <= vTMax) & rRay.AreIntervalsValid( vTMin, vTMax ) ) );
    }

}

#endif // _TRT_BOXINTERSECT_H_
