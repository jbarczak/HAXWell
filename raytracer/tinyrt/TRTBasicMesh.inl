//=====================================================================================================================
//
//   TRTBasicMesh.inl
//
//   Implementation of class: TinyRT::BasicMesh
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#include "TRTMath.h"

namespace TinyRT
{

    //=====================================================================================================================
    //
    //         Constructors/Destructors
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// \param pVertices    Array of triangle vertex positions
    /// \param pIndices     Array of vertex indices, three per face
    /// \param nVertices    Number of vertices in the mesh
    /// \param nFaces       Number of faces in the mesh
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    BasicMesh< Vec3_T,uint_t >::BasicMesh( Position_T* pVertices, Index_T* pIndices, uint32 nVertices, uint32 nFaces )
        : m_pVertices( pVertices ), m_pIndices( pIndices ), m_nVertices(nVertices), m_nTriangles(nFaces)
    {
    }

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    /// \param nObject  Index of the triangle whose box is desired
    /// \param pBox     A box which is set to the box of the triangle
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    void BasicMesh< Vec3_T,uint_t >::GetObjectAABB( uint32 nObject, AxisAlignedBox& rBox ) const
    {
        const Index_T* pIndices = m_pIndices + 3*nObject;
        const Position_T& v0 = m_pVertices[pIndices[0]];
        const Position_T& v1 = m_pVertices[pIndices[1]];
        const Position_T& v2 = m_pVertices[pIndices[2]];

        rBox.Min() = v0;
        rBox.Max() = v1;
        MinMax3( rBox.Min(), rBox.Max() );
        rBox.Expand( v2 );

    }

    //=====================================================================================================================
    /// \param rBox A box which is initialized to the AABB of the entire mesh
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    inline void BasicMesh< Vec3_T,uint_t >::GetAABB( AxisAlignedBox& rBox ) const
    {
        rBox.ComputeFromPoints( m_pVertices, m_nVertices );
    }

    //=====================================================================================================================
    /// \param pObjectRemap Array containing the triangle from the old ordering that belongs at position i
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    inline void BasicMesh< Vec3_T,uint_t >::RemapObjects( obj_id* pObjectRemap )
    {
        TinyRT::RemapArray( reinterpret_cast<Indices*>( m_pIndices ), m_nTriangles, pObjectRemap );
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    template< typename Ray_T >
    bool BasicMesh< Vec3_T,uint_t >::RayIntersect( Ray_T& rRay, TriangleRayHit& rRayHit, uint32 nObject ) const
    {
        const Index_T* pIndices = m_pIndices + 3*nObject;
        const Position_T& v0 = m_pVertices[pIndices[0]];
        const Position_T& v1 = m_pVertices[pIndices[1]];
        const Position_T& v2 = m_pVertices[pIndices[2]];
        
        float t;
        if( RayTriangleTest( v0, v1, v2, rRay, t, rRayHit.vUVCoords ) )
        {
            rRay.SetMaxDistance( t );
            rRayHit.nTriIdx = nObject;
            return true;
        }
        return false;
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    template< typename Ray_T >
    bool BasicMesh< Vec3_T,uint_t >::RayIntersect( Ray_T& rRay, TriangleRayHit& rRayHit, uint32 nFirstObj, uint32 nLastObj ) const
    {
        SimdVecf P0[3];
        SimdVecf P1[3];
        SimdVecf P2[3];
        float TRT_SIMDALIGN pTHit[ SimdVecf::WIDTH ];
        TRT_SIMDALIGN float pUV[2][ SimdVecf::WIDTH ];

        const Vec3f& rOrigin = rRay.Origin();
        const Vec3f& rDirection = rRay.Direction();
        SimdVecf vOrigin[3]    = { SimdVecf( rOrigin[0] ), SimdVecf( rOrigin[1] ), SimdVecf( rOrigin[2] ) };
        SimdVecf vDirection[3] = { SimdVecf( rDirection[0] ), SimdVecf( rDirection[1] ), SimdVecf( rDirection[2] ) }; 

        // SIMD intersection test for several triangles at a time
        //  Do this until we run out of triangles
        bool bHit=false;
        while( (nLastObj - nFirstObj) >= SimdVecf::WIDTH )
        {
            // assemble a group of triangles into SoA form
            int i=0;
            while( i < SimdVecf::WIDTH )
            {
                const Index_T* pIndices = m_pIndices + 3*(nFirstObj+i);
                const Position_T& v0 = m_pVertices[pIndices[0]];
                const Position_T& v1 = m_pVertices[pIndices[1]];
                const Position_T& v2 = m_pVertices[pIndices[2]];
                for(int j=0; j<3; j++ )
                {
                    P0[j].values[i] = v0[j];
                    P1[j].values[i] = v1[j];
                    P2[j].values[i] = v2[j];
                }
                
                i++;
            }

            // Ray/triangle intersection test (N triangles at once)
            int nMask = RayTriangleTestSimd( P0, P1, P2, vOrigin, vDirection, pTHit, pUV );
            
            // Scan the triangle list and look for hits
            int j=0;
            while( nMask )
            {
                if( (nMask & 1) && rRay.IsDistanceValid( pTHit[j] ) )
                {
                    rRayHit.nTriIdx = nFirstObj + j;
                    rRay.SetMaxDistance( pTHit[j] );
                    rRayHit.vUVCoords[0] = pUV[0][j];
                    rRayHit.vUVCoords[1] = pUV[1][j];
                    bHit = true;
                }
                j++;
                nMask >>= 1;
            }

            nFirstObj += SimdVecf::WIDTH;
        }

        // single-ray test against remaining triangles
        while( nFirstObj != nLastObj )
        {
            bHit = RayIntersect( rRay, rRayHit, nFirstObj++ ) || bHit;
        }

        return bHit;        
    }

    //=====================================================================================================================
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    void BasicMesh<Vec3_T,uint_t>::GetTriangleVertexPositions( face_id nFace, Vec3_T pVerticesOut[3] ) const
    {
        const Index_T* pIB = m_pIndices + 3*nFace;
        pVerticesOut[0] = m_pVertices[pIB[0]];
        pVerticesOut[1] = m_pVertices[pIB[1]];
        pVerticesOut[2] = m_pVertices[pIB[2]];
    }

    //=====================================================================================================================
    //
    //           Protected Methods
    //
    //=====================================================================================================================

    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================
}
