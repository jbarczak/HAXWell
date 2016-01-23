// See how many compute instructions one needs to cover the latency of a memory read message


#include "HAXWell.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENIsa.h"
#include <vector>

#include "Misc.h"

#define FILENAME "block_sa.csv"
#define REPEAT_COUNT 32
//#define READ_INVALIDATE
#define SAME_ADDRESS

static void ConstructShader( std::vector<GEN::Instruction>& ops, size_t nComputes, size_t nThreadsPerGroup )
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
            GEN::SourceOperand( GEN::DT_U32 ), nThreadsPerGroup*2
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
   




    // build read address.    Start with the write address we're using for timestamps
    ops.push_back( 
        GEN::RegMove( GEN::REG_GPR, 9, GEN::REG_GPR, 4 )
    );

    // our base address is in 32-byte units.  Convert this to cachelines
    ops.push_back( 
        GEN::BinaryInstruction( 1, GEN::OP_MUL,
            GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,9,8),1,1,1 ) ),
            GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference(GEN::REG_GPR,9,8),1,1,1) ),
            GEN::SourceOperand( GEN::DT_U32 ), 2
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
    
   
       
    for( size_t i=0; i<REPEAT_COUNT; i++ )
    {
        // send read message
        size_t nReg=10;

#ifdef READ_INVALIDATE
        ops.push_back( 
            GEN::OWordDualBlockReadAndInvalidate( HAXWell::BIND_TABLE_BASE+1, 9, nReg )
            );
#else
        ops.push_back( 
            GEN::OWordDualBlockRead( HAXWell::BIND_TABLE_BASE+1, 9, nReg )
        );
#endif


        // do some intervening math
        for( size_t j=0; j<nComputes; j++ )
        {
            ops.push_back(
                GEN::BinaryInstruction(
                    8, GEN::OP_ADD, 
                    GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,nReg+1,0),8,8,1)),
                    GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,nReg+1,0),8,8,1)),
                    GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,nReg+1,0),8,8,1))
                    )
            );
        }

        // use the read result
        ops.push_back(
            GEN::RegMove(GEN::REG_GPR,nReg+1,GEN::REG_GPR,nReg)
        );
        
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





double BlockReadTest( size_t nComputation )
{
    size_t nGroups = 32;
    size_t nThreadsPerGroup=60;
    std::vector<GEN::Instruction> ops;
    ConstructShader(ops,nComputation, nThreadsPerGroup);

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
    args.nSIMDMode = 16;
    args.nCURBEAllocsPerThread = 1;
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




void BlockReadTest()
{
    FILE* fp = fopen(FILENAME, "w" );
    fprintf(fp, "compute operations, mean shader latency\n");
    for( size_t i=1; i<32; i++ )
    {
        double tm;
        tm  = BlockReadTest(i);
        tm += BlockReadTest(i);
        tm += BlockReadTest(i);
        fprintf(fp, "%u,%f\n", i,tm/3 );
    }
    
    fclose(fp);
}