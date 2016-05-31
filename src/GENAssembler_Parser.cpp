

#include "GENDisassembler.h" // for 'IPrinter'  TODO: Move 'IPrinter' to its own header
#include "GENAssembler_Parser.h"

#include "GENIsa.h"
#include <stdarg.h>

int yylex_init_extra(GEN::Assembler::_INTERNAL::Parser* yy_user_defined,yyscan_t* ptr_yy_globals );
int yyparse( GEN::Assembler::_INTERNAL::Parser* pParser );
int yylex_destroy  (yyscan_t yyscanner);

namespace GEN{
namespace Assembler{
namespace _INTERNAL{

    
    struct SubRegNode : public ParseNode
    {
        SubRegNode( size_t line ) : ParseNode(line){}
        DataTypes eType;
        size_t nSubreg;
    };
    struct RegRefNode : public ParseNode
    {
        RegRefNode( size_t line, bool bIsDirect ) : ParseNode(line), bIsDirect(bIsDirect){}

        bool bIsDirect;
    };
    struct DirectRegRefNode : public RegRefNode
    {
        DirectRegRefNode( size_t line ) : RegRefNode(line,true) {}
        RegTypes eRegType;
        size_t nRegNum;
        
    };
    struct IndirectRegRefNode : public RegRefNode
    {
        IndirectRegRefNode( size_t line ) : RegRefNode(line,false){}
        size_t nGPR;
        size_t nAddrSubReg;
    };

    struct SourceModifierNode : public ParseNode
    {
        SourceModifierNode( size_t line ) : ParseNode(line) {}
        virtual bool IsSwizzle() =0;
    };

    struct RegionNode : public SourceModifierNode
    {
        RegionNode( size_t line ) : SourceModifierNode(line){}
    
        int width;
        int hstride;
        int vstride;
        virtual bool IsSwizzle() { return false; }
    };

    
    struct SwizzleNode : public SourceModifierNode
    {
        SwizzleNode( size_t line ) : SourceModifierNode(line){}
    
        GEN::Swizzle swizz;
        virtual bool IsSwizzle() { return true; }
    };

    struct SourceNode : public ParseNode
    {
        SourceNode( size_t line ) : ParseNode(line) {}
        virtual bool IsImmediate() = 0;
    };
    struct SourceRegNode : public SourceNode
    {
        SourceRegNode( size_t line ) : SourceNode(line){}
        GEN::DataTypes eType;
        RegReference base;
        SourceModifierNode* pRegionOrSwizzle;
        virtual bool IsImmediate() { return false; }
    
    };

    struct ImmediateNode : public SourceNode
    {
        ImmediateNode( size_t line ) : SourceNode(line){}
        SourceOperand imm;
        virtual bool IsImmediate() { return true; }
    };

    struct DestRegNode : public ParseNode
    {
        DestRegNode( size_t line ) : ParseNode(line){}
        GEN::DestOperand dest;
    };
    struct FlagReferenceNode : public ParseNode
    {
        FlagReferenceNode( size_t line ) : ParseNode(line){}
        GEN::FlagReference Flag;
    };

    struct OperationNode : public ParseNode
    {
        OperationNode( size_t line ) : ParseNode(line), pFlagRef(0){}
        const char* pName;
        size_t nExecSize;
        FlagReferenceNode* pFlagRef;
    };


    struct TokenID
    {
        const char* Token;
        uint32 ID;
    };

    const TokenID* Lookup( const TokenID* pTable, const char* str )
    {
        while( pTable->Token )
        {
            if( strcmp(pTable->Token,str) == 0 )
                return pTable;
            ++pTable;
        }
        return 0;
    }


    Parser::~Parser()
    {
        // TODO: ick...   Replace this with a pool allocator...
        for( auto it : m_Nodes )
            delete it;
    }

    void Parser::ErrorF( size_t nLine, const char* msg, ... )
    {
        m_bError = true;
        if( !m_pErrorPrinter )
            return;

        va_list vl;
        va_start(vl,msg);

        char scratch[2048];
        char* p = scratch;

        int n = _vscprintf( msg, vl );
        if( n+1 > sizeof(scratch) )
            p = (char*) malloc( n+1 );
            
        vsprintf(p,msg,vl);

        char line[64];
        sprintf(line, "Line: %u:  ", nLine );
        m_pErrorPrinter->Push(line);
        m_pErrorPrinter->Push(p);

        if( p != scratch )
            free(p);

        va_end(vl);
        
    }

    void Parser::Error( size_t nLine, const char* msg)
    {
        m_bError = true;
        if( !m_pErrorPrinter )
            return;

        char line[64];
        sprintf(line, "Line: %u:  ", nLine );
        m_pErrorPrinter->Push(line);
        m_pErrorPrinter->Push(msg);

    }

    size_t Parser::Read( char* buff, size_t max_size )
    {
        size_t nCopy=0;
        while( m_pText[nCopy] && nCopy < max_size )
        {
            buff[nCopy] = m_pText[nCopy];
            ++nCopy;
        }
        m_pText += nCopy;
        return nCopy;
    }

    const char* Parser::StoreString( const char* yytext )
    {
        m_TokenStrings.push_back( yytext );
        return m_TokenStrings.back().c_str();
    }

    void Parser::RegDeclaration( TokenStruct& name, size_t nCount )
    {
        if( FindNamedReg( name.fields.ID ) )
        {
            Error(name.LineNumber, "Reg name already in use");
            return;
        }

        GEN::DirectRegReference reg(0);
        AddNamedReg( name.fields.ID, reg, nCount );
    }
   
    void Parser::BindDeclaration( TokenStruct& name, int point )
    {
        if( FindBindPoint( name.fields.ID ) )
        {
            Error(name.LineNumber, "Bind point name already in use");
            return;
        }

        BindPoint b;
        b.pName = name.fields.ID;
        b.bind = point;
        m_BindPoints.push_back(b);


    }

    void Parser::CURBEBegin( TokenStruct& name, size_t nSizeInRegs )
    {
        if( m_bError )
            return;

        // check for name conflict
        if( FindNamedReg( name.fields.ID ) )
        {
            Error( name.LineNumber, "Reg name is already in use");
            return;
        }

        // register this curbe's name and reserve regs for it
        AddNamedReg(name.fields.ID, GEN::DirectRegReference(m_nCURBERegCount+1), nSizeInRegs );
        m_nCURBERegCount += nSizeInRegs;

        memset( m_CURBEScratch, 0, sizeof(m_CURBEScratch) );
        m_nCURBEScratchOffs = 0;
    }

    void Parser::CURBEPush( size_t nLine, const void* pBytes, size_t nBytes )
    {
        if( m_bError )
            return;

        if( m_nCURBEScratchOffs + nBytes > sizeof(m_CURBEScratch) )
        {
            Error( nLine, "Too many initializers in curbe reg.  Contents of {} cannot exceed 32 bytes");
            return;
        }

        memcpy( &m_CURBEScratch[m_nCURBEScratchOffs], pBytes, nBytes );
        m_nCURBEScratchOffs += nBytes;
    }

    void Parser::CURBEBreak(size_t nLine)
    {
        // pad curbe register with zeros
        while( m_nCURBEScratchOffs < sizeof(m_CURBEScratch) )
            m_CURBEScratch[m_nCURBEScratchOffs++] = 0;

        // push new register onto CURBE
        for( size_t i=0; i<sizeof(m_CURBEScratch); i++ )
            m_CURBE.push_back( m_CURBEScratch[i] );

        // make noise if we went over
        if( m_CURBE.size() > m_nCURBERegCount*32 )
            Error(nLine, "Too many registers in CURBE declaration");

        m_nCURBEScratchOffs=0;
    }

    void Parser::CURBEEnd()
    {
        // pad out in case any curbe regs lacked initializers
        while( m_CURBE.size() < 32*m_nCURBERegCount )
            m_CURBE.push_back(0); 
    }

    bool Parser::Label( const TokenStruct& label )
    {
        if( FindLabel( label.fields.ID ) )
        {
            Error( label.LineNumber, "Duplicate label");
            return false;
        }

        LabelInfo lbl;
        lbl.nInstructionIndex = m_Instructions.size();
        lbl.pName = label.fields.ID;
        m_Labels.push_back(lbl);

        return true;
    }

    bool Parser::ThreadCount( const TokenStruct& count )
    {
        if( count.fields.Int < 0 || count.fields.Int > 64 )
        {
            Error(count.LineNumber, "Bad thread count.  Must be 1-64");
            return false;
        }

        m_nThreadsPerGroup = count.fields.Int;
        return true; // TODO: Check
    }

    ParseNode* Parser::SubReg( TokenStruct& rToken )
    {
        // "Subreg" is a string telling us the data type and an optional sub-register offset
        //  examples:   '.u'   '.u2'   '.sb4'  '.f'   '.f2'
        //         
        GEN::DataTypes eType;
        const char* pSuffix = rToken.fields.ID;
        switch(pSuffix[0] )
        {
        case 'u':
            {
                eType = GEN::DT_U32;
                pSuffix++;

                switch(pSuffix[0] )
                {
                case 's': eType = GEN::DT_U16; pSuffix++; break;
                case 'b': eType = GEN::DT_U8; pSuffix++; break;
                }
            }
            break;

        case 'd':
            eType = GEN::DT_S32;
            pSuffix++;
            break;

        case 's':
            {
                eType = GEN::DT_S32;
                pSuffix++;

                switch(pSuffix[0] )
                {
                case 's': eType = GEN::DT_S16; pSuffix++; break;
                case 'b': eType = GEN::DT_S8; pSuffix++; break;
                }
            }
            break;

        case 'f':
            {
                eType = GEN::DT_F32;
                pSuffix++;
                if( pSuffix[0] == 'd' )
                {
                    eType = GEN::DT_F64;
                    pSuffix++;
                }
            }
            break;
        default:
            Error( rToken.LineNumber, "Bad type specifier" );
            return 0;
        }

        size_t nSubreg = 0;
        if( *pSuffix != 0 )
        {
            // parse an unsigned integer sub-reg offset
            do
            {
                if( !isdigit(*pSuffix) )
                {
                    Error( rToken.LineNumber, "Malformed sub-register" );
                    return 0;
                }

                nSubreg = 10*nSubreg + ((*pSuffix)-'0');
                ++pSuffix;
            } while( *pSuffix );

            // make sure it's in range
            switch( eType )
            {
            case GEN::DT_U32:  nSubreg *= 4; break;
            case GEN::DT_S32:  nSubreg *= 4; break;
            case GEN::DT_U16:  nSubreg *= 2; break;
            case GEN::DT_S16:  nSubreg *= 2; break;
            case GEN::DT_U8:   nSubreg *= 1; break;
            case GEN::DT_S8:   nSubreg *= 1; break;
            case GEN::DT_F64:  nSubreg *= 8; break;
            case GEN::DT_F32:  nSubreg *= 4; break;
            }
            if( nSubreg >= 32 )
            {
                Error( rToken.LineNumber, "Sub-reg number out of range" );
                return 0;
            }
        }
       
        
        SubRegNode* pN = new SubRegNode(rToken.LineNumber);
        m_Nodes.push_back(pN);
        pN->eType = eType;
        pN->nSubreg = nSubreg;
        return pN;
    }


    static const TokenID ARCH_REGS[] = {
        {"null" ,  REG_NULL         },      
        {"a0"   ,  REG_ADDRESS             },
        {"acc0" ,  REG_ACCUM0            },
        {"acc1" ,  REG_ACCUM1            },
        {"f0"   ,  REG_FLAG0               },
        {"f1"   ,  REG_FLAG1               },
        {"ce"   ,  REG_CHANNEL_ENABLE      },
        {"sp"   ,  REG_STACK_PTR           },
        {"sr0"  ,  REG_STATE              },
        {"cr0"  ,  REG_CONTROL            },
        {"n0"   ,  REG_NOTIFICATION0       },
        {"n1"   ,  REG_NOTIFICATION1       },
        {"ip"   ,  REG_INSTRUCTION_PTR     },
        {"tdr"  ,  REG_THREAD_DEPENDENCY  },
        {"tm0"  ,  REG_TIMESTAMP          },
        {"fc0"  ,  REG_FC0                },
        {"fc1"  ,  REG_FC1                },
        {"fc2"  ,  REG_FC2                },
        {"fc3"  ,  REG_FC3                },
        {"fc4"  ,  REG_FC4                },
        {"fc5"  ,  REG_FC5                },
        {"fc6"  ,  REG_FC6                },
        {"fc7"  ,  REG_FC7                },
        {"fc8"  ,  REG_FC8                },
        {"fc9"  ,  REG_FC9                },
        {"fc10" ,  REG_FC10              },
        {"fc11" ,  REG_FC11              },
        {"fc12" ,  REG_FC12              },
        {"fc13" ,  REG_FC13              },
        {"fc14" ,  REG_FC14              },
        {"fc15" ,  REG_FC15              },
    };


    bool Parser::InterpretRegName( GEN::RegTypes* pRegType, size_t* pRegNum, const TokenStruct& rToken )
    {
        // check for a match with a user-defined named reg      
        const char* pID = rToken.fields.ID;
        size_t nIDLength = strlen(pID);
        for( size_t i=0; i<m_NamedRegs.size(); i++ )
        {
            const char* pReg = m_NamedRegs[i].pName;
            if( strlen(pReg) > nIDLength )
                continue; // no match

            // see if front part of token matches reg name exactly
            size_t j=0;
            while( pReg[j] )
            {
                if( pID[j] != pReg[j] )
                    break;
                j++;
            }
            if( pReg[j] )
                continue; // no match

            // if token is of the form <reg-label>%u
            //  then it's an offset from this reg label
            //  otherwise, it refers to something else
            size_t offset=0;
            if( pID[j] )
            {
                if( sscanf( pID+j, "%u", &offset ) == 0 )
                    continue; // no match

                // now make sure user didn't screw up
                if( offset >= m_NamedRegs[i].nRegArraySize )
                {
                    Error(rToken.LineNumber, "Overrun of named register block");
                    return false;
                }
            }

            *pRegType = m_NamedRegs[i].reg.GetRegType();
            *pRegNum = m_NamedRegs[i].reg.GetRegNumber() + offset;
            return true;
        }



        // parse a GPR name
        const char* pName = rToken.fields.ID;
        if( pName[0] == 'r' )
        {
            size_t num = atoi( pName+1 );
            if( num > 127 )
            {
                Error(rToken.LineNumber, "There are only 128 registers" );
                return false;
            }
            if( num == 127 )
            {
                Error(rToken.LineNumber, "r127 is reserved" ); // r127 is reserved
                return false;
            }
            *pRegType = REG_GPR;
            *pRegNum = num;
            return true;
        }

        // check for an arch register
        const TokenID* pArchID = Lookup( ARCH_REGS, pName );
        if( pArchID )
        {
            *pRegType = (RegTypes) pArchID->ID;
            *pRegNum = 0;
            return true;
        }
       
        ErrorF( rToken.LineNumber, "Unknown register %s", pName );
        return false;
    }
    
    ParseNode* Parser::DirectRegReference( TokenStruct& rToken )
    {
        GEN::RegTypes eRegType;
        size_t nRegNum;
        if( !InterpretRegName( &eRegType, &nRegNum, rToken ) )
            return 0;

        DirectRegRefNode* pN = new DirectRegRefNode(rToken.LineNumber);
        m_Nodes.push_back(pN);
        pN->eRegType  = eRegType;
        pN->nRegNum   = nRegNum;
        pN->bIsDirect = true;
        return pN;

    }
    
    ParseNode* Parser::IndirectRegReference( TokenStruct& rGPR, TokenStruct& rAddr, int addrsub )
    {
        GEN::RegTypes eRegType;
        size_t nRegNum;
        if( !InterpretRegName( &eRegType, &nRegNum, rGPR ) )
            return 0;

        GEN::RegTypes eAddrType;
        size_t nAddrNum;
        if( !InterpretRegName( &eAddrType, &nAddrNum, rAddr ) )
            return 0;

        if( eAddrType != GEN::REG_ADDRESS || eRegType != GEN::REG_GPR || nAddrNum != 0 )
        {
            Error( rGPR.LineNumber, "Indexing only allowed on GPRs, using a0 as index");
            return 0;
        }

        IndirectRegRefNode* pN = new IndirectRegRefNode(rGPR.LineNumber);
        m_Nodes.push_back(pN);
        pN->nAddrSubReg = addrsub;
        pN->nGPR = nRegNum;
        pN->bIsDirect = false;
        return pN;
    }

    ParseNode* Parser::RegRegion( size_t line, int vstride, int width, int hstride )
    {
        // TODO: check all these

        RegionNode* pN = new RegionNode(line);
        pN->width      = width;
        pN->hstride    = hstride;
        pN->vstride    = vstride;
        pN->LineNumber = line;
        return pN;
    }
    
    ParseNode* Parser::Swizzle( TokenStruct& tok )
    {
        SwizzleNode* pN = new SwizzleNode(tok.LineNumber);
        m_Nodes.push_back(pN);

        const char* pStr = tok.fields.ID;
        uint8 pIDs[4] = {0,1,2,3};
        
        size_t i=0;
        while( pStr[i] )
        {
            if( i>= 4 )
            {
                ErrorF(tok.LineNumber, "%s is not a legal swizzle string");
                return 0;
            }

            switch( pStr[i] )
            {
            case 'x':
            case 'X':
                pIDs[i] = 0;
                break;
            case 'y':
            case 'Y':
                pIDs[i] = 1;
                break;
            case 'z':
            case 'Z':
                pIDs[i] = 2;
                break;
            case 'w':
            case 'W':
                pIDs[i] = 3;
                break;
            default:   
                ErrorF(tok.LineNumber, "%s is not a legal swizzle string");
                return 0;
            }
            i++;
        }

        pN->swizz.x = pIDs[0];
        pN->swizz.y = pIDs[1];
        pN->swizz.z = pIDs[2];
        pN->swizz.w = pIDs[3];
        return pN;
    }

    ParseNode* Parser::SourceReg( ParseNode* pReg, ParseNode* pRegionOrSwizzle, ParseNode* pSubReg )
    {
        if( !pReg || !pSubReg )
            return 0; // bail out in the event of a parse error
        
        SubRegNode* pSub = static_cast<SubRegNode*>( pSubReg );
        GEN::DataTypes eType = pSub->eType;
       
        RegRefNode* pRegRef = static_cast<RegRefNode*>(pReg);
        GEN::RegReference base;
        if( pRegRef->bIsDirect )
        {
            DirectRegRefNode* pDir = static_cast<DirectRegRefNode*>( pRegRef);
            base = GEN::DirectRegReference( pDir->eRegType, pDir->nRegNum, pSub->nSubreg );
        }
        else
        {
            IndirectRegRefNode* pInDir = static_cast<IndirectRegRefNode*>( pRegRef );
            base = GEN::IndirectRegReference( pInDir->nGPR*32, pInDir->nAddrSubReg );
        }
       
        
        SourceRegNode* pS = new SourceRegNode(pReg->LineNumber);
        m_Nodes.push_back(pS);

        pS->base = base;
        pS->eType = eType;
        pS->pRegionOrSwizzle = static_cast<SourceModifierNode*>(pRegionOrSwizzle);;
        
        return pS;
    }
      
    ParseNode* Parser::DestReg( ParseNode* pReg, int hstride, ParseNode* pSubReg )
    {
        if( !pReg || !pSubReg )
            return 0; // bail out in the event of a parse error

        SubRegNode* pSub = static_cast<SubRegNode*>( pSubReg );
        GEN::DataTypes eType = pSub->eType;

        RegRefNode* pRegRef = static_cast<RegRefNode*>(pReg);
        GEN::RegReference base;
        if( pRegRef->bIsDirect )
        {
            DirectRegRefNode* pDir = static_cast<DirectRegRefNode*>( pRegRef);
            base = GEN::DirectRegReference( pDir->eRegType, pDir->nRegNum, pSub->nSubreg );
        }
        else
        {
            IndirectRegRefNode* pInDir = static_cast<IndirectRegRefNode*>( pRegRef );
            base = GEN::IndirectRegReference( pInDir->nGPR*32, pInDir->nAddrSubReg );
        }
        
        GEN::RegisterRegion region( base, 8,hstride,1); 
        GEN::DestOperand operand( eType, region );

        DestRegNode* pD = new DestRegNode(pReg->LineNumber);
        m_Nodes.push_back(pD);
        pD->dest = operand;

        return pD;
    }

    

    ParseNode* Parser::Operation( TokenStruct& rToken, int nExecSize, ParseNode* pFlagRef )
    {
        switch( nExecSize )
        {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
            break;
        default:
            Error( rToken.LineNumber, "Bad exec size");
            return 0;
        }

      

        OperationNode* pN = new OperationNode(rToken.LineNumber);
        m_Nodes.push_back(pN);
        pN->nExecSize = nExecSize;
        pN->pName = rToken.fields.ID;
        pN->pFlagRef = static_cast<FlagReferenceNode*>(pFlagRef);
        return pN;
    }

    ParseNode* Parser::IntLiteral( size_t line, int i )
    {
        ImmediateNode* pS = new ImmediateNode(line);
        m_Nodes.push_back(pS);
        pS->imm = GEN::SourceOperand( GEN::DT_S32, i );
        return pS;
    }
    ParseNode* Parser::FloatLiteral( size_t line, float f )
    {
        ImmediateNode* pS = new ImmediateNode(line);
        m_Nodes.push_back(pS);       
        pS->imm = GEN::SourceOperand(f);
        return pS;
    }
    ParseNode* Parser::FlagReference( TokenStruct& id, TokenStruct& subReg )
    {
        GEN::RegTypes eRegType;
        size_t nNum;
        if( !InterpretRegName( &eRegType, &nNum, id ) )
            return 0;

        if( (eRegType != REG_FLAG0 && eRegType != REG_FLAG1) || 
            (subReg.fields.Int != 0 && subReg.fields.Int != 1 ) )
        {
            Error(id.LineNumber, "Invalid flag reference" );
            return 0;
        }

        FlagReferenceNode* pF = new FlagReferenceNode(id.LineNumber);
        m_Nodes.push_back(pF);
        pF->Flag.Set( eRegType - REG_FLAG0, subReg.fields.Int );
        return pF;
    }

       
    static DestOperand DestFromNode( ParseNode* pDst )
    {
        DestRegNode* pDestNode = static_cast<DestRegNode*>(pDst);
        return pDestNode->dest;
    }

    static SourceOperand SourceFromNode( ParseNode* pDst, size_t nExecSize )
    {
        SourceNode* pSrcNode = static_cast<SourceNode*>(pDst);
        if( pSrcNode->IsImmediate() )
            return static_cast<ImmediateNode*>(pSrcNode)->imm;
        
        SourceRegNode* pSrcReg = static_cast<SourceRegNode*>(pSrcNode);
        
        // default region description
        RegisterRegion reg;
        switch(nExecSize)
        {
        case 1:
            reg = RegisterRegion( pSrcReg->base, 0,1,0 );
            break;
        default:
            reg = RegisterRegion( pSrcReg->base, nExecSize,nExecSize,1 );
            break;
        }

        // default swizzle
        Swizzle swizz;


        if( pSrcReg->pRegionOrSwizzle )
        {
            if( pSrcReg->pRegionOrSwizzle->IsSwizzle() )
            {
                SwizzleNode* pN = static_cast<SwizzleNode*>(pSrcReg->pRegionOrSwizzle);
                swizz = pN->swizz;
            }
            else
            {
                // TODO: Validate region description against exec size
                RegionNode* pN = static_cast<RegionNode*>(pSrcReg->pRegionOrSwizzle);
                reg = RegisterRegion( pSrcReg->base, pN->vstride, pN->width, pN->hstride );
            }
        }
        
        return SourceOperand( pSrcReg->eType, reg, swizz );
    }


 

    static const TokenID UNARY_MATH[] ={
        {"rcp"  , MATH_INVERSE},
        {"log"  , MATH_LOG }   ,
        {"exp"  , MATH_EXP }   ,
        {"sqrt" , MATH_SQRT }  ,
        {"rsq"  , MATH_RSQ}    ,
        {"sin"  , MATH_SIN}    ,
        {"cos"  , MATH_COS}    ,
        {0,0}                  ,
    };

    static const TokenID UNARY_ARITH[] ={
        {"mov"    , OP_MOV      },
        {"movi"   , OP_MOVI     },
        {"not"    , OP_NOT      },
        {"f32to16", OP_F32TO16  },
        {"f16to32", OP_F16TO32  },
        {"bfrev"  , OP_BFREV    },
        {"frc"    , OP_FRC      },
        {"rndu"   , OP_RNDU     },
        {"rndd"   , OP_RNDD     },
        {"rnde"   , OP_RNDE     },
        {"rndz"   , OP_RNDZ     },
        {"lzd"    , OP_LZD      },
        {"fbh"    , OP_FBH      },
        {"fbl"    , OP_FBL      },
        {"cbit"   , OP_CBIT     },
        {"dim"    , OP_DIM      },
        {0,0}                  ,
    };

    void Parser::Unary( ParseNode* pOp, ParseNode* pDst, ParseNode* pSrc )
    {
        if( !pOp || !pDst || !pSrc )
            return; // pass errors through

        OperationNode* pOperation = static_cast<OperationNode*>(pOp);

        const TokenID* pID = Lookup( UNARY_MATH, pOperation->pName );
        if( pID )
        {
            m_Instructions.push_back( 
                GEN::MathInstruction( pOperation->nExecSize, (MathFunctionIDs)pID->ID, DestFromNode(pDst), SourceFromNode(pSrc,pOperation->nExecSize) )
                );
            return;
        }
       
        pID = Lookup( UNARY_ARITH, pOperation->pName );
        if( pID )
        {
            m_Instructions.push_back( 
                GEN::UnaryInstruction( pOperation->nExecSize, (Operations)pID->ID, DestFromNode(pDst), SourceFromNode(pSrc,pOperation->nExecSize) )
                );
            return;
        }

        ErrorF( pOperation->LineNumber, "Unrecognized instruction: '%s'", pOperation->pName );
    }



    static const TokenID BINARY_MATH[] = {
        { "pow"    , MATH_POW },
        { "idivmod", MATH_IDIV_BOTH },
        { "idiv"   , MATH_IDIV_QUOTIENT},
        { "imod"   , MATH_IDIV_REMAINDER },
        { "fdiv"   , MATH_FDIV },
        { 0, 0 }
    };

    static const TokenID BINARY_ARITH[] = {
        {"and"   , OP_AND },
        {"or"    , OP_OR },
        {"xor"   , OP_XOR },
        {"shr"   , OP_SHR },
        {"shl"   , OP_SHL },
        {"asr"   , OP_ASR },
        {"cmp"   , OP_CMP },
        {"cmpn"  , OP_CMPN },
        {"bfi1"  , OP_BFI1 },
        {"bfi2"  , OP_BFI2 },
        {"add"   , OP_ADD },
        {"mul"   , OP_MUL },
        {"avg"   , OP_AVG },
        {"addc"  , OP_ADDC },
        {"subb"  , OP_SUBB },
        {"sad2"  , OP_SAD2 },
        {"sada2" , OP_SADA2 },
        {"dp4"   , OP_DP4 },
        {"dph"   , OP_DPH },
        {"dp3"   , OP_DP3 },
        {"dp2"   , OP_DP2 },
        {"line"  , OP_LINE },
        {"pln"   , OP_PLN },
        {0,0}
    };


    
    static const TokenID BINARY_COMPARE[] = {
        { "cmpeq", CM_ZERO },
        { "cmpne", CM_NOTZERO },
        { "cmplt", CM_LESS_THAN },
        { "cmple", CM_LESS_EQUAL },
        { "cmpgt", CM_GREATER_THAN },
        { "cmpge", CM_GREATER_EQUAL },
        { 0, 0 }
    };


    void Parser::Binary( ParseNode* pOp, ParseNode* pDst, ParseNode* pSrc0, ParseNode* pSrc1 )
    {
        if( !pOp || !pDst || !pSrc0 || !pSrc1 )
            return; // pass errors through

        OperationNode* pOperation = static_cast<OperationNode*>(pOp);

        const TokenID* pID = Lookup( BINARY_MATH, pOperation->pName );
        if( pID )
        {
            m_Instructions.push_back( 
                GEN::MathInstruction( pOperation->nExecSize, (MathFunctionIDs)pID->ID, 
                    DestFromNode(pDst), 
                    SourceFromNode(pSrc0,pOperation->nExecSize),
                    SourceFromNode(pSrc1,pOperation->nExecSize))
                );
            return;
        }
       
        pID = Lookup( BINARY_ARITH, pOperation->pName );
        if( pID )
        {
            m_Instructions.push_back( 
                GEN::BinaryInstruction( pOperation->nExecSize, (Operations)pID->ID, 
                    DestFromNode(pDst), 
                    SourceFromNode(pSrc0,pOperation->nExecSize),
                    SourceFromNode(pSrc1,pOperation->nExecSize))
                );
            return;
        }

        pID = Lookup( BINARY_COMPARE, pOperation->pName );
        if( pID )
        {
            FlagReferenceNode* pFlag = pOperation->pFlagRef;
            if( !pFlag )
            {
                Error(pOperation->LineNumber, "Compare instructions must have a flag reference");
                return;
            }

            auto op =  GEN::BinaryInstruction( pOperation->nExecSize, OP_CMP,
                    DestFromNode(pDst),
                    SourceFromNode(pSrc0,pOperation->nExecSize),
                    SourceFromNode(pSrc1,pOperation->nExecSize) );
               
            op.SetFlagReference( pFlag->Flag );
            op.SetConditionalModifier( (ConditionalModifiers)pID->ID );
            
            m_Instructions.push_back( op );
            return;
        }

        // turn 'sub' into add with negated source
        if( strcmp( pOperation->pName, "sub" ) == 0 )
        {
            SourceOperand src1 = SourceFromNode(pSrc1,pOperation->nExecSize);
           
            src1.SetModifier(SM_NEGATE);
             GEN::BinaryInstruction inst = 
                GEN::BinaryInstruction( pOperation->nExecSize, OP_ADD,
                    DestFromNode(pDst), 
                    SourceFromNode(pSrc0,pOperation->nExecSize),
                    src1
                );
            m_Instructions.push_back( inst );
            return;
        }

         // turn binary 'fma' into ternary with dst = src
        if( strcmp( pOperation->pName, "fma" ) == 0 )
        {
            SourceOperand src1 = SourceFromNode(pSrc0,pOperation->nExecSize);
            SourceOperand src2 = SourceFromNode(pSrc1,pOperation->nExecSize);
            DestOperand dst = DestFromNode(pDst);

            SourceOperand src0 = SourceOperand(dst.GetDataType(), 
                                               RegisterRegion(dst.GetRegRegion().GetBaseRegister(),
                                                              pOperation->nExecSize,pOperation->nExecSize,1));
            
            
            GEN::TernaryInstruction inst = 
                GEN::TernaryInstruction( pOperation->nExecSize, OP_FMA,
                     dst,src0,src1,src2
                  
                );

            m_Instructions.push_back( inst );
            return;
        }

        // turn 'min' and 'max' into 'sel' with the right conditional modifier
        if( strcmp( pOperation->pName, "min" ) == 0 )
        {
            GEN::BinaryInstruction inst = 
                GEN::BinaryInstruction( pOperation->nExecSize, OP_SEL,
                    DestFromNode(pDst), 
                    SourceFromNode(pSrc0,pOperation->nExecSize),
                    SourceFromNode(pSrc1,pOperation->nExecSize)
                );

            inst.SetConditionalModifier( CM_LESS_EQUAL );
            m_Instructions.push_back( inst );
            return;
        }
        if( strcmp( pOperation->pName, "max" ) == 0 )
        {
            GEN::BinaryInstruction inst = 
                GEN::BinaryInstruction( pOperation->nExecSize, OP_SEL,
                    DestFromNode(pDst), 
                    SourceFromNode(pSrc0,pOperation->nExecSize),
                    SourceFromNode(pSrc1,pOperation->nExecSize)
                );

            inst.SetConditionalModifier( CM_GREATER_EQUAL );
            m_Instructions.push_back( inst );
            return;
        }

        
        ErrorF( pOperation->LineNumber, "Unrecognized instruction: '%s'", pOperation->pName );
    }

    void Parser::Ternary( ParseNode* pOp, ParseNode* pDst, ParseNode* pSrc0, ParseNode* pSrc1, ParseNode* pSrc2  )
    {
        if( !pOp || !pDst || !pSrc0 || !pSrc1 || !pSrc2 )
            return; // pass errors through
        
        OperationNode* pOperation = static_cast<OperationNode*>(pOp);

        
        ErrorF( pOperation->LineNumber, "Unrecognized instruction: '%s'", pOperation->pName );

        // TODO
        /*
           // ternaries
     return "bfe"    ;
     return "fma"    ;
     return "lrp"    ;
     "csel"
     */
    }

    void Parser::Send( TokenStruct& msg, TokenStruct& bind, ParseNode* pDst0, ParseNode* pDst1 )
    {
        if( !pDst0 || !pDst1 )
            return; // pass errors through

        BindPoint* pBind = FindBindPoint( bind.fields.ID );
        if( !pBind )
        {
            Error( bind.LineNumber, "Bind point not found");
            return;
        }

        GEN::RegReference Dst0Reg = static_cast<DestRegNode*>(pDst0)->dest.GetRegRegion().GetBaseRegister();
        GEN::RegReference Dst1Reg = static_cast<DestRegNode*>(pDst1)->dest.GetRegRegion().GetBaseRegister();
        
        if( strcmp( msg.fields.ID, "DwordLoad8" ) == 0 )
        {
            m_Instructions.push_back( GEN::DWordScatteredRead_SIMD8( pBind->bind, Dst1Reg, Dst0Reg ) );
        }
        else if( strcmp( msg.fields.ID, "DwordLoad16" ) == 0 )
        {
            m_Instructions.push_back( GEN::DWordScatteredRead_SIMD16( pBind->bind, Dst1Reg, Dst0Reg ) );
        }
        else if( strcmp( msg.fields.ID, "DwordStore8" ) == 0 )
        {
            m_Instructions.push_back( GEN::DWordScatteredWrite_SIMD8( pBind->bind, Dst1Reg, Dst0Reg ) );
        }
        else if( strcmp( msg.fields.ID, "DwordStore16" ) == 0 )
        {
            m_Instructions.push_back( GEN::DWordScatteredWrite_SIMD16( pBind->bind, Dst1Reg, Dst0Reg ) );
        }
        else if( strcmp( msg.fields.ID, "UntypedWrite16x2" ) == 0 )
        {
            m_Instructions.push_back( GEN::UntypedWrite_SIMD16x2( pBind->bind, Dst1Reg, Dst0Reg ) );
        }
        else if( strcmp( msg.fields.ID, "OWordBlockWrite" ) == 0 )
        {
            m_Instructions.push_back( GEN::OWordBlockWrite( pBind->bind, Dst1Reg ) );
        }
        else
        {
            Error(msg.LineNumber, "Unknown message");
        }
    }
        
    void Parser::Jmp( TokenStruct& label )
    {
        Jump jmp;
        jmp.nJumpIndex = m_Instructions.size();
        jmp.nLine = label.LineNumber;
        jmp.pFlagRef = 0;
        jmp.pLabelName = label.fields.ID;
        m_Jumps.push_back(jmp);
        m_Instructions.push_back( Instruction() ); // reserve a spot
    }

    void Parser::JmpIf( TokenStruct& label, ParseNode* pFlagRef, bool bInvert )
    {
        Jump jmp;
        jmp.nJumpIndex = m_Instructions.size();
        jmp.nLine = label.LineNumber;
        jmp.pFlagRef = pFlagRef;
        jmp.pLabelName = label.fields.ID;
        jmp.bInvertPredicate = bInvert;
        m_Jumps.push_back(jmp);
        m_Instructions.push_back( Instruction() ); // reserve a spot
    }
    
    void Parser::BeginPredBlock( ParseNode* pFlagRef )
    {
        m_nPredStart= m_Instructions.size();
        m_pPred = pFlagRef;
    }

    void Parser::EndPredBlock()
    {
        FlagReferenceNode* pFlag = static_cast<FlagReferenceNode*>( m_pPred );

        Predicate pred;
        pred.Set( GEN::PM_SEQUENTIAL_FLAG, false );

        while( m_nPredStart < m_Instructions.size() )
        {
            m_Instructions[m_nPredStart].SetPredicate(pred);
            m_Instructions[m_nPredStart].SetFlagReference(pFlag->Flag);
            m_nPredStart++;
        }
    }

    bool Parser::Begin( size_t line )
    {
        // after the pre-amble is done, allocate registers for all 'reg' declarations
        //   We allow mixing of 'reg' and 'curbe' in the pre-amble, so we need to defer
        //  the assignment of 'reg' regs until all the curbes are known
        size_t nRegNum = m_nCURBERegCount+1;
        for( size_t i=0; i<m_NamedRegs.size(); i++ )
        {
            if( m_NamedRegs[i].reg.GetRegNumber() == 0 )
            {
                if( nRegNum >= 128 )
                {
                    Error(line, "Too many reg declarations");
                    return false;
                }

                m_NamedRegs[i].reg.SetRegNumber(nRegNum);
                nRegNum += m_NamedRegs[i].nRegArraySize;
            }
        }

        // Start every program by saving off the r0 header 
        m_Instructions.push_back( GEN::RegMove(GEN::REG_GPR,127,GEN::REG_GPR,0) );
        return true;
    }

    void Parser::End()
    {
        // now that we have all the labels, build the jumps
        for( auto& it : m_Jumps )
        {
            LabelInfo* pLbl = FindLabel( it.pLabelName );
            if( !pLbl )
            {
                Error( it.nLine, "Missing label");
                return;
            }

            int nJumpLoc  = (int) it.nJumpIndex;
            int nLabelLoc = (int) pLbl->nInstructionIndex;
            
            Instruction& rInst = m_Instructions[it.nJumpIndex];

            int nOffset = (nLabelLoc - nJumpLoc)*16;

            rInst = GEN::BinaryInstruction( 1, GEN::OP_ADD, 
                                            GEN::DestOperand( GEN::DT_S32, 
                                               GEN::RegisterRegion( GEN::DirectRegReference( REG_INSTRUCTION_PTR, 0 ),1,1,1 )),
                                            GEN::SourceOperand(GEN::DT_S32,
                                               GEN::RegisterRegion( GEN::DirectRegReference( REG_INSTRUCTION_PTR, 0 ),1,1,1 )),
                                            GEN::SourceOperand(GEN::DT_S32, (uint32)(nOffset) ) );

            if( it.pFlagRef )
            {
                FlagReferenceNode* pFlagRef = static_cast<FlagReferenceNode*>(it.pFlagRef);
                rInst.SetFlagReference( pFlagRef->Flag );
                
                Predicate pred;
                pred.Set( PM_ANY16H, it.bInvertPredicate );
              
                rInst.SetPredicate(pred);
            }
        }

        // End every program by sending the EOT message to thread spawner
        //    If our assembler is ever re-purposed for other shader types, this might need to change
        //  We assume that R0 header has been moved into r127
        //    Note that docs specify that upper regs should be used to send EOT message.
        //     because new threads can spawn concurrently with the message processing
        m_Instructions.push_back( GEN::SendEOT(127) );
    }

    Parser::LabelInfo* Parser::FindLabel( const char* pLabel )
    {
        for( auto& it : m_Labels )
        {
            if( strcmp( it.pName, pLabel ) == 0 )
                return &it;
        }
        return 0;
    }

    Parser::NamedReg* Parser::FindNamedReg( const char* pLabel )
    {
        for( auto& it : m_NamedRegs )
        {
            if( strcmp( it.pName, pLabel ) == 0 )
                return &it;
        }
        return 0;
    }
    
    Parser::BindPoint* Parser::FindBindPoint( const char* pName )
    {
        for( auto& it : m_BindPoints )
        {
            if( strcmp( it.pName, pName ) == 0 )
                return &it;
        }
        return 0;
    }

    void Parser::AddNamedReg( const char* pLabel, GEN::DirectRegReference reg, size_t nArraySize )
    {
        m_NamedRegs.push_back(NamedReg(pLabel,reg, nArraySize));
    }

    void Parser::VecIMMPush( ParseNode* pN )
    {
        if( m_nVecIMMNodes >= 8 )
        {
            Error(pN->LineNumber, "Too many initializers in vector immediate");
            return;
        }
        m_pVecIMMNodes[m_nVecIMMNodes++] = pN;
    }

    ParseNode* Parser::EndVectorImmediate()
    {
        if( m_bError )
            return 0;


        switch( m_eVecImmType )
        {
        default:
            Error(m_nVecIMMLine, "INTERNAL ERROR");
            return 0;

        case GEN::DT_VEC_HALFBYTE_FLOAT:
            {
                Error(m_nVecIMMLine, "Vec-float imm not implemented");
                return 0;
            }
            break;

        case GEN::DT_VEC_HALFBYTE_SINT:
        case GEN::DT_VEC_HALFBYTE_UINT:
            {
                int pValues[8];
                for( size_t i=0; i<m_nVecIMMNodes; i++ )
                {
                    ImmediateNode* pIMM = static_cast<ImmediateNode*>(m_pVecIMMNodes[i]);
                    const uint8* pImmediateBits = pIMM->imm.GetImmediateBits();
                    switch( pIMM->imm.GetDataType() )
                    {
                    case DT_F32:    pValues[i] = *((float*)pImmediateBits); break;
                    case DT_F64:    pValues[i] = *((double*)pImmediateBits); break;
                    case DT_S32:
                    case DT_U32:
                        pValues[i] = *((int*)pImmediateBits); 
                        break;
                    }
                }
                for( size_t i=m_nVecIMMNodes; i<8; i++ )
                    pValues[i] = 0;

                // check ranges
                if( m_eVecImmType == GEN::DT_VEC_HALFBYTE_SINT )
                {
                    for( size_t i=0; i<m_nVecIMMNodes; i++ )
                    {
                        if( pValues[i] < -8 || pValues[i] > 7 )
                        {
                            ErrorF(m_nVecIMMLine,"%d is out of range for vector immediate", pValues[i] );
                            return 0;
                        }
                    }

                    
                    ImmediateNode* pImm = new ImmediateNode(m_nVecIMMLine);
                    pImm->imm = PackHalfByte_SINT(pValues);
                    m_Nodes.push_back(pImm);
                    return pImm;
                }
                else
                {
                    for( size_t i=0; i<m_nVecIMMNodes; i++ )
                    {
                        if( pValues[i] < 0 || pValues[i] > 15 )
                        {
                            ErrorF(m_nVecIMMLine,"%d is out of range for vector immediate", pValues[i] );
                            return 0;
                        }
                    }
                    
                    ImmediateNode* pImm = new ImmediateNode(m_nVecIMMLine);
                    pImm->imm = PackHalfByte_UINT((const uint32*)pValues);
                    m_Nodes.push_back(pImm);
                    return pImm;
                }
            }
            break;
        }
    }



    //=====================================================================================================================
    //=====================================================================================================================
    static size_t StripComments( char* p )
    {
        size_t nLine=1;
        int i=0;
        while( p[i] && p[i+1] )
        {
            // C++ comment.  Walk to next newline or EOF
            if( p[i] == '/' && p[i+1] == '/' )
            {
                while( p[i] &&  p[i] != '\n' )
                    p[i++] = ' ';
                        
                nLine++;
            }
            // C comment.  Walk to next */
            else if( p[i] == '/' && p[i+1] == '*' )
            {
                size_t nStartLine=nLine;
                while( p[i] != '*' || p[i+1] != '/' )
                {
                    if( !p[i] || !p[i+1] )
                        return nStartLine;// EOF in unterminated C comment

                    if( p[i] != '\n' )
                        p[i] = ' ';
                    else
                        nLine++;
                    i++;
                }

                p[i]   = ' ';
                p[i+1] = ' ';
                i += 2;
            }
            else
                i++; // skip this character           
        }
        return 0;
    }


    bool Parser::Parse( const char* pText, IPrinter* pErrorStream )
    {
        if( yylex_init_extra( this, &m_scanner ) != 0 )
        {
            m_bError = true;
            return false;
        }

        char* pMutableText = (char*)malloc( strlen(pText)+1 );
        strcpy(pMutableText,pText);
        size_t nUnterminatedCommentLine = StripComments(pMutableText);
        if( nUnterminatedCommentLine )
        {
            Error(nUnterminatedCommentLine, "Unterminated block comment");
            free(pMutableText);
            return false;
        }

        m_pText = pMutableText;
        m_pErrorPrinter = pErrorStream;
        m_bError = false;
        m_nThreadsPerGroup = 1;
        m_nCURBERegCount = 0;
       

        
        if( yyparse( this ) != 0) 
        {
            m_Instructions.clear();
            m_CURBE.clear();
            m_nThreadsPerGroup=0;
            m_bError = true;
        }

        yylex_destroy( m_scanner );
        free(pMutableText);
        return !m_bError;
    }

}}}


