
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENIsa.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace GEN
{
    namespace _INTERNAL
    {
        
      

        void Printf( IPrinter& printer, const char* Format, ... )
        {
            va_list vl;
            va_start(vl,Format);

            char scratch[2048];
            char* p = scratch;

            int n = _vscprintf( Format, vl );
            if( n+1 > sizeof(scratch) )
                p = (char*) malloc( n+1 );
            
            vsprintf(p,Format,vl);
            printer.Push(p);

            if( p != scratch )
                free(p);

            va_end(vl);
        }

        void catprintf( char* buff, const char* Format, ... )
        {
            va_list vl;
            va_start(vl,Format);
            
            vsprintf( buff+strlen(buff), Format, vl );
           
            va_end(vl);
        }

        #define OPCODE_FORMAT "%8s"
        

        static const char* GetRegPrefix( RegTypes eReg )
        {
            switch( eReg )
            {
            case REG_NULL:               return "null";     
            case REG_ADDRESS:            return "a0";
            case REG_ACCUM0:             return "acc0";
            case REG_ACCUM1:             return "acc1";
            case REG_FLAG0:              return "f0";
            case REG_FLAG1:              return "f1";
            case REG_CHANNEL_ENABLE:     return "ce";
            case REG_STACK_PTR:          return "sp";
            case REG_STATE:              return "sr0";
            case REG_CONTROL:            return "cr0";
            case REG_NOTIFICATION0:      return "n0";
            case REG_NOTIFICATION1:      return "n1";
            case REG_INSTRUCTION_PTR:    return "ip";
            case REG_THREAD_DEPENDENCY:  return "tdr";
            case REG_TIMESTAMP:          return "tm0";
            case REG_FC0:                return "fc0";
            case REG_FC1:                return "fc1";
            case REG_FC2:                return "fc2";
            case REG_FC3:                return "fc3";
            case REG_FC4:                return "fc4";
            case REG_FC5:                return "fc5";
            case REG_FC6:                return "fc6";
            case REG_FC7:                return "fc7";
            case REG_FC8:                return "fc8";
            case REG_FC9:                return "fc9";
            case REG_FC10:               return "fc10";
            case REG_FC11:               return "fc11";
            case REG_FC12:               return "fc12";
            case REG_FC13:               return "fc13";
            case REG_FC14:               return "fc14";
            case REG_FC15:               return "fc15";
            case REG_GPR:                return "r";
            default: return "???";
            }
        }

        static const char* GetSubRegPrefix(  DataTypes eType )
        {
            switch( eType )
            {
            case DT_U32:    return ".u";
            case DT_S32:    return ".s";
            case DT_U16:    return ".us";
            case DT_S16:    return ".ss";
            case DT_U8:     return ".ub";
            case DT_S8:     return ".sb";
            case DT_F64:    return ".fd";
            case DT_F32:    return ".f";
            default:        return ".??";
            }
        }

        static void FormatSubReg( char* pBuffer, DataTypes eType, size_t nByteOffset )
        {
            const char* pPrefix = GetSubRegPrefix(eType);

            switch( eType )
            {
            case DT_U32:    nByteOffset /= 4; break;
            case DT_S32:    nByteOffset /= 4; break;
            case DT_U16:    nByteOffset /= 2; break;
            case DT_S16:    nByteOffset /= 2; break;
            case DT_U8:     nByteOffset /= 1; break;
            case DT_S8:     nByteOffset /= 1; break;
            case DT_F64:    nByteOffset /= 8; break;
            case DT_F32:    nByteOffset /= 8; break;
            default:        nByteOffset = 0; break;
            }

            if( nByteOffset > 0 )
                sprintf( pBuffer, "%s%u", pPrefix, nByteOffset );
            else
                sprintf(pBuffer, "%s", pPrefix );

        }

        void FormatRegReference( char* pBuffer, DataTypes eType, const GEN::RegReference& rReg )
        {
            if( rReg.IsDirect() )
            {
                const GEN::DirectRegReference& rRegD = static_cast<const GEN::DirectRegReference&>(rReg);
                
                char pSubReg[16];
                FormatSubReg(pSubReg,eType, rRegD.GetSubRegOffset() );

                sprintf(pBuffer,"%s%u%s", GetRegPrefix(rRegD.GetRegType()),rRegD.GetRegNumber(), pSubReg );
            }
            else
            {
                const GEN::IndirectRegReference& rRegI = static_cast<const GEN::IndirectRegReference&>(rReg);
                const char* pPrefix = GetSubRegPrefix(eType);
            
                if( rRegI.GetImmediateOffset() < 0 )
                    sprintf(pBuffer,"r[a0.%u-%u]%s", rRegI.GetAddressSubReg(), abs(rRegI.GetImmediateOffset()), pPrefix );
                else if( rRegI.GetImmediateOffset() > 0 )
                    sprintf(pBuffer,"r[a0.%u+%u]%s", rRegI.GetAddressSubReg(), rRegI.GetImmediateOffset(), pPrefix );
                else
                    sprintf(pBuffer,"r[a0.%u]%s", rRegI.GetAddressSubReg(), rRegI.GetImmediateOffset(), pPrefix );
            }
        }

       

        void FormatRegRegion( char* pBuffer, DataTypes eType, const GEN::RegisterRegion& rRegion, size_t nExecSize )
        {
            FormatRegReference(pBuffer,eType,rRegion.GetBaseRegister());
            if( !nExecSize )
                return; // 0 exec size means never print a region description

            size_t h = rRegion.GetHStride();
            size_t v = rRegion.GetVStride();
            size_t w = rRegion.GetWidth();
           // if( (h==1) && (w == 8 && v == 8) )
           //     return; // don't print boring region descriptions
                
            catprintf(pBuffer,"<%u,%u,%u>", v,w,h);
        }

        void FormatDest( char* pBuffer, const GEN::DestOperand& rDest, size_t nExecSize )
        {
            // TODO write mask
            FormatRegRegion( pBuffer, rDest.GetDataType(), rDest.GetRegRegion(), nExecSize );
        }

        

        void FormatSource( char* pBuffer, const GEN::SourceOperand& rSrc, const GEN::Instruction& rInstruction )
        {
            // TODO: swizzles
            char tmp[64];

            if( rSrc.IsImmediate() )
            {
                switch( rSrc.GetDataType() )
                {
                case DT_U32:    sprintf(tmp, "%u", rInstruction.GetImmediate<uint32>()); break;
                case DT_S32:    sprintf(tmp, "%d", rInstruction.GetImmediate<int32>());  break;
                case DT_U16:    sprintf(tmp, "%u", rInstruction.GetImmediate<uint16>()); break;
                case DT_S16:    sprintf(tmp, "%d", rInstruction.GetImmediate<int16>());  break;
                case DT_U8:     sprintf(tmp, "%u", rInstruction.GetImmediate<uint8>());  break;
                case DT_S8:     sprintf(tmp, "%d", rInstruction.GetImmediate<int8>());   break;
                case DT_F64:    sprintf(tmp, "%f", rInstruction.GetImmediate<float>());  break;
                case DT_F32:    sprintf(tmp, "%f", rInstruction.GetImmediate<double>()); break;
                case DT_VEC_HALFBYTE_UINT:
                case DT_VEC_HALFBYTE_SINT:  // immediates only
                case DT_VEC_HALFBYTE_FLOAT:
                default:
                    sprintf(tmp, "IMM??" );
                }
            }
            else
            {
                size_t nExecSize = 0;
                switch( rInstruction.GetClass() )
                {
                case IC_BINARY:    nExecSize = static_cast<const BinaryInstruction&>(rInstruction).GetExecSize(); break;
                case IC_UNARY:     nExecSize = static_cast<const UnaryInstruction&>(rInstruction).GetExecSize(); break;
                case IC_TERNARY:   nExecSize = static_cast<const TernaryInstruction&>(rInstruction).GetExecSize(); break;
                case IC_MATH:      nExecSize = static_cast<const MathInstruction&>(rInstruction).GetExecSize(); break;
                }
               
                FormatRegRegion( tmp, rSrc.GetDataType(), rSrc.GetRegRegion(), nExecSize );
            }

            switch( rSrc.GetModifier() )
            {
            default:
            case SM_NONE:
                strcpy(pBuffer,tmp);
                break;
            case SM_ABS:
                sprintf( pBuffer, "|%s|", tmp );
                break;
            case SM_NEGATE:
                sprintf( pBuffer, "-%s", tmp );
                break;
            case SM_NEG_ABS:
                sprintf( pBuffer, "-|%s|", tmp );
                break;
            }
        }

        static void FormatFlagReference( char* pBuff, const FlagReference& r )
        {
            sprintf( pBuff, "f%u.%u", r.GetReg(), r.GetSubReg() );
        }
       
        static void FormatPredicate( char* pBuff, const Predicate& p, const FlagReference& flag )
        {
            if( p.GetMode() == PM_NONE )
            {
                *pBuff=0;
                return;
            }
           
            if( p.IsInverted() )
                *(pBuff++) = '~';
            
            FormatFlagReference( pBuff, flag );
            
            switch( p.GetMode() )
            {
            case PM_SEQUENTIAL_FLAG: strcat(pBuff,".seq"); break;
            case PM_SWIZZLE_X:       strcat(pBuff,".x"); break;
            case PM_SWIZZLE_Y:       strcat(pBuff,".y"); break;
            case PM_SWIZZLE_Z:       strcat(pBuff,".z"); break;
            case PM_SWIZZLE_W:       strcat(pBuff,".w"); break;
            case PM_ANY4H:           strcat(pBuff,".any4h"); break;
            case PM_ALL4H:           strcat(pBuff,".all4h"); break;
            case PM_ANYV     :       strcat(pBuff,".anyv"); break;
            case PM_ALLV     :       strcat(pBuff,".allv"); break;
            case PM_ANY2H    :       strcat(pBuff,".any2h"); break;
            case PM_ALL2H    :       strcat(pBuff,".all2h"); break;
            case PM_ANY8H    :       strcat(pBuff,".any8h"); break;
            case PM_ALL8H    :       strcat(pBuff,".all8h"); break;
            case PM_ANY16H   :       strcat(pBuff,".any16h"); break;
            case PM_ALL16H   :       strcat(pBuff,".all16h"); break;
            case PM_ANY32H   :       strcat(pBuff,".any32h"); break;
            case PM_ALL32H   :       strcat(pBuff,".all32h"); break;
            }
        }

        void DisassembleOp( IPrinter& printer, const Instruction& op )
        {
            char Src0[64];
            char Src1[64];
            char Src2[64];
            char Dst[64];
            char Op[64];
            switch( op.GetClass() )
            {
            case IC_SEND:
                {
                    const GEN::SendInstruction& rInst = static_cast<const GEN::SendInstruction&>( op );
                   
                    FormatDest(Dst,rInst.GetDest(), 0 );
                    FormatSource(Src0,rInst.GetSource(),rInst);

                    if( rInst.IsDescriptorInRegister() )
                    {
                        Printf( printer, OPCODE_FORMAT, GEN::OperationToString(op.GetOperation()) );
                        Printf(printer, "%16s, %16s  dest=%s ", Dst, Src0, GEN::SharedFunctionToString(rInst.GetRecipient()));

                        Printf(printer," desc=REG");
                    }
                    else
                    {
                        uint32 nDescriptor = rInst.GetDescriptorIMM();
                        
                        Printf( printer, OPCODE_FORMAT "%16s, %16s\n" , GEN::OperationToString(op.GetOperation()),
                               Dst, Src0 );
                            

                        Printf(printer, "          desc=0x%08x dest=%s\n", nDescriptor, GEN::SharedFunctionToString(rInst.GetRecipient()));
                        Printf(printer, "          len=%u  response=%u\n", rInst.GetMessageLengthFromDescriptor(), rInst.GetResponseLengthFromDescriptor() );
                      

                        switch( rInst.GetRecipient() )
                        {
                        case SFID_DP_DC0:
                            {
                                // TODO: Should probably abstract this
                                //  into a message class
                                uint32 nMsgType = (nDescriptor>>14)&0xf;
                                uint32 nBindTable = nDescriptor&0xff;
                                uint32 nControl = (nDescriptor>>8)&0x3f;
                                switch( nMsgType )
                                {
                                case 0x0: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "OWordBlockRead",nControl, nBindTable); break;
                                case 0x1: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "OWordBlockReadU",nControl,nBindTable); break;
                                case 0x2: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "OWordBlockReadx2",nControl,nBindTable); break;
                                case 0x3: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "DwordScatterRead",nControl,nBindTable); break;
                                case 0x4: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "ByteScatterRead",nControl,nBindTable); break;
                                case 0x7: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "MemFence",nControl,nBindTable); break;
                                case 0x8: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "OWordBlockWrite",nControl,nBindTable); break;
                                case 0xA: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "OWordBlockWritex2",nControl,nBindTable); break;
                                case 0xB: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "DwordScatterWrite",nControl,nBindTable); break;
                                case 0xC: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "ByteScatterWrite",nControl,nBindTable); break;
                                }                      
                            }                          
                            break;
                        case SFID_DP_DC1:
                            {
                                uint32 nMsgType = (nDescriptor>>14)&0xf;
                                uint32 nBindTable = nDescriptor&0xff;
                                uint32 nControl = (nDescriptor>>8)&0x3f;
                                switch( nMsgType )
                                {
                                case 0x1: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "UntypedRead",nControl, nBindTable); break;
                                case 0x2: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "UntypedAtomic",nControl,nBindTable); break;
                                case 0x3: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "UntypedAtomic4x2",nControl,nBindTable); break;
                                case 0x4: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "MediaBlockRead",nControl,nBindTable); break;
                                case 0x5: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "TypedRead",nControl,nBindTable); break;
                                case 0x6: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "TypedAtomic",nControl,nBindTable); break;
                                case 0x7: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "TypedAtomic4x2",nControl,nBindTable); break;
                                case 0x9: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "UntypedWrite",nControl,nBindTable); break;
                                case 0xA: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "MediaBlockWrite",nControl,nBindTable); break;
                                case 0xB: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "AtomicCounterOp",nControl,nBindTable); break;
                                case 0xC: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "AtomicCounterOp4x2",nControl,nBindTable); break;
                                case 0xD: Printf(printer,"          %s" " ctl=0x%x bind=0x%02x", "TypedWrite",nControl,nBindTable); break;
                                }
                            
                            }
                            break;

                        default:
                            ;
                        }

                        if( rInst.IsEOT() )
                        {
                            Printf(printer, "EOT");
                        }
                    }


                }
                break;

             case IC_MATH:
                {
                    char Dest[64];
                    char Src0[64];
                    char Src1[64];
                    const GEN::MathInstruction& rMath = static_cast<const GEN::MathInstruction&>(op);

                     
                    char Pred[64];
                    FormatPredicate(Pred,rMath.GetPredicate(), rMath.GetFlagReference() );
                    if( rMath.GetPredicate().GetMode() != PM_NONE )
                        Printf(printer, "     PRED(%s)", Pred );

                    FormatDest(Dest, rMath.GetDest(), rMath.GetExecSize() );
                    FormatSource(Src0, rMath.GetSource0(), rMath );
                    FormatSource(Src1, rMath.GetSource1(), rMath );
                    const char* pOperation = "?Math?";
                    switch( rMath.GetFunction() )
                    {
                    case MATH_INVERSE: pOperation = "rcp"; break;
                    case MATH_LOG: pOperation = "log"; break;
                    case MATH_EXP: pOperation = "exp"; break;
                    case MATH_SQRT: pOperation = "sqrt"; break;
                    case MATH_RSQ : pOperation = "rsq"; break;
                    case MATH_SIN:  pOperation = "sin"; break;
                    case MATH_COS:  pOperation = "cos"; break;
                    case MATH_FDIV: pOperation = "fdiv"; break;
                    case MATH_POW:  pOperation = "pow"; break;
                    case MATH_IDIV_BOTH: pOperation = "idivmod"; break;
                    case MATH_IDIV_QUOTIENT: pOperation = "idiv"; break;
                    case MATH_IDIV_REMAINDER: pOperation = "imod"; break;
                    }

                    Printf(printer, OPCODE_FORMAT"(%u)""%16s,%16s,%16s", pOperation, rMath.GetExecSize(), Dest, Src0, Src1 );
                   
                }
                break;

            case IC_UNARY:
                {
                    const GEN::UnaryInstruction& rInst = static_cast<const GEN::UnaryInstruction&>( op );
                    
                    char Pred[64];
                    FormatPredicate(Pred,rInst.GetPredicate(), rInst.GetFlagReference() );
                    if( rInst.GetPredicate().GetMode() != PM_NONE )
                        Printf(printer, "     PRED(%s)", Pred );

                    FormatDest(Dst,rInst.GetDest(), rInst.GetExecSize());
                    FormatSource(Src0,rInst.GetSource0(),rInst);

                    Printf( printer, OPCODE_FORMAT"(%u)" "%16s,%16s", 
                           GEN::OperationToString(op.GetOperation()),
                           rInst.GetExecSize(),
                           Dst,
                           Src0 );

                    if( rInst.GetConditionModifier() != CM_NONE )
                        Printf( printer, "  cm(%s)", GEN::ConditionalModifierToString(rInst.GetConditionModifier()) );

                   
                    
                }
                break;

            case IC_BINARY:
                {
                    const GEN::BinaryInstruction& rInst = static_cast<const GEN::BinaryInstruction&>( op );
                    char Pred[64];
                    FormatPredicate(Pred,rInst.GetPredicate(), rInst.GetFlagReference() );
                    if( rInst.GetPredicate().GetMode() != PM_NONE )
                        Printf(printer, "     PRED(%s)", Pred );

                    FormatDest(Dst,rInst.GetDest(), rInst.GetExecSize());
                    FormatSource(Src0,rInst.GetSource0(),rInst);
                    FormatSource(Src1,rInst.GetSource1(),rInst);

                    if( op.GetOperation() == OP_CMP )
                    {
                        // print compares as cmpxxx for brevity
                        char Flag[64];
                        FormatFlagReference( Flag, rInst.GetFlagReference() );

                        Printf( printer, OPCODE_FORMAT"%s(%u)(%s)" "%16s,%16s,%16s", 
                               GEN::OperationToString(op.GetOperation()),
                               GEN::ConditionalModifierToString(rInst.GetConditionModifier()),
                               rInst.GetExecSize(),
                               Flag,
                               Dst,
                               Src0,
                               Src1 );
                    }
                    else
                    {
                        Printf( printer, OPCODE_FORMAT"(%u)" "%16s,%16s,%16s", 
                               GEN::OperationToString(op.GetOperation()),
                               rInst.GetExecSize(),
                               Dst,
                               Src0,
                               Src1 );

                        if( rInst.GetConditionModifier() != CM_NONE )
                            Printf( printer, "  cm(%s)", GEN::ConditionalModifierToString(rInst.GetConditionModifier()) );
                    }

                }
                break;

            case IC_TERNARY:
                {
                    const GEN::TernaryInstruction& rInst = static_cast<const GEN::TernaryInstruction&>( op );
                    
                    char Pred[64];
                    FormatPredicate(Pred,rInst.GetPredicate(), rInst.GetFlagReference() );
                    if( rInst.GetPredicate().GetMode() != PM_NONE )
                        Printf(printer, "     PRED(%s)", Pred );

                    FormatDest(Dst,rInst.GetDest(), rInst.GetExecSize());
                    FormatSource(Src0,rInst.GetSource0(),rInst);
                    FormatSource(Src1,rInst.GetSource1(),rInst);
                    FormatSource(Src2,rInst.GetSource2(),rInst);

                    
                    Printf( printer, OPCODE_FORMAT"(%u)" "%16s,%14s,%14s,%14s", 
                            GEN::OperationToString(op.GetOperation()),
                            rInst.GetExecSize(),
                            Dst,
                            Src0,
                            Src1,
                            Src2);

              
                }
                break;
            case IC_NULL:
                {
                    Printf( printer, OPCODE_FORMAT, GEN::OperationToString(op.GetOperation()) );
                }
                break;
            case IC_BRANCH:
                {
                    const GEN::BranchInstruction& rBranch = static_cast<const GEN::BranchInstruction&>( op );

                    char Pred[64];
                    FormatPredicate(Pred,rBranch.GetPredicate(), rBranch.GetFlagReference() );

                    Printf( printer, OPCODE_FORMAT"(%u) JIP=%d UIP=%d  PRED=%s", 
                           GEN::OperationToString(op.GetOperation()),
                           rBranch.GetExecSize(),
                           rBranch.GetJIP(),
                           rBranch.GetUIP(),
                           Pred );
                
                }
                break;
            }

            if( op.IsDDCheckDisabled() )
                Printf(printer,"NoDDChk");
        }

    }



    bool Disassemble( IPrinter& printer, Decoder* pDecoder, const void* pIsa, size_t nIsaBytes )
    {
        const uint8* pIsaBytes = (const uint8*)pIsa;
        size_t offs=0;
        while( offs < nIsaBytes )
        {
            Instruction op;
            size_t nLength = pDecoder->Decode( &op, pIsaBytes+offs );
            if( offs+nLength > nIsaBytes )
            {
                _INTERNAL::Printf( printer, "Instruction at 0x%08x (offset %u) would overrun specified buffer\n", pIsaBytes+offs, offs );
                return false;
            }
            if( nLength == 0 )
            {
                _INTERNAL::Printf( printer, "No legal instruction at: 0x%08x (offset %u)\n", pIsaBytes+offs, offs );
                return false;
            }

            if( nLength == 8 )
                _INTERNAL::Printf(printer, "c ");
            else
                _INTERNAL::Printf(printer, "n ");

            _INTERNAL::DisassembleOp( printer, op );
            
            _INTERNAL::Printf(printer,"\n");

            offs += nLength;
            
        }


        return true;
    }


}