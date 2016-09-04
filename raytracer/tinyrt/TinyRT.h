
/// \defgroup TinyRT TinyRT
/**
 \mainpage

 What TRT is:

 The goal of TRT is to provide a set of clean, generic, and reasonably efficient implementations of common raytracing algorithms. 
 Its intended uses are:
    - As a reference codebase for research, prototyping, and recreational raytracing
    - As a starting point for a renderer
    - As a drop-in solution for 'lightweight' raycasting applications (such as collision detection or line-of-sight detection)

TRT is template driven, and is organized around a set of 'concepts'.  The most central concept is the ObjectSet.
To incorporate one of TRT's raytracing data structures, all that is needed is to define a class which implements TinyRT::ObjectSet_C.
Given an object set class, it is straightforward to use TinyRT to construct a variety of raytracing data structures.
\sa TRTConcepts

TRT provides a pair of object set implementations for triangle meshes, but at the moment, object support is rather sparse.
\sa TinyRT::BasicMesh, TinyRT::StridedMesh

What TRT is not:
    - TRT is not a complete renderer, only a collection of usable components.

TRT provides many of the components needed to build a simple raytracer, but geometry loading, image sampling, shading, and output
must all be implemented at the application level.  

    - TRT is not a high performance raycasting library (nor does it claim to be).  
    
TRT's priority is to be generic, flexible, and modular.  Although every effort has been made to optimize TRT's raycasting functions, 
they have been deliberately designed to use a very general interface, in order to support as many diverse data structure implementations
as possible.  This generality is likely to result in worse performance than could be acheived by an implementation which is specialized 
for a particular data structure.  For performance critical applications, it is best to pair TRT's data structure classes with a customized 
raytracing kernel.  

*/

#ifndef _TINYRT_H_
#define _TINYRT_H_

#include <vector>
#include <limits>
#include <float.h>
#include <string.h> // for memcpy


#ifndef TRT_FORCEINLINE

    #ifdef __GNUC__

        // If we use the GCC equivalent of __forceinline, GCC sometimes fails to inline things, and errors out when that happens...
        
        /// Macro to force function inlining
        #define TRT_FORCEINLINE inline  
    
    #else

        /// Macro to force function inlining
        #define TRT_FORCEINLINE __forceinline

    #endif
#endif


/// Minimum distance used for epsilon rays.  TRT clients may #define TRT_EPSILON to override its value
#ifndef TRT_EPSILON
#define TRT_EPSILON 0.000001f 
#endif

#include "TRTAssert.h"
#include "TRTTypes.h"
#include "TRTMalloc.h"
#include "TRTSimd.h"
#include "TRTMath.h"
#include "TRTSampling.h"
#include "TRTScratchMemory.h"


// Utility classes
#include "TRTAxisAlignedBox.h"
#include "TRTPacketFrustum.h"
#include "TRTPerspectiveCamera.h"
#include "TRTScopedArray.h"
#include "TRTObjectUtils.h"


// Analysis utilities
#include "TRTTreeStatistics.h"
#include "TRTCostMetric.h"

// Rays
#include "TRTRay.h"
#include "TRTEpsilonRay.h"

// Basic intersection testing
#include "TRTTriIntersect.h"
#include "TRTBoxIntersect.h"
#include "TRTSphereIntersect.h"
#include "TRTPolygonIntersect.h"

// Mailboxing
#include "TRTNullMailbox.h"
#include "TRTFifoMailbox.h"
#include "TRTDirectMapMailbox.h"
#include "TRTSimdFifoMailbox.h"

// Object sets
#include "TRTBasicMesh.h"
#include "TRTStridedMesh.h"

#include "TRTLeafPolicy.h"

// AABB trees
#include "TRTMedianCutAABBTreeBuilder.h"
#include "TRTSahAABBTreeBuilder.h"
#include "TRTAABBTree.h"
#include "TRTBVHTraversal.h"

// QBVH
#include "TRTQuadAABBTree.h"
#include "TRTMultiBVHTraversal.h"


// Uniform Grids
#include "TRTUniformGrid.h"
#include "TRTGridTraversal.h"

// KD Trees
#include "TRTKDTree.h"
#include "TRTKDTraversal.h"
#include "TRTSahKDTreeBuilder.h"
#include "TRTBoxClipper.h"

// Point KDTree
#include "TRTPointKDTree.h"
#include "TRTMedianSplitPointKDTreeBuilder.h"
#include "TRTKNNSearch.h"

#endif
