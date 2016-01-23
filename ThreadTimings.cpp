// Dispatch threads and Log thread slot assignments and thread start/end times

#include "HAXWell.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENIsa.h"
#include <vector>

#include "Misc.h"




void ConstructShader( std::vector<GEN::Instruction>& ops, size_t nThreadsPerGroup, size_t nMovs )
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
   
 
   
    // make the threads last longer
    for( size_t i=0; i<nMovs; i++ )
    {
        ops.push_back(
            GEN::UnaryInstruction(
                8, GEN::OP_MOV, 
                GEN::DestOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,20,0),8,8,1)),
                GEN::SourceOperand( GEN::DT_U32, GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR,21,0),8,8,1)) ) 
        );
    }
   

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

void ThreadTimings( size_t nThreadsPerGroup, size_t nGroups, size_t nMovs )
{
    std::vector<GEN::Instruction> ops;
    ConstructShader(ops,nThreadsPerGroup,nMovs);

    GEN::Encoder enc;
    HAXWell::Blob isa;
    isa.SetLength( enc.GetBufferSize(ops.size()) );
    isa.SetLength( enc.Encode( isa.GetBytes(), ops.data(), ops.size() ) );

    PrintISA(stdout,isa);

    int* CURBE = new int[nThreadsPerGroup*8];
    for( size_t i=0; i<nThreadsPerGroup; i++ )
        for( size_t j=0; j<8; j++ )
            CURBE[8*i+j] = 2*i;


    HAXWell::ShaderArgs args;
    args.nDispatchThreadCount = nThreadsPerGroup;
    args.nSIMDMode = 16;
    args.nCURBEAllocsPerThread = 1;
    args.pCURBE = CURBE;
    args.nIsaLength = isa.GetLength();
    args.pIsa = isa.GetBytes();
    HAXWell::ShaderHandle hShader = HAXWell::CreateShader(args);

    HAXWell::BufferHandle hBuffer = HAXWell::CreateBuffer( 0, 32*nThreadsPerGroup*nGroups );

    HAXWell::TimerHandle h = HAXWell::BeginTimer();
    HAXWell::DispatchShader( hShader, &hBuffer,1,nGroups );
    HAXWell::EndTimer( h );
    HAXWell::Finish();

    HAXWell::timer_t nTime = HAXWell::ReadTimer(h);
    
    unsigned int* pBuff = (unsigned int*)HAXWell::MapBuffer(hBuffer);

    
    char name[256];
    sprintf(name, "timings_%ux%u_%u.csv", nThreadsPerGroup,nGroups, nMovs);

    FILE* plot = fopen(name, "w");
    fprintf(plot, "ThreadSlot, DispatchTID, GroupID, ThreadIDInGroup, HW-EUID, HW-TID,Slice, start, end, start(eu), end(tu), GL time: %u ns\n", nTime );

    unsigned __int64* pLocalStart = new unsigned __int64[nThreadsPerGroup*nGroups];
    unsigned __int64* pLocalEnd = new unsigned __int64[nThreadsPerGroup*nGroups];
    GetEULocalTimes( pBuff, (unsigned __int64*)(pBuff+1),(unsigned __int64*)(pBuff+3),32,
                     nThreadsPerGroup*nGroups, 
                     pLocalStart, pLocalEnd );
    

    for( size_t i=0; i<nThreadsPerGroup*nGroups; i++ )
    {
        unsigned int sr = pBuff[8*i];
        unsigned int startlo = pBuff[8*i+1];
        unsigned int starthi = pBuff[8*i+2];
        unsigned int endlo = pBuff[8*i+3];
        unsigned int endhi = pBuff[8*i+4];
        int EUID     = (sr&0xf00)>>8;
        int Slot     = (sr&0xff);
        int SubSlice = (sr&0x1000)>>12;

       
        // EU numbering is a tad unusual.  We have
        //   a jump between 4 and 8, like so:
        //
        //     0000  and  1000
        //     0001       1001
        //     0010       1010
        //     0011       1011
        //     0100       1100
        //     
        //  It looks like they're planning to scale to up to 16 EUs/slice, but aren't there yet
        //
        // for our graphs, lets just count thread slots
        int nLinearEUID = (EUID& 0x7) + 5*(EUID>>3);
        nLinearEUID += 10*SubSlice;
        int nThreadID = 7*nLinearEUID + Slot;


        // remap so that whole-numbers are EUs and threads are fractional
        //  This lets us draw gridlines at 7-unit intervals to seperate EUs
        float fTID = (nThreadID + 0.5f) / 7.0f;
        fprintf(plot,"%f, %u, %u, %u, %u, %u, %u, %u, %u, %llu, %llu\n", fTID, 
                i, i / nThreadsPerGroup, i%nThreadsPerGroup, 
                EUID, Slot, SubSlice, 
                startlo, endlo, pLocalStart[i], pLocalEnd[i]  ); 
    }

    fclose(plot);
}

