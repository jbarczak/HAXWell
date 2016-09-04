//=====================================================================================================================
//
//   TRTPolygonIntersect.h
//
//   Ray/polygon intersection for general polygons
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2014 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_POLYGONINTERSECT_H_
#define _TRT_POLYGONINTERSECT_H_

namespace TinyRT
{
    //=====================================================================================================================
    //=====================================================================================================================
    template< class Ray_T >
    bool RayPolygonTest( const Ray_T& ray, const Vec3f& N, const Vec3f* pVerts, uint nVerts, float& rTOut )
    {
        const Vec3f& O = ray.Origin();
        const Vec3f& D = ray.Direction();

        // ray-plane test first
        float d = Dot3( N, pVerts[0] );
        float t = (d-Dot3(N,O) ) / Dot3(N,D);
        if( !ray.IsDistanceValid(t) )
            return false;

        // choose axis to project onto for determinant tests
        uint nMax = (fabs(N.x)>fabs(N.y)) ? 0 : 1;
        nMax      = (fabs(N.z)>fabs(N[nMax])) ? 2 :nMax;
        uint axis0 = (nMax+1) % 3; // 1,2,0 
        uint axis1 = (nMax+2) % 3; // 2,0,1

        float xh = O[axis0] + t*D[axis0];
        float yh = O[axis1] + t*D[axis1];
        float x0,x1,y0,y1;

        uint n=0;
        for( uint i=0; i<nVerts-1; i++ )
        {
            x0 = pVerts[i][axis0];
            y0 = pVerts[i][axis1];
            x1 = pVerts[i+1][axis0];
            y1 = pVerts[i+1][axis1];
            if( y1 < y0 )
            {
                std::swap(y0,y1);
                std::swap(x0,x1);
            }
            n += ( yh >= y0 && yh <= y1 ) &&
                 (( x1 - x0 )*( yh - y0 ) >= ( xh - x0 )*( y1 - y0 )) ? 1 : 0;
        }

        // wraparound edge
        x0 = pVerts[nVerts-1][axis0];
        y0 = pVerts[nVerts-1][axis1];
        x1 = pVerts[0][axis0];
        y1 = pVerts[0][axis1];
        if( y1 < y0 )
        {
            std::swap(y0,y1);
            std::swap(x0,x1);
        }
        n += ( yh >= y0 && yh <= y1 ) &&
                 (( x1 - x0 )*( yh - y0 ) >= ( xh - x0 )*( y1 - y0 )) ? 1 : 0;

        if( (n&1) == 0 )
            return false;
        
        rTOut=t;
        return true;
    }
}

#endif