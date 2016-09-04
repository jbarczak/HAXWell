//=====================================================================================================================
//
//   TRTPacketFrustum.h
//
//   Definition of class: TinyRT::PacketFrustum
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_PACKETFRUSTUM_H_
#define _TRT_PACKETFRUSTUM_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Encapsulates a four-plane frustum which bounds a group of rays
    ///
    ///  This class provides an implementation of various SIMD-optimized frustum intersection tests, 
    ///    but it is designed around the fast frustum/box test described by Reshetov et al. in the MLRT paper (Siggraph 2005).
    ///
    ///  The frustum is defined by a set of four planes whose normals point inside frustum
    ///
    //=====================================================================================================================
    class PacketFrustum
    {
    public:

        /// Result codes used for precise culling tests
        enum CullResult
        {
            CULL_INSIDE,    ///< The bounding volume is completely contained in the frustum
            CULL_OUTSIDE,   ///< The bounding volume is completely outside the frustum
            CULL_STRADDLE   ///< The bounding volume is straddling the frustum
        };

     
        /// \brief Derives a bounding frustum for a set of common-origin bounding rays
        /// \param rOrigin          The shared ray origin
        /// \param vBoundingRays    The four ray directions which bound the ray set.  
        ///                          Elements 0,1,and 2 contain X,Y, and Z components, respectively.
        ///                          The rays must NOT be degenerate (all four rays must be unique).
        inline void SetFromBoundingRays( const Vec3f& rOrigin, const SimdVec4f vBoundingRays[3] )
        {
            // The bounding rays are laid out like this:
            //  A   B     ^
            //  C   D   U |
            //            *--> V
            // 
            // The normals for the four planes are:  AxC, DxB, CxD, and BxA (left,right,bottom,top)
            //   If we look at the ray indices its:  0x2, 3x1, 2x3, and 1x0
            //
            // We compute the plane normals using a vectorized cross product
            SimdVec4f vA[4] = {
                vBoundingRays[0].Swizzle<0,3,2,1>(),
                vBoundingRays[1].Swizzle<0,3,2,1>(),
                vBoundingRays[2].Swizzle<0,3,2,1>()
            };
            SimdVec4f vB[4] = {
                vBoundingRays[0].Swizzle<2,1,3,0>(),
                vBoundingRays[1].Swizzle<2,1,3,0>(),
                vBoundingRays[2].Swizzle<2,1,3,0>()
            };
            
            SimdVec4f vNormals[3];
            Cross3( vA, vB, vNormals );
            
            // compute 'D' term using vectorized dot product (note that it needs to be negated)
            SimdVec4f vOrigin[3] = { SimdVec4f( -rOrigin[0] ), SimdVec4f( -rOrigin[1] ), SimdVec4f( -rOrigin[2] ) };
            SimdVec4f vD = Dot3( vNormals, vOrigin );

            SetFromPlanes( vNormals[0], vNormals[1], vNormals[2], vD );
        };

        /// \brief Sets the four bounding planes for the frustum explicitly, based on four plane equations.
        ///
        ///  We use the plane equation form:  Ax + By + Cz + D = 0
        ///  The plane order is:  left, right, bottom, top.  Plane normals must point inside the frustum
        inline void SetFromPlanes( const SimdVec4f& vA, const SimdVec4f& vB, const SimdVec4f& vC, const SimdVec4f& vD )
        {
            X[0] = (vA < 0.0f) & vA ;
            X[1] = (vA > 0.0f) & vA ;
            Y[0] = (vB < 0.0f) & vB ;
            Y[1] = (vB > 0.0f) & vB ;
            Z[0] = (vC < 0.0f) & vC ;
            Z[1] = (vC > 0.0f) & vC ;
            D = vD;
        }

        /// \brief Performs a conservative frustum-box rejection test.  Returns true if the frustum and box are seperated.
        inline bool RejectBox( const Vec3f& rMin, const Vec3f& rMax ) const
        {
            // frustum-box trivial rejection test, based on Reshetov's MLRT paper
            // This code is basically doing four plane-point dot-products in parallel.  The X[0],X[1] stuff
            // is there to select the appropriate corner of the bounding box for each plane
            SimdVec4f vPlane = D;
            vPlane += ( X[0]*SimdVec4f(rMin.x) ) + ( X[1]*SimdVec4f(rMax.x) );
            vPlane += ( Y[0]*SimdVec4f(rMin.y) ) + ( Y[1]*SimdVec4f(rMax.y) );
            vPlane += ( Z[0]*SimdVec4f(rMin.z) ) + ( Z[1]*SimdVec4f(rMax.z) );
            int nMask = SimdVec4f::Mask( vPlane < SimdVec4f::Zero() );
            return ( nMask != 0 );
        };

        /// \brief Performs a frustum-triangle rejection test.  Returns true if the frustum and triangle are seperated
        inline bool RejectTri( const Vec3f& v0, const Vec3f& v1, const Vec3f& v2 ) const 
        {
            SimdVec4f t0 = ( X[0] + X[1] ) * SimdVec4f(-v0.x);
                    t0 += ( Y[0] + Y[1] ) * SimdVec4f(-v0.y);
                    t0 += ( Z[0] + Z[1] ) * SimdVec4f(-v0.z);
                        
            SimdVec4f t1 = ( X[0] + X[1] ) * SimdVec4f(-v1.x);
                    t1 += ( Y[0] + Y[1] ) * SimdVec4f(-v1.y);
                    t1 += ( Z[0] + Z[1] ) * SimdVec4f(-v1.z);
                        
            SimdVec4f t2 = ( X[0] + X[1] ) * SimdVec4f(-v2.x);
                    t2 += ( Y[0] + Y[1] ) * SimdVec4f(-v2.y);
                    t2 += ( Z[0] + Z[1] ) * SimdVec4f(-v2.z);
            
            int nMask = SimdVec4f::Mask( D < SimdVec4f::Min( t0, SimdVec4f::Min( t1, t2 ) ) );
            return ( nMask != 0 );
        }


        /// \sa RejectBox
        /// \brief Performs a richer frustum culling test and returns one of three cases:  OUTSIDE, INSIDE, STRADDLING
        inline CullResult CullBox( const Vec3f& rMin, const Vec3f& rMax ) const
        {    
            // In this case, we want a ternary result (inside, outside, or straddling)

            // test near corner
            SimdVec4f vPlane = D;
            vPlane += ( X[0]*SimdVec4f(rMin.x) ) + ( X[1]*SimdVec4f(rMax.x) );
            vPlane += ( Y[0]*SimdVec4f(rMin.y) ) + ( Y[1]*SimdVec4f(rMax.y) );
            vPlane += ( Z[0]*SimdVec4f(rMin.z) ) + ( Z[1]*SimdVec4f(rMax.z) );
            int nMask = SimdVec4f::Mask( vPlane < SimdVec4f::Zero() );
            if( nMask != 0 )
                return CULL_OUTSIDE; // nearest corner is outside a plane, reject

            // test far corner
            vPlane = D;
            vPlane += ( X[1]*SimdVec4f(rMin.x) ) + ( X[0]*SimdVec4f(rMax.x) );
            vPlane += ( Y[1]*SimdVec4f(rMin.y) ) + ( Y[0]*SimdVec4f(rMax.y) );
            vPlane += ( Z[1]*SimdVec4f(rMin.z) ) + ( Z[0]*SimdVec4f(rMax.z) );
            nMask = SimdVec4f::Mask( vPlane < SimdVec4f::Zero() );
            if( nMask != 0 )
                return CULL_STRADDLE;  // furthest corner is outside a plane, straddling case
            else
                return CULL_INSIDE;  // near and far corners are inside all planes
        };

        /// Returns true if a particular point is outside the frustum 
        inline bool RejectPoint( const Vec3f& rPoint ) const
        {
            SimdVec4f vPlane = D;
            vPlane += ( X[1]*SimdVec4f(rPoint.x) ) + ( X[0]*SimdVec4f(rPoint.x) );
            vPlane += ( Y[1]*SimdVec4f(rPoint.y) ) + ( Y[0]*SimdVec4f(rPoint.y) );
            vPlane += ( Z[1]*SimdVec4f(rPoint.z) ) + ( Z[0]*SimdVec4f(rPoint.z) );
            return SimdVec4f::Mask( vPlane < SimdVec4f::Zero() ) != 0 ;
        };

        /// \brief Helper method which calls 'RejectBox' on the two corners of an AxisAlignedBox
        /// \sa RejectBox
        inline bool RejectBox( const AxisAlignedBox& rBox ) const { return RejectBox( rBox.Min(), rBox.Max() ); };
        
        /// \brief Helper method which calls 'CullBox' on the two corners of an AxisAlignedBox
        /// \sa CullBox
        inline CullResult CullBox( const AxisAlignedBox& rBox ) const { return CullBox( rBox.Min(), rBox.Max() ); };


        //  Each of the X,Y, and Z members contains two copies of the corresponding component of the plane normals.
        //   The X[0], Y[0] and Z[0] terms contain the plane normal values (if it is negative), or 0 otherwise.
        //   The X[1], Y[1] and Z[1] terms contain the plane normal values (if they are positive), or 0 otherwise.
    
        SimdVec4f X[2];
        SimdVec4f Y[2];
        SimdVec4f Z[2];
        SimdVec4f D;
        
    };



}

#endif // _TRT_PACKETFRUSTUM_H_
