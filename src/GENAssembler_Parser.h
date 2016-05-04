


#include <string>
#include <list>
#include <vector>

#include "GENIsa.h"

typedef void* yyscan_t;

namespace GEN
{
    class IPrinter;

    namespace Assembler{
    namespace _INTERNAL{

        struct ParseNode
        {
            ParseNode( size_t line ) : LineNumber(line){}
            size_t LineNumber;
        };

        
        struct TokenStruct
        {
            size_t LineNumber;
            union
            {
                int Int;
                float Float;
                double Double;
                const char* ID;
                ParseNode* node;
            } fields;
        };

        class Parser
        {
        public:
            ~Parser();

            const std::vector<Instruction>& GetInstructions() { return m_Instructions; }
            const std::vector<uint8>& GetCURBE() const { return m_CURBE; }
            size_t GetThreadsPerGroup() const { return m_nThreadsPerGroup; }

            bool Parse( const char* pText, IPrinter* pErrorStream );

            bool Begin( size_t line );
            void End();

            // methods below are "private" but are called by generated flex/bison code
            //  and this is easier with trying to mess with friendship and namespaces...
     
            void ErrorF( size_t nLine, const char* msg, ... );
            void Error( size_t nLine, const char* msg );
            size_t Read( char* buff, size_t max_size );

            yyscan_t GetScanner() const { return m_scanner; }

            const char* StoreString( const char* pyyText );

            
            void RegDeclaration( TokenStruct& name, size_t count );
            void BindDeclaration( TokenStruct& name, int BindPoint );
            

            void CURBEBegin( TokenStruct& name, size_t nCURBESizeInRegs );
            void CURBEPush( size_t nLine, const void* pBytes, size_t nBytes );
            void CURBEBreak( size_t nLine );
            void CURBEEnd( );
            bool ThreadCount( const TokenStruct& count );
            bool Label( const TokenStruct& label );

            ParseNode* Operation( TokenStruct& rToken, int nExecSize, ParseNode* pFlagReference );
            ParseNode* SubReg( TokenStruct& rToken );
            ParseNode* DirectRegReference( TokenStruct& rToken );
            ParseNode* IndirectRegReference( TokenStruct& rGPR, TokenStruct& rAddr, int addrsub );
            ParseNode* RegRegion( size_t nLine, int v, int w, int h);
            ParseNode* SourceReg( ParseNode* pReg, ParseNode* pRegion, ParseNode* pSubReg );
            ParseNode* DestReg( ParseNode* pReg, int hstride, ParseNode* pSubReg );
            ParseNode* FloatLiteral( size_t line, float f );
            ParseNode* IntLiteral( size_t line, int n );
            ParseNode* FlagReference( TokenStruct& id, TokenStruct& subReg );

            void Unary( ParseNode* pOp, ParseNode* pDst, ParseNode* pSrc );
            void Binary( ParseNode* pOp, ParseNode* pDst, ParseNode* pSrc0, ParseNode* pSrc1 );
            void Ternary( ParseNode* pOp, ParseNode* pDst, ParseNode* pSrc, ParseNode* pSrc1, ParseNode* pSrc2  );
            void Send( TokenStruct& msg, TokenStruct& bind, ParseNode* pDst0, ParseNode* pDst1 );
            
            void Jmp( TokenStruct& label );
            void JmpIf( TokenStruct& label, ParseNode* pFlagRef, bool bInvert );
            
            void BeginPredBlock( ParseNode* pFlagRef );
            void EndPredBlock();

            yyscan_t m_scanner;

        private:

            bool InterpretRegName( GEN::RegTypes* pRegType, size_t* pRegNum, const TokenStruct& rToken);
  
            struct NamedReg
            {
                NamedReg( const char* p, GEN::DirectRegReference d, size_t nRegs ) : pName(p), reg(d), nRegArraySize(nRegs) {}
                const char* pName;
                GEN::DirectRegReference reg;
                size_t nRegArraySize;
            };

            struct Jump
            {
                ParseNode* pFlagRef; 
                bool bInvertPredicate;
                size_t nLine;
                size_t nJumpIndex;
                const char* pLabelName;
            };

            struct BindPoint
            {
                const char* pName;
                int bind;
            };
            struct LabelInfo
            {
                size_t nInstructionIndex;
                const char* pName;
            };

            BindPoint* FindBindPoint( const char* pName );
            NamedReg* FindNamedReg( const char* pName );
            void AddNamedReg( const char* pName, GEN::DirectRegReference reg, size_t nArraySize );
            LabelInfo* FindLabel( const char* pName );

            size_t m_nThreadsPerGroup;

            GEN::IPrinter* m_pErrorPrinter;
            const char* m_pText;
            std::list< std::string > m_TokenStrings;
            std::vector< ParseNode* > m_Nodes;
            std::vector< NamedReg > m_NamedRegs;
            std::vector< BindPoint > m_BindPoints;
            std::vector< LabelInfo > m_Labels;
            std::vector< Jump > m_Jumps;
            std::vector<uint8> m_CURBE;
            std::vector<Instruction> m_Instructions;
            bool m_bError;

            uint8 m_CURBEScratch[32];
            size_t m_nCURBEScratchOffs;
            size_t m_nCURBERegCount;

            size_t m_nPredStart;
            ParseNode* m_pPred;
           
            
        };





    }}


};
