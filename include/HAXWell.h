
#ifndef _HAXWELL_H_
#define _HAXWELL_H_

#include "HAXWell_Utils.h"

namespace HAXWell
{
    typedef void* ShaderHandle;
    typedef void* BufferHandle;
    typedef void* TimerHandle;
    typedef unsigned int timer_t;

    struct ShaderArgs
    {
 
        size_t nDispatchThreadCount;    ///< Number of EU HW threads to dispatch per thread group.  Maximum is 64
        size_t nSIMDMode;               ///< Must be 8, 16, or 32
        
        const void* pIsa;       ///< Pointer to the instructions
        size_t nIsaLength;      ///< Length of instruction stream in bytes

        size_t nCURBEAllocsPerThread;   ///< Number of 256-bit constant vectors per dispatched thread
                                        ///<   Each thread's N constant vecotrs are pre-loaded into GPRs 1-N
        const void* pCURBE;             ///< Pointer to the constant data
    };

    enum
    {
        MAX_DISPATCH_COUNT = 64, ///< Thread counts higher than this have led to instability
        MAX_BUFFERS =   4,
        BIND_TABLE_BASE = 0x38, // Shader storage buffers start at this bind table index
    };

    
    bool Init( bool bCreateGLContext );

    BufferHandle CreateBuffer( const void* pOptionalInitialData, size_t nDataSize );
    void* MapBuffer( BufferHandle h );
    void UnmapBuffer( BufferHandle h );
    void ReleaseBuffer( BufferHandle hBuffer );

    ShaderHandle CreateShader( const ShaderArgs& rShader );
    void ReleaseShader( ShaderHandle hShader );


    TimerHandle BeginTimer();
    void EndTimer( TimerHandle hTimer );
    timer_t ReadTimer( TimerHandle hTimer );

    void DispatchShader( ShaderHandle hShader, BufferHandle* pBuffers, size_t nBuffers, size_t nThreadGroups );
    void Finish();

    /// Compile GLSL and extract its ISA
    bool RipIsaFromGLSL( Blob& blob, const char* pGLSL );

}


#endif