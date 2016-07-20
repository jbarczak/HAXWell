
#ifndef _GEN_ISA_H_
#define _GEN_ISA_H_

#include <new>

namespace GEN
{
    typedef unsigned __int64 uint64;
    typedef unsigned int     uint32;
    typedef int int32;
    typedef unsigned short   uint16;
    typedef unsigned char    uint8;
    typedef short int16;

    static_assert( sizeof(uint64) == 8, "derp" );
    static_assert( sizeof(uint32) == 4, "derp" );
    static_assert( sizeof(int32)  == 4, "derp" );
    static_assert( sizeof(uint16) == 2, "derp" );
    static_assert( sizeof(int16)  == 2, "derp" );
    static_assert( sizeof(uint8)  == 1, "derp" );


    enum Operations
    {
        OP_ILLEGAL = 0,
        OP_MOV     ,
        OP_SEL     ,
        OP_MOVI    ,
        OP_NOT     ,
        OP_AND     ,
        OP_OR      ,
        OP_XOR     ,
        OP_SHR     ,
        OP_SHL     ,
        OP_DIM     , // double-precision immediate move
        OP_ASR     , // arithmetic right shift
        OP_CMP     , //Component-wise compare, store condition code in destination
        OP_CMPN    , //Component-wise nan, store condition code in destination
        OP_CSEL    , // 3-src conditional select
        OP_F32TO16 ,
        OP_F16TO32 ,
        OP_BFREV   , // bit reverse
        OP_BFE     ,
        OP_BFI1    , // bit-field insert macro-op 1
        OP_BFI2    , // """ 2
        OP_JMPI    , // indexed jump
        OP_BRD     , // diverging branch
        OP_IF      ,
        OP_BRC     , // converging branch
        OP_ELSE    ,
        OP_ENDIF   ,
        OP_CASE    ,
        OP_WHILE   ,
        OP_BREAK   ,
        OP_CONT    ,
        OP_HALT    ,
        OP_CALLA   , // call absolute
        OP_CALL    ,
        OP_RETURN  ,
        OP_WAIT    ,
        OP_SEND    ,
        OP_SENDC   ,
        OP_MATH    ,
        OP_ADD     ,
        OP_MUL     ,
        OP_AVG     ,
        OP_FRC     ,
        OP_RNDU    , // round up (ceil)
        OP_RNDD    , // round down (floor)
        OP_RNDE    , // round-nearest even
        OP_RNDZ    , // round toward 0
        OP_MAC     ,
        OP_MACH    , // mul-accumulate high
        OP_LZD     , // leading zero detection
        OP_FBH     , // first bit high
        OP_FBL     , // "  " low
        OP_CBIT    , // count bits
        OP_ADDC    ,
        OP_SUBB    ,
        OP_SAD2    ,
        OP_SADA2   , // SAD-accumulate
        OP_DP4     ,
        OP_DPH     , // DP4 homogeneous
        OP_DP3     ,
        OP_DP2     ,
        OP_LINE    , // line evaluation
        OP_PLN     , // plane evaluation
        OP_FMA     ,
        OP_LRP     ,
        OP_NOP     ,
        
        OP_COUNT,
        NOT_AN_OP = OP_COUNT  // must be last

        //
        // ILLEGAL is not the same as 'NOT_AN_OP'
        //   'ILLEGAL' is a well defined operation in the Gen architecture
        //     its meaning is "throw an exception", and it could be used for
        //     padding between subroutines
        //
        //  NOT_AN_OP is what this codebase uses for things it doesn't recognize
        //   throwing an exception when executed.  ILLEGAL is a real instruction
        //   intended for padding.  
    };

    enum MathFunctionIDs
    {
        MATH_INVERSE,
        MATH_LOG,
        MATH_EXP,
        MATH_SQRT,
        MATH_RSQ,
        MATH_SIN,
        MATH_COS,
        MATH_FDIV,
        MATH_POW,
        MATH_IDIV_BOTH,
        MATH_IDIV_QUOTIENT,
        MATH_IDIV_REMAINDER,
        MATH_INVALID
    };


    enum ConditionalModifiers
    {
        CM_NONE             = 0x00,
        CM_ZERO             = 0x01,
        CM_NOTZERO          = 0x02,
        CM_GREATER_THAN     = 0x03,
        CM_GREATER_EQUAL    = 0x04,
        CM_LESS_THAN        = 0x05,
        CM_LESS_EQUAL       = 0x06,
        CM_SIGNED_OVERFLOW  = 0x08,
        CM_UNORDERED        = 0x09,
    };
    enum AddressModes 
    {
        ACCESS_DIRECT=0x0,
        ACCESS_INDEXED=0x1,
    };
    enum RegAccessModes
    {
        AM_DIRECT    = 0, // Byte-aligned register addressing.  Source swizzle control and dst mask control are not supported
        AM_INDIRECT  = 1,
    };

    enum SourceModifiers : uint8
    {
        SM_NONE=0,
        SM_ABS =1,
        SM_NEGATE=2,
        SM_NEG_ABS=3,
    };

    enum DataTypes : uint8
    {
        // These are setup to match the
        //  Encoding values for source registers
        //   But there are different type codes for immediates
        DT_U32,
        DT_S32,
        DT_U16,
        DT_S16,
        DT_U8,
        DT_S8,
        DT_F64,
        DT_F32,
        DT_VEC_HALFBYTE_UINT,
        DT_VEC_HALFBYTE_SINT,   // immediates only
        DT_VEC_HALFBYTE_FLOAT,
        DT_INVALID,
    };

   

    enum ThreadControl : uint8
    {
        TC_NONE,
        TC_ATOMIC = 0x1, // Give this instruction maximum scheduling priority
        TC_SWITCH = 0x2, // Switch threads after executing this intruction
    };


    enum RegTypes : uint8
    {
        REG_NULL,
        REG_ADDRESS,
        REG_ACCUM0, 
        REG_ACCUM1,
        REG_FLAG0,
        REG_FLAG1,
        REG_CHANNEL_ENABLE, // 32-bit execution mask
        REG_STACK_PTR,
        REG_STATE,
        REG_CONTROL,
        REG_NOTIFICATION0,
        REG_NOTIFICATION1,
        REG_INSTRUCTION_PTR,
        REG_THREAD_DEPENDENCY, // thread dependency reg
        REG_TIMESTAMP,

        REG_FC0, // flow control registers.
        REG_FC1, // flow control registers.
        REG_FC2, // flow control registers.
        REG_FC3, // flow control registers.
        REG_FC4, // flow control registers.
        REG_FC5, // flow control registers.
        REG_FC6, // flow control registers.
        REG_FC7, // flow control registers.
        REG_FC8, // flow control registers.
        REG_FC9, // flow control registers.
        REG_FC10, // flow control registers.
        REG_FC11, // flow control registers.
        REG_FC12, // flow control registers.
        REG_FC13, // flow control registers.
        REG_FC14, // flow control registers.
        REG_FC15, // flow control registers.
        // TODO: Docs claim there's 39 FC registers
        //   but if that's the case, they're not all addressible

        REG_GPR,
        REG_IMM, // sentinel for an immediate.  Not valid for dest registers
        REG_INVALID,
    };


    // Shared function IDs for 'send' instruction
    enum SharedFunctionIDs : uint8
    {
        SFID_NULL ,
        SFID_SAMPLER ,
        SFID_GATEWAY ,
        SFID_DP_SAMPLER, // sampler cache dataport
        SFID_DP_RC, // render cache dataport
        SFID_URB,
        SFID_SPAWNER,
        SFID_VME,
        SFID_DP_CC, // constant cache dataport
        SFID_DP_DC0,
        SFID_DP_DC1, // DC1 is the same recipient as DC0.  Intel just ran out of message bits.
        SFID_PI, // pixel interpolator
        SFID_CRE, // check and refinement engine
        SFID_INVALID
    };

    enum PredicationModes : uint8
    {
        PM_NONE,
        PM_SEQUENTIAL_FLAG, 
        PM_SWIZZLE_X,       // swizzle modes are align16 only
        PM_SWIZZLE_Y,
        PM_SWIZZLE_Z,
        PM_SWIZZLE_W, 
        PM_ANY4H,
        PM_ALL4H,

        // modes below are align1 only
        PM_ANYV,
        PM_ALLV,
        PM_ANY2H,
        PM_ALL2H,
        PM_ANY8H,
        PM_ALL8H,
        PM_ANY16H,
        PM_ALL16H,
        PM_ANY32H,
        PM_ALL32H,

    };
 

    enum InstructionClass : uint8
    {
        IC_NULL,            // Used for NOPs, ILLEGALs, and uninitialized 'Instruction' instances
        IC_UNARY,           // one source
        IC_BINARY,          // two source --> Same layout as 1src but second src is redundant
        IC_TERNARY,         // three source
        IC_SEND,
        IC_MATH,            // 'Math' is special because there are dual-dest ops there
        IC_BRANCH
    };


    struct Swizzle
    {
        Swizzle() : x(0), y(1), z(2), w(3){};
        Swizzle( uint32 packed )
            :x(packed&3),
             y((packed>>2)&3),
             z((packed>>4)&3),
             w((packed>>6)&3)
        {
        }
        unsigned x : 2;
        unsigned y : 2;
        unsigned z : 2;
        unsigned w : 2;

        bool IsIdentity() const { return x == 0 && y == 1 && z == 2 && w == 3; }
        uint32 PackBits() const { return x | (y<<2) | (z<<4) | (w<<6);};
    };

    class Predicate
    {
    public:
        Predicate() : m_eMode(PM_NONE), m_bInvert(0){}

        void Set( PredicationModes eMode, bool bInvert )
        {
            m_eMode   = eMode;
            m_bInvert = bInvert;
        }
        PredicationModes GetMode() const { return (PredicationModes)m_eMode; }
        bool IsInverted() const { return m_bInvert; }

    private:
        unsigned m_eMode : 7;
        unsigned m_bInvert : 1;
    };

    class FlagReference
    {
    public:
        FlagReference() : m_nReg(0), m_nSubReg(0){}
        FlagReference( size_t nReg, size_t nSubReg ) : m_nReg(nReg), m_nSubReg(nSubReg){}

        void Set( size_t nReg, size_t nSubReg ) { 
            m_nReg = nReg;
            m_nSubReg = nSubReg;
        }
        size_t GetReg() const { return m_nReg; }
        size_t GetSubReg() const { return m_nSubReg; }

    private:
        uint8 m_nReg;
        uint8 m_nSubReg;
    };

    class RegReference
    {
    public:
        
        bool IsDirect() const { return !m_bIndirect; }

        RegTypes GetRegType() const { return (RegTypes) m_eRegType; }

    protected:
        friend class RegisterRegion;
        
        unsigned m_bIndirect : 1;
        unsigned m_eRegType : 7;
       
        union
        {
            struct{
                uint8 m_nRegNum;    // register number
                uint8 m_nRegOffset; // byte offset into register (sub-register)
            } Direct;
            struct{ // regtype is implicitly 'GPR'
                int16 m_nImmediateOffset;   /// Byte offset into register file
                uint8 m_nAddressSubReg;
            } Indirect;
        };
    };


    class DirectRegReference : public RegReference
    {
    public:
        DirectRegReference( RegTypes eRegType, size_t nRegNum, size_t nSubReg )
        {
            m_bIndirect = 0;
            m_eRegType = eRegType;
            Direct.m_nRegNum = nRegNum;
            Direct.m_nRegOffset = nSubReg;
        };
        DirectRegReference( RegTypes eRegType, size_t nRegNum )
        {
            m_bIndirect = 0;
            m_eRegType = eRegType;
            Direct.m_nRegNum = nRegNum;
            Direct.m_nRegOffset = 0;
        };

        DirectRegReference( size_t nRegNum )
        {
            m_bIndirect = 0;
            m_eRegType = REG_GPR;
            Direct.m_nRegNum = nRegNum;
            Direct.m_nRegOffset = 0;
        };
    
        /// Measured in 256-bit reg units
        size_t GetRegNumber() const { return Direct.m_nRegNum; }

        /// Measured in bytes
        size_t GetSubRegOffset() const { return Direct.m_nRegOffset; }

        void SetRegNumber( size_t reg ) { Direct.m_nRegNum = reg; }
    };

    class IndirectRegReference : public RegReference
    {
    public:
        IndirectRegReference( int16 nImmediateOffset, uint8 nAddressSubReg )
        {
            m_bIndirect = 1;
            m_eRegType = REG_GPR;
            Indirect.m_nAddressSubReg = nAddressSubReg;
            Indirect.m_nImmediateOffset = nImmediateOffset;
        }

        int GetImmediateOffset() const  { return Indirect.m_nImmediateOffset; }
        size_t GetAddressSubReg() const { return Indirect.m_nAddressSubReg; }
    };

    class RegisterRegion
    {
    public:

        RegisterRegion() :  m_nHStride(0), m_nVStride(0), m_nWidth(0)
        {
            new (&m_Base) DirectRegReference(REG_INVALID,0,0);
        };
        RegisterRegion( RegReference reg, size_t vstride, size_t width, size_t hstride  ) 
            : m_Base(reg), m_nHStride(hstride), m_nVStride(vstride), m_nWidth(width)
        {
        };
      

        RegReference GetBaseRegister() const { return m_Base; }

        bool IsValid() const { 
            return m_Base.GetRegType() != REG_INVALID;
        }

        size_t GetHStride() const { return m_nHStride; }
        size_t GetVStride() const { return m_nVStride; }
        size_t GetWidth() const   { return m_nWidth; }

    private:
        RegReference m_Base;
        uint8 m_nHStride;   ///< Step size in elements between adjacent elements in a "row"
        uint8 m_nVStride;   ///< Step size in elements between successive rows
        uint8 m_nWidth;     ///< Number of data elements per row

        // See Gen docs for further explanation

    };

  

    class SourceOperand
    {
    public:

        SourceOperand( ) : m_eDataType(DT_INVALID), m_bImmediate(false), m_eModifier(SM_NONE){}
        SourceOperand( DataTypes eType, uint32 IMM ) : m_eDataType(eType), m_bImmediate(true),m_eModifier(SM_NONE)
        {
            memcpy(fields.Imm,&IMM,sizeof(IMM));
        }
        SourceOperand( float IMM ) : m_eDataType(DT_F32), m_bImmediate(true),m_eModifier(SM_NONE)
        {
            memcpy(fields.Imm,&IMM,sizeof(IMM));
        }

        SourceOperand( DataTypes eType, const RegisterRegion& reg ) : m_eDataType(eType), m_bImmediate(false),m_eModifier(SM_NONE)
        {
            SetRegRegion(reg);
        }
        SourceOperand( DataTypes eType, const RegisterRegion& reg, Swizzle swizz ) : m_eDataType(eType), m_Swizzle(swizz), m_bImmediate(false),m_eModifier(SM_NONE)
        {
            SetRegRegion(reg);
        }
        
        SourceOperand( DataTypes eType, SourceModifiers eMod ) : m_eDataType(eType), m_bImmediate(true),m_eModifier(eMod){};
        SourceOperand( DataTypes eType, const RegisterRegion& reg, SourceModifiers eMod ) 
            : m_eDataType(eType), m_bImmediate(false) ,m_eModifier(eMod)
        {
            SetRegRegion(reg);
        }
        SourceOperand( DataTypes eType, const RegisterRegion& reg, Swizzle swizz, SourceModifiers eMod ) 
            : m_eDataType(eType), m_bImmediate(false) ,m_eModifier(eMod), m_Swizzle(swizz)
        {
            SetRegRegion(reg);            
        }


        bool IsImmediate() const { return m_bImmediate != 0; }
        DataTypes GetDataType() const { return (DataTypes)m_eDataType; }
        RegisterRegion GetRegRegion() const { return RegisterRegion(fields.Reg.m_Base,fields.Reg.m_nVStride,fields.Reg.m_nWidth, fields.Reg.m_nHStride); }
        SourceModifiers GetModifier() const { return (SourceModifiers)m_eModifier; };
        void SetModifier(SourceModifiers eMod) { m_eModifier = eMod; };

        void SetRegRegion( RegisterRegion reg )
        {
            fields.Reg.m_Base     = reg.GetBaseRegister();
            fields.Reg.m_nHStride = reg.GetHStride();
            fields.Reg.m_nVStride = reg.GetVStride();
            fields.Reg.m_nWidth   = reg.GetWidth();
        }

        Swizzle GetSwizzle() const { return m_Swizzle; }

        bool IsValid() const
        {
            return m_eDataType != DT_INVALID && (m_bImmediate || fields.Reg.m_Base.GetRegType() != REG_INVALID );
        }

        const uint8* GetImmediateBits() const { return fields.Imm; }
    private:

        Swizzle m_Swizzle;
        DataTypes m_eDataType;
        uint8 m_bImmediate;
        uint8 m_eModifier;
        union
        {
            struct 
            {
                RegReference m_Base;
                uint8 m_nHStride;   ///< Step size in elements between adjacent elements in a "row"
                uint8 m_nVStride;   ///< Step size in elements between successive rows
                uint8 m_nWidth;     ///< Number of data elements per row
            } Reg;
            uint8 Imm[8];
        } fields;
    };


    class DestOperand
    {
    public:

        DestOperand() : m_eDataType(DT_INVALID), m_nWriteMask(0){}
        
        DestOperand( DataTypes eType, RegisterRegion region ) : m_eDataType(eType), m_nWriteMask(0xff), m_RegRegion(region){}
        
        DestOperand( DataTypes eType, RegisterRegion region, uint8 nWriteMask )
            : m_nWriteMask(nWriteMask), m_eDataType(eType), m_RegRegion(region)
        {
        }

        DataTypes GetDataType() const { return (DataTypes)m_eDataType; }
        RegisterRegion GetRegRegion() const { return m_RegRegion; }

        bool IsValid() const
        {
            return m_eDataType != DT_INVALID && m_RegRegion.IsValid();
        }

    private:
        uint8 m_nWriteMask;
        uint8 m_eDataType;
        RegisterRegion m_RegRegion;
    };

   





    class Instruction
    {
    public:
        Instruction() 
            : m_eClass(IC_NULL), m_eOp(OP_ILLEGAL), m_bEOT(0), m_bNoDDChk(0), m_bMsgDescriptorFromReg(0)
        {}

        Predicate GetPredicate() const { return m_Predicate; }
        InstructionClass GetClass() const { return (InstructionClass)m_eClass; };
        Operations GetOperation() const { return (Operations)m_eOp; }

        template< class T> T GetImmediate() const { return *((const T*)&m_ImmediateOperand[0]); }

        bool IsWriteMaskDisabled() const { return m_bNoWriteMask; }

        bool IsDDCheckDisabled() const { return m_bNoDDChk; }
        void DisableDDCheck() { m_bNoDDChk = 1; };

        FlagReference GetFlagReference() const { return m_Flags; }
        void SetFlagReference( const FlagReference& rFlag ) { m_Flags = rFlag; }
        void SetPredicate( Predicate p ) { m_Predicate = p; }
        
        size_t GetExecSize() const { return m_nExecSize; }
        
    protected:
        Instruction( InstructionClass e ) 
            : m_eClass(e),
              m_eOp(NOT_AN_OP),
              m_nExecSize(0),
              m_bEOT(0), 
              m_bNoDDChk(0), 
              m_bMsgDescriptorFromReg(0)
        {
        };

        friend class Decoder;

        uint8 m_eOp;
        uint8 m_eClass;
        uint8 m_nExecSize;
        unsigned m_bNoWriteMask : 1;
        unsigned m_bNoDDChk : 1;
        Predicate m_Predicate;

        /////////////////////////////////////
        // Send instruction only
        unsigned m_bEOT : 1;
        unsigned m_bMsgDescriptorFromReg : 1;
        ////////////////////////////////////



        union
        {
            uint8 m_ImmediateOperand[8];
            struct
            {
                int32 JIP;
                int32 UIP;
            } m_BranchOffsets;
        };
    
        union
        {
            uint8 m_eFunctionCtrl;   // math
            uint8 m_eSFID;          // send
            uint8 m_eCondModifier;  // others
        };
        

        DestOperand m_Dest;
        SourceOperand m_Source0;
        SourceOperand m_Source1;
        SourceOperand m_Source2;
        FlagReference m_Flags;

    };





    class UnaryInstruction : public Instruction
    {
    public:
        UnaryInstruction() : Instruction(IC_UNARY) {}
        UnaryInstruction( size_t nExecSize, Operations eOp, DestOperand dst, SourceOperand src ) 
            : Instruction(IC_UNARY) 
        {
            m_eOp = eOp;
            m_nExecSize = nExecSize;
            m_Dest = dst;
            m_Source0 = src;
            m_eCondModifier = CM_NONE;
            if( src.IsImmediate() )
                memcpy( &m_ImmediateOperand, src.GetImmediateBits(), 8 );
        }
       

        ConditionalModifiers GetConditionModifier() const { return (ConditionalModifiers)m_eCondModifier; }
        void SetConditionalModifier( ConditionalModifiers eMod ) { m_eCondModifier = eMod; }


        size_t GetExecSize() const { return m_nExecSize; }
        const DestOperand& GetDest() const { return m_Dest; }
        const SourceOperand& GetSource0() const { return m_Source0; };

    private:

    };

    class BinaryInstruction : public Instruction
    {
    public:
        BinaryInstruction()  : Instruction(IC_BINARY) {};
        
        BinaryInstruction( size_t nExecSize, Operations eOp, DestOperand dst, SourceOperand src0, SourceOperand src1 ) 
            : Instruction(IC_BINARY) 
        {
            m_eOp = eOp;
            m_nExecSize = nExecSize;
            m_Dest = dst;
            m_Source0 = src0;
            m_Source1 = src1;
            m_eCondModifier = CM_NONE;
            if( src1.IsImmediate() )
                memcpy( &m_ImmediateOperand, src1.GetImmediateBits(), 8 );
            
        }
       
           
        ConditionalModifiers GetConditionModifier() const { return (ConditionalModifiers)m_eCondModifier; }
        void SetConditionalModifier( ConditionalModifiers eMod ) { m_eCondModifier = eMod; }

        size_t GetExecSize() const { return m_nExecSize; }
        const DestOperand& GetDest() const { return m_Dest; }
        const SourceOperand& GetSource0() const { return m_Source0; };
        const SourceOperand& GetSource1() const { return m_Source1; };
    
        
    };

    class TernaryInstruction : public Instruction
    {
    public:
        TernaryInstruction() : Instruction(IC_TERNARY) {}
        TernaryInstruction( size_t nExecSize, Operations eOp, DestOperand dst, SourceOperand src0, SourceOperand src1, SourceOperand src2 )
            : Instruction(IC_TERNARY)
        {
            m_eOp = eOp;
            m_nExecSize = nExecSize;
            m_Dest = dst;
            m_Source0 = src0;
            m_Source1 = src1;
            m_Source2 = src2;
            m_eCondModifier = CM_NONE;
        }

        ConditionalModifiers GetConditionModifier() const { return (ConditionalModifiers)m_eCondModifier; }
        void SetConditionalModifier( ConditionalModifiers eMod ) { m_eCondModifier = eMod; }

        size_t GetExecSize() const { return m_nExecSize; }
          
        const DestOperand& GetDest() const { return m_Dest; }
        const SourceOperand& GetSource0() const { return m_Source0; };
        const SourceOperand& GetSource1() const { return m_Source1; };
        const SourceOperand& GetSource2() const { return m_Source2; };
    };

    class SendInstruction : public Instruction
    {
    public:
        SendInstruction() : Instruction( IC_SEND )
        {
            m_eSFID = SFID_NULL;
            m_bMsgDescriptorFromReg = 0;
            m_bEOT = 0;
        }
        SendInstruction( size_t nExec, SharedFunctionIDs eDest, uint32 nDescriptor, DestOperand dst, SourceOperand src0 )
            : Instruction( IC_SEND )
        {
            m_eOp = OP_SEND;
            m_Source0   = src0;
            m_Dest      = dst;
            m_nExecSize = nExec;
            m_eSFID = eDest;
            m_bEOT=0;
            m_bMsgDescriptorFromReg=0;
            memcpy( m_ImmediateOperand, &nDescriptor, sizeof(nDescriptor) );
        }

        bool IsEOT() const { return m_bEOT; }
        bool IsDescriptorInRegister() const { return m_bMsgDescriptorFromReg; }
        uint32 GetDescriptorIMM() const { return GetImmediate<uint32>(); }
        const DestOperand& GetDest() const { return m_Dest; }
        const SourceOperand& GetSource() const { return m_Source0; }
        size_t GetExecSize() const { return m_nExecSize; }
        SharedFunctionIDs GetRecipient() const { return (SharedFunctionIDs)m_eSFID; }

        void SetEOT( ) { m_bEOT = true; };


        /// Message descriptor format is:
        ///
        ///    28:25 -> message length in GPRs (4 bits)
        ///    24:20 -> response length in GPRs (5 bits)
        ///    19    -> header present (adds an extra GPR to top of payload under certain circumstances)
        ///    18:0  -> depends on recipient
        uint32 GetMessageLengthFromDescriptor() const
        {
            return (GetImmediate<uint32>()>>25)&0xf;
        }
        uint32 GetResponseLengthFromDescriptor() const
        {
            return (GetImmediate<uint32>()>>20)&0x1f;
        }


    };

 

    class MathInstruction : public Instruction
    {
    public:
        MathInstruction() : Instruction( IC_MATH ){}
        MathInstruction( size_t nExecSize, MathFunctionIDs eFunction, DestOperand dst, SourceOperand src0, SourceOperand src1 ) 
            : Instruction(IC_MATH) 
        {
            m_eOp = OP_MATH;
            m_eFunctionCtrl = eFunction;
            m_nExecSize = nExecSize;
            m_Dest = dst;
            m_Source0 = src0;
            m_Source1 = src1;
        }
        MathInstruction( size_t nExecSize, MathFunctionIDs eFunction, DestOperand dst, SourceOperand src0 ) 
            : Instruction(IC_MATH) 
        {
            m_eOp = OP_MATH;
            m_eFunctionCtrl = eFunction;
            m_nExecSize = nExecSize;
            m_Dest = dst;
            m_Source0 = src0;
            m_Source1 = SourceOperand(GEN::DT_INVALID, GEN::RegisterRegion( GEN::DirectRegReference( REG_NULL, 0 ),0,0,0 ) );
        }
        size_t GetExecSize() const { return m_nExecSize; }
        const DestOperand& GetDest() const { return m_Dest; }
        const SourceOperand& GetSource0() const { return m_Source0; }
        const SourceOperand& GetSource1() const { return m_Source1; }

        MathFunctionIDs GetFunction() const { return (MathFunctionIDs) m_eFunctionCtrl; }

    };

    class BranchInstruction : public Instruction
    {
    public:
        BranchInstruction() : Instruction( IC_BRANCH ){}
        size_t GetExecSize() const { return m_nExecSize; }
        int GetJIP() const { return m_BranchOffsets.JIP; }
        int GetUIP() const { return m_BranchOffsets.UIP; }
    };


    
    const char* OperationToString( Operations op );
    const char* ConditionalModifierToString( ConditionalModifiers op );

    const char* SharedFunctionToString( SharedFunctionIDs eFunc );
    
    size_t GetTypeSize( DataTypes eTypes );

    // We use a sort of "static polymorphism" in this class hierarchy.
    //  Base classes are designed so that they contain everything that any subclass might use
    //   This enables safe slicing, memcpying, stack-allocation, and bulk-storage of arbitrary instances
    //    simply by using the base class type
    //  
    //  The subclasses still obey an "is-a" relationship, but are in some ways more akin to interfaces
    //   
    //  This setup may not be strictly standard-compliant, but a sane compiler
    //    should not have any problems with it.
    // 
    static_assert( sizeof(DirectRegReference)   == sizeof(RegReference), "derp" );
    static_assert( sizeof(IndirectRegReference) == sizeof(RegReference), "derp" );
    static_assert( sizeof(Instruction) == sizeof(UnaryInstruction),      "derp" );
    static_assert( sizeof(Instruction) == sizeof(BinaryInstruction),     "derp" );
    static_assert( sizeof(Instruction) == sizeof(TernaryInstruction),    "derp" );
    static_assert( sizeof(Instruction) == sizeof(SendInstruction),       "derp" );
    static_assert( sizeof(Instruction) == sizeof(MathInstruction),       "derp" );
    static_assert( sizeof(Instruction) == sizeof(BranchInstruction),     "derp" );

  
    

    /// Send EOT message to thread spawner as payload
    ///    -  Caller should move original r0 header into source reg
    ///    -  Note that GEN Isa docs stipulate EOT payload must be sent
    ///         from the upper few regs in the GRF.   r127 is what driver likes to use
    SendInstruction SendEOT(uint32 nSourceGPR);

    /// Send a dword scatter write message.  
    ///     Source reg 0 contains dword offsets from bind table location
    ///     Source reg 1 contains write data
    SendInstruction DwordScatterWriteSIMD8( uint32 nBindTableIndex, uint32 nSourceGPR );


    /// Send OWord dual block write message (useful for writing an entire GPR)
    /// 
    ///   Source[0].u2 contains the offset, measured in owords
    ///   Source[1] contains write data
    ///
    SendInstruction OWordDualBlockWrite( uint32 nBindTableIndex, uint32 nSourceGPR );
    SendInstruction OWordBlockWrite( uint32 nBindTableIndex,  GEN::RegReference source );
      

    /// Send OWord dual block write message (useful for writing an entire GPR)
    /// 
    ///   nAddress.u2 contains the offset, measured in owords
    ///   nData receives result
    ///
    SendInstruction OWordDualBlockRead( uint32 nBindTableIndex, uint32 nAddress, uint32 nData );
    SendInstruction OWordDualBlockReadAndInvalidate( uint32 nBindTableIndex, uint32 nAddress, uint32 nData );
      

    /// Send DWORD gather read/write message
    ///     Writes must have addr+data together in consecutive regs
      
    SendInstruction DWordScatteredRead_SIMD8( uint32 nBindTableIndex, RegReference address, RegReference data );
    SendInstruction DWordScatteredRead_SIMD16( uint32 nBindTableIndex, RegReference nAddress, RegReference nData );
    SendInstruction DWordScatteredWrite_SIMD8( uint32 nBindTableIndex, RegReference address, RegReference writeCommit );
    SendInstruction DWordScatteredWrite_SIMD16( uint32 nBindTableIndex, RegReference nAddress, RegReference writeCommit );
    SendInstruction UntypedRead_SIMD8x4( uint32 nBindTableIndex, GEN::RegReference addr, GEN::RegReference data );
    SendInstruction UntypedRead_SIMD16x4( uint32 nBindTableIndex, GEN::RegReference addr, GEN::RegReference data );
    SendInstruction UntypedWrite_SIMD16x2( uint32 nBindTableIndex, RegReference nAddress, RegReference writeCommit );
 
    inline SendInstruction DWordScatteredReadSIMD8( uint32 nBindTableIndex, uint32 nAddress, uint32 nData )
    {
        return DWordScatteredRead_SIMD8( nBindTableIndex,  
                                       GEN::DirectRegReference(REG_GPR,nAddress), 
                                       GEN::DirectRegReference(REG_GPR, nData ) );
    }
      
    
    UnaryInstruction RegMove(  RegTypes eDstType, uint32 nDstReg, RegTypes eSrcType, uint32 nSrcReg );
    UnaryInstruction RegMoveIMM(  uint32 nDstReg, uint32 IMM );

    BinaryInstruction DoMath( uint32 nExecSize, Operations eOp, DataTypes eType, uint32 nDstReg, uint32 nSrc0, uint32 nSrc1 );
    BinaryInstruction DoMathIMM( uint32 nExecSize, Operations eOp, uint32 nDstReg, uint32 nSrc0, uint32 imm );


    /// Send a timestamp request to the message gateway
    ///    Source[0] is MBZ except for dword 5, high bit, which controls
    ///       which half of the dest GPR you want the reply to go to
    /// 
    ///    Dest[0] receives
    ///        DWORDs 0,1 are absolute time, 2,3, are relative
    ///            Not sure of the difference between "absolute" and "relative"
    ///          Docs claim that 'Relative' is counting GPU clocks
    ///
    SendInstruction ReadGatewayTimestamp( uint32 nDestReg, uint32 nSrcReg );


    SourceOperand PackHalfByte_SINT( const int32* pInts );
    SourceOperand PackHalfByte_UINT( const uint32* pInts );

    void UnpackHalfByte_SINT( int* pInts, uint32 bits );
    void UnpackHalfByte_UINT( unsigned int* pInts, uint32 bits );

}


#endif