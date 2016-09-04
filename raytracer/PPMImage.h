//=====================================================================================================================
//
//   PPMImage.h
//
//   Definition of class: Simpleton::PPMImage
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2010 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================

#ifndef _PPMIMAGE_H_
#define _PPMIMAGE_H_

#include <stdio.h>

namespace Simpleton
{

    //=====================================================================================================================
    /// \ingroup Simpleton
    /// \brief Very basic image class which supports PPM image I/O
    //=====================================================================================================================
    class PPMImage
    {
    public:

        struct PIXEL
        {
	        unsigned char r;
	        unsigned char g;
	        unsigned char b;
        } ;

        PPMImage();
        PPMImage( unsigned int width, unsigned int height );
        PPMImage(const PPMImage& img);
	    ~PPMImage();

        const PPMImage& operator=(const PPMImage& img);

	    inline unsigned int GetHeight() const { return m_nHeight;};
	    inline unsigned int GetWidth() const { return m_nWidth;};

        void GetPixelBytes( int x, int y, unsigned char bytes[3] ) const;
        const void* GetRawBytes() const { return (const void*) m_pPixels; };
        void GetPixel( int x, int y, float rgb[3] ) const;
	    void SetPixel(int x, int y, float r, float g, float b );
    	void SetPixel( int x, int y, unsigned char r, unsigned char g, unsigned char b );
	    bool SaveFile(const char* sFile);

	    bool LoadFile(const char* sFile);
        bool Equals( const PPMImage& img ) const;

        void SetSize( unsigned int width, unsigned int height )
        {
            AllocPixels( width, height );
        }

        void Pow( float x );
        

    private:

        bool LoadPPM( FILE* fp );
        bool LoadBMP( FILE* fp );

        void SkipToToken( FILE* fp );
	    void AllocPixels(unsigned int iWidth, unsigned int iHeight);
	    void FreePixels();

	    PIXEL* m_pPixels;
	    unsigned int m_nWidth;
	    unsigned int m_nHeight;

    };

    
}

#endif // _PPMIMAGE_H_
