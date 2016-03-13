
#ifndef _GEN_ASM_H_
#define _GEN_ASM_H_

#include <vector>
#include "GENIsa.h"

namespace GEN
{
    class IPrinter;
    class Encoder;   
 
    namespace Assembler
    {
        class Program
        {
        public:

            Program();

            ~Program();

            bool Assemble( Encoder* pCoder, const char* pText, IPrinter* pErrorStream );

            void Clear();
            
            const void* GetIsa() const { return m_pIsa; }
            size_t GetIsaLengthInBytes() const { return m_nIsaLengthInBytes; }

            const void* GetCURBE() const { return m_pCURBE; }
            size_t GetCURBERegCount() const { return m_nCURBECount; }
            size_t GetThreadsPerDispatch() const { return m_nThreadsPerGroup; }
        private:
            
            size_t m_nThreadsPerGroup;
            size_t m_nIsaLengthInBytes;
            size_t m_nCURBECount;
            void* m_pIsa;
            void* m_pCURBE;
        };


    }
};

#endif