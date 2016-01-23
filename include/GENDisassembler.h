
#ifndef _GEN_DISASSEMBLER_H_
#define _GEN_DISASSEMBLER_H_

namespace GEN
{
    typedef unsigned char uint8;

    class Decoder;

    class IPrinter
    {
    public:
        virtual void Push( const char* text ) = 0;
    };

    bool Disassemble( IPrinter& rPrinter, Decoder* pDecoder, const void* pIsaBytes, size_t nIsaBytes );

}

#endif