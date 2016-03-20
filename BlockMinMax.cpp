

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

// Compute the min/max of 16 pixel blocks in a scalar image
const char* BLOCKMINMAX_GLSL = GLSL(

     layout (local_size_x = 16) in;

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
           vec2 g_Blocks[];
        };

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
            uvec2 vBlockIdx = uvec2( tid &g_nTIDMask, tid >> g_nTIDShift );
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

            g_Blocks[ tid ] = vec2(lo,hi);
        }
    );



const char* BLOCKMINMAX_HXW = STRINGIFY(

curbe DX[2] = { {0,1,2,3,0,1,2,3}, {0,1,2,3,0,1,2,3} }
curbe DY[2] = { {0,0,0,0,1,1,1,1}, {2,2,2,2,3,3,3,3} }

curbe INDICES[1] = {0,1,2,3,4,5,6,7}

reg pixels[16] // reg i contains the ith pixel for each of the 8 blocks
reg pmin[8]
reg pmax[8]
reg out[4]
reg base[2]
reg offs[2]
reg addr[2]
reg globals
reg blockX
reg blockY
reg blockID



bind GlobalBuffer 0x38  // globals:  { width,height,tid_shift,tid_mask}
bind image   0x39
bind output  0x3a

begin:


send DwordLoad8(GlobalBuffer), globals.u, INDICES.u
mul(8) blockID.u, r0.u1<0,1,0>, 8       
add(8) blockID.u, blockID.u, INDICES.u  // block_id = 8*hw_tid + (0,1,2,3,4,5,6,7)

shr(8) blockY.u, blockID.u, globals.u2<0,1,0> // block_id / block_width
and(8) blockX.u, blockID.u, globals.u3<0,1,0> // block_id % block_width
mul(8) blockY.u, blockY.u, 4  // pixel index of top-left corner for each block
mul(8) blockX.u, blockX.u, 4

// block base addresses for 8 blocks
mul(8) base0.u, blockY.u, globals.u0<0,1,0>  // base = blockY*width + blockX
add(8) base0.u, blockX.u, base0.u
mov(8) base1.u, base0.u

// offsets for the 16 pixels within each block
mul(16) offs.u, DY.u, globals.u<0,1,0> //
add(16) offs.u, offs.u, DX.u           // offs = width*dy + dx

// load all the pixels for each of the 8 blocks
//  We use 16-wide loads to coalesce pixels i and i+1 into the same message
add(8) addr0.u, base0.u, offs0.u0<0,1,0> // TODO: I bet there's a clever way to exploit regioning
add(8) addr1.u, base1.u, offs0.u1<0,1,0> //   and do this with 1 SIMD16 add
send DwordLoad16(image), pixels0.f, addr.u

add(8) addr0.u, base0.u, offs0.u2<0,1,0>
add(8) addr1.u, base1.u, offs0.u3<0,1,0>
send DwordLoad16(image), pixels2.f, addr.u

add(8) addr0.u, base0.u, offs0.u4<0,1,0>
add(8) addr1.u, base1.u, offs0.u5<0,1,0>
send DwordLoad16(image), pixels4.f, addr.u

add(8) addr0.u, base0.u, offs0.u6<0,1,0>
add(8) addr1.u, base1.u, offs0.u7<0,1,0>
send DwordLoad16(image), pixels6.f, addr.u

add(8) addr0.u, base0.u, offs1.u0<0,1,0>
add(8) addr1.u, base1.u, offs1.u1<0,1,0>
send DwordLoad16(image), pixels8.f, addr.u

add(8) addr0.u, base0.u, offs1.u2<0,1,0>
add(8) addr1.u, base1.u, offs1.u3<0,1,0>
send DwordLoad16(image), pixels10.f, addr.u

add(8) addr0.u, base0.u, offs1.u4<0,1,0>
add(8) addr1.u, base1.u, offs1.u5<0,1,0>
send DwordLoad16(image), pixels12.f, addr.u

add(8) addr0.u, base0.u, offs1.u6<0,1,0>
add(8) addr1.u, base1.u, offs1.u7<0,1,0>
send DwordLoad16(image), pixels14.f, addr.u
 
// pixels[i] now contains the ith pixel in each of 8 blocks
//    reduce this down to 8 mins and maxes (one per block)
//
min(16) pmin0.f, pixels0.f,  pixels2.f
min(16) pmin2.f, pixels4.f,  pixels6.f
min(16) pmin4.f, pixels8.f,  pixels10.f
min(16) pmin6.f, pixels12.f, pixels14.f
max(16) pmax0.f, pixels0.f,  pixels2.f
max(16) pmax2.f, pixels4.f,  pixels6.f
max(16) pmax4.f, pixels8.f,  pixels10.f
max(16) pmax6.f, pixels12.f, pixels14.f
min(16) pmin0.f, pmin0.f,    pmin2.f
min(16) pmin2.f, pmin4.f,    pmin6.f
max(16) pmax0.f, pmax0.f,    pmax2.f
max(16) pmax2.f, pmax4.f,    pmax6.f
min(16) pmin0.f, pmin0.f,    pmin2.f
max(16) pmax0.f, pmax0.f,    pmax2.f
min(8)  pmin0.f, pmin0.f,    pmin1.f
max(8)  pmax0.f, pmax0.f,    pmax1.f

// write mins/maxes interleaved
//  This is a SIMD16 write which writes the lanes as follows:
//  0 8  1 9  2 10,.... etc....
//
mul(8) out0.u, blockID.u, 2
add(8) out1.u, out0.u,  1
mov(8) out2.f, pmin0.f
mov(8) out3.f, pmax0.f
send     DwordStore16(output), null.u, out0.u

end

    );




static void Test( int N, int nThreads, HAXWell::ShaderHandle hShader  )
{
    float* pImage = new float[N*N];
    for( size_t i=0; i<N*N; i++ )
        pImage[i] = (i % 255)/255.0f;

    unsigned char* pBlocks = new unsigned char[(N/4)*(N/4)*8];

    struct Globals
    {
        unsigned int nWidth;  
        unsigned int nHeight;
        unsigned int nWidthShift;
        unsigned int nWidthMask;
    };
    Globals g;
    g.nWidth = N;
    g.nHeight=N;
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
    HAXWell::Finish();
   
    printf("%08u\n", HAXWell::ReadTimer(hTimer) );
   
    // check it
    float* pOut = (float*) HAXWell::MapBuffer(hBlocks);
    
    for( size_t y=0; y<N; y+= 4 )
    {
        for( size_t x=0; x<N; x+= 4 )
        {
            float fMin = pImage[ y*N + x ];
            float fMax = fMin;
            for( size_t j=1; j<16; j++ )
            {
                size_t yyy = y + (j/4);
                size_t xxx = x + (j%4);
                float f = pImage[ yyy*N + xxx];
                if( f < fMin )
                    fMin = f;
                if( f > fMax )
                    fMax = f;
            }

            size_t block = (y/4)*(N/4) + (x/4);
            float fOutMin = pOut[2*block];
            float fOutMax = pOut[2*block+1];
        
            if( fOutMin != fMin || fOutMax != fMax )
                printf("foo");
        }
    }
    
    HAXWell::UnmapBuffer(hBlocks);


}






void BlockMinMax()
{
    HAXWell::Blob blob;
    HAXWell::RipIsaFromGLSL( blob, BLOCKMINMAX_GLSL );
    PrintISA(stdout, blob );
    
    int WIDTH = 2048;
    int NUM_BLOCKS = (WIDTH/4)*(WIDTH/4);
    HAXWell::ShaderHandle hGLSL = HAXWell::CreateGLSLShader(BLOCKMINMAX_GLSL);
    Test(WIDTH,NUM_BLOCKS/16,hGLSL );


    
    class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            printf("%s", p );
        }
    };

    GEN::Encoder encoder;
    GEN::Decoder decoder;

    Printer pr;
    GEN::Assembler::Program program;
    if( program.Assemble( &encoder, BLOCKMINMAX_HXW, &pr ) )
    {
        HAXWell::ShaderArgs args;
        args.nCURBEAllocsPerThread = program.GetCURBERegCount();
        args.nDispatchThreadCount = program.GetThreadsPerDispatch();
        args.nSIMDMode = 16;
        args.nIsaLength = program.GetIsaLengthInBytes();
        args.pCURBE = program.GetCURBE();
        args.pIsa = program.GetIsa();

        HAXWell::ShaderHandle hShader = HAXWell::CreateShader( args );
        
        Test( WIDTH, NUM_BLOCKS/8, hShader );
        
    }

}