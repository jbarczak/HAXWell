
#include "GENIsa.h"
#include "GENCoder.h"

#include <string.h>

namespace GEN
{
    namespace _INTERNAL
    {
      
        
        typedef uint32 DWORD;
        typedef unsigned __int64 QWORD;

        struct EnumLUT
        {
            uint32 eEnum;
            uint32  nEncoding;
        };

        static uint32 LUTLookup( const EnumLUT* pLUT, size_t nLUTSize,uint32 en, uint32 TInvalid )
        {
            size_t n = nLUTSize/sizeof(EnumLUT);
            for( size_t i=0; i<n; i++ ) 
                if( pLUT[i].nEncoding == en ) 
                    return pLUT[i].eEnum; 
            return TInvalid;
        }
        static uint32 InverseLUTLookup( const EnumLUT* pLUT, size_t nLUTSize,uint32 en, uint32 TInvalid )
        {
            size_t n = nLUTSize/sizeof(EnumLUT);
            for( size_t i=0; i<n; i++ ) 
                if( pLUT[i].eEnum == en ) 
                    return pLUT[i].nEncoding; 
            return TInvalid;
        }

        #define BEGIN_TRANSLATOR(T,Name) static const EnumLUT LUT_##T_##Name[] = {
        #define ENUM(Enum,Value) { Enum, Value },
        #define END_TRANSLATOR(T,Name,TInvalid) \
            };\
            T DECODE_##Name( size_t en ) {\
                const EnumLUT* pLUT = LUT_##T_##Name;\
                size_t n = sizeof(LUT_##T_##Name);\
                return (T) LUTLookup(pLUT,n,en,TInvalid);\
            }; \
            uint32 Encode_##Name( T de ) {\
                const EnumLUT* pLUT = LUT_##T_##Name;\
                size_t n = sizeof(LUT_##T_##Name);\
                return (size_t) InverseLUTLookup(pLUT,n,de,TInvalid);\
            };

        enum 
        {
            OPCODE_MASK = 0x7f
        };
     
        /// Encoded register file IDs
        enum RegisterFiles
        {
            RF_ARF = 0,
            RF_GRF = 1,
            RF_IMM = 3,
        };

        // register data type field -> abstract enum
        BEGIN_TRANSLATOR(DataTypes,RegDataTypes)
           ENUM(DT_U32, 0x0 ) 
           ENUM(DT_S32, 0x1 )
           ENUM(DT_U16, 0x2 )
           ENUM(DT_S16, 0x3 )
           ENUM(DT_U8,  0x4 )
           ENUM(DT_S8,  0x5 )
           ENUM(DT_F64, 0x6 )
           ENUM(DT_F32, 0x7 )
        END_TRANSLATOR(DataTypes,RegDataTypes,DT_INVALID)

          // register data type field -> abstract enum
        BEGIN_TRANSLATOR(DataTypes,RegDataTypes3Src)
           ENUM(DT_F32, 0x0 ) 
           ENUM(DT_S32, 0x1 )
           ENUM(DT_U32, 0x2 )
           ENUM(DT_F64, 0x3 )
        END_TRANSLATOR(DataTypes,RegDataTypes3Src,DT_INVALID)

        // IMM data type field -> abstract enum
        BEGIN_TRANSLATOR(DataTypes,ImmDataTypes)
           ENUM(DT_U32,                0x0 ) 
           ENUM(DT_S32,                0x1 )
           ENUM(DT_U16,                0x2 )
           ENUM(DT_S16,                0x3 )
           ENUM(DT_VEC_HALFBYTE_UINT,  0x4 )
           ENUM(DT_VEC_HALFBYTE_FLOAT, 0x5 )
           ENUM(DT_VEC_HALFBYTE_SINT,  0x6 )
           ENUM(DT_F32,                0x7 )
        END_TRANSLATOR(DataTypes,ImmDataTypes,DT_INVALID)

        // maps ARF regnum into an arch register anum
        BEGIN_TRANSLATOR(RegTypes,ArchRegisters)
            ENUM(REG_NULL,              0x00)
            ENUM(REG_ADDRESS,           0x10 )
            ENUM(REG_ACCUM0,            0x20 )
            ENUM(REG_ACCUM1,            0x21 )
            ENUM(REG_FLAG0,             0x30 )
            ENUM(REG_FLAG1,             0x31 )
            ENUM(REG_CHANNEL_ENABLE,    0x40 )
            ENUM(REG_STACK_PTR,         0x60 )
            ENUM(REG_STATE,             0x70 )
            ENUM(REG_CONTROL,           0x80 )
            ENUM(REG_NOTIFICATION0,     0x90 )
            ENUM(REG_NOTIFICATION1,     0x91 )
            ENUM(REG_INSTRUCTION_PTR,   0xA0 )
            ENUM(REG_THREAD_DEPENDENCY, 0xB0 )
            ENUM(REG_TIMESTAMP,         0xC0 )
            ENUM(REG_FC0,               0xD0 )
            ENUM(REG_FC1,               0xD1 )
            ENUM(REG_FC2,               0xD2 )
            ENUM(REG_FC3,               0xD3 )
            ENUM(REG_FC4,               0xD4 )
            ENUM(REG_FC5,               0xD5 )
            ENUM(REG_FC6,               0xD6 )
            ENUM(REG_FC7,               0xD7 )
            ENUM(REG_FC8,               0xD8 )
            ENUM(REG_FC9,               0xD9 )
            ENUM(REG_FC10,              0xDA )
            ENUM(REG_FC11,              0xDB )
            ENUM(REG_FC12,              0xDC )
            ENUM(REG_FC13,              0xDD )
            ENUM(REG_FC14,              0xDE )
            ENUM(REG_FC15,              0xDF )
        END_TRANSLATOR(RegTypes,ArchRegisters,REG_INVALID)


        // Map encoded HSTRIDE to the actual stride
        BEGIN_TRANSLATOR(DWORD,HStride)
            ENUM(0,0x0)
            ENUM(1,0x1)
            ENUM(2,0x2)
            ENUM(4,0x3)
        END_TRANSLATOR(DWORD,HStride,0xffffffff)

        // Map encoded WIDTH to the actual width
        BEGIN_TRANSLATOR(DWORD,Width)
            ENUM( 1, 0x0 )
            ENUM( 2, 0x1 )
            ENUM( 4, 0x2 )
            ENUM( 8, 0x3 )
            ENUM( 16,0x4 )
        END_TRANSLATOR(DWORD,Width,0xffffffff)

        // Map encoded VSTRIDE to actual stride
        BEGIN_TRANSLATOR(DWORD,VStride)
            ENUM(0,0x0)
            ENUM(1,0x1)
            ENUM(2,0x2)
            ENUM(4,0x3)
            ENUM(8,0x4)
            ENUM(16,0x5)
            ENUM(32,0x6)
            ENUM(0xbaadf00d,0xf) // TODO: This is the magic encoding for VxH and Vx1 indirect addressing modes
        END_TRANSLATOR(DWORD,VStride,0xffffffff)

        BEGIN_TRANSLATOR(DWORD,ExecSize)
            ENUM(1,0)
            ENUM(2,1)
            ENUM(4,2)
            ENUM(8,3)
            ENUM(16,4)
            ENUM(32,5)
        END_TRANSLATOR(DWORD,ExecSize,0)

        BEGIN_TRANSLATOR(Operations,Operations)
           ENUM(OP_MOV     ,0x01)
           ENUM(OP_SEL     ,0x02)
           ENUM(OP_MOVI    ,0x03)
           ENUM(OP_NOT     ,0x04)
           ENUM(OP_AND     ,0x05)
           ENUM(OP_OR      ,0x06)
           ENUM(OP_XOR     ,0x07)
           ENUM(OP_SHR     ,0x08)
           ENUM(OP_SHL     ,0x09)
           ENUM(OP_DIM     ,0x0A) // double-precision immediate move
           ENUM(OP_ASR     ,0x0C)// arithmetic right shift
            // 0x0D-0x0F reserved
           ENUM(OP_CMP     ,0x10 ) //Component-wise compare, store condition code in destination
           ENUM(OP_CMPN    ,0x11 ) //Component-wise nan, store condition code in destination
           ENUM(OP_CSEL    ,0x12 ) // 3-src conditional select
           ENUM(OP_F32TO16 , 0x13)
           ENUM(OP_F16TO32 , 0x14)
           // 0x15-0x16 reserved
           ENUM(OP_BFREV  , 0x17) // bit reverse
           ENUM(OP_BFE    , 0X18)
           ENUM(OP_BFI1   , 0x19) // bit-field insert macro-op 1
           ENUM(OP_BFI2   , 0x1A) // """ 2
           // 0x1B-0x1F reserved
           ENUM(OP_JMPI   , 0x20) // indexed jump
           ENUM(OP_BRD    , 0x21) // diverging branch
           ENUM(OP_IF     , 0x22)
           ENUM(OP_BRC    , 0x23) // converging branch
           ENUM(OP_ELSE   , 0x24)
           ENUM(OP_ENDIF  , 0x25)
           ENUM(OP_CASE   , 0x26)
           ENUM(OP_WHILE  , 0x27)
           ENUM(OP_BREAK  , 0x28)
           ENUM(OP_CONT   , 0x29)
           ENUM(OP_HALT   , 0x2A)
           ENUM(OP_CALLA  , 0x2B) // call absolute
           ENUM(OP_CALL   , 0x2C)
           ENUM(OP_RETURN , 0x2D)
            // 0x2D,2F reserved
           ENUM(OP_WAIT  ,  0x30)
           ENUM(OP_SEND  ,  0x31)
           ENUM(OP_SENDC ,  0x32)
            // 0x33-0x37 reserved
           ENUM(OP_MATH  ,  0x38)
            // 0x39-0x3f reserved
           ENUM(OP_ADD   ,0x40)
           ENUM(OP_MUL   ,0x41)
           ENUM(OP_AVG   ,0x42)
           ENUM(OP_FRC   ,0x43)
           ENUM(OP_RNDU  ,0x44) // round up (ceil)
           ENUM(OP_RNDD  ,0x45) // round down (floor)
           ENUM(OP_RNDE  ,0x46) // round-nearest even
           ENUM(OP_RNDZ  ,0x47) // round toward 0
           ENUM(OP_MAC   ,0x48)
           ENUM(OP_MACH  ,0x49) // mul-accumulate high
           ENUM(OP_LZD   ,0x4A) // leading zero detection
           ENUM(OP_FBH   ,0x4B) // first bit high
           ENUM(OP_FBL   ,0x4C) // "  " low
           ENUM(OP_CBIT  ,0x4D) // count bits
           ENUM(OP_ADDC  ,0x4E)
           ENUM(OP_SUBB  ,0x4F)
           // 4B-4F reserved
           ENUM(OP_SAD2  , 0x50)
           ENUM(OP_SADA2 , 0x51) // SAD-accumulate
            // 0X52-53 reserved
           ENUM(OP_DP4, 0x54)
           ENUM(OP_DPH, 0x55) // DP4 homogeneous
           ENUM(OP_DP3, 0x56)
           ENUM(OP_DP2, 0x57)
            // 0x58 reserved
           ENUM(OP_LINE, 0x59) // line evaluation
           ENUM(OP_PLN , 0x5A) // plane evaluation
           ENUM(OP_FMA , 0x5B)
           ENUM(OP_LRP , 0x5C)
            // 0x5D-5F reserved
           ENUM(OP_NOP    , 0x7E)
           ENUM(OP_ILLEGAL, 0x0) // ILLEGAL opcode signals an invalid opcode exception
        END_TRANSLATOR(Operations,Operations,NOT_AN_OP);


        BEGIN_TRANSLATOR(SharedFunctionIDs,SharedFunctions)
            ENUM( SFID_NULL ,      0x0) 
            ENUM( SFID_SAMPLER ,   0x2)
            ENUM( SFID_GATEWAY ,   0x3)
            ENUM( SFID_DP_SAMPLER, 0x4)     // sampler cache dataport
            ENUM( SFID_DP_RC,      0x5)     // render cache dataport
            ENUM( SFID_URB,        0x6)
            ENUM( SFID_SPAWNER,    0x7)
            ENUM( SFID_VME,        0x8)
            ENUM( SFID_DP_CC,      0x9)     // constant cache dataport
            ENUM( SFID_DP_DC0,     0xA)
            ENUM( SFID_PI,         0xB)     // pixel interpolator
            ENUM( SFID_DP_DC1,     0xC)     // DC1 is the same recipient as DC0.  Intel just ran out of message bits.
            ENUM( SFID_CRE,        0xD)     // check and refinement engine
        END_TRANSLATOR(SharedFunctionIDs,SharedFunctions,SFID_INVALID)

        BEGIN_TRANSLATOR(MathFunctionIDs,MathFunctions)
            ENUM( MATH_INVERSE,          0x1 )
            ENUM( MATH_LOG,              0x2 )
            ENUM( MATH_EXP,              0x3 )
            ENUM( MATH_SQRT,             0x4 )
            ENUM( MATH_RSQ,              0x5 )
            ENUM( MATH_SIN,              0x6 )
            ENUM( MATH_COS,              0x7 )
            ENUM( MATH_FDIV,             0x9 )
            ENUM( MATH_POW,              0xA )
            ENUM( MATH_IDIV_BOTH,        0xB )
            ENUM( MATH_IDIV_QUOTIENT,    0xC )
            ENUM( MATH_IDIV_REMAINDER,   0xD )
        END_TRANSLATOR(MathFunctionIDs,MathFunctions,MATH_INVALID)

        
        BEGIN_TRANSLATOR(ConditionalModifiers,CondModifiers)
            ENUM( CM_NONE            ,0x00 )
            ENUM( CM_ZERO            ,0x01 )
            ENUM( CM_NOTZERO         ,0x02 )
            ENUM( CM_GREATER_THAN    ,0x03 )
            ENUM( CM_GREATER_EQUAL   ,0x04 )
            ENUM( CM_LESS_THAN       ,0x05 )
            ENUM( CM_LESS_EQUAL      ,0x06 )
            ENUM( CM_SIGNED_OVERFLOW ,0x08 )
            ENUM( CM_UNORDERED       ,0x09 )
        END_TRANSLATOR(ConditionalModifiers,CondModifiers,MATH_INVALID)

           
        BEGIN_TRANSLATOR(PredicationModes,PredModesAlign16)
            ENUM( PM_NONE,              0 )
            ENUM( PM_SEQUENTIAL_FLAG,   1 )
            ENUM( PM_SWIZZLE_X,         2 )
            ENUM( PM_SWIZZLE_Y,         3 )
            ENUM( PM_SWIZZLE_Z,         4 )
            ENUM( PM_SWIZZLE_W,         5 )
            ENUM( PM_ANY4H,             6 )
            ENUM( PM_ALL4H,             7 )
        END_TRANSLATOR(PredicationModes,PredModesAlign16,PM_NONE)


        BEGIN_TRANSLATOR(PredicationModes,PredModesAlign1)
            ENUM( PM_NONE,              0 )
            ENUM( PM_SEQUENTIAL_FLAG,   1 )
            ENUM( PM_ANYV,              2 )
            ENUM( PM_ALLV,              3 )
            ENUM( PM_ANY2H,             4 )
            ENUM( PM_ALL2H,             5 )
            ENUM( PM_ANY4H,             6 )
            ENUM( PM_ALL4H,             7 )
            ENUM( PM_ANY8H,             8 )
            ENUM( PM_ALL8H,             9 )
            ENUM( PM_ANY16H,            10 )
            ENUM( PM_ALL16H,            11 )
            ENUM( PM_ANY32H,            12 )
            ENUM( PM_ALL32H,            13 )
        END_TRANSLATOR(PredicationModes,PredModesAlign1,PM_NONE)


        int SignExtend( DWORD dw, DWORD bit )
        {
            dw |= ~( (dw & (1<<bit))-1);
            return (int)dw;
        }

        bool IsBasicThreeSource( Operations eOp )
        {
            switch( eOp )
            {
            case OP_LRP:
            case OP_CSEL:
            case OP_BFE:
            case OP_FMA:
                return true;
            default:
                return false;
            }
        }

        bool IsCompressedInstruction( uint32 dw )
        {
            return (dw&(1<<29)) != 0;
        }

        DWORD ReadDWORD( const uint8* p ) { return * ((uint32*)p); }

        DWORD ReadBit( const uint8* p, DWORD bit )
        {
            return (p[bit/8]>>(bit%8))&1;
        }
        DWORD ReadBits( const uint8* p, DWORD hi, DWORD lo )
        {
            // TODO: Optimize this once its clear we don't need to span dword boundaries
            DWORD a=0;
            DWORD n = 0;
            while( lo+n <= hi )
            {
                DWORD bit = ReadBit(p,lo+n);
                a |= (bit<<n);
                n++;
            }
            return a;
        }

        void WriteBit(  uint8* p, DWORD bit, DWORD lo )
        {
            DWORD byte = p[lo/8];
            bit = bit << (lo%8);
            byte = (byte & ~(1<<(lo%8))) | bit;
            p[lo/8] = byte;
        }
        void WriteBits( uint8* p, DWORD bits, DWORD hi, DWORD lo )
        {
            while( lo <= hi )
            {
                DWORD bit = bits&1;
                DWORD byte = p[lo/8];
                bit = bit << (lo%8);
                byte = (byte & ~bit) | bit;
                p[lo/8] = byte;
                bits = bits>>1;
                lo++;
            }
        }
       

        // MSVC doesn't do binary literals yet, but the following code can be used
        //  to turn copy-pasted binary strings into hex
        //  This is what I used to generate the instruction compaction tables
#if 0
        void DoStr( const char* str )
        {
            unsigned int val=0;
            size_t l = strlen(str);
            for( size_t i=0; i<l; i++ )
            {
                char bit = str[l-(i+1)]-'0';
                val |= (bit<<i);
            }
            printf("    0x%08x, // %s\n",val,str);
        }

        void DoTables()
        {
            printf("static const DWORD COMPACT1[32] = {\n");
            for( size_t i=0; i<32; i++ )
                DoStr( COMPACT1[i] );
            printf("};\n");
            printf("static const DWORD COMPACT1[32] = {\n");
            for( size_t i=0; i<32; i++ )
                DoStr( COMPACT2[i] );
            printf("};\n");

            printf("static const DWORD COMPACT1[32] = {\n");
            for( size_t i=0; i<32; i++ )
                DoStr( COMPACT3[i] );
            printf("};\n");
            exit(0);
        }
#endif
        static const DWORD COMPACT1[32] = {
            0x00000002, // 0000000000000000010
            0x00004000, // 0000100000000000000
            0x00004001, // 0000100000000000001
            0x00004002, // 0000100000000000010
            0x00004003, // 0000100000000000011
            0x00004004, // 0000100000000000100
            0x00004005, // 0000100000000000101
            0x00004007, // 0000100000000000111
            0x00004008, // 0000100000000001000
            0x00004009, // 0000100000000001001
            0x0000400d, // 0000100000000001101
            0x00006000, // 0000110000000000000
            0x00006001, // 0000110000000000001
            0x00006002, // 0000110000000000010
            0x00006003, // 0000110000000000011
            0x00006004, // 0000110000000000100
            0x00006005, // 0000110000000000101
            0x00006007, // 0000110000000000111
            0x00006009, // 0000110000000001001
            0x0000600d, // 0000110000000001101
            0x00006010, // 0000110000000010000
            0x00006100, // 0000110000100000000
            0x00008000, // 0001000000000000000
            0x00008002, // 0001000000000000010
            0x00008004, // 0001000000000000100
            0x00008100, // 0001000000100000000
            0x00016000, // 0010110000000000000
            0x00016010, // 0010110000000010000
            0x00018000, // 0011000000000000000
            0x00018100, // 0011000000100000000
            0x00028000, // 0101000000000000000
            0x00028100, // 0101000000100000000
        };
        static const DWORD COMPACT2[32] = {
            0x00008001, // 001000000000000001
            0x00008020, // 001000000000100000
            0x00008021, // 001000000000100001
            0x00008061, // 001000000001100001
            0x000080bd, // 001000000010111101
            0x000082fd, // 001000001011111101
            0x000083a1, // 001000001110100001
            0x000083a5, // 001000001110100101
            0x000083bd, // 001000001110111101
            0x00008421, // 001000010000100001
            0x00008c20, // 001000110000100000
            0x00008c21, // 001000110000100001
            0x000094a5, // 001001010010100101
            0x00009ca4, // 001001110010100100
            0x00009ca5, // 001001110010100101
            0x0000f3bd, // 001111001110111101
            0x0000f79d, // 001111011110011101
            0x0000f7bc, // 001111011110111100
            0x0000f7bd, // 001111011110111101
            0x0000ffbc, // 001111111110111100
            0x0000020c, // 000000001000001100
            0x0000803d, // 001000000000111101
            0x000080a5, // 001000000010100101
            0x00008420, // 001000010000100000
            0x000094a4, // 001001010010100100
            0x00009c84, // 001001110010000100
            0x0000a509, // 001010010100001001
            0x0000dfbd, // 001101111110111101
            0x0000ffbd, // 001111111110111101
            0x0000bdac, // 001011110110101100
            0x0000a528, // 001010010100101000
            0x0000ad28, // 001010110100101000
        };
        static const DWORD COMPACT3[32] = {
            0x00000000, // 000000000000000
            0x00000001, // 000000000000001
            0x00000008, // 000000000001000
            0x0000000f, // 000000000001111
            0x00000010, // 000000000010000
            0x00000080, // 000000010000000
            0x00000100, // 000000100000000
            0x00000180, // 000000110000000
            0x00000200, // 000001000000000
            0x00000210, // 000001000010000
            0x00000280, // 000001010000000
            0x00001000, // 001000000000000
            0x00001001, // 001000000000001
            0x00001081, // 001000010000001
            0x00001082, // 001000010000010
            0x00001083, // 001000010000011
            0x00001084, // 001000010000100
            0x00001087, // 001000010000111
            0x00001088, // 001000010001000
            0x0000108e, // 001000010001110
            0x0000108f, // 001000010001111
            0x00001180, // 001000110000000
            0x000011e8, // 001000111101000
            0x00002000, // 010000000000000
            0x00002180, // 010000110000000
            0x00003000, // 011000000000000
            0x00003c87, // 011110010000111
            0x00004000, // 100000000000000
            0x00005000, // 101000000000000
            0x00006000, // 110000000000000
            0x00007000, // 111000000000000
            0x0000701c, // 111000000011100
        };

        static const DWORD COMPACT4[32] = {
            0x00000000, // 000000000000
            0x00000002, // 000000000010
            0x00000010, // 000000010000
            0x00000012, // 000000010010
            0x00000018, // 000000011000
            0x00000020, // 000000100000
            0x00000028, // 000000101000
            0x00000048, // 000001001000
            0x00000050, // 000001010000
            0x00000070, // 000001110000
            0x00000078, // 000001111000
            0x00000300, // 001100000000
            0x00000302, // 001100000010
            0x00000308, // 001100001000
            0x00000310, // 001100010000
            0x00000312, // 001100010010
            0x00000320, // 001100100000
            0x00000328, // 001100101000
            0x00000338, // 001100111000
            0x00000340, // 001101000000
            0x00000342, // 001101000010
            0x00000348, // 001101001000
            0x00000350, // 001101010000
            0x00000360, // 001101100000
            0x00000368, // 001101101000
            0x00000370, // 001101110000
            0x00000371, // 001101110001
            0x00000378, // 001101111000
            0x00000468, // 010001101000
            0x00000469, // 010001101001
            0x0000046a, // 010001101010
            0x00000588, // 010110001000
        };


        void ExpandCompressedInstruction( uint8* pOut, const uint8* pIn )
        {
        
            memset(pOut,0,16);
            DWORD dwOpcode = ReadBits(pIn,6,0);
            WriteBits( pOut, dwOpcode, 6, 0 ); // opcode

            DWORD dwLookup1 = COMPACT1[ReadBits(pIn,12,8)];
            WriteBits( pOut, dwLookup1, 23, 8 );  dwLookup1 >>= (23-8+1);
            WriteBits( pOut, dwLookup1, 31, 31 ); dwLookup1 >>= 1;
            WriteBits( pOut, dwLookup1, 90, 89 );
           

            DWORD dwLookup2 = COMPACT2[ReadBits(pIn,17,13)];
            WriteBits( pOut, dwLookup2, 46,32 ); dwLookup2 >>= (46-32+1);
            WriteBits( pOut, dwLookup2, 63,61 ); 
            

            DWORD dwLookup3 = COMPACT3[ReadBits(pIn,22,18)];
            WriteBits(pOut, dwLookup3,52,48);dwLookup3 >>= (52-48+1);
            WriteBits(pOut, dwLookup3,68,64);dwLookup3 >>= (68-64+1);
            WriteBits(pOut, dwLookup3,100,96);

            WriteBits( pOut, ReadBits(pIn,23,23), 23,23 );
            WriteBits( pOut, ReadBits(pIn,27,24), 27,24 );
            WriteBit( pOut,0, 29 ); // clear compression control bit

            WriteBits( pOut, COMPACT4[ReadBits(pIn,34,30)], 88, 77 );
            WriteBits( pOut, ReadBits(pIn, 47,40), 60,53 );
            WriteBits( pOut, ReadBits(pIn, 55,48), 76,69 );

            DWORD dwSrc1RegFile = ReadBits(pOut, 43,42);
            if( dwSrc1RegFile == 3 )
            {
                // src1 is an immediate.  The immediate is sliced together
                //  from various fiends
                DWORD dwImmHi = COMPACT4[ReadBits(pIn,39,35)];
                DWORD dwImmLo = ReadBits(pIn,63,56);
                DWORD dwImm   = ((dwImmHi<<8)|dwImmLo);
                dwImm = SignExtend(dwImm,13);

                WriteBits( pOut, dwImm, 127, 96 ); 

            }
            else
            {
                // src1 is not an immediate
                WriteBits( pOut, COMPACT4[ReadBits(pIn,39,35)], 120, 109 );
                WriteBits( pOut, ReadBits(pIn,63,56), 108,101 );
            }

        }



        struct RegisterFields
        {
            DWORD bAlign16          ;
            DWORD dwRegFile;
            DWORD dwDataType;
            DWORD dwHStride;    
            DWORD dwVStride;   
            DWORD dwWidth;
            DWORD dwModifier;
            DWORD dwChanSel;
            DWORD bIsIndirect;
            DWORD dwRegNum;
            DWORD dwSubRegNum;
            DWORD dwAddrSubRegNum;
            int   nAddrImm;
        };
    
        struct InstructionFields
        {
            DWORD bAlign16;
            DWORD dwOpcode          ;
            DWORD bMaskControl      ;
            DWORD bNoDDChk;
            DWORD bNoDDClr;
            DWORD nQtrControl       ;
            DWORD nThreadControl    ;
            DWORD nPredControl      ;
            DWORD bPredInvert       ;
            DWORD nExecSize         ; 
            DWORD nCondModifier     ; // or msg recipient, or shared function id, or...
            DWORD bAccumWrite       ;
            DWORD bSat              ;
            DWORD dwNibControl      ;
            DWORD dwDestWriteMask   ;
            DWORD dwFlagRegNum      ;
            DWORD dwFlagSubRegNum   ;
            
            DWORD IMM32;
            QWORD IMM64;
            RegisterFields Dest;
            RegisterFields Src0;
            RegisterFields Src1;
            RegisterFields Src2;
        };

        
        void FillFlagFields( InstructionFields& rFields, const GEN::FlagReference& rFlags )
        {
            rFields.dwFlagRegNum    = rFlags.GetReg();
            rFields.dwFlagSubRegNum = rFlags.GetSubReg();
        }

        void FillPredicationFields( InstructionFields& rFields, const GEN::Predicate& rPred )
        {
            rFields.bPredInvert = rPred.IsInverted();
            if( rFields.bAlign16 )
                rFields.nPredControl = Encode_PredModesAlign16( rPred.GetMode() );
            else
                rFields.nPredControl = Encode_PredModesAlign1( rPred.GetMode() );
        }


        void FillRegFields( RegisterFields& reg,  const GEN::RegisterRegion& rRegion, DataTypes eType )
        {
            const GEN::RegReference& rReg = rRegion.GetBaseRegister();
            reg.dwDataType  = Encode_RegDataTypes(eType);
            reg.dwVStride   = Encode_VStride(rRegion.GetVStride()) ;
            reg.dwHStride   = Encode_HStride(rRegion.GetHStride());
            reg.dwWidth     = Encode_Width(rRegion.GetWidth());

            if( rReg.IsDirect() )
            {
                const GEN::DirectRegReference& rDirect = static_cast<const GEN::DirectRegReference&>( rReg);
                reg.bIsIndirect = 0;
                reg.dwRegNum    = rDirect.GetRegNumber();
                reg.dwSubRegNum = rDirect.GetSubRegOffset();
              
                if( rReg.GetRegType() == REG_GPR )
                    reg.dwRegFile = RF_GRF; 
                else
                {
                    reg.dwRegNum = Encode_ArchRegisters( rReg.GetRegType() );
                    reg.dwRegFile = RF_ARF;
                }
            }
            else
            {
                const GEN::IndirectRegReference& rIndirect = static_cast<const GEN::IndirectRegReference&>( rReg);
                reg.bIsIndirect     = 1;
                reg.dwAddrSubRegNum = rIndirect.GetAddressSubReg();
                reg.nAddrImm        = rIndirect.GetImmediateOffset();
                reg.dwRegFile       = RF_GRF;
            }
        }

        void FillSourceRegFields( RegisterFields& reg, const GEN::SourceOperand& rOperand )
        {
            reg.dwModifier = rOperand.GetModifier();
            reg.dwChanSel = rOperand.GetSwizzle().PackBits();
            if( rOperand.GetSwizzle().IsIdentity() )
                reg.bAlign16   = false;
            else
                reg.bAlign16 = true;
            
            if( rOperand.IsImmediate() )
            {
                reg.dwDataType  = Encode_ImmDataTypes(rOperand.GetDataType() );
                reg.dwRegFile   = RF_IMM;
            }
            else
            {
                FillRegFields(reg,rOperand.GetRegRegion(),rOperand.GetDataType() );
            }
        }

        void FillDestRegFields( InstructionFields& reg, const GEN::DestOperand& rOperand )
        {
            // TODO: write mask
            // TODO: sat modifier
            reg.bAlign16 = false;
            reg.dwDestWriteMask = 0xffffffff;
            reg.bSat = false;
            FillRegFields(reg.Dest,rOperand.GetRegRegion(), rOperand.GetDataType() );
        }
     

        void FillDestRegFields3Src( InstructionFields& fields, const GEN::DestOperand& rOperand )
        {
            // TODO: write mask
            // TODO: sat modifier

            auto& rDestReg = static_cast<DirectRegReference&>( rOperand.GetRegRegion().GetBaseRegister());
            fields.Dest.bAlign16 = true;
            fields.Dest.dwDataType = Encode_RegDataTypes3Src(rOperand.GetDataType());
            fields.Dest.dwRegNum = rDestReg.GetRegNumber();
            fields.Dest.dwSubRegNum = rDestReg.GetSubRegOffset();
            fields.dwDestWriteMask = 0xffffffff;
        }
    
        
        void FillSourceRegFields3Src( RegisterFields& reg, const GEN::SourceOperand& rOperand )
        {
            auto& rSrcReg = static_cast<DirectRegReference&>( rOperand.GetRegRegion().GetBaseRegister());
            reg.bAlign16 = true;
            reg.dwDataType = Encode_RegDataTypes3Src(rOperand.GetDataType());
            reg.dwRegNum = rSrcReg.GetRegNumber();
            reg.dwSubRegNum = rSrcReg.GetSubRegOffset();
            reg.dwModifier = rOperand.GetModifier();    
            reg.dwChanSel = rOperand.GetSwizzle().PackBits();
        }
    
        
        void WriteSourceRegFields( uint8* pLoc, const RegisterFields& reg, bool bAlign1 )
        {
            WriteBit( pLoc, reg.bIsIndirect, 15 );
            WriteBits( pLoc, reg.dwModifier, 14,13 );
            WriteBits( pLoc, reg.dwVStride, 24,21 );

            if( bAlign1 )
            {
                WriteBits( pLoc, reg.dwWidth, 20, 18 );
                WriteBits( pLoc, reg.dwHStride, 17,16 );
                if( reg.bIsIndirect )
                {
                    WriteBits( pLoc, reg.nAddrImm, 9,0 );
                    WriteBits( pLoc, reg.dwAddrSubRegNum,12,10);
                }
                else
                {
                    WriteBits( pLoc, reg.dwRegNum,12, 5 );
                    WriteBits( pLoc, reg.dwSubRegNum, 4, 0 );
                }
            }
            else
            {
                WriteBits( pLoc, reg.dwChanSel, 3, 0 );
                WriteBits( pLoc, reg.dwChanSel>>4, 19,16 );
                if( reg.bIsIndirect )
                {
                    WriteBits( pLoc, reg.nAddrImm>>4, 9,4 );
                    WriteBits( pLoc, reg.dwAddrSubRegNum, 12,10);
                }
                else
                {
                    WriteBits( pLoc, reg.dwRegNum, 12, 5 );
                    WriteBit( pLoc, reg.dwSubRegNum>>4,4 );
                }
            }
        }

        void WriteInstructionFields2Src( uint8* pLoc, const InstructionFields& rFields, Operations eOp )
        {
            // use align16 mode if any of our reg. references requires it
            bool bAlign16 = rFields.Dest.bAlign16 ||
                            rFields.Src0.bAlign16 ||
                            rFields.Src1.bAlign16 ||
                            rFields.Src2.bAlign16;

            WriteBits( pLoc, rFields.dwOpcode, 6,0);
            WriteBits( pLoc, bAlign16, 8, 8 );            

            WriteBits(pLoc,rFields.nPredControl, 19,16);
            WriteBit(pLoc,rFields.bPredInvert,20);
            
            WriteBits(pLoc,rFields.nThreadControl, 15,14);
            WriteBit( pLoc, rFields.bMaskControl, 9 );
            WriteBits( pLoc, (rFields.bNoDDChk<<1)|rFields.bNoDDClr, 11, 10 );
            WriteBits( pLoc, rFields.nExecSize, 23,21 );
            WriteBits( pLoc, rFields.nCondModifier, 27, 24 );
            WriteBits( pLoc, rFields.Dest.dwRegFile,  33, 32 );
            WriteBits( pLoc, rFields.Dest.dwDataType, 36, 34 );
            WriteBits( pLoc, rFields.Src0.dwRegFile,  38, 37 );
            WriteBits( pLoc, rFields.Src0.dwDataType, 41, 39 );
            WriteBits( pLoc, rFields.Src1.dwRegFile,  43, 42 );
            WriteBits( pLoc, rFields.Src1.dwDataType, 46, 44 );
    
            WriteBit( pLoc, rFields.dwFlagRegNum,    90 );
            WriteBit( pLoc, rFields.dwFlagSubRegNum, 89 );

            

            if( bAlign16 )
            {
                if( rFields.Dest.bIsIndirect )
                {
                    WriteBits(pLoc, 1, 63,63 );
                    WriteBits( pLoc, rFields.Dest.dwAddrSubRegNum, 60,58);
                    WriteBits( pLoc, rFields.Dest.nAddrImm >>4, 57,52 );
                }
                else
                {
                    WriteBits(pLoc, rFields.Dest.dwRegNum, 60,53 );
                    WriteBits(pLoc, rFields.Dest.dwSubRegNum>>3, 52,52 );
                }
                WriteBits(pLoc, rFields.dwDestWriteMask, 51,48 );
                WriteBits(pLoc, rFields.Dest.dwHStride, 62,61 );

            }
            else
            {
                if( rFields.Dest.bIsIndirect )
                {
                    WriteBits(pLoc, 1, 63,63 );
                    WriteBits( pLoc, rFields.Dest.dwAddrSubRegNum, 60,58);
                    WriteBits( pLoc, rFields.Dest.nAddrImm, 57,48 );
                }
                else
                {
                    WriteBits(pLoc, rFields.Dest.dwRegNum, 60,53 );
                    WriteBits(pLoc, rFields.Dest.dwSubRegNum, 52,48 );
                }
                     
                WriteBits(pLoc, rFields.Dest.dwHStride, 62,61 );
          
            }

            if( eOp == OP_DIM )
            {
                // double-precision immediate move
                memcpy( pLoc + 8, &rFields.IMM64, 8 );
            }
            else
            {
                // Write Src0 reg
                WriteSourceRegFields( pLoc+8, rFields.Src0, !bAlign16 );

                if( rFields.Src0.dwRegFile == RF_IMM ||
                    rFields.Src1.dwRegFile == RF_IMM ||
                    eOp == OP_SEND ||
                    eOp == OP_SENDC )
                {
                    // Write IMM32                
                    memcpy(pLoc+12, &rFields.IMM32, 4 );
                }
                else
                {
                    //Write Src1 reg
                    WriteSourceRegFields( pLoc+12, rFields.Src1, !bAlign16 );
                }
            }
        }
       
        void WriteInstructionFields3Src( uint8* pLoc, const InstructionFields& rFields, Operations eOp )
        {
           
            WriteBits( pLoc, rFields.dwOpcode, 7, 0 );
            WriteBit( pLoc, rFields.bMaskControl, 9 );
            WriteBits( pLoc, (rFields.bNoDDChk<<1)|rFields.bNoDDClr, 11, 10 );
            WriteBits( pLoc, rFields.nQtrControl, 13,12 );
            WriteBits( pLoc, rFields.nThreadControl, 15,14 );
            WriteBits( pLoc, rFields.nPredControl,19,16);
            WriteBit( pLoc, rFields.bPredInvert,20 );
            WriteBits( pLoc, rFields.nExecSize,23,21);
            WriteBits( pLoc, rFields.nCondModifier,27,24);
            WriteBit( pLoc, rFields.bAccumWrite,28 );
            WriteBit( pLoc, rFields.bAccumWrite,31 );
            
            WriteBit( pLoc, rFields.dwFlagSubRegNum, 33 );
            WriteBit( pLoc, rFields.dwFlagRegNum, 34 );
            WriteBits( pLoc, rFields.Src0.dwModifier, 37,36);
            WriteBits( pLoc, rFields.Src1.dwModifier, 39,38);
            WriteBits( pLoc, rFields.Src2.dwModifier, 41,40);
            WriteBits( pLoc, rFields.Src0.dwDataType, 43,42);
            WriteBits( pLoc, rFields.Dest.dwDataType, 45,44);
            WriteBits( pLoc, rFields.dwDestWriteMask, 52,49);
            
            WriteBits( pLoc, rFields.Dest.dwSubRegNum>>2, 55,53);
            WriteBits( pLoc, rFields.Dest.dwRegNum,       63,56);
            
            WriteBits( pLoc, rFields.Src0.dwChanSel,      72,65);
            WriteBits( pLoc, rFields.Src0.dwSubRegNum>>2, 75,73);
            WriteBits( pLoc, rFields.Src0.dwRegNum,       83,76);
            
            WriteBits( pLoc, rFields.Src1.dwChanSel,      93,86);
            WriteBits( pLoc, rFields.Src1.dwSubRegNum>>2, 95,94);
            WriteBits( pLoc, rFields.Src1.dwRegNum,       104,97);
            
            WriteBits( pLoc, rFields.Src2.dwChanSel,      114,107);
            WriteBits( pLoc, rFields.Src2.dwSubRegNum>>2, 117,115);
            WriteBits( pLoc, rFields.Src2.dwRegNum,       125,118);           
        }


        static void DecodeRegNumAndType2Src( RegisterFields& reg )
        {
            switch( reg.dwRegFile )
            {
            case RF_GRF:
                reg.dwDataType = DECODE_RegDataTypes(reg.dwDataType);
                break;
            case RF_ARF:
                reg.dwDataType = DECODE_RegDataTypes(reg.dwDataType);
                reg.dwRegNum   = DECODE_ArchRegisters(reg.dwRegNum);
                break;
            case RF_IMM:
                reg.dwDataType = DECODE_ImmDataTypes(reg.dwDataType);
                break;
            }
        }

        
     
        static void DecodePredicationMode( InstructionFields& fields )
        {
            if( fields.bAlign16 )
                fields.nPredControl = DECODE_PredModesAlign16(fields.nPredControl);
            else
                fields.nPredControl = DECODE_PredModesAlign1(fields.nPredControl);
        }
      
        void ReadSourceRegFields( RegisterFields& reg, const uint8* pIn, bool align1 )
        {
            reg.bIsIndirect = ReadBit(pIn,15);
            reg.dwModifier  = ReadBits(pIn,14,13);
            reg.dwVStride   = DECODE_VStride( ReadBits(pIn,24,21) );
            if( align1 )
            {
                reg.dwChanSel = 0xE4; // default swizzles:   3 2 1 0 -> 11 10 01 00
                reg.dwWidth   = DECODE_Width( ReadBits(pIn,20,18) );
                reg.dwHStride = DECODE_HStride( ReadBits(pIn,17,16) );
                if( reg.bIsIndirect )
                {
                    reg.nAddrImm = SignExtend( ReadBits(pIn,9,0), 9 );
                    reg.dwAddrSubRegNum = ReadBits(pIn,12,10);
                }
                else
                {
                    reg.dwRegNum = ReadBits(pIn,12,5);
                    reg.dwSubRegNum = ReadBits(pIn,4,0);
                }
            }
            else
            {
                reg.dwWidth     = 4;
                reg.dwHStride   = 1;
                reg.dwChanSel   = ReadBits(pIn,3,0)|ReadBits(pIn,19,16)<<4;
                if( reg.bIsIndirect )
                {
                    reg.nAddrImm        = SignExtend( ReadBits(pIn,9,4)<<4, 9 );
                    reg.dwAddrSubRegNum = ReadBits(pIn,12,10);
                }
                else
                {
                    reg.dwRegNum    = ReadBits(pIn,12,5);
                    reg.dwSubRegNum = ReadBit(pIn,4)<<4;
                }
            }

            DecodeRegNumAndType2Src(reg);
        }

       

        void ReadInstructionFields ( InstructionFields& fields, const uint8* pIn )
        {
            memset(&fields,0,sizeof(fields));
            fields.bAlign16 = ReadBits(pIn,8,8);
            fields.dwOpcode              = DECODE_Operations(ReadBits(pIn,7,0));
            fields.bMaskControl          = ReadBits(pIn,9,9);
            fields.bNoDDChk              = ReadBit(pIn,11);
            fields.bNoDDClr              = ReadBit(pIn,10);
            fields.nQtrControl           = ReadBits(pIn,13,12);
            fields.nThreadControl        = ReadBits(pIn,15,14);
            fields.nPredControl          = ReadBits(pIn,19,16);
            fields.bPredInvert           = ReadBits(pIn,20,20);
            fields.nExecSize             = ReadBits(pIn,23,21);
            fields.nCondModifier         = ReadBits(pIn,27,24);
            fields.bAccumWrite           = ReadBits(pIn,28,28);
            fields.bSat                  = ReadBits(pIn,31,31);

            fields.Dest.dwRegFile        = ReadBits(pIn,33,32);
            fields.Dest.dwDataType       = ReadBits(pIn,36,34);
            fields.Src0.dwRegFile        = ReadBits(pIn,38,37);
            fields.Src0.dwDataType       = ReadBits(pIn,41,39);
            fields.Src1.dwRegFile        = ReadBits(pIn,43,42);
            fields.Src1.dwDataType       = ReadBits(pIn,46,44);

            fields.dwNibControl          = ReadBit( pIn,47 );
            fields.Dest.bIsIndirect      = ReadBit( pIn,63 );
            bool align1 = fields.bAlign16 == 0;
            


            // Decode destination register
            if( align1 )
            {
                fields.dwDestWriteMask = 0xffff;
                fields.Dest.dwHStride = DECODE_HStride(ReadBits( pIn, 62,61));
                if( fields.Dest.bIsIndirect )
                {
                    fields.Dest.nAddrImm  = SignExtend( ReadBits(pIn,57,48), 9 );
                    fields.Dest.dwAddrSubRegNum = ReadBits(pIn,60,58);
                }
                else
                {
                    fields.Dest.dwRegNum    = ReadBits(pIn, 60,53);
                    fields.Dest.dwSubRegNum = ReadBits(pIn, 52,48);
                }
            }
            else
            {
                fields.Dest.dwHStride = 1;
                fields.dwDestWriteMask = ReadBits(pIn,51,48);
               
                if( fields.Dest.bIsIndirect )
                {
                    fields.Dest.nAddrImm  = SignExtend( ReadBits(pIn,57,52)<<4, 9 );
                    fields.Dest.dwAddrSubRegNum = ReadBits(pIn,60,58);
                }
                else
                {
                    fields.Dest.dwRegNum = ReadBits(pIn,60,53);
                    fields.Dest.dwSubRegNum = ReadBit(pIn,52)<<4;
                }
            }
            
            DecodeRegNumAndType2Src(fields.Dest);

            ReadSourceRegFields(fields.Src0,pIn+8, align1);
            ReadSourceRegFields(fields.Src1,pIn+12, align1);

            memcpy( &fields.IMM64, pIn+8,8);
            memcpy( &fields.IMM32, pIn+12,4);
            
            fields.dwFlagRegNum    = ReadBit(pIn,90);
            fields.dwFlagSubRegNum = ReadBit(pIn,89);

            // Translate field values, and compute derived fields
            fields.nExecSize = DECODE_ExecSize(fields.nExecSize);
            DecodePredicationMode(fields);

            switch( fields.dwOpcode )
            {
            case OP_MATH:
                fields.nCondModifier = DECODE_MathFunctions(fields.nCondModifier);
                break;
            case OP_SEND:
                fields.nCondModifier = DECODE_SharedFunctions(fields.nCondModifier);
                break;
            default:
                fields.nCondModifier = DECODE_CondModifiers(fields.nCondModifier);
                break;
            }
         
            // dest width/stride follows from exec size 
            fields.Dest.dwWidth   = fields.nExecSize;
            fields.Dest.dwVStride = fields.Dest.dwWidth*fields.Dest.dwHStride;
           
            fields.Dest.bAlign16 = fields.bAlign16;
            fields.Src0.bAlign16 = fields.bAlign16;
            fields.Src1.bAlign16 = fields.bAlign16;
            fields.Src2.bAlign16 = fields.bAlign16;

            if( fields.Src1.dwRegFile == RF_IMM ) 
                fields.Src1.dwModifier = 0;
            if( fields.Src0.dwRegFile == RF_IMM )
                fields.Src0.dwModifier = 0;

        }

        void ReadInstructionFieldsThreeSrc( InstructionFields& fields, const unsigned char* pIn )
        {
            memset(&fields,0,sizeof(fields));
            fields.Src0.bAlign16 = true;
            fields.Src1.bAlign16 = true;
            fields.Src2.bAlign16 = true;
            fields.Dest.bAlign16 = true;
            fields.bAlign16 = true;
            fields.dwOpcode              = ReadBits(pIn,7,0);
            fields.bMaskControl          = ReadBits(pIn,9,9);
            fields.bNoDDChk              = ReadBit(pIn,11);
            fields.bNoDDClr              = ReadBit(pIn,10);
            fields.nQtrControl           = ReadBits(pIn,13,12);
            fields.nThreadControl        = ReadBits(pIn,15,14);
            fields.nPredControl          = ReadBits(pIn,19,16);
            fields.bPredInvert           = ReadBits(pIn,20,20);
            fields.nExecSize             = ReadBits(pIn,23,21);
            fields.nCondModifier         = ReadBits(pIn,27,24);
            fields.bAccumWrite           = ReadBits(pIn,28,28);
            fields.bSat                  = ReadBits(pIn,31,31);

            fields.dwFlagSubRegNum = ReadBit(pIn,33);
            fields.dwFlagRegNum = ReadBit(pIn,34);
            
            fields.Src0.dwModifier = ReadBits(pIn,37,36);
            fields.Src1.dwModifier = ReadBits(pIn,39,38);
            fields.Src2.dwModifier = ReadBits(pIn,41,40);
            
            DWORD dwSrcType = DECODE_RegDataTypes3Src(ReadBits(pIn,43,42));
            fields.Src0.dwDataType = dwSrcType;
            fields.Src1.dwDataType = dwSrcType;
            fields.Src2.dwDataType = dwSrcType;
            fields.Dest.dwDataType = DECODE_RegDataTypes3Src(ReadBits(pIn,45,44));

            fields.dwDestWriteMask = ReadBits(pIn,52,49);
  
            fields.Dest.dwRegFile = RF_GRF;
            fields.Src0.dwRegFile = RF_GRF;
            fields.Src1.dwRegFile = RF_GRF;
            fields.Src2.dwRegFile = RF_GRF;

            fields.Dest.dwRegNum        = ReadBits(pIn,63,56);
            fields.Dest.dwSubRegNum     = ReadBits(pIn,55,53)<<2;

            fields.Src0.dwChanSel       = ReadBits(pIn, 72,65 );
            fields.Src0.dwSubRegNum     = ReadBits(pIn,75,73)<<2;
            fields.Src0.dwRegNum        = ReadBits(pIn,83,76);
            
            fields.Src1.dwChanSel       = ReadBits(pIn,93,86);
            fields.Src1.dwSubRegNum     = ReadBits(pIn,95,94)<<2;
            fields.Src1.dwRegNum        = ReadBits(pIn,104,97);

            fields.Src2.dwChanSel       = ReadBits(pIn,114,107);
            fields.Src2.dwSubRegNum     = ReadBits(pIn,117,115)<<2;
            fields.Src2.dwRegNum        = ReadBits(pIn,125,118);

            fields.Src0.dwVStride = 8;
            fields.Src1.dwVStride = 8;
            fields.Src2.dwVStride = 8;
            fields.Src0.dwWidth   = 8;
            fields.Src1.dwWidth   = 8;
            fields.Src2.dwWidth   = 8;
            fields.Src0.dwHStride = 1;
            fields.Src1.dwHStride = 1;
            fields.Src2.dwHStride = 1;
            fields.Dest.dwHStride = 1;
            fields.Dest.dwVStride = 8;
            fields.Dest.dwWidth   = 8;

            fields.nExecSize = DECODE_ExecSize(fields.nExecSize);
            


            DecodePredicationMode(fields);
        }
      


        GEN::RegisterRegion InterpretRegisterRegion( RegisterFields& reg )
        {
            DWORD dwWidth   = reg.dwWidth ;
            DWORD dwHStride = reg.dwHStride;
            DWORD dwVStride = reg.dwVStride;

            if( reg.dwRegFile == RF_ARF )
            {
                RegTypes eRegType = (RegTypes)(reg.dwRegNum);
                return GEN::RegisterRegion( GEN::DirectRegReference( eRegType, 0, reg.dwSubRegNum ), 
                                            dwVStride, dwWidth, dwHStride );
            }
            else
            {
                if( reg.bIsIndirect )
                {
                     // TODO: Handle the indirect VxH and Vx1 voodoo
                    return GEN::RegisterRegion( GEN::IndirectRegReference( reg.nAddrImm, reg.dwAddrSubRegNum ),
                                                dwVStride, dwWidth, dwHStride );
                }
                else
                {
                    return GEN::RegisterRegion( GEN::DirectRegReference( GEN::REG_GPR, reg.dwRegNum, reg.dwSubRegNum ),
                                                dwVStride, dwWidth, dwHStride  );
                }
            }
        }

        GEN::SourceOperand InterpretSource( RegisterFields& rReg )
        {
            SourceModifiers eMod = (SourceModifiers) rReg.dwModifier;
            if( rReg.dwRegFile == RF_IMM )
            {
                DataTypes eDataType = (DataTypes) rReg.dwDataType ;
                return GEN::SourceOperand(eDataType, eMod);
            }
            else
            {
                DataTypes eDataType = (DataTypes) rReg.dwDataType ;
                return GEN::SourceOperand(eDataType,InterpretRegisterRegion(rReg), GEN::Swizzle(rReg.dwChanSel), eMod);
            }
        }

        GEN::DestOperand InterpretDest( InstructionFields& rInstruction )
        {
            DataTypes eDataType = (DataTypes) rInstruction.Dest.dwDataType ;
            return GEN::DestOperand(eDataType,InterpretRegisterRegion(rInstruction.Dest), rInstruction.dwDestWriteMask);
        }

        GEN::FlagReference InterpretFlagReference( InstructionFields& rInst )
        {
            return GEN::FlagReference( rInst.dwFlagRegNum, rInst.dwFlagSubRegNum );
        }


    }



    Operations Decoder::GetOperation( const uint8* pInstructionBytes )
    {
        return _INTERNAL::DECODE_Operations(_INTERNAL::ReadDWORD( pInstructionBytes ) & _INTERNAL::OPCODE_MASK);
    }

   
    size_t Decoder::DetermineLength( const uint8* pInstructionBytes )
    {
        uint32 nDWORD = _INTERNAL::ReadDWORD( pInstructionBytes );
        Operations eOp = _INTERNAL::DECODE_Operations(nDWORD & _INTERNAL::OPCODE_MASK);
        switch( eOp )
        {
        case NOT_AN_OP:
            return 0;

            // The Intel PRM states that 'illegal' can't be compressed
            //    I beg to differ.  8 bytes of zero immediately following a compressed
            //        instruction might be seemingly interpretted as a compressed ILLEGAL
            //    Returning the shorter length is safer.  At worst, we'll see two 'ILLEGAL'
            //        instructions where we should have seen one
        case OP_ILLEGAL:
            return 8; 

            // nop, math, send are all special puppies
            //  Docs claim that these are not permitted to be compressed
        case OP_NOP: 
        case OP_SEND:
        case OP_SENDC:
            return 16;
           
        default:
            // docs claim that 3-source ops can't be compressed
            if( _INTERNAL::IsBasicThreeSource(eOp) )
                return 16;

            // for everything else, check the compression control bit
            return _INTERNAL::IsCompressedInstruction(nDWORD) ? 8 : 16;
        }
    }

    size_t Decoder::Expand( uint8* pOutput, const uint8* pInput )
    {
        uint32 nDWORD  = _INTERNAL::ReadDWORD( pInput );
        uint32 nOpcode = nDWORD & _INTERNAL::OPCODE_MASK;
        Operations eOp = _INTERNAL::DECODE_Operations(nOpcode);
     
        switch( eOp )
        {
        case NOT_AN_OP:
            return 0;

            // The Intel PRM states that 'illegal' can't be compressed
            //    I beg to differ.  8 bytes of zero immediately following a compressed
            //        instruction might be seemingly interpretted as a compressed ILLEGAL
            //    Returning the shorter length is safer.  At worst, we'll see two 'ILLEGAL'
            //        instructions where we should have seen one
        case OP_ILLEGAL:
            memset( pOutput,0,16 );
            return 8; 

            // nop, send are all special puppies
            //  Docs claim that these are not permitted to be compressed
        case OP_NOP: 
        case OP_SEND:
        case OP_SENDC:
            memcpy( pOutput, pInput, 16 );
            return 16;
           
        default:
            // docs claim that 3-source ops can't be compressed
            if( _INTERNAL::IsBasicThreeSource(eOp) )
            {
                memcpy( pOutput, pInput, 16 );
                return 16;
            }

            // for everything else, check the compression control bit
            if( _INTERNAL::IsCompressedInstruction(nDWORD) )
            {
                _INTERNAL::ExpandCompressedInstruction( pOutput, pInput );
                return 8;
            }
            else
            {
                memcpy( pOutput, pInput, 16 );
                return 16;
            }
        }
    }


    /// Build a high-level representation of an instruction
    size_t Decoder::Decode( Instruction* pInst, const uint8* pInstructionBytes )
    {
        uint32 nDWORD  = _INTERNAL::ReadDWORD( pInstructionBytes );
        uint32 nOpcode = nDWORD & _INTERNAL::OPCODE_MASK;
        Operations eOp = _INTERNAL::DECODE_Operations(nOpcode);
                
        memset(pInst,0,sizeof(Instruction));
        pInst->m_eClass = IC_NULL;
        pInst->m_eOp    = eOp;

        switch( eOp )
        {
        case NOT_AN_OP:
            return 0;

        case OP_ILLEGAL:
            {
                pInst->m_eClass = IC_NULL;
                pInst->m_eOp    = OP_ILLEGAL;
                pInst->m_nExecSize = 0;
                return 8; 
            }
            break;

        case OP_NOP:
            {
                pInst->m_eClass = IC_NULL;
                pInst->m_eOp    = OP_NOP;
                pInst->m_nExecSize = 0;
                return 16;
            }
            break;

      

        case OP_SEND:
        case OP_SENDC:
            {
                return DecodeSend(pInst,pInstructionBytes,eOp);
            }
            break;

        default:
            {
                if( _INTERNAL::IsBasicThreeSource( eOp ) )
                {
                    return Decode3Source( pInst, pInstructionBytes, eOp );
                }
                else 
                {
                    if( _INTERNAL::IsCompressedInstruction(nDWORD) )
                    {
                        uint8 pNativeOp[16];
                        _INTERNAL::ExpandCompressedInstruction( pNativeOp, pInstructionBytes );
                        if( DecodeNativeOp(pInst,pNativeOp,eOp) )
                            return 8;
                        else
                            return 0;
                    }
                    else
                    {
                        if( DecodeNativeOp(pInst,pInstructionBytes,eOp) )
                            return 16;
                        else
                            return 0;
                    }

                }
            }
            break;
        }
    }

   


    size_t Decoder::Decode3Source( Instruction* pInst, const uint8* pIn, Operations eOp)
    {
        pInst->m_eOp    = eOp;
        pInst->m_eClass = IC_TERNARY;
      
        _INTERNAL::InstructionFields fields;
        _INTERNAL::ReadInstructionFieldsThreeSrc( fields, pIn );

        pInst->m_nExecSize    = fields.nExecSize;
        pInst->m_bNoWriteMask = fields.bMaskControl;
        pInst->m_bNoDDChk     = fields.bNoDDChk;
        pInst->m_Dest         = _INTERNAL::InterpretDest( fields );
        pInst->m_Source0      = _INTERNAL::InterpretSource(fields.Src0);
        pInst->m_Source1      = _INTERNAL::InterpretSource(fields.Src1);
        pInst->m_Source2      = _INTERNAL::InterpretSource(fields.Src2);
        pInst->m_Predicate.Set( (PredicationModes) fields.nPredControl, fields.bPredInvert !=0);
        pInst->m_Flags         = _INTERNAL::InterpretFlagReference(fields);
        return 16;
    }

    bool Decoder::DecodeNativeOp( Instruction* pInst, const uint8* pInstructionBytes, Operations eOp )
    {
        _INTERNAL::InstructionFields fields;
        _INTERNAL::ReadInstructionFields( fields, pInstructionBytes );

        pInst->m_eOp          = eOp;
        pInst->m_nExecSize    = fields.nExecSize;
        pInst->m_bNoWriteMask = fields.bMaskControl;
        pInst->m_bNoDDChk     = fields.bNoDDChk;
        pInst->m_Predicate.Set( (PredicationModes) fields.nPredControl, fields.bPredInvert!=0 );
        pInst->m_Flags         = _INTERNAL::InterpretFlagReference(fields);
        if( eOp == OP_DIM )
            memcpy( pInst->m_ImmediateOperand, &fields.IMM64, 8 );
        else
            memcpy( pInst->m_ImmediateOperand, &fields.IMM32, 4 );
        
        switch( eOp )
        {
        case OP_DIM     : // double-precision immediate move
            break;
        case OP_MATH:
            DecodeMath(pInst,pInstructionBytes);
            return true;

        case OP_MOV     :
        case OP_MOVI    :
        case OP_NOT     :
        case OP_ASR     : // arithmetic right shift
        case OP_F32TO16 :
        case OP_F16TO32 :
        case OP_BFREV   : // bit reverse
        case OP_FRC     :
        case OP_RNDU    : // round up (ceil)
        case OP_RNDD    : // round down (floor)
        case OP_RNDE    : // round-nearest even
        case OP_RNDZ    : // round toward 0
        case OP_FBH     : // first bit high
        case OP_FBL     : // "  " low
        case OP_CBIT    : // count bits
            {
                // one source arithmetic
                pInst->m_eClass  = IC_UNARY;
                pInst->m_Dest    = _INTERNAL::InterpretDest(fields);
                pInst->m_Source0 = _INTERNAL::InterpretSource(fields.Src0);
                pInst->m_eCondModifier = fields.nCondModifier;
                return pInst->m_Dest.IsValid() && pInst->m_Source0.IsValid();
            }
            break;
            
        case OP_ADD     :
        case OP_MUL     :
        case OP_AVG     :
        case OP_MAC     :
        case OP_MACH    : // mul-accumulate high
        case OP_SEL     :
        case OP_AND     :
        case OP_OR      :
        case OP_XOR     :
        case OP_SHR     :
        case OP_SHL     :
        case OP_CMP     : //Component-wise compare, store condition code in destination
        case OP_CMPN    : //Component-wise nan, store condition code in destination
        case OP_LZD     : // leading zero detection
        case OP_ADDC    :
        case OP_SUBB    :
        case OP_SAD2    :
        case OP_SADA2   : // SAD-accumulate
        case OP_DP4     :
        case OP_DPH     : // DP4 homogeneous
        case OP_DP3     :
        case OP_DP2     :
        case OP_BFI1    : // bit-field insert macro-op 1
        case OP_BFI2    : // """ 2
        case OP_LINE    : // line evaluation
        case OP_PLN     : // plane evaluation
            {
                // two source arithmetic
                pInst->m_eClass         = IC_BINARY;
                pInst->m_Dest           = _INTERNAL::InterpretDest(fields);
                pInst->m_Source0        = _INTERNAL::InterpretSource(fields.Src0);
                pInst->m_Source1        = _INTERNAL::InterpretSource(fields.Src1);
                pInst->m_eCondModifier  = fields.nCondModifier;
                return pInst->m_Dest.IsValid() && pInst->m_Source0.IsValid() && pInst->m_Source1.IsValid();
            }
            break; // two source arithmetic

            // TODO....

            // 'if' instruction has UIP and JIP
            // 
            //  else 
        case OP_IF      :
        case OP_ELSE    :
        case OP_WHILE   :
        case OP_BREAK   :
        case OP_CONT    :
        case OP_ENDIF   :
            {
                const int16* pWords = (const int16*)pInstructionBytes;
                int16 JIP = pWords[6];
                int16 UIP = pWords[7];
                pInst->m_BranchOffsets.JIP = JIP;
                pInst->m_BranchOffsets.UIP = UIP;
                pInst->m_eClass = IC_BRANCH;
                return true;
            }
            break;
        

            // one source banch
        case OP_JMPI    : // indexed jump
        case OP_BRD     : // diverging branch
        case OP_BRC     : // converging branch
        case OP_HALT    :
        case OP_CALLA   : // call absolute
        case OP_CALL    :
        case OP_RETURN  :
            break;
        
        case OP_CASE    :
            break;

        case OP_WAIT    :
            break;
        }

        return true;
    }

   

    size_t Decoder::DecodeSend( Instruction* pInst, const uint8* pInstructionBytes, Operations eO )
    {
        pInst->m_eClass = IC_SEND;
        pInst->m_eOp = eO;

        _INTERNAL::InstructionFields fields;
        _INTERNAL::ReadInstructionFields(fields,pInstructionBytes);

        uint32 dwMessageDescriptor = fields.IMM32 & 0x1fffffff;
        memcpy( pInst->m_ImmediateOperand, &dwMessageDescriptor, sizeof(dwMessageDescriptor) );

        bool bMsgDescriptorFromReg = fields.Src1.dwRegFile != _INTERNAL::RF_IMM;
        bool bEOT = (fields.IMM32 & 0x80000000) != 0;
        pInst->m_bMsgDescriptorFromReg = bMsgDescriptorFromReg;
        pInst->m_Dest           = _INTERNAL::InterpretDest(fields);
        pInst->m_Source0        = _INTERNAL::InterpretSource(fields.Src0);
        pInst->m_eSFID          = fields.nCondModifier;
        pInst->m_nExecSize      = fields.nExecSize;
        pInst->m_bNoWriteMask   = fields.bMaskControl;
        pInst->m_bEOT           = bEOT;
        pInst->m_Predicate.Set( (PredicationModes) fields.nPredControl, fields.bPredInvert!=0 );
        pInst->m_Flags         = _INTERNAL::InterpretFlagReference(fields);
        return 16;
    }

    size_t Decoder::DecodeMath( Instruction* pInst, const uint8* pInstructionBytes ) 
    {
        pInst->m_eClass = IC_MATH;
        pInst->m_eOp    = OP_MATH;
        
        
        _INTERNAL::InstructionFields fields;
        _INTERNAL::ReadInstructionFields(fields,pInstructionBytes);

        pInst->m_eFunctionCtrl  = fields.nCondModifier;
        pInst->m_Dest           = _INTERNAL::InterpretDest(fields);
        pInst->m_Source0        = _INTERNAL::InterpretSource(fields.Src0);
        pInst->m_Source1        = _INTERNAL::InterpretSource(fields.Src1);
        pInst->m_nExecSize      = fields.nExecSize;
        pInst->m_bNoWriteMask   = fields.bMaskControl;
        pInst->m_Predicate.Set( (PredicationModes) fields.nPredControl, fields.bPredInvert !=0);
        pInst->m_Flags         = _INTERNAL::InterpretFlagReference(fields);
        return 16;
    }

    



    size_t Encoder::GetBufferSize( size_t nOps )
    {
        return nOps*16;
    }


    size_t Encoder::Encode( void* pOutputBuffer, const GEN::Instruction* pOps, size_t nOps )
    {
        uint8* pOutputBytes = (uint8*)pOutputBuffer;
        for( size_t i=0; i<nOps; i++ )
        {
            const Instruction& rInst = pOps[i];

            _INTERNAL::InstructionFields fields;
            memset(&fields,0,sizeof(fields));
            fields.bMaskControl = rInst.IsWriteMaskDisabled();
            fields.bNoDDChk = rInst.IsDDCheckDisabled();
            
            // TODO: Handle Other conditions where we must pick align16 or align1
            fields.bAlign16 = false;
            switch( rInst.GetPredicate().GetMode() )
            {
            case PM_SWIZZLE_X:
            case PM_SWIZZLE_Y:
            case PM_SWIZZLE_Z:
            case PM_SWIZZLE_W:
                fields.bAlign16 = true;
                break;
            }

            _INTERNAL::FillFlagFields( fields, rInst.GetFlagReference() );
            _INTERNAL::FillPredicationFields( fields, rInst.GetPredicate() );

            memset( pOutputBytes, 0, 16 );

            switch( pOps[i].GetClass() )
            {
            case IC_UNARY:
                {
                    const UnaryInstruction& it = static_cast<const UnaryInstruction&> (rInst);
                    fields.dwOpcode  = _INTERNAL::Encode_Operations(rInst.GetOperation());
                    fields.nExecSize = _INTERNAL::Encode_ExecSize(it.GetExecSize());
                    fields.IMM32 = it.GetImmediate<uint32>();
                    fields.IMM64 = it.GetImmediate<uint64>();
                    fields.nCondModifier = _INTERNAL::Encode_CondModifiers(it.GetConditionModifier());
                    _INTERNAL::FillDestRegFields( fields, it.GetDest() );
                    _INTERNAL::FillSourceRegFields( fields.Src0, it.GetSource0() );
                    _INTERNAL::WriteInstructionFields2Src(pOutputBytes, fields, rInst.GetOperation() );
                }
                break;
            case IC_BINARY:
                {
                    const BinaryInstruction& it = static_cast<const BinaryInstruction&> (rInst);
                    fields.dwOpcode  = _INTERNAL::Encode_Operations(rInst.GetOperation());
                    fields.nExecSize = _INTERNAL::Encode_ExecSize(it.GetExecSize());
                    fields.nCondModifier = _INTERNAL::Encode_CondModifiers(it.GetConditionModifier());
                    fields.IMM32 = it.GetImmediate<uint32>();
                    fields.IMM64 = it.GetImmediate<uint64>();
                    _INTERNAL::FillDestRegFields( fields, it.GetDest() );
                    _INTERNAL::FillSourceRegFields( fields.Src0, it.GetSource0() );
                    _INTERNAL::FillSourceRegFields( fields.Src1, it.GetSource1() );
                  
                    // Docs state that the 'switch threads' threadmode must be used if dest is null
                    //  Smells like a HW bug.  Force it here just in case
                    switch( rInst.GetOperation() )
                    {
                    case OP_CMP:
                    case OP_CMPN:
                        {
                            if( it.GetDest().GetRegRegion().GetBaseRegister().GetRegType() == REG_NULL )
                                fields.nThreadControl = TC_SWITCH;
                        }
                        break;
                    }

                    _INTERNAL::WriteInstructionFields2Src(pOutputBytes, fields, rInst.GetOperation() );
                }
                break;
            case IC_TERNARY:
                {
                    const TernaryInstruction& it = static_cast<const TernaryInstruction&> (rInst);
                    fields.dwOpcode  = _INTERNAL::Encode_Operations(rInst.GetOperation());
                    fields.nExecSize = _INTERNAL::Encode_ExecSize(it.GetExecSize());
                    fields.nCondModifier = _INTERNAL::Encode_CondModifiers(it.GetConditionModifier());
                    _INTERNAL::FillDestRegFields3Src( fields, it.GetDest() );
                    _INTERNAL::FillSourceRegFields3Src( fields.Src0, it.GetSource0() );
                    _INTERNAL::FillSourceRegFields3Src( fields.Src1, it.GetSource1() );
                    _INTERNAL::FillSourceRegFields3Src( fields.Src2, it.GetSource2() );
                    
                    _INTERNAL::WriteInstructionFields3Src(pOutputBytes, fields, rInst.GetOperation() );
                }
                break;

            case IC_MATH:
                {
                    const MathInstruction& it = static_cast<const MathInstruction&>( rInst );
                    fields.dwOpcode         = _INTERNAL::Encode_Operations(OP_MATH);
                    fields.nExecSize        = _INTERNAL::Encode_ExecSize(it.GetExecSize());
                    fields.nCondModifier    = _INTERNAL::Encode_MathFunctions(it.GetFunction());
                    fields.IMM32 = it.GetImmediate<uint32>();
                    fields.IMM64 = it.GetImmediate<uint64>();
                 
                    _INTERNAL::FillDestRegFields( fields, it.GetDest() );
                    _INTERNAL::FillSourceRegFields( fields.Src0, it.GetSource0() );
                    _INTERNAL::FillSourceRegFields( fields.Src1, it.GetSource1() );
                    
                    _INTERNAL::WriteInstructionFields2Src(pOutputBytes, fields, rInst.GetOperation() );
                  
                }
                break;

            case IC_NULL:
                {
                    memset( pOutputBytes,0,16 );
                    _INTERNAL::WriteBits( pOutputBytes,_INTERNAL::Encode_Operations( rInst.GetOperation() ),6,0 );
                }
                break;
            case IC_BRANCH:
                {
                    // TODO
                }
                break;

            case IC_SEND:
                {
                    const SendInstruction& it = static_cast<const SendInstruction&>( rInst );
                    fields.dwOpcode         = _INTERNAL::Encode_Operations(it.GetOperation());
                    fields.nExecSize        = _INTERNAL::Encode_ExecSize(it.GetExecSize());
                    fields.nCondModifier    = _INTERNAL::Encode_SharedFunctions(it.GetRecipient());
                    fields.IMM32  = it.GetDescriptorIMM();
                    if( it.IsEOT() )
                        fields.IMM32 |= 0x80000000;

                    _INTERNAL::FillDestRegFields( fields, it.GetDest() );
                    _INTERNAL::FillSourceRegFields( fields.Src0, it.GetSource() );
                    if( !it.IsDescriptorInRegister() )
                        fields.Src1.dwRegFile = _INTERNAL::RF_IMM;

                    _INTERNAL::WriteInstructionFields2Src(pOutputBytes, fields, rInst.GetOperation() );
                }
                break;
            }

                
            pOutputBytes += 16;
        }

        return 16*nOps; // TODO: Compression
    }
}
