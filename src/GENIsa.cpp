

#include "GENIsa.h"


namespace GEN
{
    const char* SharedFunctionToString( SharedFunctionIDs eFunc )
    {
        switch( eFunc )
        {
        case SFID_NULL       : return "NULL";
        case SFID_SAMPLER    : return "SAMPLER";
        case SFID_GATEWAY    : return "GATEWAY";
        case SFID_DP_SAMPLER : return "DP_SAMPLER ";
        case SFID_DP_RC      : return "DP_RC";
        case SFID_URB        : return "URB";
        case SFID_SPAWNER    : return "SPAWNER";
        case SFID_VME        : return "VME";
        case SFID_DP_CC      : return "DP_CC";
        case SFID_DP_DC0     : return "DP_DC0";
        case SFID_DP_DC1     : return "DP_DC1";
        case SFID_PI         : return "PI";
        case SFID_CRE        : return "CRE";
        default: return "???";
        }
    }
    
    const char* OperationToString( Operations op )
    {
        switch( op )
        {
        case OP_MOV     : return "mov"    ;
        case OP_SEL     : return "sel"    ;
        case OP_MOVI    : return "movi"   ;
        case OP_NOT     : return "not"    ;
        case OP_AND     : return "and"    ;
        case OP_OR      : return "or"     ;
        case OP_XOR     : return "xor"    ;
        case OP_SHR     : return "shr"    ;
        case OP_SHL     : return "shl"    ;
        case OP_ASR     : return "asr"    ;
        case OP_CMP     : return "cmp"    ;
        case OP_CMPN    : return "cmpn"   ;
        case OP_F32TO16 : return "f32to16";
        case OP_F16TO32 : return "f16to32";
        case OP_BFREV   : return "bfrev"  ;
        case OP_BFE     : return "bfe"    ;
        case OP_BFI1    : return "bfi1"   ;
        case OP_BFI2    : return "bfi2"   ;
        case OP_JMPI    : return "jmpi"   ;
        case OP_BRD     : return "brd"    ;
        case OP_IF      : return "if"     ;
        case OP_BRC     : return "brc"    ;
        case OP_ELSE    : return "else"   ;
        case OP_ENDIF   : return "endif"  ;
        case OP_CASE    : return "case"   ;
        case OP_WHILE   : return "while"  ;
        case OP_BREAK   : return "break"  ;
        case OP_CONT    : return "cont"   ;
        case OP_HALT    : return "halt"   ;
        case OP_CALL    : return "call"   ;
        case OP_RETURN  : return "return" ;
        case OP_WAIT    : return "wait"   ;
        case OP_SEND    : return "send"   ;
        case OP_SENDC   : return "sendc"  ;
        case OP_MATH    : return "math"   ;
        case OP_ADD     : return "add"    ;
        case OP_MUL     : return "mul"    ;
        case OP_AVG     : return "avg"    ;
        case OP_FRC     : return "frc"    ;
        case OP_RNDU    : return "rndu"   ;
        case OP_RNDD    : return "rndd"   ;
        case OP_RNDE    : return "rnde"   ;
        case OP_RNDZ    : return "rndz"   ;
        case OP_MAC     : return "mac"    ;
        case OP_MACH    : return "mach"   ;
        case OP_LZD     : return "lzd"    ;
        case OP_FBH     : return "fbh"    ;
        case OP_FBL     : return "fbl"    ;
        case OP_CBIT    : return "cbit"   ;
        case OP_ADDC    : return "addc"   ;
        case OP_SUBB    : return "subb"   ;
        case OP_SAD2    : return "sad2"   ;
        case OP_SADA2   : return "sada2"  ;
        case OP_DP4     : return "dp4"    ;
        case OP_DPH     : return "dph"    ;
        case OP_DP3     : return "dp3"    ;
        case OP_DP2     : return "dp2"    ;
        case OP_LINE    : return "line"   ;
        case OP_PLN     : return "pln"    ;
        case OP_FMA     : return "fma"    ;
        case OP_LRP     : return "lrp"    ;
        case OP_ILLEGAL : return "ILLEGAL";
        case OP_NOP     : return "nop"    ; 
        default:
            return "????";
        }
    }


    const char* ConditionalModifierToString( ConditionalModifiers m )
    {
        switch(m)
        {
        default: return "????";
        case CM_NONE            : return "";
        case CM_ZERO            : return "eq";
        case CM_NOTZERO         : return "ne";
        case CM_GREATER_THAN    : return "gt";
        case CM_GREATER_EQUAL   : return "ge";
        case CM_LESS_THAN       : return "lt";
        case CM_LESS_EQUAL      : return "le";
        case CM_SIGNED_OVERFLOW : return "so";
        case CM_UNORDERED       : return "uo";
        }
    }


    SendInstruction SendEOT( uint32 nSourceGPR )
    {
        SendInstruction eot( SFID_SPAWNER, 0x02000010, 
                              DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_NULL,0),8,1,8)),
                              SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(REG_GPR,nSourceGPR),8,1,8)) );
        eot.SetEOT();
        return eot;
    }


    SendInstruction DwordScatterWriteSIMD8( uint32 nBindTableIndex, uint32 nSourceGPR )
    {
        uint32 dwDescriptor = 0;
        dwDescriptor |= (0x02)<<25; // message length
        dwDescriptor |= (0x9) <<14; // message type:  "Untyped Write"
        dwDescriptor |= (0x2e) <<8;  
        dwDescriptor |= (nBindTableIndex & 0xff);
            
        SendInstruction write( SFID_DP_DC1, dwDescriptor,
                                DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_NULL,0),8,1,8)),
                                SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(REG_GPR,nSourceGPR),8,1,8)) );
        return write;
    }

    SendInstruction OWordDualBlockWrite( uint32 nBindTableIndex, uint32 nSourceGPR)
    {
        uint32 dwDescriptor = 0;
        dwDescriptor  = 0x04000000;
        dwDescriptor |= (1<<19); // this message needs a header
        dwDescriptor |= 0x8<<14; // message type (block write)
        dwDescriptor |= 0x200;   // block length (in owords)
        dwDescriptor |=  (nBindTableIndex&0xff);

        SendInstruction write( SFID_DP_DC0, dwDescriptor,
                                DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_NULL,0),8,8,1)),
                                SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(REG_GPR,nSourceGPR),8,8,1)) );
        return write;
    }

    SendInstruction OWordDualBlockRead( uint32 nBindTableIndex, uint32 nAddress, uint32 nData )
    {
        uint32 dwDescriptor = 0;
        dwDescriptor  = (0x1<<25); // message length (excluding header)
        dwDescriptor |= (1<<20);   // response length (1 gpr)
        dwDescriptor |= (1<<19);   // this message needs a header
        dwDescriptor |= 0x0<<14;   // message type (block read)
        dwDescriptor |= 0x200;     // block length (in owords)
        dwDescriptor |=  (nBindTableIndex&0xff);

        SendInstruction write( SFID_DP_DC0, dwDescriptor,
                                DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_GPR,nData),8,8,1)),
                                SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(REG_GPR,nAddress),8,8,1)) );
        return write;
    }

    
    SendInstruction OWordDualBlockReadAndInvalidate( uint32 nBindTableIndex, uint32 nAddress, uint32 nData )
    {
        uint32 dwDescriptor = 0;
        dwDescriptor  = (0x1<<25); // message length (excluding header)
        dwDescriptor |= (1<<13);   // invalidate lines after reading
        dwDescriptor |= (1<<20);   // response length (1 gpr)
        dwDescriptor |= (1<<19);   // this message needs a header
        dwDescriptor |= 0x0<<14;   // message type (block read)
        dwDescriptor |= 0x200;     // block length (in owords)
        dwDescriptor |=  (nBindTableIndex&0xff);

        SendInstruction write( SFID_DP_DC0, dwDescriptor,
                                DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_GPR,nData),8,8,1)),
                                SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(REG_GPR,nAddress),8,8,1)) );
        return write;
    }

   
    SendInstruction DWordScatteredReadSIMD8( uint32 nBindTableIndex, uint32 nAddress, uint32 nData )
    {
        uint32 dwDescriptor = 0;
        dwDescriptor  = (0x1<<25); // message length (1 gpr)
        dwDescriptor |= (1<<20);   // response length (1 gpr)
        dwDescriptor |= 0x3<<14;   // message type (dword scattered read)
        dwDescriptor |= 0x200;     // block length (8 dwords)
        dwDescriptor |=  (nBindTableIndex&0xff);

        SendInstruction write( SFID_DP_DC0, dwDescriptor,
                                DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_GPR,nData),8,8,1)),
                                SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(REG_GPR,nAddress),8,8,1)) );
        return write;
    }
   

    BinaryInstruction DoMath( uint32 nExecSize, Operations eOp, DataTypes eType, uint32 nDstReg, uint32 nSrc0, uint32 nSrc1 )
    {
        size_t v,w,h;
        switch( nExecSize )
        {
        case 1:
            v=1;
            h=1;
            w=1;
            break;
        case 8:
        case 16:
            v=8;
            w=8;
            h=1;
            break;
        }

        return GEN::BinaryInstruction( nExecSize, eOp, 
                                      DestOperand( eType, RegisterRegion( DirectRegReference(nDstReg),v,w,h) ),
                                      SourceOperand(eType,RegisterRegion( DirectRegReference(nSrc0),v,w,h) ),
                                      SourceOperand(eType,RegisterRegion( DirectRegReference(nSrc1),v,w,h) )
                                      );

    }


    BinaryInstruction DoMathIMM( uint32 nExecSize, Operations eOp, uint32 nDstReg, uint32 nSrc0, uint32 imm )
    {
        DataTypes eType = DT_U32;
        size_t v,w,h;
        switch( nExecSize )
        {
        case 1:
            v=1;
            h=1;
            w=1;
            break;
        case 8:
        case 16:
            v=0;
            w=1;
            h=0;
            break;
        }

        return GEN::BinaryInstruction( nExecSize, eOp, 
                                      DestOperand( eType, RegisterRegion( DirectRegReference(nDstReg),v,w,h) ),
                                      SourceOperand(eType,RegisterRegion( DirectRegReference(nSrc0),v,w,h) ),
                                      SourceOperand(eType), imm 
                                      );
    }


    UnaryInstruction RegMove( RegTypes eDstType, uint32 nDstReg, RegTypes eSrcType, uint32 nSrcReg )
    {
        return GEN::UnaryInstruction( 8, 
                                      GEN::OP_MOV,
                                      DestOperand( DT_F32, RegisterRegion( DirectRegReference(eDstType,nDstReg,0),8,8,1) ),
                                      SourceOperand(DT_F32, RegisterRegion( DirectRegReference(eSrcType,nSrcReg,0),8,8,1) )
                                      );
    }
    
    UnaryInstruction RegMoveIMM( uint32 nDstReg, uint32 IMM )
    {
        return GEN::UnaryInstruction( 8, 
                                      GEN::OP_MOV,
                                      DestOperand( DT_U32, RegisterRegion( DirectRegReference(REG_GPR,nDstReg,0),8,8,1) ),
                                      SourceOperand(DT_U32), IMM 
                                      );
    }


    SendInstruction ReadGatewayTimestamp( uint32 nDest, uint32 nSrc )
    {
        uint32 dwDescriptor = 0;
        dwDescriptor |= (1<<25); // message length = 1
        dwDescriptor |= (1<<20); // response length = 1
        dwDescriptor |= (1<<14); // yes hardware, please send me that ack message I'm expecting
        dwDescriptor |= (0x3);   // message type

        SendInstruction msg( SFID_GATEWAY, dwDescriptor,
                                DestOperand( DT_U32, RegisterRegion( DirectRegReference(nDest),8,8,1)),
                                SourceOperand( DT_U32,  RegisterRegion( DirectRegReference(nSrc),8,8,1)) );
        return msg;
    }

}