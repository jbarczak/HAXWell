//=====================================================================================================================
//
//   TRTStridedMesh..h
//
//   Definition of class: TinyRT::StridedMesh
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2009 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTSTRIDEDMESH_H_
#define _TRTSTRIDEDMESH_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief A mesh with a fixed index array, and a vertex array of unknown stride
    ///
    /// The vertices are assumed to be three-component floats, and are assumed to be the first thing in the vertex.
    ///
    ///  This class implements the ObjectSet_C concept.
    /// \param uint_t  The vertex data type.  Must be an unsigned integral type
    //=====================================================================================================================
    template< class uint_t >
    class StridedMesh
    {
    public:

        typedef uint32 obj_id;
        typedef Vec3f  Position_T;
        typedef uint_t Index_T;
        typedef uint_t face_id;

        typedef TriangleClipper< StridedMesh<uint_t> > Clipper;

        /// Constructs a basic mesh around arrays of vertex and index data.  
        inline StridedMesh( const uint8* pVertices, Index_T* pIndices, Index_T nVertices, uint nFaces, uint nVertexStride );

        /// Returns the number of triangles in the mesh
        inline uint32 GetObjectCount() const { return m_nFaces; };

        /// Computes the bounding box of the specified triangle
        inline void GetObjectAABB( uint32 nObject, AxisAlignedBox& rBox ) const;

        /// Computes the bounding box of the mesh
        inline void GetAABB( AxisAlignedBox& rBox ) const;

        /// Rearranges the order of faces in the mesh
        inline void RemapObjects( const obj_id* pObjectRemap ) ;

        /// Performs an intersection test between this object and a ray, returning true if a hit was found
        template< typename Ray_T >
        inline bool RayIntersect( Ray_T& rRay, TriangleRayHit& rRayHit, uint32 nObject ) const;

        /// Performs an intersection test between a ray and a series of objects, returning true if a hit was found
        template< typename Ray_T >
        inline bool RayIntersect( Ray_T& rRay, TriangleRayHit& rRayHit, uint32 nFirstObject, uint32 nLastObject ) const;


        /// Accessor for the vertex array
        inline const Position_T& VertexPosition( uint32 i ) const { return *reinterpret_cast<const Position_T*>( m_pVertices+i*m_nVertexStride ); };

        /// Accessor for the index array
        inline Index_T Index( uint32 nFace, uint32 i ) const { return m_pIndices[ 3*nFace + i ]; };

        /// Accessor for the three vertex positions of a triangle
        inline void GetTriangleVertexPositions( face_id nFace, Vec3f pVerticesOut[3] ) const;

        inline Index_T VertexCount() const { return m_nVertices; };
        inline const uint8* VertexArray() const { return m_pVertices; };
        inline const Index_T* IndexArray() const { return m_pIndices; };

    private:

        struct Indices
        {
            Index_T indices[3];
        };

        
        const uint8* m_pVertices;
        Index_T* m_pIndices;

        Index_T m_nVertices;
        uint m_nVertexStride;
        uint m_nFaces;

    };
    
}

#include "TRTStridedMesh.inl"

#endif // _TRTSTRIDEDMESH_H_
