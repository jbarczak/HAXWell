//=====================================================================================================================
//
//   PlyLoader.h
//
//   Simple loader for PLY mesh files.  Based on the rply library by Diego Nehab
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2014 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================
#ifndef _PLYLOADER_H_
#define _PLYLOADER_H_

#include "Types.h"

namespace Simpleton
{
    enum PlyFlags
    {
        /// Rescale the mesh to fit in a unit box, and recenter so that lower bound is at y=0, and x/z are centered
        PF_STANDARDIZE_POSITIONS  = (1<<0), 
   
        /// Disregard texture coordinates in the file
        PF_IGNORE_UVS       = (1<<2),   

        /// Disregard normals in the file
        PF_IGNORE_NORMALS   = (1<<3),   

        /// Disregard colors in the file
        PF_IGNORE_COLORS    = (1<<4),

        /// Generate vertex normals from face normals if vertex normals not present
        PF_REQUIRE_NORMALS  = (1<<5)

    };

    struct PlyMesh
    {        
        typedef float Float2[2];
        typedef float Float3[3];

        union Color
        {
            struct { uint8 r; uint8 g; uint8 b; uint8 a; };
            uint8 Channels[4];
        } ;


        Float3 bbMin;
        Float3 bbMax;

        uint nVertices;
        Float3* pPositions;
        Float3* pNormals;
        Float2* pUVs;

        uint nTriangles;
        uint32* pVertexIndices;
        Color* pFaceColors;
    };

    bool LoadPly( const char* pFileName, PlyMesh& rMesh, unsigned int Flags );

   
    void FreePly( PlyMesh& rMesh );
}

#endif