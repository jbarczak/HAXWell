
#include "HAXWell.h"
#include "HAXWell_Utils.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENAssembler.h"
#include "../Misc.h"
#include <stdio.h>



void RaytraceHarness( HAXWell::ShaderHandle hShader, size_t nRaysPerGroup, float sah );

#define STRINGIFY(...) #__VA_ARGS__






static void PredTest()
{
   class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            printf("%s", p );
        }
    };


   
    
    const char* PREDTEST = STRINGIFY(

    curbe INDICES[4] = {{0,1,2,3,4,5,6,7},
                        {8,9,10,11,12,13,14,15},
                        {16,17,18,19,20,21,22,23}, 
                        {24,25,26,27,28,29,30,31}}


    reg tmp[2]
    reg out[4]
    bind output 0x38  

    begin:

    /*
    // yes, a 1 bit predicate masks the first channel of execution
    // regardless of reg offset
    mov(1) f0.us0, 1
    mov(16) tmp.f, 0
    pred(f0.0)
    {
        mov(1) tmp.u0, INDICES.u1
        mov(1) tmp.u1, INDICES.u2
        mov(2) tmp.u2, INDICES.u3
    }
    */

    mov(2) f0.us0, 0
    
    mov(2) f1.us0, 0
    
    mov(8) r83.u, INDICES.u


    mov(8) tmp1.u, INDICES.u
    mov(8) r68.u, 0xf
    and(8) tmp1.u, tmp1.u, 1
    cmpgt(8)(f1.0) null.u, tmp1.u, 0

    pred(f1.0)
    {
        mov(8) r68.u, r83.u
    }



    mov(16) out.u, INDICES.u
    mov(16) out2.u, r68.u
    send     DwordStore16(output), null.u, out0.u
    
    end
    );

    GEN::Encoder encoder;
    
    Printer pr;
    GEN::Assembler::Program program;
    if( program.Assemble( &encoder, PREDTEST, &pr ) )
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
    printf("\n");

    exit(1);
 
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
    
    
    mov(16) tmp.u, INDICES.u<4,4,0> // Replicates first element: 0,0,0,0,4,4,4,4,8,8,8,8,12,12,12,12
    mov(16) tmp.u, INDICES.u<4,1,0> // 0,4,0,4,0,4,0,4,8,12,8,12,8,12,8,12
    mov(16) tmp.u, INDICES.u<0,4,1> // Replicates first 4 elements:  0,1,2,3,0,1,2,3 8,9,10,11, 8,9,10,11

    mov(16) tmp.u, INDICES.u4<4,0,1> // Rotates reg lefT: 4,5,6,7,0,1,2,3

    mov(16) tmp.u, INDICES.u4<0,4,1> // Replicates second 4 elements:  4,5,6,7,4,5,6,7,12,13,14,15,12,13,14,15

    mov(16) tmp.u, 0
    mov(4) tmp.u, INDICES.u0<2,1,1> // selects even elements:  0,2,4,6

    mov(16) tmp.u, 0
    mov(16) tmp.u, INDICES.u<4,0,1>

    mov(16) out.u, INDICES.u
    mov(16) out2.u, tmp.u
    send     DwordStore16(output), null.u, out0.u
    
    end
    );

    GEN::Encoder encoder;
    
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
    printf("\n");

    exit(1);
 
}








static void AddrTest()
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


    reg foo
    reg bar
    reg baz
    reg tmp[3]
    reg out[4]
    bind output 0x38  
    bind data 0x39
    reg arr

    begin:

    
        /*
        
    mov(16) tmp0.u, 7
    // interesting HW bug here
    //   two back to back simd8 loads followed by simd16 mov
    //   out of same regs will NOT scoreboard the second reg properly
    
    send DwordLoad8(data), tmp0.u, INDICES.u
    send DwordLoad8(data), tmp1.u, INDICES1.u

    mov(16) out.u, INDICES.u
   //mov(8) out2.u, tmp0.u // this works
   //mov(8) out3.u, tmp1.u
    mov(16) out2.u, tmp0.u // this doesn't
    send     DwordStore16(output), null.u, out0.u
    */


    mov(16) tmp0.u, 7
    // interesting HW bug here
    //   two back to back simd8 loads followed by simd16 mov
    //   out of same regs will NOT scoreboard the second reg properly
    
    mov(1) a0.us0, 32
    
    send DwordLoad8(data), tmp0[a0.0].u, INDICES.u

    add(1) a0.us0, a0.us0, -32
    
    send DwordLoad8(data), tmp0[a0.0].u, INDICES1.u

    mov(16) out.u, INDICES.u
    mov(8) out2.u, tmp0[a0.0].u // this works
    add(1) a0.us0, a0.us0, 32 
    mov(8) out3.u, tmp0[a0.0].u
    //mov(16) out2.u, tmp0.u // this doesn't
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
 
        static const unsigned int DATA[4][8] =
            {{0,1,2,3,4,5,6,7},
                        {8,9,10,11,12,13,14,15},
                        {16,17,18,19,20,21,22,23}, 
                        {24,25,26,27,28,29,30,31}};
        

        HAXWell::BufferHandle hData = HAXWell::CreateBuffer(&DATA[0][0],sizeof(DATA));
 

       GEN::Disassemble( pr, &decoder, program.GetIsa(), program.GetIsaLengthInBytes() );
    
       HAXWell::BufferHandle hBuffers[2]={hOut,hData};
        HAXWell::DispatchShader( hShader, hBuffers, 2, 1 );
        HAXWell::Finish();
        unsigned int* pBuff = (unsigned int*) HAXWell::MapBuffer(hOut);

        for( size_t i=0; i<16; i++ )
            printf("%u,", pBuff[i]);
    }
    printf("\n");

    exit(1);
 
}





static void SwizzTest()
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

    curbe FP[4] = {0.25f,0.25f,0.25f,0.25f,1.25f,1.25f,1.25f,0.25f}

    reg tmp[5]
    reg out[4]
    bind output 0x38  

    reg arr

    begin:

    mov(8) tmp0.u, 12
    mov(8) tmp1.us, imm_uvec(1,2,3,4,5,6,7,8)
    mov(8) tmp1.us8, imm_uvec(9,10,11,12,13,14,15,0)

    add(16) tmp3.u, tmp0.u<0,1,0>, tmp1.us

    mov(8) out0.u, INDICES.u
    mov(8) out1.u, INDICES1.u
    mov(8) out2.u, tmp3.u
    mov(8) out3.u, tmp4.u

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
 
       GEN::Disassemble( pr, &decoder, program.GetIsa(), program.GetIsaLengthInBytes() );
    
        HAXWell::DispatchShader( hShader, &hOut, 1, 1 );
        HAXWell::Finish();
        unsigned int* pBuff = (unsigned int*) HAXWell::MapBuffer(hOut);

        for( size_t i=0; i<16; i++ )
            printf("%u,", pBuff[i]);
    }
    printf("\n");

    exit(1);
 
}



static void BFETest()
{
   class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            printf("%s", p );
        }
    };


   
    
    const char* TEST = STRINGIFY(

    curbe INDICES[4] = {{0,1,2,3,4,5,6,7},
                        {8,9,10,11,12,13,14,15},
                        {16,17,18,19,20,21,22,23}, 
                        {24,25,26,27,28,29,30,31}}

    curbe FP[4] = {0.25f,0.25f,0.25f,0.25f,1.25f,1.25f,1.25f,0.25f}

    reg tmp[5]
    reg out[4]
    reg FIELD_WIDTH[2]
    reg OFFSET[2]
    reg DATA[2]
    bind output 0x38  

    reg arr

    begin:

    mov(8) tmp0.u, 12
    mov(8) tmp1.us, imm_uvec(1,2,3,4,5,6,7,8)
    mov(8) tmp1.us8, imm_uvec(9,10,11,12,13,14,15,0)

    mov(16) FIELD_WIDTH.u, 1
    mov(16) OFFSET.u, 1
    mov(16) DATA.u, 0x6

    mov(16) tmp3.u, 0
    bfe(1) tmp3.u, FIELD_WIDTH.u, OFFSET.u, DATA.u
    

    mov(8) out0.u, INDICES.u
    mov(8) out1.u, INDICES1.u
    mov(8) out2.u, tmp3.u
    mov(8) out3.u, tmp4.u

    send     DwordStore16(output), null.u, out0.u

    end
    );

    GEN::Encoder encoder;
    GEN::Decoder decoder;
    Printer pr;
    GEN::Assembler::Program program;
    if( program.Assemble( &encoder, TEST, &pr ) )
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
 
       GEN::Disassemble( pr, &decoder, program.GetIsa(), program.GetIsaLengthInBytes() );
    
        HAXWell::DispatchShader( hShader, &hOut, 1, 1 );
        HAXWell::Finish();
        unsigned int* pBuff = (unsigned int*) HAXWell::MapBuffer(hOut);

        for( size_t i=0; i<16; i++ )
            printf("%u,", pBuff[i]);
    }
    printf("\n");

    exit(1);
 
}








void Raytrace()
{
  //  PredTest();

    class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            printf("%s", p );
        }
    };
     
    GEN::Encoder encoder;
    GEN::Decoder decoder;

    std::string RAYTRACE_HSW = ReadTextFile("raytracer/single_ray_vectri_x8.inl");


    Printer pr;
    GEN::Assembler::Program program;
    if( !program.Assemble( &encoder, RAYTRACE_HSW.c_str(), &pr ) )
        return;

    GEN::Disassemble( pr, &decoder, program.GetIsa(), program.GetIsaLengthInBytes() );
    
    
    HAXWell::ShaderArgs args;
    args.nCURBEAllocsPerThread = program.GetCURBERegCount();
    args.nDispatchThreadCount = program.GetThreadsPerDispatch();
    args.nSIMDMode = 16;
    args.nIsaLength = program.GetIsaLengthInBytes();
    args.pCURBE = program.GetCURBE();
    args.pIsa = program.GetIsa();

    HAXWell::ShaderHandle hShader = HAXWell::CreateShader( args  );
 

    std::string str = ReadTextFile("raytracer/raytracer.glsl");

    HAXWell::ShaderHandle hGLSL = HAXWell::CreateGLSLShader( str.c_str() );
    /*
    HAXWell::Blob blob;
    HAXWell::RipIsaFromGLSL( blob, str.c_str() );
    PrintISA(stdout, blob );
    */
    

   // RaytraceHarness( hGLSL, 8, 1.2f );
    

   // RaytraceHarness( hShader, 8, 1.2f ); // eight_ray
    //RaytraceHarness( hShader, 1, 1.2f ); // single_ray
     RaytraceHarness( hShader, 1, 0.5f ); // vectri_x8
}