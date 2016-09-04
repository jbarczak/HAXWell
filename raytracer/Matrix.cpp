//=====================================================================================================================
//
//   Matrix.cpp
//
//   Matrix Utilities
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2010 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================

#include "Matrix.h"
#include "MiscMath.h"

namespace Simpleton
{

   
    Matrix4f MatrixScale( float x, float y, float z )
    {
        float values[] = {
            x,0,0,0,
            0,y,0,0,
            0,0,z,0,
            0,0,0,1
        };

        return Matrix4f(values);
    }


    Matrix4f MatrixTranslate( float x, float y, float z )
    {
        float values[] = { // NOTE: column major!
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            x,y,z,1
        };

        return Matrix4f(values);
    }

    Matrix4f MatrixRotate( float x, float y, float z, float fAngleInRads )
    {
        Vec3f r(x,y,z);
        r = Normalize3(r);

        float cosx = cos( fAngleInRads );
        float sinx = sin( fAngleInRads );

        // see Akenine-Moeller Haines, real-time rendering, pg 43
        // originally published by ron goldman
        Matrix4f rotation;
        rotation.Set( 0,0, cosx + ((1-cosx)*r.x*r.x) );
        rotation.Set( 0,1, ((1-cosx)*r.x*r.y) - (r.z*sinx) );
        rotation.Set( 0,2, ((1-cosx)*r.x*r.z) + (r.y*sinx) );
        rotation.Set( 0,3,0);
        
        rotation.Set( 1,0, ((1-cosx)*r.x*r.y) + r.z*sinx );
        rotation.Set( 1,1, cosx + (1-cosx)*r.y*r.y );
        rotation.Set( 1,2, ((1-cosx)*r.y*r.z) - r.x*sinx );
        rotation.Set( 1,3,0);
        
        rotation.Set( 2,0, ((1-cosx)*r.x*r.z) - r.y*sinx );
        rotation.Set( 2,1, ((1-cosx)*r.y*r.z) + r.x*sinx );
        rotation.Set( 2,2, cosx + (1-cosx)*r.z*r.z );
        rotation.Set( 2,3,0); 

        rotation.Set(3,0,0);
        rotation.Set(3,1,0);
        rotation.Set(3,2,0);
        rotation.Set(3,3,1);

        return rotation;
    }

    Matrix4f MatrixLookAtLH( const Vec3f& rEye, const Vec3f& rAt, const Vec3f& rUp )
    {
        Vec3f z = Normalize3(rAt - rEye);
        Vec3f x = Cross3( rUp, z );
        Vec3f y = Cross3( z, x );
  
        Matrix4f mat;
        mat.Set(0, 0, x.x);
	    mat.Set(0, 1, x.y);
	    mat.Set(0, 2, x.z);
	    mat.Set(0, 3, -Dot3(rEye,x));

	    mat.Set(1, 0, y.x);
	    mat.Set(1, 1, y.y);
	    mat.Set(1, 2, y.z);
	    mat.Set(1, 3, -Dot3(rEye,y));
    	
	    mat.Set(2, 0, z.x);
	    mat.Set(2, 1, z.y);
	    mat.Set(2, 2, z.z);
	    mat.Set(2, 3, -Dot3(rEye,z));

        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,0);
        mat.Set(3,3,1);

        return mat;        
    }   


    Matrix4f Inverse( const Matrix4f& rM )
    {
        // code borrowed from: http://rodolphe-vaillant.fr/?e=7 
        //  who in turn borrowed it from mesa
        const float* m = rM.GetColumnMajor();
        float inv[16], det;
 
        inv[ 0] =  m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[ 4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[ 8] =  m[4] * m[ 9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[ 9];
        inv[12] = -m[4] * m[ 9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[ 9];
        inv[ 1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[ 5] =  m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[ 9] = -m[0] * m[ 9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[ 9];
        inv[13] =  m[0] * m[ 9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[ 9];
        inv[ 2] =  m[1] * m[ 6] * m[15] - m[1] * m[ 7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[ 7] - m[13] * m[3] * m[ 6];
        inv[ 6] = -m[0] * m[ 6] * m[15] + m[0] * m[ 7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[ 7] + m[12] * m[3] * m[ 6];
        inv[10] =  m[0] * m[ 5] * m[15] - m[0] * m[ 7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[ 7] - m[12] * m[3] * m[ 5];
        inv[14] = -m[0] * m[ 5] * m[14] + m[0] * m[ 6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[ 6] + m[12] * m[2] * m[ 5];
        inv[ 3] = -m[1] * m[ 6] * m[11] + m[1] * m[ 7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[ 9] * m[2] * m[ 7] + m[ 9] * m[3] * m[ 6];
        inv[ 7] =  m[0] * m[ 6] * m[11] - m[0] * m[ 7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[ 8] * m[2] * m[ 7] - m[ 8] * m[3] * m[ 6];
        inv[11] = -m[0] * m[ 5] * m[11] + m[0] * m[ 7] * m[ 9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[ 9] - m[ 8] * m[1] * m[ 7] + m[ 8] * m[3] * m[ 5];
        inv[15] =  m[0] * m[ 5] * m[10] - m[0] * m[ 6] * m[ 9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[ 9] + m[ 8] * m[1] * m[ 6] - m[ 8] * m[2] * m[ 5];
 
        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
 
        if(det == 0)
            return Matrix4f::Zero();
 
        det = 1.f / det;
 
        float invOut[16];
        for(int i = 0; i < 16; i++)
            invOut[i] = inv[i] * det;
 
        return Matrix4f(invOut);
    }


    Matrix4f MatrixLookAtRH( const Vec3f& rEye, const Vec3f& rAt, const Vec3f& rUp )
    {
        Vec3f z = Normalize3(rEye - rAt );
        Vec3f x = Cross3( rUp, z );
        Vec3f y = Cross3( z, x );
  
        Matrix4f mat;
        mat.Set(0, 0, x.x);
	    mat.Set(0, 1, x.y);
	    mat.Set(0, 2, x.z);
	    mat.Set(0, 3, -Dot3(rEye,x));

	    mat.Set(1, 0, y.x);
	    mat.Set(1, 1, y.y);
	    mat.Set(1, 2, y.z);
	    mat.Set(1, 3, -Dot3(rEye,y));
    	
	    mat.Set(2, 0, z.x);
	    mat.Set(2, 1, z.y);
	    mat.Set(2, 2, z.z);
	    mat.Set(2, 3, -Dot3(rEye,z));

        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,0);
        mat.Set(3,3,1);

        return mat;        
    }   

    Matrix4f MatrixPerspectiveFovLH( float fAspect, float fFOV, float fNear, float fFar )
    {
        float yScale = 1.0f/tanf( Radians(fFOV/2) );
        float xScale = yScale/fAspect;
        
        Matrix4f mat;
        mat.Set(0,0,xScale);
        mat.Set(0,1,0);
        mat.Set(0,2,0);
        mat.Set(0,3,0);

        mat.Set(1,0,0);
        mat.Set(1,1,yScale);
        mat.Set(1,2,0);
        mat.Set(1,3,0);
        
        mat.Set(2,0,0);
        mat.Set(2,1,0);
        mat.Set(2,2,fFar/(fFar-fNear));
        mat.Set(2,3,-fNear*fFar/(fFar-fNear));
        
        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,1);
        mat.Set(3,3,0);

        return mat;
    }

    Matrix4f MatrixPerspectiveFovRH( float fAspect, float fFOV, float fNear, float fFar )
    {
        float yScale = 1.0f/tanf( Radians(fFOV/2) );
        float xScale = yScale/fAspect;
        
        Matrix4f mat;
        mat.Set(0,0,xScale);
        mat.Set(0,1,0);
        mat.Set(0,2,0);
        mat.Set(0,3,0);

        mat.Set(1,0,0);
        mat.Set(1,1,yScale);
        mat.Set(1,2,0);
        mat.Set(1,3,0);
        
        mat.Set(2,0,0);
        mat.Set(2,1,0);
        mat.Set(2,2,-fFar/(fFar-fNear));
        mat.Set(2,3,-fNear*fFar/(fFar-fNear));
        
        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,-1);
        mat.Set(3,3,0);

        return mat;
    }

    Matrix4f MatrixOrthoRH( float w, float h, float fNear, float fFar )
    {
        Matrix4f mat;
        mat.Set(0,0,2/w);
        mat.Set(0,1,0);
        mat.Set(0,2,0);
        mat.Set(0,3,0);

        mat.Set(1,0,0);
        mat.Set(1,1,2/h);
        mat.Set(1,2,0);
        mat.Set(1,3,0);
        
        mat.Set(2,0,0);
        mat.Set(2,1,0);
        mat.Set(2,2,1/(fNear-fFar));
        mat.Set(2,3,fNear/(fNear-fFar));
        
        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,0);
        mat.Set(3,3,1);
        return mat;
    }

    Matrix4f MatrixOrthoLH( float w, float h, float fNear, float fFar )
    {
        Matrix4f mat;
        mat.Set(0,0,2/w);
        mat.Set(0,1,0);
        mat.Set(0,2,0);
        mat.Set(0,3,0);

        mat.Set(1,0,0);
        mat.Set(1,1,2/h);
        mat.Set(1,2,0);
        mat.Set(1,3,0);
        
        mat.Set(2,0,0);
        mat.Set(2,1,0);
        mat.Set(2,2,1/(fFar-fNear));
        mat.Set(2,3,-fNear/(fFar-fNear));
        
        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,0);
        mat.Set(3,3,1);
        return mat;
    }
   
    
    Matrix4f MatrixCoordinateFrame( const Vec3f& x, const Vec3f& y, const Vec3f& z )
    {  
        Matrix4f mat;
        mat.Set(0, 0, x.x);
	    mat.Set(0, 1, x.y);
	    mat.Set(0, 2, x.z);
	    mat.Set(0, 3, 0);

	    mat.Set(1, 0, y.x);
	    mat.Set(1, 1, y.y);
	    mat.Set(1, 2, y.z);
	    mat.Set(1, 3, 0);
    	
	    mat.Set(2, 0, z.x);
	    mat.Set(2, 1, z.y);
	    mat.Set(2, 2, z.z);
	    mat.Set(2, 3, 0);

        mat.Set(3,0,0);
        mat.Set(3,1,0);
        mat.Set(3,2,0);
        mat.Set(3,3,1);

        return mat;
    }

    Matrix4f MatrixIdentity()
    {
        float m[] = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        return Matrix4f(m);
    }


    Vec3f AffineTransformPoint( const Matrix4f& rM, const Vec3f& P )
    {
        const float* pM = rM.GetColumnMajor();
        return Vec3f( pM[0]*P.x + pM[4]*P.y + pM[8]*P.z + pM[12],
                      pM[1]*P.x + pM[5]*P.y + pM[9]*P.z + pM[13],
                      pM[2]*P.x + pM[6]*P.y + pM[10]*P.z + pM[14] );
    }
    
    Vec3f TransformPoint( const Matrix4f& rM, const Vec3f& P )
    {
        const float* pM = rM.GetColumnMajor();
        Vec3f v = Vec3f( pM[0]*P.x + pM[4]*P.y + pM[8]*P.z + pM[12],
                      pM[1]*P.x + pM[5]*P.y + pM[9]*P.z + pM[13],
                      pM[2]*P.x + pM[6]*P.y + pM[10]*P.z + pM[14] );
        float w = pM[3]*P.x + pM[7]*P.y + pM[11]*P.z + pM[15];
        return v*(1.0f/w);
    }

    Vec3f AffineTransformDirection( const Matrix4f& rM, const Vec3f& P )
    {
        const float* pM = rM.GetColumnMajor();
        return Vec3f( pM[0]*P.x + pM[4]*P.y + pM[8]*P.z ,
                      pM[1]*P.x + pM[5]*P.y + pM[9]*P.z ,
                      pM[2]*P.x + pM[6]*P.y + pM[10]*P.z  );
    }

    Vec3f AffineTransformNormal( const Matrix4f& rM, const Vec3f& P )
    {
        const float* pM = rM.GetColumnMajor();
        return Vec3f( pM[0]*P.x + pM[1]*P.y + pM[2]*P.z ,
                      pM[4]*P.x + pM[5]*P.y + pM[6]*P.z ,
                      pM[8]*P.x + pM[9]*P.y + pM[10]*P.z  );
    }

   
}

