
#include <stdio.h>
#include <string>

namespace HAXWell
{
    class Blob;
};


void PrintISA( FILE* fp, HAXWell::Blob& blob );
void PrintISA( FILE* fp, const void* pBytes, size_t nBytes );

int GetLinearThreadID( size_t sr );

/// Make EU-specific timestamps relative to the lowest time seen for a given EU
///   Thread stride is distance in bytes between successive times in state/reg and time arrays
void GetEULocalTimes(  const unsigned int* pStateRegs, 
                       const unsigned __int64* pEUStartTimes, 
                       const unsigned __int64* pEUEndTimes, 
                       size_t nThreadStride, size_t nThreads,
                       unsigned __int64* pLocalStartTimes,
                       unsigned __int64* pLocalEndTimes );

void CountOps( size_t nIsaLength, const unsigned char* pIsa, unsigned int* pALU, unsigned int* pSend );

std::string ReadTextFile( const char* pPath );