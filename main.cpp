

#include "HAXWell.h"
#include <windows.h>
#define MACHINE_THREAD_COUNT 140
#define MACHINE_EU_COUNT 20
#define SUBSLICE_EU_COUNT 10
#define EU_THREAD_COUNT 7

void ThreadTimings( size_t nThreadsPerGroup, size_t nGroups, size_t nMovs);
void InstructionIssueTest( size_t nRegs, size_t simd );

void FindICacheCliff();
void IssueTest();
void BlockReadTest();
void ScatteredReadTest();

void Nbody();

int main( int argc, char* argv[] )
{
    HAXWell::Init(true);

    

    // different ways to run 4200 threads
    //ThreadTimings( 1,SUBSLICE_EU_COUNT*6*10*7, 0 ); // single thread groups
    //ThreadTimings( SUBSLICE_EU_COUNT*6, 70, 0 );    // fat groups
    //ThreadTimings( 7, SUBSLICE_EU_COUNT*6*10, 0 );  // skinnier groups

    //IssueTest();
    //BlockReadTest();
    //ScatteredReadTest();

    
     /* 
      InstructionIssueTest(16,4);
      InstructionIssueTest(8,4);
      InstructionIssueTest(4,4);
      InstructionIssueTest(2,4);
      InstructionIssueTest(1,4);
      
      InstructionIssueTest(16,8);
      InstructionIssueTest(8,8);
      InstructionIssueTest(4,8);
      InstructionIssueTest(2,8);
      InstructionIssueTest(1,8);
      
      InstructionIssueTest(16,16); 
      InstructionIssueTest(8,16); 
      InstructionIssueTest(4,16); 
      InstructionIssueTest(2,16); 
      InstructionIssueTest(1,16); 
      */
   
    Nbody();


    return 0;
}