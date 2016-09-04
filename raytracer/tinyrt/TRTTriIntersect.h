//=====================================================================================================================
//
//   TRTTriIntersect.h
//
//   Ray-triangle intersection tests
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_TRIINTERSECT_H_
#define _TRT_TRIINTERSECT_H_


namespace TinyRT
{
     //=====================================================================================================================
    /// \ingroup TinyRT
    /// \param P0           First vertex
    /// \param P1           Second vertex
    /// \param P2           Third vertex
    /// \param rRay         The ray to be tested
    /// \param rfTHit       Receives the distance to the hit, if one is found
    /// \param rUVCoords    Receives the barycentric coordinates at the intersection location.  
    ///                         These are updated only in the event of a hit.
    ///
    /// \return True if a hit was found, false otherwise
    //=====================================================================================================================
    template< typename Ray_T, class Vec3_T, class Vec2_T >
    bool RayTriangleTest( const Vec3_T& P0, const Vec3_T& P1, const Vec3_T& P2, const Ray_T& rRay, float& rfTHit, Vec2_T& rUVCoords )
    {
        
        Vec3_T v10 = ( P1 - P0 );
        Vec3_T v02 = ( P0 - P2 );
        Vec3_T v10x02 = Cross3( v10, v02 );
        Vec3_T v0A = ( P0 - rRay.Origin() );
        Vec3_T v02x0a = Cross3( v02, v0A );

        const Vec3_T& rDirection = rRay.Direction();
        float V = 1.0f / Dot3( v10x02, rDirection );
        float A = V * Dot3( v02x0a, rDirection );
     
        if( A >= 0.0f )
        {
            Vec3_T v10x0a = Cross3( v10, v0A );
            float B = V * Dot3( v10x0a, rDirection );
            
            if( B >= 0.0f && (A+B) <= 1.0f )
            {
                float VA = Dot3( v10x02, v0A );
                float T = VA*V;
                if( rRay.IsDistanceValid( T ) )
                {
                    rfTHit = T;
                    rUVCoords[0] = 1.0f - (A+B);
                    rUVCoords[1] = A;
                    return true;
                }
            }
        }

        return false;        
    }

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// A vectorized ray-triangle test
    ///
    /// Tests one ray against N triangles in SIMD.  All parameters are given in SoA form
    //=====================================================================================================================
    inline int RayTriangleTestSimd( const SimdVecf P0[3], const SimdVecf P1[3], const SimdVecf P2[3], 
                                    const SimdVecf vOrigin[3], const SimdVecf vDirection[3],
                                    float pTHit[SimdVecf::WIDTH], float pUV[2][SimdVecf::WIDTH] )
    {
        
        SimdVecf v10[3] = { P1[0] - P0[0], P1[1] - P0[1], P1[2]-P0[2] };
        SimdVecf v02[3] = { P0[0] - P2[0], P0[1] - P2[1], P0[2]-P2[2] };
        SimdVecf v10x02[3];
        Cross3( v10, v02, v10x02 );
        
        SimdVecf v0a[3] = { P0[0] - vOrigin[0], P0[1] - vOrigin[1], P0[2] - vOrigin[2] };
        
        SimdVecf v02x0a[3];
        SimdVecf v10x0a[3];
        Cross3( v02, v0a, v02x0a );
        Cross3( v10, v0a, v10x0a );

        SimdVecf V  = SimdVecf::Rcp( Dot3( v10x02, vDirection ) );
        SimdVecf A  = V * Dot3( v02x0a, vDirection );
        SimdVecf B  = V * Dot3( v10x0a, vDirection );
        SimdVecf VA = Dot3( v10x02, v0a );
        SimdVecf T = VA * V;

        int nMask = SimdVecf::Mask( A >= SimdVecf::Zero() & B >= SimdVecf::Zero() & (A+B) <= SimdVecf(1.0f) );
       
        T.Store( pTHit );
        SimdVecf vU = SimdVecf( 1.0f ) - (A+B) ;
        SimdVecf vV = A;
        vU.Store( pUV[0] );
        vV.Store( pUV[1] );
    
        return nMask;
    }
}

#endif // _TRT_TRIINTERSECT_H_



