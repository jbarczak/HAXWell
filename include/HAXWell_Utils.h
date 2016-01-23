
#ifndef _HAXWELL_UTILS_H_
#define _HAXWELL_UTILS_H_

namespace HAXWell
{
    typedef unsigned int DWORD;
    static_assert( sizeof(DWORD) == 4, "derp" );

    struct ShaderArgs;

    class Blob
    {
    public:
        Blob( ) : m_pBytes(0), m_nLength(0)
        {
        }

        ~Blob( );

        void* GetBytes() { return m_pBytes; }
        const void* GetBytes() const { return m_pBytes; }
        size_t GetLength() const { return m_nLength; }

        void SetLength( size_t nLength );

    private:
        Blob( const Blob& blob ) = delete;
        Blob& operator=( const Blob& blob ) = delete;

        void* m_pBytes;
        size_t m_nLength;
    };
    
  


    // Hash function used by Intel OpenGL driver to sign blobs
    ///   The hash length is 64-bits (2 dwords)
    void DriverHashFunction( DWORD* pCRCOut, const DWORD* pBlob, DWORD nBlobLengthInDWORDs );

    /// Locate valid GEN Isa inside an Intel OpenGL program blob
    bool FindIsaInBlob( size_t* pIsaOffset, size_t* pIsaLength, const void* pBlob, size_t nBlobLength );

    /// Create a new Intel OpenGL program blob by patching an existing one
    void PatchBlob( Blob& rOutputBlob, const ShaderArgs& rArgs, const Blob& rTemplateBlob, size_t nTemplateIsaStart );

}


#endif