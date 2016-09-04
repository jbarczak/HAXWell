

//=====================================================================================================================
//
//   PPMImage.cpp
//
//   Implementation of class: Simpleton::PPMImage
//     Very ancient code, some of which I wrote in college.  Don't judge me...
//
//   The lazy man's utility library
//   Joshua Barczak
//   Copyright 2010 Joshua Barczak
//
//   LICENSE:  See Doc\License.txt for terms and conditions
//
//=====================================================================================================================

#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "PPMImage.h"

namespace Simpleton
{

    //=====================================================================================================================
    //
    //         Constructors/Destructors
    //
    //=====================================================================================================================


    PPMImage::PPMImage()
    {
	    m_pPixels = NULL;
	    m_nWidth = 0;
	    m_nHeight = 0;
    }

    PPMImage::PPMImage( unsigned int width, unsigned int height ) :m_nWidth(width),m_nHeight(height), m_pPixels(0)
    {
        AllocPixels(width,height);
    }

    PPMImage::PPMImage(const PPMImage& img) : m_pPixels(0)
    {
        this->AllocPixels(img.m_nWidth, img.m_nHeight);
        memcpy(this->m_pPixels, img.m_pPixels, sizeof(PIXEL)*m_nWidth*m_nHeight);

    }

    PPMImage::~PPMImage()
    {
	    FreePixels();
    }

    //=====================================================================================================================
    //
    //            Public Methods
    //
    //=====================================================================================================================
 
    const PPMImage& PPMImage::operator =(const PPMImage& img)
    {
        this->FreePixels();
        this->AllocPixels(img.m_nWidth, img.m_nHeight);
        memcpy(this->m_pPixels, img.m_pPixels, sizeof(PIXEL)*m_nWidth*m_nHeight);

        return *this;
    }


    void PPMImage::GetPixelBytes( int x, int y, unsigned char bytes[] ) const
    {
        PIXEL* pPix = &m_pPixels[y*m_nWidth+x];
        bytes[0] = pPix->r;
        bytes[1] = pPix->g;
        bytes[2] = pPix->b;
    }

    void PPMImage::SetPixel(int x, int y, float r, float g, float b )
    {
        if(x >= 0 && x < (int)m_nWidth && y >= 0 && y < (int)m_nHeight)
        {
            m_pPixels[y*m_nWidth + x].r=(unsigned char)(r*255.0f);
            m_pPixels[y*m_nWidth + x].g=(unsigned char)(g*255.0f);
            m_pPixels[y*m_nWidth + x].b=(unsigned char)(b*255.0f);
        }
    }	
    void PPMImage::SetPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b )
    {
        if(x >= 0 && x < (int)m_nWidth && y >= 0 && y < (int)m_nHeight)
        {
            m_pPixels[y*m_nWidth + x].r=r;
            m_pPixels[y*m_nWidth + x].g=g;
            m_pPixels[y*m_nWidth + x].b=b;
        }
    }	

    /// Writes a PPM (P6) file containing the image
    bool PPMImage::SaveFile(const char* sFile)
    {
        if( m_pPixels == NULL )
            return false;

	    // binary write must be specified or this code blows up on windows
	    FILE* fp = fopen(sFile, "wb");	
        		
	    // sanity check
	    if(fp == NULL)
		    return false;
    	
	    // print PPM header stuff
	    fprintf(fp, "P6\n%d %d\n%d\n", this->m_nWidth, this->m_nHeight, 255);
        
        // write pixels
	    fwrite( m_pPixels, 3, m_nWidth*m_nHeight, fp );

	    fclose(fp);

	    return true;

    }


    bool PPMImage::LoadFile(const char* filename)
    {
        // try and open the image file
        FILE* fp = fopen(filename, "rb");
        if(fp == NULL)
        {
            return false;
        }

      

        // first two bytes had better be "P6"
        char c0 = fgetc(fp);
        char c1 = fgetc(fp);
        #define MAGIC(c0,c1) ((c0<<8)|c1)
        const unsigned int P6 = MAGIC('P','6');
        const unsigned int BM = MAGIC('B','M');
        unsigned int Magic = MAGIC(c0,c1);
        
        switch( Magic )
        {
        case P6: return LoadPPM(fp);
        case BM: return LoadBMP(fp);
        default:
            fclose(fp);
            return false;
        }

    }

    bool PPMImage::Equals( const PPMImage& img ) const
    {
        if( img.GetWidth() != GetWidth() || img.GetHeight() != GetHeight() )
            return false;
        return memcmp( m_pPixels, img.m_pPixels, m_nWidth*m_nHeight*sizeof(PIXEL) ) == 0 ;
    }

    void PPMImage::Pow( float x)
    {
        unsigned int  n = m_nWidth*m_nHeight;
        for( unsigned int i=0; i<n; i++ )
        {
            float r = m_pPixels[i].r/255.0f;
            float g = m_pPixels[i].g/255.0f;
            float b = m_pPixels[i].b/255.0f;
            r = 255.0f*powf( r, x );
            g = 255.0f*powf( g, x );
            b = 255.0f*powf( b, x );
            m_pPixels[i].r = (unsigned char)r;
            m_pPixels[i].g = (unsigned char)g;
            m_pPixels[i].b = (unsigned char)b;
        }
    }
    

    void PPMImage::GetPixel( int x, int y, float rgb[3] ) const
    {
        const PIXEL* pPix = &m_pPixels[y*m_nWidth + x];
        rgb[0] = pPix->r/255.0f;
        rgb[1] = pPix->g/255.0f;
        rgb[2] = pPix->b/255.0f;
    }



    //=====================================================================================================================
    //
    //            Private Methods
    //
    //=====================================================================================================================

    bool PPMImage::LoadBMP(FILE* fp)
    {
        typedef unsigned int DWORD;
        typedef int LONG;
        typedef unsigned short WORD;

        enum Compression
        {
            BI_RGB=0,
            BI_RLE8=1,
            BI_RLE4=2,
            BI_BITFIELDS=3,
            BI_JPEG=4,
            BI_PNG=5,
        };

        struct FileHeader
        {
            DWORD FileSize;
            WORD  bfReserved1;
            WORD  bfReserved2;
            DWORD BitmapOffset;
        };

        struct BITMAPINFOHEADER
        {
            DWORD biSize;
            LONG  biWidth;
            LONG  biHeight;
            WORD  biPlanes;
            WORD  biBitCount;
            DWORD biCompression;
            DWORD biSizeImage;
            LONG  biXPelsPerMeter;
            LONG  biYPelsPerMeter;
            DWORD biClrUsed;
            DWORD biClrImportant;
        };


        static_assert( sizeof(DWORD) == 4  
                       && sizeof(WORD) == 2  
                       && sizeof(LONG) == 4  
                       && sizeof(FileHeader) == 12 
                       && sizeof(BITMAPINFOHEADER) == 40 
                       ,"Check the compiler"   );

        FileHeader hdr;
        size_t nCount = fread( &hdr, sizeof(FileHeader), 1, fp );
        if( nCount != 1 )
            goto fail;

        DWORD dibHeaderSize = hdr.BitmapOffset - (sizeof(FileHeader)+2);
        switch( dibHeaderSize )
        {
        case 12:    goto fail; // OS2 bitmapcoreheader
        case 64:    goto fail; // OS2 bitmapcoreheader2
        case 52:    goto fail; // various other bastardizations 
        case 56:    goto fail; // ...
        case 40:    // various flavors of windows BITMAPINFOHEADER
        case 108:
        case 124:
        default: 
            {
                BITMAPINFOHEADER info;
                nCount = fread( &info,sizeof(BITMAPINFOHEADER), 1, fp ); 
                if( nCount != 1 )
                    goto fail;
                if( info.biSize != dibHeaderSize || info.biPlanes != 1 )
                    goto fail;                
                if( info.biCompression != BI_RGB )
                    goto fail;

                DWORD nWidth  = info.biWidth;
                DWORD nHeight = abs(info.biHeight);
                DWORD nPixels = nWidth*nHeight;
                        
                fseek( fp, hdr.BitmapOffset, SEEK_SET );
                switch( info.biBitCount )
                {
                case 1:
                case 4:
                case 8:
                case 16:
                    goto fail;
                case 24:
                    {
                        AllocPixels( nWidth, nHeight );

                        // read 24-bit pixels in bgr order
                        if( fread( m_pPixels, sizeof(PIXEL), m_nWidth*m_nHeight, fp ) != nPixels )
                            goto fail;

                        // tranpose bottom-up bitmaps and invert channel order
                        if( info.biHeight > 1 )
                        {
                            PIXEL* pTop = m_pPixels;
                            PIXEL* pBottom = m_pPixels + nWidth*(nHeight-1);
                            for( DWORD y=0; y<nHeight/2; y++ )
                            {
                                for( DWORD x=0; x<nWidth; x++ )
                                {
                                    PIXEL p0 = pTop[x];
                                    PIXEL p1 = pBottom[x];
                                    pTop[x].r = p1.b;
                                    pTop[x].g = p1.g;
                                    pTop[x].b = p1.r;
                                    pBottom[x].r = p0.b;
                                    pBottom[x].g = p0.g;
                                    pBottom[x].b = p0.r;
                                }     
                                pTop += nWidth;
                                pBottom -= nWidth;
                            }
                        }
                        else
                        {
                            // just invert channel order
                            for( DWORD p=0; p<nPixels; p++ )
                            {
                                char r = m_pPixels[p].r;
                                char b = m_pPixels[p].b;
                                m_pPixels[p].b = r;
                                m_pPixels[p].r = b;
                            }
                        }
                    }
                    break;
                case 32:
                    {
                        // read 32-bit pixels in bgr order
                        DWORD* tmp = (DWORD*)malloc(sizeof(DWORD)*nPixels);
                        if( fread( m_pPixels, sizeof(DWORD), nPixels, fp ) != nPixels )
                        {
                            free(tmp);
                            goto fail;
                        }

                        AllocPixels( nWidth, nHeight );
                        
                        if( info.biHeight > 1 )
                        {
                            // tranpose bottom-up bitmaps and invert channel order                        
                            PIXEL* pOut = m_pPixels;                                
                            for( int y=info.biHeight; y>0; y-- )
                            {
                                const DWORD* pIn = tmp + y*nWidth;
                                for( DWORD x=0; x<nWidth; x++ )
                                {
                                    pOut[x].b =  pIn[x]&0xff;
                                    pOut[x].g = (pIn[x]&(0xff00)) >>8;
                                    pOut[x].r = (pIn[x]&(0xff0000)) >> 16;
                                }
                                pOut += nWidth;
                            }
                        }
                        else
                        {
                            // copy and invert channel order
                            for( DWORD p=0; p<nPixels; p++ )
                            {
                                m_pPixels[p].b = tmp[p]&0xff;
                                m_pPixels[p].g = (tmp[p]&(0xff00)) >>8;
                                m_pPixels[p].r = (tmp[p]&(0xff0000)) >> 16;
                            }
                        }

                        free(tmp);
                    }
                    break;
                }


            }
        }

        fclose(fp);
        return true;

    fail:
        FreePixels();
        fclose(fp);
        return false;
    }

    bool PPMImage::LoadPPM(FILE* fp)
    {        
        
          /*
            For reference, here is the PPM image format, direct from the man page.  I stole this from
            the following URL: http://www.cis.ohio-state.edu/~parent/classes/681/ppm/ppm-man.html
            
            A "magic number" for identifying the file type. A ppm file's magic number is the two characters "P3". 
            Whitespace (blanks, TABs, CRs, LFs). 
            A width, formatted as ASCII characters in decimal. 
            Whitespace. 
            A height, again in ASCII decimal. 
            Whitespace. 
            The maximum color-component value, again in ASCII decimal. 
            Whitespace. 
            Width * height pixels, each three ASCII decimal values between 0 and the specified maximum value, starting at the top-left 
            corner of the pixmap, proceeding in normal English reading order. The three values for each pixel represent red, 
            green, and blue, respectively; a value of 0 means that color is off, and the maximum value means that color is maxxed out. 
            Characters from a "#" to the next end-of-line are ignored (comments). 
            No line should be longer than 70 characters. 
            
            The "magic number" is "P6" instead of "P3". 
            The pixel values are stored as plain bytes, instead of ASCII decimal. 
            Whitespace is not allowed in the pixels area, and only a single character of whitespace (typically a newline) is 
            allowed after the maxval. 
        */
        unsigned int width, height, maxval;

        // parse out the width, height, and maxval, ignoring whitespace and comments.
        bool at_bits = false, got_width = false, got_height = false, got_maxval = false;
        while(!at_bits && !feof(fp))
        {
            SkipToToken(fp);

            if(!got_width)
            {
                // read width
                if(fscanf(fp, "%d", &width) != 1)
                {
                    fclose(fp);
                    return false;
                }
                got_width = true;
            }
            else if(!got_height)
            {
                // read height
                if(fscanf(fp, "%d", &height) != 1)
                {
                    fclose(fp);
                    return false;
                }
                got_height = true;
            }
            else if(!got_maxval)
            {
                // read maxval
                if(fscanf(fp, "%d", &maxval) != 1)
                {
                    fclose(fp);
                    return false;
                }
                got_maxval = true;
                at_bits = true;
            }
        }

        // verify that we got all the header information we needed
        // if we're EOF, it means we did not
        if(feof(fp) && !got_width || !got_height || !got_maxval)
        {
            fclose(fp);
            return false;
        }

        // there are now 3*width*height bytes left in the file.
        // excluding the extraneous whitespace that may or may not be there
        // allocate enough space for the rest of the data
        unsigned char* bytes = (unsigned char*)malloc(3*width*height);


        // store current file position
        long offs = ftell(fp);

        
        // read the data
        size_t bytes_read = fread(bytes, 1, 3*width*height,fp);
        if( bytes_read < 3*width*height  )
        {
            // not enough bytes
            fclose(fp);
            free(bytes);
            return false;
        }
        else if(  !feof(fp) )
        {
            // still more data in file, means that there was
            // extraneous whitespace before that needs to be skipped

            int extra_bytes=0;
            while( !feof(fp) )  // count number of bytes to skip
            {
                extra_bytes++;
                fgetc(fp);
            }

            extra_bytes--; // disregard EOF character

            fseek(fp, offs + extra_bytes, SEEK_SET);      // seek back to start of data
            bytes_read = fread(bytes, 1, 3*width*height,fp);
            if( bytes_read != 3*width*height )
            {
                // something is wrong
                fclose(fp);
                free(bytes);
                return false;
            }
        }
        
        
        
        // convert data then store the bytes.  We have to account for PPM's "max value"
        AllocPixels(width, height);

        int i=0;
        for(int y=0; y<GetHeight(); y++)
        {
            for(int x=0; x<GetWidth(); x++)
            {
                float r = bytes[i]   / (float)maxval;
                float g = bytes[i+1] / (float)maxval;
                float b = bytes[i+2] / (float)maxval;        
                i += 3;
                SetPixel(x,y,r,g,b);
            }

        }
        
        free(bytes);
        return true;
    }

    void PPMImage::SkipToToken(FILE* fp)
    {
        bool hit_token = false;
        while(!feof(fp) && !hit_token)
        {
            // skip to beginning of next token (width, height, or maxval)
            char c = fgetc(fp);
            if( c == '#')
            {
                // comment, skip ahead till next newline
                while(!feof(fp) && c != '\n' && c != '\f' && c != '\r')
                {
                    c = fgetc(fp);
                }
            }
            else if(c == '\n' || c == '\f' || c == '\r' || c == '\t' || c == ' ')
            {
                // whitespace, skip it
            }
            else
            {
                hit_token = true;

                // we need that character we just read, so backtrack so we're pointed at it
	            ungetc(c,fp);
            }
        }

    }


    void PPMImage::AllocPixels(unsigned int iWidth, unsigned int iHeight)
    {
	    // prevent accidental memory leaks
	    if(m_pPixels != NULL)
	    {
		    FreePixels();
	    }

	    m_nWidth = iWidth;
	    m_nHeight = iHeight;

	    // and make new pixel memory
	    m_pPixels = (PIXEL*) malloc(m_nHeight * m_nWidth*sizeof(PIXEL));
    }



    void PPMImage::FreePixels()
    {
	    free(m_pPixels);
	    m_pPixels = NULL;
    }
}

