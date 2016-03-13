#include <Windows.h>
#include <GL/GL.h>
#include <stdio.h>

#include "HAXWell.h"
#include "HAXWell_Utils.h"

#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_PROGRAM_BINARY_LENGTH          0x8741
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_DEBUG_OUTPUT                   0x92E0

#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA

#define GL_SHADER_STORAGE_BUFFER          0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING  0x90D3
#define GL_SHADER_STORAGE_BUFFER_START    0x90D4
#define GL_SHADER_STORAGE_BUFFER_SIZE     0x90D5

#define GL_TIME_ELAPSED                   0x88BF
#define GL_QUERY_RESULT                   0x8866

void OpcodeWalk( void* pBuffer, size_t nBufferSize );


namespace HAXWell
{
    typedef GLuint (__stdcall *PCREATESHADER)( GLenum );
    typedef void (__stdcall *PSHADERSOURCE)(	GLuint ,
 	    GLsizei ,
 	    const char **string,
 	    const GLint *length);
 
    typedef void (__stdcall *PCOMPILESHADER) (GLuint);

    typedef void (__stdcall *PGETSHADERINFOLOG) ( GLuint  	,
 	    GLsizei  	,
 	    GLsizei * 	,
 	    char * 	);

    typedef GLuint (__stdcall* PCREATEPROGRAM)();
    typedef void (__stdcall* PATTACHSHADER)( GLuint,GLuint);
    typedef void (__stdcall* PLINKPROGRAM) (GLuint);


    typedef void APIENTRY DEBUGPROC(GLenum source​, GLenum type​, GLuint id​,
       GLenum severity​,
       GLsizei length​, const char* message​, const void* userParam​);

    typedef void (__stdcall*PDEBUGMESSAGECALLBACK)(DEBUGPROC callback​, void* userParam​);

    typedef void (__stdcall* PGETPROGRAMBINARY) (
        GLuint ,
 	    GLsizei ,
 	    GLsizei *,
 	    GLenum *,
 	    void *);

    typedef void ( __stdcall* PGETPROGRAMIV) (	GLuint ,
 	                                            GLenum ,
 	                                            GLint *);

    typedef void (__stdcall* PPROGRAMBINARY) (GLuint program,
 	                                          GLenum binaryFormat,
 	                                          const void *binary,
 	                                          GLsizei length);

    typedef void (__stdcall* PPROGRAMINFOLOG)(	GLuint program,
 	    GLsizei maxLength,
 	    GLsizei *length,
 	    char *infoLog);

    typedef void (__stdcall* PDISPATCHCOMPUTE) ( GLuint x, GLuint y, GLuint z );

    typedef void (__stdcall* PGENBUFFERS) ( GLsizei n, GLuint* p );
    typedef void (__stdcall* PBINDBUFFER) ( GLenum target,
 	                                        GLuint buffer);
    typedef void (__stdcall* PBUFFERDATA) ( GLenum target, GLsizei size, const void *data, GLenum usage);

    typedef void* (__stdcall* PMAPBUFFER)  (GLenum target, GLenum access);
    typedef void (__stdcall* PUNMAPBUFFER) (GLenum target );
    typedef void (__stdcall* PBINDBUFFERBASE) (GLenum target, GLuint index, GLuint buffer);
    typedef void (__stdcall* PRELEASECOMPILER) ();
    
    typedef void (__stdcall* PBEGINQUERY) ( GLenum target, GLuint id );
    typedef void (__stdcall* PENDQUERY) ( GLenum target );
    
    typedef void (__stdcall* PGETQUERYOBJECTUIV)(	GLuint id,
 	    GLenum pname,
 	    GLuint * params);

    PCREATESHADER           glCreateShader;
    PSHADERSOURCE           glShaderSource;
    PCOMPILESHADER          glCompileShader;
    PGETSHADERINFOLOG       glGetShaderInfoLog;
    PCREATEPROGRAM          glCreateProgram;
    PATTACHSHADER           glAttachShader;
    PLINKPROGRAM            glLinkProgram;
    PDEBUGMESSAGECALLBACK   glDebugMessageCallback;
    PGETPROGRAMBINARY       glGetProgramBinary;
    PGETPROGRAMIV           glGetProgramiv;
    PPROGRAMBINARY          glProgramBinary;
    PPROGRAMINFOLOG         glGetProgramInfoLog;
    PLINKPROGRAM            glUseProgram;
    
    PGENBUFFERS  glGenBuffers;
    PGENBUFFERS  glDeleteBuffers;
    PLINKPROGRAM glDeleteProgram;
    PCOMPILESHADER glDeleteShader;

    PBINDBUFFER  glBindBuffer;
    PBUFFERDATA  glBufferData;
    PMAPBUFFER   glMapBuffer;
    PUNMAPBUFFER glUnmapBuffer;
    PBINDBUFFERBASE glBindBufferBase;
    PRELEASECOMPILER glReleaseShaderCompiler;

    PDISPATCHCOMPUTE glDispatchCompute;

    PGENBUFFERS        glGenQueries;
    PGENBUFFERS        glDeleteQueries;
    PBEGINQUERY        glBeginQuery;
    PENDQUERY          glEndQuery;
    PGETQUERYOBJECTUIV glGetQueryObjectuiv;


    void __stdcall glDebugProc( GLenum source, GLenum type, GLuint id,
       GLenum severity, GLsizei length, const char* message, const void* userParam)
    {
        printf("FROM DRIVER: %s\n", message );

    }

   
    #define STRINGIFY(...) "#version 430 core\n" #__VA_ARGS__

    // Template blob which we will base our shaders on
    //  We use the template blob to provide shader storage buffers for us to read and write to 
    static const char* SHADER0 = STRINGIFY(
        layout (local_size_x = 8) in;

        layout (std430,binding=0)
        buffer Output0
        {
           uint g_TID0[];
        };

        
        layout (std430,binding=1)
        buffer Output1
        {
           uint g_TID1[];
        };

        
        layout (std430,binding=2)
        buffer Output2
        {
           uint g_TID2[];
        };

        
        layout (std430,binding=3)
        buffer Output3
        {
           uint g_TID3[];
        };

         
        layout (std430,binding=4)
        buffer Output4
        {
           uint g_TID4[];
        };

         
        layout (std430,binding=5)
        buffer Output5
        {
           uint g_TID5[];
        };


        void main() 
        {
            uvec2 tid = gl_GlobalInvocationID.xy;
            g_TID0[tid.x] = tid.x;
            g_TID1[tid.x] = tid.x*2;
            g_TID2[tid.x] = tid.x*4;
            g_TID3[tid.x] = tid.x*8;
            g_TID4[tid.x] = tid.x*16;           
            g_TID5[tid.x] = tid.x*32;
        }
    );


    Blob g_TemplateBlob;
    size_t g_nTemplateIsaOffset;
    GLenum g_eBinaryFormat;

    HDC g_hDC = 0;
    HGLRC g_hGLContext=0;

    bool Init( bool bCreateGLContext )
    {
        if( bCreateGLContext )
        {
            g_hDC = GetDC(0);
            if( !g_hDC )
                return false;

            PIXELFORMATDESCRIPTOR pfd =
	        {
		        sizeof(PIXELFORMATDESCRIPTOR),
		        1,
		        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		        32,                        //Colordepth of the framebuffer.
		        0, 0, 0, 0, 0, 0,
		        0,
		        0,
		        0,
		        0, 0, 0, 0,
		        24,                        //Number of bits for the depthbuffer
		        8,                        //Number of bits for the stencilbuffer
		        0,                        //Number of Aux buffers in the framebuffer.
		        PFD_MAIN_PLANE,
		        0,
		        0, 0, 0
	        };

	        int  letWindowsChooseThisPixelFormat;
	        letWindowsChooseThisPixelFormat = ChoosePixelFormat(g_hDC, &pfd); 
	        SetPixelFormat(g_hDC,letWindowsChooseThisPixelFormat, &pfd);

	        g_hGLContext = wglCreateContext(g_hDC);
            if( !g_hGLContext )
            {
                ReleaseDC(0,g_hDC);
                g_hDC=0;
                return false;
            }

	        wglMakeCurrent (g_hDC, g_hGLContext);
        }

        glCreateShader          = (PCREATESHADER)         wglGetProcAddress( "glCreateShader" );
        glShaderSource          = (PSHADERSOURCE)         wglGetProcAddress( "glShaderSource" );
        glCompileShader         = (PCOMPILESHADER)        wglGetProcAddress( "glCompileShader");
        glGetShaderInfoLog      = (PGETSHADERINFOLOG)     wglGetProcAddress( "glGetShaderInfoLog" );
        glCreateProgram         = (PCREATEPROGRAM)        wglGetProcAddress( "glCreateProgram" );
        glAttachShader          = (PATTACHSHADER)         wglGetProcAddress( "glAttachShader" );
        glLinkProgram           = (PLINKPROGRAM)          wglGetProcAddress( "glLinkProgram" );
        glGetProgramBinary      = (PGETPROGRAMBINARY)     wglGetProcAddress( "glGetProgramBinary" );
        glDebugMessageCallback  = (PDEBUGMESSAGECALLBACK) wglGetProcAddress( "glDebugMessageCallback" );
        glGetProgramiv          = (PGETPROGRAMIV)         wglGetProcAddress( "glGetProgramiv" );
        glProgramBinary         = (PPROGRAMBINARY)        wglGetProcAddress( "glProgramBinary" );
        glGetProgramInfoLog     = (PPROGRAMINFOLOG)       wglGetProcAddress( "glGetProgramInfoLog");
        glUseProgram            = (PLINKPROGRAM)          wglGetProcAddress( "glUseProgram" );

        glGenBuffers            = (PGENBUFFERS)      wglGetProcAddress( "glGenBuffers" );
        glDeleteBuffers         = (PGENBUFFERS)      wglGetProcAddress( "glDeleteBuffers" );
        glBindBuffer            = (PBINDBUFFER)      wglGetProcAddress( "glBindBuffer" );
        glBufferData            = (PBUFFERDATA)      wglGetProcAddress( "glBufferData" );
        glMapBuffer             = (PMAPBUFFER)       wglGetProcAddress( "glMapBuffer" );
        glUnmapBuffer           = (PUNMAPBUFFER)     wglGetProcAddress( "glUnmapBuffer" );
        glBindBufferBase        = (PBINDBUFFERBASE)  wglGetProcAddress( "glBindBufferBase");
        glDispatchCompute       = (PDISPATCHCOMPUTE) wglGetProcAddress( "glDispatchCompute" );
        glDeleteProgram         = (PLINKPROGRAM)     wglGetProcAddress( "glDeleteProgram");
        glDeleteShader          = (PCOMPILESHADER)   wglGetProcAddress( "glDeleteShader");
        glReleaseShaderCompiler = (PRELEASECOMPILER) wglGetProcAddress( "glReleaseShaderCompiler");

        glGenQueries            = (PGENBUFFERS)         wglGetProcAddress( "glGenQueries" );
        glDeleteQueries         = (PGENBUFFERS)         wglGetProcAddress( "glDeleteQueries" );
        glBeginQuery            = (PBEGINQUERY)         wglGetProcAddress( "glBeginQuery" );
        glEndQuery              = (PENDQUERY)           wglGetProcAddress( "glEndQuery" );
        glGetQueryObjectuiv     = (PGETQUERYOBJECTUIV)  wglGetProcAddress( "glGetQueryObjectuiv" );

        glEnable(GL_DEBUG_OUTPUT);

        glDebugMessageCallback( &glDebugProc,0);

        const char* CODE = SHADER0;
        GLint length  = strlen(CODE);
        GLuint hShader = glCreateShader( GL_COMPUTE_SHADER );
        glShaderSource( hShader, 1, &CODE, &length );
        glCompileShader( hShader );

    
        GLuint hProgram = glCreateProgram();
        glAttachShader(hProgram,hShader);
        glLinkProgram(hProgram);



        GLint nBinaryLength;
        glGetProgramiv( hProgram, GL_PROGRAM_BINARY_LENGTH, &nBinaryLength );
        
        g_TemplateBlob.SetLength(nBinaryLength);

        glGetProgramBinary( hProgram, nBinaryLength, &nBinaryLength, &g_eBinaryFormat, g_TemplateBlob.GetBytes() );

        size_t nIsaLen=0;
        if( !HAXWell::FindIsaInBlob( &g_nTemplateIsaOffset, &nIsaLen, 
                                      g_TemplateBlob.GetBytes(), g_TemplateBlob.GetLength() ) )
        {
            return false;
        }

        glUseProgram(0);
        glDeleteProgram(hProgram);
        glDeleteShader(hShader);
        

        
        printf( "GL_RENDERER: %s\n", glGetString( GL_RENDERER ) );
        printf( "GL_VERSION: %s\n", glGetString( GL_VERSION ) );

        return true;
    }




    ShaderHandle CreateShader( const HAXWell::ShaderArgs& rArgs )
    {
        
        Blob blob;
        HAXWell::PatchBlob( blob, rArgs, g_TemplateBlob, g_nTemplateIsaOffset );

        GLuint hProgram = glCreateProgram();
        glProgramBinary( hProgram, g_eBinaryFormat, blob.GetBytes(), blob.GetLength());

        GLint status;
        glGetProgramiv( hProgram, GL_LINK_STATUS, &status );
        if( !status )
        {
            glDeleteProgram(hProgram);
            return 0;
        }

        return (ShaderHandle)hProgram;
    }

    ShaderHandle CreateGLSLShader( const char* pGLSL )
    {
        // TODO: Handle compile/link fails
        GLint length  = strlen(pGLSL);
        GLuint hShader = glCreateShader( GL_COMPUTE_SHADER );
        glShaderSource( hShader, 1, &pGLSL, &length );
        glCompileShader( hShader );

        
        char pLog[2048];
        GLsizei nLogSize;

        glGetShaderInfoLog( hShader, sizeof(pLog), &nLogSize, pLog );

        printf(pLog);

        GLuint hProgram = glCreateProgram();
        glAttachShader(hProgram,hShader);
        glLinkProgram(hProgram);
        return (ShaderHandle)hProgram;
    }

    void ReleaseShader( ShaderHandle h )
    {
        glDeleteProgram( (GLuint)h );
    }

    BufferHandle CreateBuffer( const void* pOptionalInitialData, size_t nDataSize )
    {
        GLuint ssbo = 0;
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, nDataSize, pOptionalInitialData, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return (BufferHandle)ssbo;
    }

    void* MapBuffer( BufferHandle h )
    {
        GLuint hBuffer = (GLuint)h;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, hBuffer);
        return glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE );
    }

    void UnmapBuffer( BufferHandle h )
    {
        GLuint hBuffer = (GLuint)h;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, hBuffer);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }


    void ReleaseBuffer( BufferHandle h )
    {
        glDeleteBuffers( 1, (GLuint*)&h );
    }
     

    TimerHandle BeginTimer()
    {
        GLuint hQuery;
        glGenQueries( 1, &hQuery );
        glBeginQuery( GL_TIME_ELAPSED, hQuery );
        return (TimerHandle)hQuery;
    }
 
    void EndTimer( TimerHandle hTimer )
    {
        glEndQuery( GL_TIME_ELAPSED );
    }

    timer_t ReadTimer( TimerHandle hTimer )
    {
        GLuint hTimerName = (GLuint) hTimer;
        timer_t time;
        glGetQueryObjectuiv( hTimerName, GL_QUERY_RESULT, &time );
        glDeleteQueries( 1, &hTimerName );
        return time;
    }


    void DispatchShader( ShaderHandle hShader, BufferHandle* pBuffers, size_t nBuffers, size_t nThreadGroups )
    {
        glUseProgram( (GLuint) hShader);

        for( size_t i=0; i<nBuffers; i++ )
        {
            GLuint hBuffer = (GLuint) pBuffers[i];
            glBindBuffer( GL_SHADER_STORAGE_BUFFER, hBuffer );
            glBindBufferBase( GL_SHADER_STORAGE_BUFFER, i, hBuffer );
        }

        glDispatchCompute( nThreadGroups,1,1 );
    }

    void Finish()
    {
        glFinish();
    }

    bool RipIsaFromGLSL( Blob& rBlob, const char* pGLSL )
    {
        bool success = false;
        
       
        
        // TODO: Handle compile/link fails
        GLint length  = strlen(pGLSL);
        GLuint hShader = glCreateShader( GL_COMPUTE_SHADER );
        glShaderSource( hShader, 1, &pGLSL, &length );
        glCompileShader( hShader );

        
        char pLog[2048];
        GLsizei nLogSize;

        glGetShaderInfoLog( hShader, sizeof(pLog), &nLogSize, pLog );

        printf(pLog);

        GLuint hProgram = glCreateProgram();
        glAttachShader(hProgram,hShader);
        glLinkProgram(hProgram);

        GLint nBinaryLength;
        glGetProgramiv( hProgram, GL_PROGRAM_BINARY_LENGTH, &nBinaryLength );
        
        void* blob = malloc(nBinaryLength);

        GLenum eFormat;
        glGetProgramBinary( hProgram, nBinaryLength, &nBinaryLength, &eFormat, blob );

        size_t nIsaOffset;
        size_t nIsaLength;
        if( FindIsaInBlob( &nIsaOffset, &nIsaLength, blob, nBinaryLength ) )
        {
            rBlob.SetLength(nIsaLength);
            memcpy( rBlob.GetBytes(), ((char*)blob)+nIsaOffset, nIsaLength);
            success = true;
        }

      

        glDeleteProgram(hProgram);
        glDeleteShader(hShader);
        free(blob);
        return success;
    }

}