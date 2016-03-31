

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

        vec4 Fetch4( uvec4 offs )
        {
            vec4 block;
            block.x = g_Pixels[ offs.x ];
            block.y = g_Pixels[ offs.y ];
            block.z = g_Pixels[ offs.z ];
            block.w = g_Pixels[ offs.w ];
            return block;
        }

       
        void main() 
        {
            // NOTE: Doing this with 1D indexing since that's what HAXWell supports
            //     2D would be a little better
            uint tid = gl_GlobalInvocationID.x;
            uvec2 vBlockIdx = uvec2( tid &g_nTIDMask, tid >> g_nTIDShift );
            uvec2 vCorner   = vBlockIdx*4;

            uvec4 base  = uvec4( vCorner.y*g_nWidth + vCorner.x );
            uvec4 offs0 = base + uvec4( 0, 1, 2, 3 ) ;
            uvec4 offs1 = offs0 + uvec4( g_nWidth );
            uvec4 offs2 = offs1 + uvec4( g_nWidth );
            uvec4 offs3 = offs2 + uvec4( g_nWidth );
            vec4 block0     = Fetch4( offs0 );
            vec4 block1     = Fetch4( offs1 );
            vec4 block2     = Fetch4( offs2 );
            vec4 block3     = Fetch4( offs3 );

	        // Find the Min and Max values
	        vec4 lo4 = min(min(block0, block1), min(block2, block3));
	        vec4 hi4 = max(max(block0, block1), max(block2, block3));
	        float lo = min(min(lo4.x, lo4.y), min(lo4.z, lo4.w));
	        float hi = max(max(hi4.x, hi4.y), max(hi4.z, hi4.w));

            g_Blocks[ tid ] = vec2(lo,hi);
        }
    );







const char* BLOCKMINMAX_HXW = STRINGIFY(

curbe INDICES[1] = {0,1,2,3,4,5,6,7}

reg pixels[16] // reg i contains the ith pixel for each of the 8 blocks
reg tmin[8]
reg tmax[8]
reg pmin[2]
reg pmax[2]
reg out[4]
reg base
reg addr[4]
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

// A SIMT implementation would do a seperate load for each local pixel in each block
//  (vectorized across 16 different blocks)
//
//  We get better results if we do the loads row-wise, pulling in 4 pixels per block per load
//   This does two things:
//     1.  Each load touches fewer cachelines and thus completes faster
//     2.  Multiple blocks can be serviced in a single load, meaning we need less load messages


// first 4 blocks, top rows
add(4) addr0.u0, base0.u0<0,1,0>, INDICES.u<4,4,1> // TODO: There is probably a way to use regioning to collapse this to a SIMD16 op
add(4) addr0.u4, base0.u1<0,1,0>, INDICES.u<4,4,1> 
add(4) addr1.u0, base0.u2<0,1,0>, INDICES.u<4,4,1>
add(4) addr1.u4, base0.u3<0,1,0>, INDICES.u<4,4,1> 
send DwordLoad16(image), pixels0.f, addr.u

// second 4 blocks, top rows
add(4) addr2.u0, base0.u4<0,1,0>, INDICES.u<4,4,1>
add(4) addr2.u4, base0.u5<0,1,0>, INDICES.u<4,4,1>
add(4) addr3.u0, base0.u6<0,1,0>, INDICES.u<4,4,1>
add(4) addr3.u4, base0.u7<0,1,0>, INDICES.u<4,4,1> 
send DwordLoad16(image), pixels2.f, addr2.u

 // second rows
add(16) addr.u,  addr.u,  globals.u0<0,1,0>
send DwordLoad16(image), pixels4.f, addr.u
add(16) addr2.u, addr2.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels6.f, addr2.u

// third rows
add(16) addr.u,  addr.u,  globals.u0<0,1,0>
send DwordLoad16(image), pixels8.f, addr.u
add(16) addr2.u, addr2.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels10.f, addr2.u

// fourth rows
add(16) addr.u,  addr.u,  globals.u0<0,1,0>
send DwordLoad16(image), pixels12.f, addr.u
add(16) addr2.u, addr2.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels14.f, addr2.u

// Block pixels are spread out amongst 16 regs, each of which contains pixels from 2 different blocks
//  Like so:
//
//   0[0,1,2,3][0,1,2,3]  1[0,1,2,3][0,1,2,3]   2[0,1,2,3][0,1,2,3]  3[0,1,2,3][0,1,2,3]
//   4[4,5,6,7][4,5,6,7]  5[4,5,6,7][4,5,6,7]   6[4,5,6,7][4,5,6,7]  7[4,5,6,7][4,5,6,7]
//
//  Reduce this down to 4 reg's worth
//
min(16) tmin0.f, pixels0.f, pixels4.f
min(16) tmin2.f, pixels2.f, pixels6.f
min(16) tmin4.f, pixels8.f, pixels12.f
min(16) tmin6.f, pixels10.f, pixels14.f
        
max(16) tmax0.f, pixels0.f, pixels4.f
max(16) tmax2.f, pixels2.f, pixels6.f
max(16) tmax4.f, pixels8.f, pixels12.f
max(16) tmax6.f, pixels10.f, pixels14.f
        
min(16) tmin0.f, tmin0.f, tmin4.f
min(16) tmin2.f, tmin2.f, tmin6.f
max(16) tmax0.f, tmax0.f, tmax4.f
max(16) tmax2.f, tmax2.f, tmax6.f

// We now have four partial results per block, spread out across registers t0:t3 like so:
//  
//   0[0,1,2,3][0,1,2,3]  1[0,1,2,3][0,1,2,3]   2[0,1,2,3][0,1,2,3]  3[0,1,2,3][0,1,2,3]
//
// We need horizontal (cross-lane) math in order to go further
//
// Register regioning to the rescue!  In a SIMD8 instruction, the <4,2,2> regioning pattern
//  selects alternating elements from a register pair and combines them.  
//  Horizontal operations can be done by using offset regions in the same register.
//   
//  Given:  reg0(0,1,2,3,4,5,6,7)  and reg1(8,9,10,11,12,13,14,15)
//
//   min reg0.f<4,2,2> reg0.f1<4,2,2> does:   min( {0,2,4,6,8,10,12,14} , {1,3,5,7,9,11,13,15} )
// 

min(8) pmin0.f, tmin0.f<4,2,2>, tmin0.f1<4,2,2> 
min(8) pmin1.f, tmin2.f<4,2,2>, tmin2.f1<4,2,2>
       
max(8) pmax0.f, tmax0.f<4,2,2>, tmax0.f1<4,2,2> 
max(8) pmax1.f, tmax2.f<4,2,2>, tmax2.f1<4,2,2>
 
min(8) out2.f, pmin0.f<4,2,2>, pmin0.f1<4,2,2>
max(8) out3.f, pmax0.f<4,2,2>, pmax0.f1<4,2,2>


// write mins/maxes interleaved
//  This is a SIMD16 write which writes the lanes as follows:
//  0 8  1 9  2 10,.... etc....
//
mul(8) out0.u, blockID.u, 2
add(8) out1.u, out0.u,  1
send     DwordStore16(output), null.u, out0.u

end

    );





// This version is what you get if you do it the 'SIMT' way
//  Implemented using HXW for comparison with the GL driver
const char* BLOCKMINMAX_OTHERWAY_HXW = STRINGIFY(

    curbe INDICES[2] = {{0,1,2,3,4,5,6,7},{8,9,10,11,12,13,14,15}}

   
reg pixels[32]
reg out[8]
reg base[2]
reg addr[6]
reg globals
reg blockX[2]
reg blockY[2]
reg blockID[2]
reg min[4]
reg max[4]

bind GlobalBuffer 0x38  // globals:  { width,height,tid_shift,tid_mask}
bind image   0x39
bind output  0x3a

begin:


send DwordLoad8(GlobalBuffer), globals.u, INDICES.u
mul(16) blockID.u, r0.u1<0,1,0>, 16       
add(16) blockID.u, blockID.u, INDICES.u  // block_id = 8*hw_tid + (0,1,2,3,4,5,6,7)

shr(16) blockY.u, blockID.u, globals.u2<0,1,0> // block_id / block_width
and(16) blockX.u, blockID.u, globals.u3<0,1,0> // block_id % block_width
mul(16) blockY.u, blockY.u, 4  // pixel index of top-left corner for each block
mul(16) blockX.u, blockX.u, 4

// block base addresses for 16 blocks
mul(16) base0.u, blockY.u, globals.u0<0,1,0>  // base = blockY*width + blockX
add(16) base0.u, blockX.u, base0.u

send DwordLoad16(image), pixels0.f, base0.u

add(16) addr0.u, base0.u, 1
send DwordLoad16(image), pixels2.f, addr0.u

add(16) addr2.u, base0.u, 2
send DwordLoad16(image), pixels4.f, addr2.u 

add(16) addr4.u, base0.u, 3
send DwordLoad16(image), pixels6.f, addr4.u

add(16) base0.u, base0.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels8.f, base0.u

add(16) addr0.u, addr0.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels10.f, addr0.u

add(16) addr2.u, addr2.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels12.f, addr2.u 

add(16) addr4.u, addr4.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels14.f, addr4.u

add(16) base0.u, base0.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels16.f, base0.u

add(16) addr0.u, addr0.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels18.f, addr0.u

add(16) addr2.u, addr2.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels20.f, addr2.u 

add(16) addr4.u, addr4.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels22.f, addr4.u

add(16) base0.u, base0.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels24.f, base0.u

add(16) addr0.u, addr0.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels26.f, addr0.u

add(16) addr2.u, addr2.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels28.f, addr2.u

add(16) addr4.u, addr4.u, globals.u0<0,1,0>
send DwordLoad16(image), pixels30.f, addr4.u

min(16) min0.f, pixels0.f, pixels2.f
max(16) max0.f, pixels0.f, pixels2.f
min(16) min2.f, pixels4.f, pixels6.f
max(16) max2.f, pixels4.f, pixels6.f

min(16) min0.f, min0.f, pixels10.f
max(16) max0.f, max0.f, pixels10.f
min(16) min2.f, min2.f, pixels12.f
max(16) max2.f, max2.f, pixels12.f

min(16) min0.f, min0.f, pixels14.f
max(16) max0.f, max0.f, pixels14.f
min(16) min2.f, min2.f, pixels16.f
max(16) max2.f, max2.f, pixels16.f

min(16) min0.f, min0.f, pixels18.f
max(16) max0.f, max0.f, pixels18.f
min(16) min2.f, min2.f, pixels20.f
max(16) max2.f, max2.f, pixels20.f

min(16) min0.f, min0.f, pixels22.f
max(16) max0.f, max0.f, pixels22.f
min(16) min2.f, min2.f, pixels24.f
max(16) max2.f, max2.f, pixels24.f

min(16) min0.f, min0.f, pixels26.f
max(16) max0.f, max0.f, pixels26.f
min(16) min2.f, min2.f, pixels28.f
max(16) max2.f, max2.f, pixels28.f

min(16) min0.f, min0.f, pixels30.f
max(16) max0.f, max0.f, pixels30.f
min(16) min2.f, min2.f, pixels8.f // OOPS, missed one
max(16) max2.f, max2.f, pixels8.f 

mul(16) out0.u, blockID.u, 8
min(16) out2.f, min0.f, min2.f
max(16) out4.f, max0.f, max2.f
send     UntypedWrite16x2(output), null.u, out0.u


end


    );






static void Test( int N, int nThreads, HAXWell::ShaderHandle hShader  )
{
    float* pImage = new float[N*N];
    for( size_t i=0; i<N*N; i++ )
        pImage[i] = (i % 255)/255.0f;

    unsigned char* pBlocks = new unsigned char[(N/ 4)*(N/4)*8];

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
    
    unsigned int nALU, nSend;
    CountOps( blob.GetLength(),(const unsigned char*) blob.GetBytes(), &nALU, &nSend );
    printf("%u/%u\n", nALU, nSend );

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
    if( program.Assemble( &encoder, BLOCKMINMAX_OTHERWAY_HXW, &pr ) )
    {
        HAXWell::ShaderArgs args;
        args.nCURBEAllocsPerThread = program.GetCURBERegCount();
        args.nDispatchThreadCount = program.GetThreadsPerDispatch();
        args.nSIMDMode = 16;
        args.nIsaLength = program.GetIsaLengthInBytes();
        args.pCURBE = program.GetCURBE();
        args.pIsa = program.GetIsa();

        unsigned int nALU, nSend;
        CountOps( program.GetIsaLengthInBytes(),(const unsigned char*) program.GetIsa(), &nALU, &nSend );
        printf("%u/%u\n", nALU, nSend );


        HAXWell::ShaderHandle hShader = HAXWell::CreateShader( args );
        
        Test( WIDTH, NUM_BLOCKS/16, hShader );
        
    }

}










static void RegionTest()
{
   class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            printf("%s", p );
        }
    };


   
    
    const char* REGIONTEST = STRINGIFY(

    curbe INDICES[4] = {{0,1,2,3,4,5,6,7},
                        {8,9,10,11,12,13,14,15},
                        {16,17,18,19,20,21,22,23}, 
                        {24,25,26,27,28,29,30,31}}

    reg tmp[2]
    reg out[4]
    bind output 0x38  

    begin:

    mov(8) tmp.u, INDICES.u<4,2,2> // selects every other element -> condenses S16 into S8  0,2,4,6,8,10,12,14
    mov(8) tmp.u, INDICES.u<8,4,4> // 0,4,0,4,8,12,8,12 
    mov(8) tmp.u, INDICES.u<4,1,0> // 0,4,0,4,8,12,8,12 
    
    mov(8) tmp.u, INDICES.u<4,2,2>
    mov(8) tmp1.u, INDICES.u1<4,2,2>

    mov(16) out.u, INDICES.u
    mov(16) out2.u, tmp.u
    send     DwordStore16(output), null.u, out0.u

    end
    );

    GEN::Encoder encoder;
    GEN::Decoder decoder;

    Printer pr;
    GEN::Assembler::Program program;
    if( program.Assemble( &encoder, REGIONTEST, &pr ) )
    {
        HAXWell::ShaderArgs args;
        args.nCURBEAllocsPerThread = program.GetCURBERegCount();
        args.nDispatchThreadCount = program.GetThreadsPerDispatch();
        args.nSIMDMode = 16;
        args.nIsaLength = program.GetIsaLengthInBytes();
        args.pCURBE = program.GetCURBE();
        args.pIsa = program.GetIsa();

        HAXWell::ShaderHandle hShader = HAXWell::CreateShader( args );
 
        HAXWell::BufferHandle hOut = HAXWell::CreateBuffer(0, 64 );
 
   
    HAXWell::DispatchShader( hShader, &hOut, 1, 1 );
    HAXWell::Finish();
    unsigned int* pBuff = (unsigned int*) HAXWell::MapBuffer(hOut);

    for( size_t i=0; i<16; i++ )
        printf("%u,", pBuff[i]);
    }

    exit(1);
 
}
