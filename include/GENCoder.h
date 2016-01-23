
#ifndef _GEN_DECODER_H_
#define _GEN_DECODER_H_

namespace GEN
{
    typedef unsigned char uint8;
    typedef char int8;
    typedef unsigned int uint32;
    typedef int int32;

    enum Operations;
    class Instruction;

    ///
    /// 'Decoder' and 'Encoder are classes that seperates logical instruction
    ///     fields from physical instruction layouts
    ///
    ///  The intent is for 'Coder' to eventually be refactored into a
    ///     virtual interface as needed to support multiple processor generations,
    ///     if and when this should become necesssary or useful.
    ///
    class Decoder
    {
    public:

        /// Deduce the opcode from a pointer to a GEN instruction
        ///   'NOT_AN_OP' is returned for if the value of the opcode field is not recognized
        /// 
        Operations GetOperation( const uint8* pInstructionBytes );

        /// Determine instruction length in bytes given a pointer to a GEN instruction
        ///
        ///  0 is returned if the operation is 'NOT_AN_OP'
        ///    An non-zero length is returned if the opcode is 'ILLEGAL'
        ///
        size_t DetermineLength( const uint8* pInstructionBytes );
        
        /// Build a high-level representation of an instruction from raw bits
        ///    Returns the encoded instruction length
        size_t Decode( Instruction* pInst, const uint8* pInstructionBytes );

        /// Determine instruction length, and expand instruction (if compressed)
        ///   or copy it (if native)
        size_t Expand( uint8* pOut, const uint8* pInstruction );

    private:

        void DecodeInstructionHeader( Instruction* pInst, const uint8* pBytes );
        void DecodeOperandControls( Instruction* pInst, const uint8* pBytes, bool align1 );
  
        size_t Decode3Source(  Instruction* pInst, const uint8* pInstructionBytes,Operations eOp );
        bool DecodeNativeOp( Instruction* pInst, const uint8* pInstructionBytes,Operations eOp );
        size_t DecodeSend( Instruction* pInst, const uint8* pInstructionBytes, Operations eOp  );
        size_t DecodeMath( Instruction* pInst, const uint8* pInstructionBytes  );
    };




    class Encoder
    {
    public:

        /// Query required buffer size for encoding
        size_t GetBufferSize( size_t nOps );

        ///
        /// Encode an array of GEN instructions.   Caller must supply a buffer of required size
        ///   Returns size of encoded instructions.  This may be less than required size
        ///      if compressed instructions were used
        ///
        size_t Encode( void* pOutputBuffer, const GEN::Instruction* pOps, size_t nOps );

    };
}



#endif