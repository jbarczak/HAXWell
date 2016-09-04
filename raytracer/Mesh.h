//=====================================================================================================================
//
//   Mesh.h
//
//   Various mesh processing utilities
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2014 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================

#ifndef _SIMPLETON_MESH_H_
#define _SIMPLETON_MESH_H_

#include <math.h>

namespace Simpleton
{
    template< class PositionAccessor_T, class Index_T, class NormalWriter_T >
    void ComputeFaceNormals( const Index_T* pIndices, unsigned int nTriangles, PositionAccessor_T GetPosition, NormalWriter_T WriteNormal )
    {
        for( unsigned int i=0; i<nTriangles; i++ )
        {
            float P0[3];
            float P1[3];
            float P2[3];
            GetPosition( P0, pIndices[3*i] );
            GetPosition( P1, pIndices[3*i+1] );
            GetPosition( P2, pIndices[3*i+2] );

            float V0[3];
            float V1[3];
            for( int k=0; k<3; k++ )
            {
                V0[k] = P1[k]-P0[k];
                V1[k] = P2[k]-P0[k];
            }

            float N[3] = {
                V0[1]*V1[2] - V0[2]*V1[1],
                V0[2]*V1[0] - V0[0]*V1[2],
                V0[0]*V1[1] - V0[1]*V1[0]
            };

            float nrm = 1.0f / sqrtf( N[0]*N[0] + N[1]*N[1] + N[2]*N[2] );
            N[0] *= nrm;
            N[1] *= nrm;
            N[2] *= nrm;
            WriteNormal(N,i);
        }
    }

    template< class Index_T, class PositionAccessor_T, class NormalReader_T, class NormalWriter_T >
    void ComputeVertexNormals( const Index_T* pIndices, unsigned int nTriangles, unsigned int nVertices,
                               PositionAccessor_T GetPosition, NormalReader_T ReadNormal, NormalWriter_T WriteNormal )
    {
        float zero[3] = {0,0,0};
        for( unsigned int i=0; i<nVertices; i++ )
            WriteNormal(zero,i);
        
        for( unsigned int i=0; i<nTriangles; i++ )
        {
            unsigned int v0 = pIndices[3*i];
            unsigned int v1 = pIndices[3*i+1];
            unsigned int v2 = pIndices[3*i+2];
            
            float P0[3];
            float P1[3];
            float P2[3];
            GetPosition( P0, v0 );
            GetPosition( P1, v1 );
            GetPosition( P2, v2 );

            float V0[3];
            float V1[3];
            for( int i=0; i<3; i++ )
            {
                V0[i] = P1[i]-P0[i];
                V1[i] = P2[i]-P0[i];
            }

            float N[3] = {
                V0[1]*V1[2] - V0[2]*V1[1],
                V0[2]*V1[0] - V0[0]*V1[2],
                V0[0]*V1[1] - V0[1]*V1[0]
            };

            float Nv0[3];
            float Nv1[3];
            float Nv2[3];
            ReadNormal(Nv0,v0);
            ReadNormal(Nv1,v1);
            ReadNormal(Nv2,v2);
            for( int i=0; i<3; i++ )
            {
                Nv0[i] += N[i];
                Nv1[i] += N[i];
                Nv2[i] += N[i];
            }
            WriteNormal(Nv0,v0);
            WriteNormal(Nv1,v1);
            WriteNormal(Nv2,v2);
        }
        
        for( unsigned int i=0; i<nVertices; i++ )
        {
            float N[3];
            ReadNormal(N,i);
            float nrm = 1.0f / sqrtf( N[0]*N[0] + N[1]*N[1] + N[2]*N[2] );
            N[0] *= nrm;
            N[1] *= nrm;
            N[2] *= nrm;
            WriteNormal(N,i);
        }
       
    }


    void ExpandTriangleStrip( uint* pList, const uint* pStrip, uint nTriangles )
    {
        pList[0] = pStrip[0];
        pList[1] = pStrip[1];
        pList[2] = pStrip[2];
        for( uint i=1; i<nTriangles; i++ )
        {
            pList[3*i+0] = pStrip[i];
            pList[3*i+1] = pStrip[i+1];
            pList[3*i+2] = pStrip[i+2];
        }

        // flip winding on every other tri
        for( uint i=1; i<nTriangles; i += 2 )
        {
            uint i0 = pList[3*i+0];
            uint i2 = pList[3*i+2];
            pList[3*i+0] = i2;
            pList[3*i+2] = i0;
        }
    }

}


#endif