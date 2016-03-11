
#include "GENAssembler.h"
#include "GENAssembler_Parser.h"
#include "GENCoder.h"

namespace GEN{
namespace Assembler{
    
    Program::Program()
        : m_nThreadsPerGroup(0), m_nIsaLengthInBytes(0), m_nCURBECount(0), m_pIsa(0), m_pCURBE(0)
    {
    }

    Program::~Program()
    {
        free(m_pIsa);
        free(m_pCURBE);
    }

    void Program::Clear()
    {
        free(m_pIsa);
        free(m_pCURBE);
        m_nThreadsPerGroup=0;
        m_nIsaLengthInBytes=0; 
        m_nCURBECount=0;
        m_pIsa=0;
        m_pCURBE=0;
    }

    bool Program::Assemble( Encoder* pEncoder, const char* pText, IPrinter* pErrorStream )
    {
        Clear();

        GEN::Assembler::_INTERNAL::Parser parser;
        if( !parser.Parse( pText, pErrorStream ) )
            return false;

        if( !pEncoder )
            return true; // just check the syntax

        size_t nIsaSize = pEncoder->GetBufferSize( parser.GetInstructions().size() );
        size_t nCURBESize = parser.GetCURBE().size();

        m_pIsa   = malloc(nIsaSize);
        m_pCURBE = malloc(nCURBESize);
        if( !m_pIsa || !m_pCURBE )
        {
            Clear();
            return false;
        }

        m_nIsaLengthInBytes = pEncoder->Encode( m_pIsa, parser.GetInstructions().data(), parser.GetInstructions().size() );
        m_nCURBECount = parser.GetCURBE().size()/32;
        memcpy( m_pCURBE, parser.GetCURBE().data(), m_nCURBECount*32 ); 

        m_nThreadsPerGroup = parser.GetThreadsPerGroup();
        return true;
    }

    
}}