// Test instruction latency by comparing stacks of independent regs with a dependency chain


#include "HAXWell.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENIsa.h"
#include <vector>

#include "Misc.h"

#define FILENAME "issue_fdiv.csv"

#define OPERATION GEN::MATH_IDIV_QUOTIENT
#define DATATYPE  GEN::DT_F32

#define INSTRUCTION(width,r)\
      GEN::MathInstruction(\
                        width, OPERATION, \
                        GEN::DestOperand( DATATYPE, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,r,0),8,8,1)),\
                        GEN::SourceOperand( eType, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,r,0),8,8,1)),\
                        GEN::SourceOperand( DATATYPE, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,r,0),8,8,1))\
                        )\

#define NUM_VEC4S 2048
#define NUM_VEC8S 2048
#define NUM_VEC16S 0



static void ConstructShader( std::vector<GEN::Instruction>& ops, size_t nRegs, size_t nThreadsPerGroup, size_t simd )
{
    // copy EOT payload
    ops.push_back( 
        GEN::UnaryInstruction( 8, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(127),8,8,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(0),8,8,1) ) 
            )
    );

  
    ///////////////////////////////////////////////////////
    GEN::DataTypes eType = DATATYPE;
    size_t nFirstOp=ops.size();
    switch( simd )
    {
    case 4:
        {
            // SIMD4
     
            // do some math
            for( size_t i=0; i<NUM_VEC4S; i++ )
            {
                size_t r = 8 + (i%nRegs) ; // write alternating registers
                ops.push_back(
                    INSTRUCTION(4,r)
                );
            }
        }
        break;
    case 8:
        {
            // SIMD8
     
            // do some math
            for( size_t i=0; i<NUM_VEC8S; i++ )
            {
                size_t r = 8 + (i%nRegs) ; // write alternating registers
                ops.push_back(
                    INSTRUCTION(8,r)
                );
            
            }
        }
        break;
    case 16:
        {
            // SIMD16
      
   
            // do some math
            for( size_t i=0; i<NUM_VEC16S; i++ )
            {
                size_t r = 8 + 2*(i%nRegs); // write alternating registers
                ops.push_back(
                    INSTRUCTION(16,r)
                );
            }
        }
        break;
    }

#ifdef NODDCHK
    // set the noDDChk bits
    //   Using NoDDChk with f-add seems to kill dual-issue
    while( nFirstOp < ops.size() )
        ops[nFirstOp++].DisableDDCheck();
#endif
    ///////////////////////////////////////////////////
    
   
   
    
    ops.push_back( 
        GEN::SendEOT(127)
    );
}





HAXWell::timer_t InstructionIssueTest( size_t nRegs, size_t simd )
{
    size_t nGroups = 128;
    size_t nThreadsPerGroup=60;
    std::vector<GEN::Instruction> ops;
    ConstructShader(ops,nRegs,nThreadsPerGroup, simd);

    GEN::Encoder enc;
    HAXWell::Blob isa;
    isa.SetLength( enc.GetBufferSize(ops.size()) );
    isa.SetLength( enc.Encode( isa.GetBytes(), ops.data(), ops.size() ) );

   // PrintISA(stdout,isa);

    int CURBE[HAXWell::MAX_DISPATCH_COUNT][8];
    for( size_t i=0; i<HAXWell::MAX_DISPATCH_COUNT; i++ )
        for( size_t j=0; j<8; j++ )
            CURBE[i][j] = 2*i;


    HAXWell::ShaderArgs args;
    args.nDispatchThreadCount = nThreadsPerGroup;
    args.nSIMDMode = 16;//(simd <= 8) ? 8 : 16; // dispatch mode really doesn't seem to matter
    args.nCURBEAllocsPerThread = 1;
    args.pCURBE = CURBE;
    args.nIsaLength = isa.GetLength();
    args.pIsa = isa.GetBytes();
    HAXWell::ShaderHandle hShader = HAXWell::CreateShader(args);

    HAXWell::BufferHandle hBuffer = HAXWell::CreateBuffer( 0, 32*nGroups*args.nDispatchThreadCount );

    HAXWell::TimerHandle h = HAXWell::BeginTimer();
    HAXWell::DispatchShader( hShader, &hBuffer,1,nGroups );
    HAXWell::EndTimer( h );
    
    HAXWell::Finish();


    HAXWell::ReleaseBuffer(hBuffer);
    HAXWell::ReleaseShader(hShader);
    return HAXWell::ReadTimer(h);
}




void IssueTest()
{
    FILE* fp = fopen(FILENAME, "w" );
    fprintf(fp, ",1-reg, 2-reg,4-reg,8-reg,16-reg\n");

    for( size_t simd=4; simd<=16; simd *= 2 )
    {
        fprintf(fp,"simd%u, ", simd );
        for( size_t i=1; i<32; i*= 2 )
        {
            HAXWell::timer_t tm;
            tm  = InstructionIssueTest(i,simd);
            tm += InstructionIssueTest(i,simd);
            tm += InstructionIssueTest(i,simd);
            fprintf(fp, "%u,", tm/3 );
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
}