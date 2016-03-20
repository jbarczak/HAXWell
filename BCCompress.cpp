

#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENAssembler.h"

#include "HAXWell.h"
#include "Misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#define GLSL(...) "#version 430 core\n" #__VA_ARGS__
#define STRINGIFY(...) #__VA_ARGS__

// BC4 compressor ported from Humus's code: http://www.humus.name/index.php?page=3D&ID=78
//  Unlike Humus we do not use 'Gather4' here, because HAXWell doesn't images
//   This skews the comparison somewhat in HAXWELL's favor, but the access pattern is still 
//    going to be comparable.   Each GLSL thread will touch more lines the its HAXWELL counterpart
//
const char* COMPRESS_GLSL = GLSL(

     layout (local_size_x = 8, local_size_y=1) in;

        layout (std430,binding=0)
        buffer Consts
        {
           uint g_nWidth;
           uint g_nHeight;
           uint g_nTIDShift;
           uint g_nTIDMask;
        };
        
        layout (std430,binding=1)
        buffer InputImage
        {
           float g_Pixels[];
        };

        layout (std430,binding=2)
        buffer OutputBlocks
        {
           uvec2 g_Blocks[];
        };

        
        uvec4 remap(vec4 k)
        {
	        return uvec4(  (k == vec4(0))? 1 : (k == vec4(7))? 0 : vec4(8) - k );
        }
        vec4 Fetch4( uvec2 coords )
        {
            uint base = coords.y*g_nWidth + coords.x ;
            vec4 block;
            block.x = g_Pixels[ base ];
            block.y = g_Pixels[ base+1 ];
            block.z = g_Pixels[ base+g_nWidth];
            block.w = g_Pixels[ base+g_nWidth+1];
            return block;
        }

        void main() 
        {
            // NOTE: Doing this with 1D indexing since that's what HAXWell supports
            //     2D would be a little better
            uint tid = gl_GlobalInvocationID.x;
            uvec2 vBlockIdx = uvec2( tid &g_nTIDShift, tid >> g_nTIDMask );
            uvec2 vCorner   = vBlockIdx*4;
            vec4 block0     = Fetch4( vCorner );
            vec4 block1     = Fetch4( vCorner + uvec2(2,0) );
            vec4 block2     = Fetch4( vCorner + uvec2(0,2) );
            vec4 block3     = Fetch4( vCorner + uvec2(2,2) );

	        // Find the Min and Max values
	        vec4 lo4 = min(min(block0, block1), min(block2, block3));
	        vec4 hi4 = max(max(block0, block1), max(block2, block3));
	        float lo = min(min(lo4.x, lo4.y), min(lo4.z, lo4.w));
	        float hi = max(max(hi4.x, hi4.y), max(hi4.z, hi4.w));

	        // Compute interpolation factors in this range for each sample, and then remap to BC4 indices.
	        float invDiff = 7.0f / (hi - lo);
            uvec4 i0 = remap( round((block0-lo)*invDiff) );
            uvec4 i1 = remap( round((block1-lo)*invDiff) );
            uvec4 i2 = remap( round((block2-lo)*invDiff) );
            uvec4 i3 = remap( round((block3-lo)*invDiff) );


	        // Assemble the 64bits of compressed data and return as two unsigned integers.
	        uvec2 block;
	        block.x =
		        uint(hi*255.0f) | (uint(lo*255.0f) << 8) |
		        (i0.x << 16) | (i0.y << 19) | (i1.x << 22) | (i1.y << 25) | (i0.z << 28) | (i0.w << 31);
	        block.y =
		        (i0.w >>  1) | (i1.z <<  2) | (i1.w <<  5) | (i2.x <<  8) | (i2.y << 11) | (i3.x << 14) |
		        (i3.y << 17) | (i2.z << 20) | (i2.w << 23) | (i3.z << 26) | (i3.w << 29);
            
            g_Blocks[ vBlockIdx.y*(g_nWidth>>2) + vBlockIdx.x ] = block;
        }
    );




const char* COMPRESS_HXW = STRINGIFY(

    curbe DX[2] = { {0,1,2,3,0,1,2,3}, {0,1,2,3,0,1,2,3} }
    curbe DY[2] = { {0,0,0,0,1,1,1,1}, {2,2,2,2,3,3,3,3} }

curbe INDICES[1] = {0,1,2,3,4,5,6,7}

reg pixels[16]; // reg i contains the ith pixel for each of the 8 blocks
reg pmin;
reg pmax;
reg base[2];
reg offs[2];
reg globals;
reg blockX;
reg blockY;
reg invDiff;
reg loDiff;
reg k[2];


bind GlobalBuffer 0x38  // globals:  { width,height,tid_shift,tid_mask}
bind image   0x39
bind output  0x3a

begin:

// map 8 blocks (4x4 block neighborhood) onto each thread

send DWordLoad8(GlobalBuffer), globals.u, INDICES.u

mul(8) blockID.u, r0.u1<0,1,0>, 8       // block_id = 8*hw_tid + (0,1,2,3,4,5,6,7)
add(8) blockID.u, blockID.u, INDICES.u

shr(8) blockY.u, blockID.u, globals.u2<1,1,1> // block_id / block_width
and(8) blockX.u, blockID.u, globals.u3<1,1,1> // block_id % block_width
mul(8) blockY.u, 4  // pixel index of top-left corner for each block
mul(8) blockX.u, 4

// block base addresses for 8 blocks
mul(8) base0.u, blockY.u, globals.u0<0,1,0>
add(8) base0.u, blockX.u, base0.u
mov(8) base1.u, base0.u

// offsets for the 16 pixels within each block
mul(16) offs.u, DY, globals.u<0,1,0> // width*dy
add(16) offs.u, offs.u, DX           // width*dy + dx

// load all the pixels.  Pull in pixels i and i+1 in the same message
add(8) addr0.u, base0.u, offs0.u0<0,1,0>
add(8) addr1.u, base1.u, offs0.u1<0,1,0>
send DWordLoad16(image), pixels0, addr

add(8) addr0.u, base0.u, offs0.u2<0,1,0>
add(8) addr1.u, base1.u, offs0.u3<0,1,0>
send DWordLoad16(image), pixels2, addr

add(8) addr0.u, base0.u, offs0.u4<0,1,0>
add(8) addr1.u, base1.u, offs0.u5<0,1,0>
send DWordLoad16(image), pixels4, addr

add(8) addr0.u, base0.u, offs0.u6<0,1,0>
add(8) addr1.u, base1.u, offs0.u7<0,1,0>
send DWordLoad16(image), pixels6, addr

add(8) addr0.u, base0.u, offs1.u0<0,1,0>
add(8) addr1.u, base1.u, offs1.u1<0,1,0>
send DWordLoad16(image), pixels8, addr

add(8) addr0.u, base0.u, offs1.u2<0,1,0>
add(8) addr1.u, base1.u, offs1.u3<0,1,0>
send DWordLoad16(image), pixels10, addr

add(8) addr0.u, base0.u, offs1.u4<0,1,0>
add(8) addr1.u, base1.u, offs1.u5<0,1,0>
send DWordLoad16(image), pixels12, addr

add(8) addr0.u, base0.u, offs1.u6<0,1,0>
add(8) addr1.u, base1.u, offs1.u7<0,1,0>
send DWordLoad16(image), pixels14, addr

// pixels[i] contains the ith pixel in each of 8 blocks
//  reduce this down hierarchically to 8 mins and maxes. 
// This formulation requires considerably fewer instructions than the 
//   SIMT-16 version (28 Simd8 ops/8 blocks)

min(16) pmin0.f, pixels0.f,  pixels2.f
max(16) pmax0.f, pixels0.f,  pixels2.f
min(16) pmin2.f, pixels4.f,  pixels6.f
max(16) pmax2.f, pixels4.f,  pixels6.f
min(16) pmin4.f, pixels8.f,  pixels10.f
max(16) pmax4.f, pixels8.f,  pixels10.f
min(16) pmin6.f, pixels12.f, pixels14.f
max(16) pmax6.f, pixels12.f, pixels14.f
min(16) pmin0.f, pmin0.f,    pmin2.f
max(16) pmax0.f, pmax0.f,    pmax2.f
min(16) pmin2.f, pmin4.f,    pmin6.f
max(16) pmax2.f, pmax4.f,    pmax6.f
min(8)  pmin0.f, pmin0.f,    pmin1.f
max(8)  pmax0.f, pmax0.f,    pmax1.f

// invDiff = 7.0f / (hi-lo)
sub(1) invDiff.f, pmax.f<1,1,1>, pmin.f<1,1,1>
rcp(1) invDiff.f, invDiff.f<1,1,1>
mul(1) invDiff.f, 7.0f


//  8
//  8

//  8
//  8

//  


/*
// compute min/max reduction within blocks
min(8) pmin.f, pixels0.f, pixels1.f
max(8) pmax.f, pixels0.f, pixels1.f
min(4) pmin.f, pmin.f, pmin.f4 
max(4) pmax.f, pmax.f, pmax.f4
min(2) pmin.f, pmin.f, pmin.f2
max(2) pmax.f, pmax.f, pmax.f2
min(1) pmin.f, pmin.f, pmin.f1
max(1) pmax.f, pmax.f, pmax.f1
*/



// pixels = round( (pixels - lo)*(7/(hi-lo)) )
sub(16)  pixels.f, pixels.f, pmin.f<0,1,0>
mul(16)  pixels.f, pixels.f, invDiff.f<0,1,0>
rnde(16) pixels.f, pixels.f

// k = ( k == 0 ) ? 1  : (k==7) ? 0 : 8-k
cmpeq(16)(f0.0) null.u, pixels.f, 0.0f
cmpeq(16)(f0.1) null.u, pixels.f, 7.0f
mov(16)  k.f, 8.0f          // k = 8.0f - pixels
sub(16)  k.f, pixels.f      // TODO: support negate modifiers properly and write as: add k, -pixels, 8.0f
pred(f0.0) { mov(16) k.f, 1.0f }
pred(f0.1) { mov(16) k.f, 0.0f }

// now to integer
mov(16) k.u, k.f

// now combine everything


end

    );



void Test( int N, int nThreads, HAXWell::ShaderHandle hShader  )
{
    float* pImage = new float[N*N];
    for( size_t i=0; i<N*N; i++ )
        pImage[i] = (i % 255)/255.0f;

    unsigned char* pBlocks = new unsigned char[(N/4)*(N/4)*8];

    struct Globals
    {
        unsigned int nWidth;  
        unsigned int nHeight;
        unsigned int nBlockWidth;
        unsigned int nWidthShift;
        unsigned int nWidthMask;
    };
    Globals g;
    g.nWidth = N;
    g.nHeight=N;
    g.nBlockWidth = N/4;
    g.nWidthShift = _tzcnt_u32(N/4);
    g.nWidthMask  = (1<<g.nWidthShift)-1;

    HAXWell::BufferHandle hGlobals = HAXWell::CreateBuffer( &g, sizeof(g) );
    HAXWell::BufferHandle hImage   = HAXWell::CreateBuffer( pImage, sizeof(float)*N*N );
    HAXWell::BufferHandle hBlocks  = HAXWell::CreateBuffer( 0, (N/4)*(N/4)*8 );

    HAXWell::BufferHandle pBuffers[3] = {
        hGlobals,hImage,hBlocks
    };
    HAXWell::TimerHandle hTimer = HAXWell::BeginTimer();
    HAXWell::DispatchShader( hShader, pBuffers, 3, nThreads );
    HAXWell::EndTimer(hTimer);
    printf("%08u\n", HAXWell::ReadTimer(hTimer) );
    HAXWell::Finish();

}






void BlockCompress()
{
    HAXWell::Blob blob;
    HAXWell::RipIsaFromGLSL( blob, COMPRESS_GLSL );
    PrintISA(stdout, blob );
    
    int WIDTH = 2048;
    int NUM_BLOCKS = (WIDTH/4)*(WIDTH/4);
    HAXWell::ShaderHandle hGLSL = HAXWell::CreateGLSLShader(COMPRESS_GLSL);
    int nThreadsGLSL = (NUM_BLOCKS)/8;
    Test(4096,nThreadsGLSL,hGLSL );


}