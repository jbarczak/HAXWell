
#include "HAXWell_Utils.h"
#include "GENCoder.h"
#include "GENIsa.h"

#include "HAXWell.h"

namespace HAXWell
{

    Blob::~Blob()
    {
        free(m_pBytes);
    }


    void Blob::SetLength( size_t n )
    {
        if( n > m_nLength )
        {
            free(m_pBytes);
            m_pBytes = malloc(n);
        }
        
        m_nLength = n;
        
    }


    // I determined by this by passing an excessively large blob size and then
    //   disassembling the instruction on which the driver finally crashed.
    //       ( Thanks to Tomasz Stachowiak for the idea )
    //  
    // The function is relatively short, so I just backported the disassembly to C
    //
    void DriverHashFunction( DWORD* pCRC, const DWORD* pData, DWORD nDwords )
    {
        DWORD eax;
        DWORD esi = 0x428A2F98;
        DWORD edx = 0x71374491;
        DWORD edi = 0x0B5C0FBCF;
        DWORD ebx = nDwords;
        const DWORD* ecx = pData;
        while( ebx )
        {
            esi ^= *ecx;       //xor         esi,dword ptr [ecx]  
            eax  = edi;        //mov         eax,edi  
            esi -= edi;        //sub         esi,edi  
            eax >>= 0xD;       //shr         eax,0Dh  
            esi -= edx;        //sub         esi,edx  
            esi ^= eax;        //xor         esi,eax  
            edx -= edi;        //sub         edx,edi  
            edx -= esi;        //sub         edx,esi  
            eax = esi;         //mov         eax,esi  
            eax <<= 8;         //shl         eax,8  
            edx ^= eax;        //xor         edx,eax  
            edi -= edx;        //sub         edi,edx  
            edi -= esi;        //sub         edi,esi  
            eax = edx;         //mov         eax,edx  
            eax >>= 0xD;       //shr         eax,0Dh  
            edi ^= eax;        //xor         edi,eax  
            esi -= edi;        //sub         esi,edi  
            esi -= edx;        //sub         esi,edx  
            eax = edi;         //mov         eax,edi  
            eax >>= 0x0C;      //shr         eax,0Ch  
            esi ^= eax;        //xor         esi,eax  
            edx -= edi;        //sub         edx,edi  
            edx -= esi;        //sub         edx,esi  
            eax = esi;         //mov         eax,esi  
            eax <<= 0x10;      //shl         eax,10h  
            edx ^= eax;        //xor         edx,eax  
            edi -= edx;        //sub         edi,edx  
            edi -= esi;        //sub         edi,esi  
            eax = edx;         //mov         eax,edx  
            eax >>= 5;         //shr         eax,5  
            edi ^= eax;        //xor         edi,eax  
            esi -= edi;        //sub         esi,edi  
            eax = edi;         //mov         eax,edi  
            eax >>= 3;         //shr         eax,3  
            esi -= edx;        //sub         esi,edx  
            esi ^= eax;        //xor         esi,eax  
            edx -= edi;        //sub         edx,edi  
            eax = esi;         //mov         eax,esi  
            eax <<= 0x0A;      //shl         eax,0Ah  
            edx -= esi;        //sub         edx,esi  
            edx ^= eax;        //xor         edx,eax  
            edi -= edx;        //sub         edi,edx  
            eax = edx;         //mov         eax,edx  
            edi -= esi;        //sub         edi,esi  
            eax >>= 0x0F;      //shr         eax,0Fh  
            edi ^= eax;        //xor         edi,eax  
            ecx++;             //lea         ecx,[ecx+4]  
            ebx--;             //dec         ebx  
        }

        eax = edi;
        pCRC[0] = eax;
        pCRC[1] = edx;
    }
     
    
    DWORD FetchDWORD( const unsigned char* pBytes ) { return *((DWORD*)pBytes); }
    void WriteDWORD( unsigned char* pBytes, DWORD dw ) { *((DWORD*)pBytes) = dw; }
    void WriteBYTE( unsigned char* pBytes, DWORD dw ) { *pBytes = (unsigned char)dw; }
    
    

    // Given a blob, attempt to locate a GEN program by searching for valid opcodes
    bool FindIsaInBlob( size_t* pIsaOffset, size_t* pIsaLength, const void* pBlob, size_t nBlobLength )
    {
        const unsigned char* pBytes = (const unsigned char*)pBlob;

        GEN::Decoder decoder;

        
        // All GEN threads must in a SEND instruction with the 'EOT' field set 
        //     Find this instruction
        
        size_t nSendInstruction = 0;
        while( nSendInstruction < nBlobLength )
        {
            const unsigned char* pWhere = pBytes + nSendInstruction;
            
            GEN::Operations eOp = decoder.GetOperation(pBytes + nSendInstruction);
            if( eOp == GEN::OP_SEND )
            {
                GEN::SendInstruction inst;
                if( decoder.Decode( &inst, pWhere ) )
                {
                    if( !inst.IsDescriptorInRegister() && inst.IsEOT() )
                        break;
                }
            }
         
            nSendInstruction++;
        }

        if( nSendInstruction == nBlobLength )
            return false;
        
        
        // Now work our way backwards to the first non-instruction we see
        //  This won't work if there exists a native instruction whose upper half
        //   happens to exactly match a compressed one, but its the best we've got
        const unsigned char* pIsaEnd   = (pBytes + nSendInstruction+16);
        const unsigned char* scan = pIsaEnd;
        while( scan > pBytes )
        {
            const unsigned char* p2x = scan-8;
            const unsigned char* p4x = scan-16;
            DWORD* pD2x = (DWORD*)p2x;
            DWORD* pD4x = (DWORD*)p4x;

            size_t len2 = decoder.DetermineLength(p2x);
            size_t len4 = decoder.DetermineLength(p4x);
            GEN::Operations eOp2 = decoder.GetOperation(p2x);
            GEN::Operations eOp4 = decoder.GetOperation(p4x);
        
            if( eOp2 != GEN::OP_ILLEGAL && eOp2 != GEN::NOT_AN_OP && len2 == 8 )
            {
                // legal compacted instruction
                scan = p2x;
            }
            else if( eOp2 != GEN::OP_ILLEGAL && eOp2 != GEN::NOT_AN_OP && len2 == 16 )
            {
                // this means the last iteration mistook the upper half of a native instruction for a compressed instruction
                scan = p2x;
            }
            else if( eOp4 != GEN::OP_ILLEGAL && eOp4 != GEN::NOT_AN_OP && len4==16 )
            {
                // legal native instruction
                scan = p4x;
            }
            else
            {
                // the end is nigh
                break;
            }
        }


        if( scan < pBytes )
            return false;

        if( pIsaOffset )
            *pIsaOffset = scan - pBytes;
        if( pIsaLength )
            *pIsaLength = pIsaEnd - scan;

        return true;
    }

   

    void PatchBlob( Blob& rOutputBlob, const ShaderArgs& rArgs, const Blob& rTemplateBlob, size_t nTemplateIsaStart )
    {
        
        //
        //  Summary of what I know about the blob layout:
        //
        // 
        //  DWORD 0: 0x00003142  This is a 4CC code "B1"
        //  DWORD 1: 0x020423c8.  Version number?  Device number? dunno
        //  DWORD 2 looks like it contains the blob length minus some constant
        //            but this field is itself shifted one byte.  the lower 8 bits are 0xB...?
        //  ^
        //  |  Rest is unknown
        //  V
        //  DWORD 16 threadgroup size X
        //  DWORD 17 " " Y (-1 if not specified)
        //  DWORD 18 " " Z (-1 if not specified)
        //  ^
        //  |
        //  | Unknown length and contents
        //  |   AT SOME POINT IN HERE, WE STOP BEING DWORD ALIGNED
        //  |
        //  |
        //  V 
        //  ^
        //  |  Various pre-isa fields, a few of which are understood
        //  |      (see below)
        //  V
        //  ^
        //  |
        //  | ISA   !NOT DWORD ALIGNED!
        //  |
        //  V
        //  ^
        //  |
        //  | Padding to align ISA to 64 bytes
        //  |
        //  V
        //  ^
        //  |
        //  | 512 bytes of zero padding
        //  |
        //  V
        //  CURBE data
        //   ....
        //   ....
        //  ^
        //  |
        //  | Unknown length and content
        //  V
        //  Last two DWORDS are a hash


        //
        //    By comparing different shaders I was able to decipher the important looking parts of the blob prior to the isa
        //     
        //        At isa-24 is the number of threads in the OpenGL workgroup grid (width*height*depth)
        //        At isa-32 is the SIMD mode (0 for SIMD0, 1 for SIMD16)
        //        At isa-104 and isa-702 is the number of 256-bit CURBE entries per hardware thread
        //        At isa-100 and isa-700 is the number of HW threads
        //        At isa-4 and isa-128 is the length of the isa block (including padding)
        //        At isa-40 is the length of the CURBE data
        //        
        //    
        //     The Isa block is always padded with 512 bytes (128 DWORDS) of zero.  
        //        Intel docs state that the 128 bytes following the last instruction are MBZ
        //          because instruction prefetching will yank those bytes in before the EU realizes the thread is done
        //
        //       512 bytes is more padding than we need.  Either the docs are wrong, or the driver is over-zealous
        //           Or else there's something there I haven't figured out yet.
 
        const unsigned char* pTemplate = ((const unsigned char*)rTemplateBlob.GetBytes());
        const unsigned char* isa = pTemplate  + nTemplateIsaStart;
        
        DWORD dwGridSize        = FetchDWORD(isa-24);  // width*height*depth for the GL threadgroup
        DWORD dwMode            = FetchDWORD(isa-32);  // 0 for SIMD8, 1 for SIMD16          
        DWORD dwCURBEEntrySize  = FetchDWORD(isa-104); 
        DWORD dwThreadCount     = FetchDWORD(isa-100); // also at isa-700
        DWORD dwIsaLength       = FetchDWORD(isa-4);   // also at isa-128
        DWORD dwCURBELength     = FetchDWORD(isa-40);

        size_t nPreIsaLength    = nTemplateIsaStart;
        size_t nPostCURBEStart  = nTemplateIsaStart + dwIsaLength + dwCURBELength;
        size_t nPostCURBELength = rTemplateBlob.GetLength() - nPostCURBEStart;

        size_t nNewIsaLength    = 512 + ( (rArgs.nIsaLength + 63) & ~63);
        size_t nNewCURBELength  =  rArgs.nCURBEAllocsPerThread*rArgs.nDispatchThreadCount*32;

        size_t nNewBlobSize = nPreIsaLength + nPostCURBELength + nNewIsaLength + nNewCURBELength;
        
        rOutputBlob.SetLength(nNewBlobSize);

        unsigned char* pNewBlob = (unsigned char*) rOutputBlob.GetBytes();
        unsigned char* pNewIsa       = pNewBlob  + nPreIsaLength;
        unsigned char* pNewCURBE     = pNewIsa   + nNewIsaLength;
        unsigned char* pNewPostCURBE = pNewCURBE + nNewCURBELength;

        // copy pre-Isa blob
        memcpy( pNewBlob, pTemplate, nPreIsaLength );

        // copy the new Isa into place and add the padding
        memcpy( pNewIsa, rArgs.pIsa, rArgs.nIsaLength );
        memset( pNewIsa + rArgs.nIsaLength, 0, nNewIsaLength - rArgs.nIsaLength  );

        // copy the new CURBE into place
        memcpy( pNewCURBE, rArgs.pCURBE, nNewCURBELength );

        // copy post-CURBE blob
        memcpy(pNewPostCURBE, pTemplate + nPostCURBEStart, nPostCURBELength );

        // override the fields we might need to change

        DWORD nDispatchMode = 0;
        switch( rArgs.nSIMDMode )
        {
        case 16: nDispatchMode = 1; break;
        case 32: nDispatchMode = 2; break;
        case 8:
        default:
           break; // keep SIMD8 dispatch (mode=0)
        }

        WriteDWORD( pNewIsa-32, nDispatchMode );

        // change thread grid size
        WriteDWORD( pNewIsa-24, (8<<nDispatchMode)*rArgs.nDispatchThreadCount );

        // change HW thread count
        WriteDWORD( pNewIsa-100, rArgs.nDispatchThreadCount );
        WriteBYTE( pNewIsa-700, rArgs.nDispatchThreadCount );

        // change CURBE allocation size
        WriteDWORD( pNewIsa-104, rArgs.nCURBEAllocsPerThread ); 
        WriteBYTE( pNewIsa-702, rArgs.nCURBEAllocsPerThread ); 

        // change CURBE length
        WriteDWORD( pNewIsa-40, nNewCURBELength );

        // change Isa length
        WriteDWORD( pNewIsa-4, nNewIsaLength );
        WriteDWORD( pNewIsa-128, nNewIsaLength );

        // Near the top, we have blob_length minus some constant.  696, in the case of our sample blobs
        //  This looks to be a "how many bytes follow" field
        //
        //   Not sure if the 696 is fixed or varies with the particular GLSL program
        //    Let's assume it varies...
        //
        DWORD nSizeDifference = rTemplateBlob.GetLength() - FetchDWORD( pTemplate + 13 );
        WriteDWORD( pNewBlob + 13, nNewBlobSize - nSizeDifference );

        // fix the hash
        DriverHashFunction( (DWORD*)(pNewBlob + (nNewBlobSize-8)), (DWORD*)pNewBlob, (nNewBlobSize-8)/4 );

    }

}