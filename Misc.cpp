
#include "HAXWell.h"
#include <stdio.h>
#include "Misc.h"
#include "GENCoder.h"
#include "GENDisassembler.h"
#include <map>

void PrintISA( FILE* fp, HAXWell::Blob& blob )
{
    class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            fprintf(fp,"%s", p );
        }
        FILE* fp;
    };
    Printer printer;
    printer.fp = fp;
    GEN::Decoder dec;
    GEN::Disassemble( printer, &dec, blob.GetBytes(), blob.GetLength() );
}

void PrintISA( FILE* fp, const void* pBytes, size_t nBytes )
{
    class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            fprintf(fp,"%s", p );
        }
        FILE* fp;
    };
    Printer printer;
    printer.fp = fp;
    GEN::Decoder dec;
    GEN::Disassemble( printer, &dec, pBytes, nBytes );
}


int GetLinearThreadID( size_t sr )
{
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
    int EUID     = (sr&0xf00)>>8;
    int Slot     = (sr&0xff);
    int SubSlice = (sr&0x1000)>>12;


    int nLinearEUID = (EUID& 0x7) + 5*(EUID>>3);
    nLinearEUID += 10*SubSlice;
    int nThreadID = 7*nLinearEUID + Slot;
    return nThreadID;
}


void GetEULocalTimes(  const unsigned int* pStateRegs, 
                       const unsigned __int64* pEUStartTimes, 
                       const unsigned __int64* pEUEndTimes, 
                       size_t nThreadStride, size_t nThreads,
                       unsigned __int64* pLocalStartTimes,
                       unsigned __int64* pLocalEndTimes
                       )
{
    struct Times
    {
        __int64 Min;
        __int64 Max;
       
    };
    std::map<unsigned int, Times> TimeMap;

    for( size_t i=0; i<nThreads; i++ )
    {
        unsigned int* pState      = (unsigned int*)  (((char*)pStateRegs)+i*nThreadStride);
        unsigned __int64* pEUStartTime = (unsigned __int64*)(  ((char*)pEUStartTimes)+i*nThreadStride);
        unsigned __int64* pEUEndTime = (unsigned __int64*)(  ((char*)pEUEndTimes)+i*nThreadStride);
        
        int EU = GetLinearThreadID(*pState)/7;
        if( TimeMap.find( EU )  == TimeMap.end() )
        {
            Times tm;
            tm.Max = *pEUStartTime;
            tm.Min = *pEUEndTime;
            TimeMap.insert( std::pair<unsigned int,Times>(EU,tm) );
        }

        Times& time = TimeMap[EU];
        time.Min = (time.Min > *pEUStartTime ) ? *pEUStartTime : time.Min;
        time.Max = (time.Max < *pEUEndTime ) ? *pEUEndTime : time.Max;
    }

    for( size_t i=0; i<nThreads; i++ )
    {
        unsigned int* pState      = (unsigned int*)  (((char*)pStateRegs)+i*nThreadStride);
        unsigned __int64* pEUStartTime = (unsigned __int64*)(  ((char*)pEUStartTimes)+i*nThreadStride);
        unsigned __int64* pEUEndTime = (unsigned __int64*)(  ((char*)pEUEndTimes)+i*nThreadStride);
        
        pLocalStartTimes[i] = (*pEUStartTime) - TimeMap[ GetLinearThreadID(*pState)/7 ].Min;
        pLocalEndTimes[i]   = (*pEUEndTime) - TimeMap[ GetLinearThreadID(*pState)/7 ].Min;
    }
}
