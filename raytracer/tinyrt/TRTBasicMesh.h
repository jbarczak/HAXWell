//=====================================================================================================================
//
//   TRTBasicMesh.h
//
//   Definition of class: TinyRT::BasicMesh
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_BASICMESH_H_
#define _TRT_BASICMESH_H_

#include "TRTTriangleRayHit.h"
#include "TRTTriangleClipper.h"

namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A very simple triangle mesh
    ///
    ///  The BasicMesh class is little more than an adaptor for a set of vertex and index arrays, which are provided
    ///   by the user.  The BasicMesh class does not assume ownership of its arrays.
    ///
    ///  This class implements the ObjectSet_C concept.
    /// \param Vec3_T  Vertex data type.  Must be a three-component vector type (such as TinyRT::Vec3f)
    /// \param uint_t  The vertex data type.  Must be an unsigned integral type
    //=====================================================================================================================
    template< class Vec3_T, class uint_t >
    class BasicMesh
    {
    public:

        typedef uint32 obj_id;
        typedef Vec3_T Position_T;
        typedef uint_t Index_T;
        typedef uint_t face_id;

        typedef TriangleClipper< BasicMesh<Vec3_T,uint_t> > Clipper;

        /// Constructs a basic mesh around arrays of vertex and index data.  
        inline BasicMesh( Position_T* pVertices, Index_T* pIndices, uint32 nVertices,  uint32 nFaces );

        /// Returns the number of triangles in the mesh
        inline uint32 GetObjectCount() const { return m_nTriangles; };

        /// Computes the bounding box of the specified triangle
        inline void GetObjectAABB( uint32 nObject, AxisAlignedBox& rBox ) const;

        /// Computes the bounding box of the mesh
        inline void GetAABB( AxisAlignedBox& rBox ) const;

        /// Rearranges the order of faces in the mesh
        inline void RemapObjects( obj_id* pObjectRemap ) ;

        /// Performs an intersection test between this object and a ray, returning true if a hit was found
        template< typename Ray_T >
        inline bool RayIntersect( Ray_T& rRay, TriangleRayHit& rRayHit, uint32 nObject ) const;

        /// Performs an intersection test between a ray and a series of objects, returning true if a hit was found
        template< typename Ray_T >
        inline bool RayIntersect( Ray_T& rRay, TriangleRayHit& rRayHit, uint32 nFirstObject, uint32 nLastObject ) const;

        /// Accessor for the vertex array
        inline const Position_T& VertexPosition( uint32 i ) const { return m_pVertices[i]; };

        /// Accessor for the index array
        inline Index_T Index( uint32 nFace, uint32 i ) const { return m_pIndices[ 3*nFace + i ]; };

        /// Returns the number of vertices in the mesh
        inline uint32 VertexCount() const { return m_nVertices; };
        
        /// Returns the underlying array of vertex positions
        inline const Vec3_T* VertexArray() const { return m_pVertices; };
        
        /// Returns the underlying array of indices
        inline const Index_T* IndexArray() const { return m_pIndices; };
        
        /// Returns the underlying array of vertices
        inline Vec3_T* VertexArray() { return m_pVertices; };
        
        /// Returns the underlying array of indices
        inline Index_T* IndexArray() { return m_pIndices; };

        /// Returns the three vertex positions of a triangle
        inline void GetTriangleVertexPositions( face_id nFace, Vec3_T pVerticesOut[3] ) const;

    private:

        struct Indices  // used by 'RemapObjects' implementation
        {
            Index_T indices[3];
        };

        Vec3_T*  m_pVertices;
        Index_T* m_pIndices;
        uint32 m_nTriangles;
        uint32 m_nVertices;
    };

}

#include "TRTBasicMesh.inl"


#endif // _TRT_BASICMESH_H_
