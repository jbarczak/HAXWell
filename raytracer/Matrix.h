//=====================================================================================================================
//
//   Matrix.h
//
//   Matrix utilities
//      Very ancient code, some of which I wrote in college.  Don't judge me...
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2010 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <string.h>
#include "VectorMath.h"

namespace Simpleton
{

    //=====================================================================================================================
    /// \ingroup Simpleton
    /// \brief Simple templatized matrix class, column-major storage
    ///
    /// 
    //=====================================================================================================================
    template <class T, int SIZE>
    class Matrix
    {
    public:

        Matrix()
        {
        };

        Matrix( const T values[(SIZE*SIZE)] )
        {
            memcpy(m_values, values, (SIZE*SIZE)*sizeof(T));
        };

        Matrix( const Matrix<T,SIZE>& rhs )
        {
            *this = rhs;
        };

        const Matrix<T,SIZE>& operator=( const Matrix<T,SIZE>& rhs )
        {
            memcpy(m_values, rhs.m_values, (SIZE*SIZE)*sizeof(T));
            return *this;
        };

        /// Returns pointer to matrix data in COLUMN-major order
        const T* GetColumnMajor() const { return m_values; };
        T* GetColumnMajor()  { return m_values; };

        operator const T*() const { return (const T*) this; };

        operator T*() { return (T*) this; };

        Matrix<T,SIZE>& operator *= ( const Matrix<T,SIZE>& rhs )
        {
            T newdata[(SIZE*SIZE)];
            for( int c=0; c<SIZE; c++ )
	        {
		        for( int r=0; r<SIZE; r++ )
		        {
			        /** loop across rth row and down cth column **/
			        newdata[(SIZE*c)+r] = 0.0;
			        for( int x=0; x<SIZE; x++)
			        {
				        newdata[(SIZE*c)+r] += m_values[(SIZE*x)+r] * rhs.m_values[(SIZE*c)+x];
			        }
		        }
	        }
	        memcpy(m_values,newdata,16*sizeof(T));
            return *this;
        };
        
        Matrix<T,SIZE> operator* ( const Matrix<T,SIZE>& rhs ) const
        {
            T newdata[(SIZE*SIZE)];
	        for( int c=0; c<SIZE; c++ )
	        {
		        for( int r=0; r<SIZE; r++ )
		        {
			        /** loop across rth row and down cth column **/
			        newdata[(SIZE*c)+r] = 0.0;
			        for( int x=0; x<SIZE; x++)
			        {
				        newdata[(SIZE*c)+r] += m_values[(SIZE*x)+r] * rhs.m_values[(SIZE*c)+x];
			        }
		        }
	        }

            return Matrix<T,SIZE>(newdata);
        }

        Matrix<T,SIZE> Transpose() const
        {
            T newdata[(SIZE*SIZE)];

            int i,j;
            for( i=0; i<SIZE; i++)
	        {
		        for( j=0; j<SIZE; j++)
		        {
			        /** loop across ith row and down jth column **/
			        newdata[(SIZE*j)+i] = m_values[(SIZE*i)+j];
		        }
	        }

            return Matrix<T,SIZE>(newdata);
        };

        void Set( unsigned int row, unsigned int col, T val )
        {
            m_values[ (col*SIZE) + row ] = val;
        };

        T Get( unsigned int row, unsigned int col ) const
        {
            return m_values[ (col*SIZE) + row ];
        };

        static Matrix<T,SIZE> Identity()
        {
            Matrix<T,SIZE> r;
            for( int i=0; i<SIZE; i++ )
            {
                for( int j=0; j<i; j++ )
                    r.Set( i,j,0 );
                
                r.Set(i,i,1);

                for( int j=i+1; j<SIZE; j++ )
                    r.Set(i,j,0);
            }
            return r;
        }

        static Matrix<T,SIZE> Zero()
        {
            Matrix<T,SIZE> r;
            for( uint i=0; i<SIZE*SIZE; i++ )
                r.m_values[i] = 0;
            return r;
        }

    private:

        T m_values[(SIZE*SIZE)];

    };


    typedef Matrix<float,2> Matrix2f;
    typedef Matrix<float,3> Matrix3f;
    typedef Matrix<float,4> Matrix4f;

    typedef Matrix<double,2> Matrix2d;
    typedef Matrix<double,3> Matrix3d;
    typedef Matrix<double,4> Matrix4d;

    
    /// Returns a scaling matrix
    Matrix4f MatrixScale( float x, float y, float z );
   
    /// Returns a translation matrix
    Matrix4f MatrixTranslate( float x, float y, float z );

    /// Returns a rotation matrix about the given axis.  Rotation is clockwise
    Matrix4f MatrixRotate( float x, float y, float z, float fAngleInRads );

   
    Matrix4f MatrixLookAtLH( const Vec3f& rEye, const Vec3f& rAt, const Vec3f& rUp );
    Matrix4f MatrixLookAtRH( const Vec3f& rEye, const Vec3f& rAt, const Vec3f& rUp );
    
    /// fov is top-to-bottom in degrees
    ///  Near/Far are distances
    Matrix4f MatrixPerspectiveFovLH( float fAspect, float fFOV, float fNear, float fFar );
    Matrix4f MatrixPerspectiveFovRH( float fAspect, float fFOV, float fNear, float fFar );
    Matrix4f MatrixOrthoRH( float w, float h, float fNear, float fFar );
    Matrix4f MatrixOrthoLH( float w, float h, float fNear, float fFar );
   
    /// Returns a rotation matrix which aligns the given vector along the positive Z axis
    Matrix4f MatrixAlignZToVector( const Vec3f& rVec );

    /// Returns a rotation matrix which aligned the given vector along the positive Y axis
    Matrix4f MatrixAlignYToVector( const Vec3f& rVec );

    /// Transforms into a coordinate system defined by three basis vectors
    Matrix4f MatrixCoordinateFrame( const Vec3f& rXAxis, const Vec3f& rYAxis, const Vec3f& rZAxis );

    /// Generate an identity matrix
    Matrix4f MatrixIdentity();

    /// If matrix is non-invertible, returns a zero matrix
    Matrix4f Inverse( const Matrix4f& rM );

    /// Transforms a point (w=1) by a matrix, multiplying from the right
    Vec3f AffineTransformPoint( const Matrix4f& rM, const Vec3f& P );
    
    /// Transforms a point (w=1) by a matrix, multiplying from the right
    Vec3f TransformPoint( const Matrix4f& rM, const Vec3f& P );
    

    /// Transforms a direction (w=0) by a matrix, multiplying from the right
    Vec3f AffineTransformDirection( const Matrix4f& rM, const Vec3f& P );

    /// Transforms a direction (w=0) by the TRANSPOSE of a matrix, multiplying from the left
    Vec3f AffineTransformNormal( const Matrix4f& rM, const Vec3f& P );
 
    
}

#endif // _LUMATRIX_H_
