// Run increasing numbers of instructions and attempt to find the "instruciton cache cliff"

#include "HAXWell.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENIsa.h"
#include <vector>

#include "Misc.h"

static void ConstructShader( std::vector<GEN::Instruction>& ops, size_t nOps, size_t nThreadsPerGroup )
{
    // read timestamp register, first thing
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,6,0),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_TIMESTAMP,0),2,2,1))
            )
    );
   
    // read state register
    ops.push_back(
        GEN::RegMove( GEN::REG_GPR, 5, GEN::REG_STATE, 0 )
    );

    // write address is threadgroup_id*threads_per_group*2 + 2*local_thread_id 
    //  
    ops.push_back( 
        GEN::DoMath( 8, GEN::OP_XOR, GEN::DT_U32, 4,4,4 )
    );
    // copy initial timestamp into output
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,5,4),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,6,0),2,2,1))
            )
    );
  
    // address mul
    ops.push_back(
        GEN::BinaryInstruction( 1, GEN::OP_MUL,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,4,8),1,1,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,0,4),1,1,1) ),
            GEN::SourceOperand( GEN::DT_U32 , nThreadsPerGroup*2 )
            )
    );
    // copy EOT payload
    ops.push_back( 
        GEN::UnaryInstruction( 8, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(127),8,8,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(0),8,8,1) ) 
            )
    );
    // address offset
    ops.push_back( // assume address offset (2*local) passed in curbe
        GEN::BinaryInstruction( 1, GEN::OP_ADD,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,4,8),1,1,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,4,8),1,1,1) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(1),1,1,1))
            )
    );
   
  
  

  
    // do some math
    for( size_t i=0; i<nOps; i++ )
    {
        size_t r = 8 + (i%8) ; // write alternating registers
        ops.push_back(
            GEN::BinaryInstruction(
                8, GEN::OP_MAC, 
                GEN::DestOperand( GEN::DT_F32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,r,0),8,8,1)),
                GEN::SourceOperand( GEN::DT_F32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,r,0),8,8,1)),
                GEN::SourceOperand( GEN::DT_F32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,r,0),8,8,1))
                )
        );
    }
    
    // read timestamp again to get delta
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,5,12),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_TIMESTAMP,0),2,2,1))
            )
    );

    // read EUID information from state reg
    ops.push_back( 
         GEN::UnaryInstruction( 1, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,5,0),1,1,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_STATE,0),1,1,1))
            )
    );



    // read final timestamp
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,5,12),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_TIMESTAMP,0),2,2,1))
            )
    );

    // write output (address in r4, data in r5)
    ops.push_back( 
        GEN::OWordDualBlockWrite( HAXWell::BIND_TABLE_BASE, 4 )
    );

    
    ops.push_back( 
        GEN::SendEOT(127)
    );
}



void FindCliff( size_t nOps, FILE* plot )
{
    size_t nGroups = 32;
    size_t nThreadsPerGroup=60;
    std::vector<GEN::Instruction> ops;
    ConstructShader(ops,nOps,nThreadsPerGroup);

    GEN::Encoder enc;
    HAXWell::Blob isa;
    isa.SetLength( enc.GetBufferSize(ops.size()) );
    isa.SetLength( enc.Encode( isa.GetBytes(), ops.data(), ops.size() ) );

    //PrintISA(stdout,isa);

    int CURBE[HAXWell::MAX_DISPATCH_COUNT][8];
    for( size_t i=0; i<HAXWell::MAX_DISPATCH_COUNT; i++ )
        for( size_t j=0; j<8; j++ )
            CURBE[i][j] = 2*i;


    HAXWell::ShaderArgs args;
    args.nDispatchThreadCount = nThreadsPerGroup;
    args.nSIMDMode = 8;
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

    HAXWell::timer_t nTime = HAXWell::ReadTimer(h);
    
    size_t nThreads = nGroups*args.nDispatchThreadCount;

    unsigned int* pBuff = (unsigned int*)HAXWell::MapBuffer(hBuffer);
  
    size_t nAvg=0;
    for( size_t i=0; i<nThreadsPerGroup*nGroups; i++ )
    {
        unsigned int startlo = pBuff[8*i+1];
        unsigned int endlo = pBuff[8*i+3];
        nAvg += endlo-startlo;
    }

    HAXWell::UnmapBuffer(hBuffer);
    HAXWell::ReleaseShader(hShader);
    HAXWell::ReleaseBuffer(hBuffer);
    double latency = ((double)nAvg) / (nThreadsPerGroup*nGroups);
    
    fprintf(plot, "%u, %f, %u\n", 16*ops.size(), latency,  ops.size() );
    printf("%u, %f, %u\n", 16*ops.size(), latency, ops.size() );

}


void FindICacheCliff()
{
    char name[256];
    sprintf(name, "icachecliff.csv" );

    FILE* plot = fopen(name, "w");
    fprintf(plot, "program bytes, avg latency, op count\n");

    for( size_t i=0; i<10000; i += 16 )
        FindCliff(i,plot);
    
    fclose(plot);

}