// See how many compute instructions one needs to cover the latency of a memory read message


#include "HAXWell.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENIsa.h"
#include <vector>

#include "Misc.h"



#define FILENAME "Scattervsgather.csv"
#define REPEAT_COUNT 32
#define SAME_ADDRESS

static void ConstructShader( std::vector<GEN::Instruction>& ops, size_t nThreadsPerGroup, bool bBlockReads )
{
    // write address is threadgroup_id*threads_per_group*2 + 2*local_thread_id 
    //  
    ops.push_back( 
        GEN::DoMath( 8, GEN::OP_XOR, GEN::DT_U32, 4,4,4 )
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
   


#ifdef SAME_ADDRESS
    ops.push_back( 
        GEN::RegMoveIMM( 9, 0 )
    );
#endif

    // read timestamp register
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,6,0),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_TIMESTAMP,0),2,2,1))
            )
    );

    ///////////////////////////////////////////////////
    
   
    if( bBlockReads )
    {
        for( size_t i=0; i<REPEAT_COUNT; i++ )
        {
           ops.push_back( 
               GEN::OWordDualBlockRead( HAXWell::BIND_TABLE_BASE + 1, 3, 2 )
               );

            // use the read result
            ops.push_back(
                GEN::RegMove(GEN::REG_GPR,7,GEN::REG_GPR,2)
            );
        
        }
    }
    else
    {
        for( size_t i=0; i<REPEAT_COUNT; i++ )
        {
           ops.push_back( 
               GEN::DWordScatteredReadSIMD8( HAXWell::BIND_TABLE_BASE + 1, 2, 3 )
               );

            // use the read result
            ops.push_back(
                GEN::RegMove(GEN::REG_GPR,7,GEN::REG_GPR,3)
            );
        
        }
    }

    ///////////////////////////////////////////////////
    
   
   
    
    // read timestamp again to get delta
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,7,0),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_TIMESTAMP,0),2,2,1))
            )
    );

    // collect timestamps in output reg
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,5,0),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,6,0),2,2,1))
            )
            );
    
    ops.push_back( 
         GEN::UnaryInstruction( 2, GEN::OP_MOV,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,5,8),2,2,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,7,0),2,2,1))
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





double DoTest( bool bBlockReads )
{
    size_t nGroups = 32;
    size_t nThreadsPerGroup=60;
    std::vector<GEN::Instruction> ops;
    ConstructShader(ops,nThreadsPerGroup, bBlockReads );

    GEN::Encoder enc;
    HAXWell::Blob isa;
    isa.SetLength( enc.GetBufferSize(ops.size()) );
    isa.SetLength( enc.Encode( isa.GetBytes(), ops.data(), ops.size() ) );

    //PrintISA(stdout,isa);

    int CURBE[HAXWell::MAX_DISPATCH_COUNT][3][8];
    for( size_t i=0; i<HAXWell::MAX_DISPATCH_COUNT; i++ )
    {
        // first CURBE reg is offset into timings buffer
        for( size_t j=0; j<8; j++ )
            CURBE[i][0][j] = 2*i;

        // second CURBE reg is per-channel read offsets for scatter read
        for( size_t j=0; j<8; j++ )
            CURBE[i][1][j] = 8*i + j;

        // third CURBE reg per-thread base address for block reads
        // note that blcok read offset goes in second dword of register
        for( size_t j=0; j<8; j++ )
            CURBE[i][2][j] = 0;
        CURBE[i][2][2] = 2*i;
    }


    HAXWell::ShaderArgs args;
    args.nDispatchThreadCount = nThreadsPerGroup;
    args.nSIMDMode = 16;
    args.nCURBEAllocsPerThread = 3;
    args.pCURBE = CURBE;
    args.nIsaLength = isa.GetLength();
    args.pIsa = isa.GetBytes();
    HAXWell::ShaderHandle hShader = HAXWell::CreateShader(args);

    HAXWell::BufferHandle hBuffers[] =
    {
        HAXWell::CreateBuffer( 0, 32*nGroups*args.nDispatchThreadCount ),
        HAXWell::CreateBuffer( 0, 32*nGroups*args.nDispatchThreadCount*1024 ) ,
    };

    HAXWell::DispatchShader( hShader, hBuffers,2,nGroups );
    
    HAXWell::Finish();

    
    unsigned int* pBuff = (unsigned int*)HAXWell::MapBuffer(hBuffers[0]);
  
    size_t nAvg=0;
    for( size_t i=0; i<nThreadsPerGroup*nGroups; i++ )
    {
        unsigned int startlo = pBuff[8*i];
        unsigned int endlo = pBuff[8*i+2];
        nAvg += endlo-startlo;
    }

    HAXWell::UnmapBuffer(hBuffers[0]);


    HAXWell::ReleaseBuffer(hBuffers[0]);
    HAXWell::ReleaseBuffer(hBuffers[1]);
    HAXWell::ReleaseShader(hShader);
  
    return (nAvg)/(double)(nGroups*nThreadsPerGroup);
}




void ScatterVsGather()
{
   
    double avg_block = 0;
    double avg_scat = 0;
    
    for( size_t i=0; i<5; i++ )
    {
        avg_block += DoTest( false );
        avg_scat += DoTest( true );
    }

    printf(" block %.2f\nscat: %.2f\n", avg_block/5, avg_scat/5 );

    
}