
// nat June 2001
// 
// May 2002 -- modified by nat
// LoadDIB() was not working with grayscale images. Corrected.
//

#ifndef __YARPDIBConverterh__
#define __YARPDIBConverterh__

//
// This class is mostly ok also for QNX.
//	- LATER: implement iplConvertToDIB within FakeIpl.
//

#ifdef __WIN__

#include "YARPImage.h"
#include "iplWind.h"

class YARPDIBConverter
{
public:
	// default constructor.
	YARPDIBConverter();
	// contructor -- image specified.
	YARPDIBConverter(const YARPGenericImage& img);
	~YARPDIBConverter();
	// resize internal image.
	void Resize(const YARPGenericImage& img);

	// convert image to DIB.
	inline const unsigned char *ConvertToDIB (const YARPGenericImage& img)
	{
		assert (img.GetIplPointer != NULL);

		_refresh_dib(img);
		return bufDIB;
	};

	inline void ConvertFromDIB (YARPGenericImage& img)
	{
		img.Resize(dimX, dimY);
		
		iplConvertFromDIB((BITMAPINFOHEADER *)bufDIB, img);
	};

	inline const unsigned char *GetBuffer()
	{
		return bufDIB;
	};

	inline bool LoadDIB(const char *filename)
	{

		BITMAPFILEHEADER fhdr;
		BITMAPINFOHEADER Hdr;

		int	addedBytesDIB = _pad_bytes(dimX,4);

		FILE  *fp = fopen(filename, "rb");
		if (fp == NULL)
			return false;
		
		char *dummy = (char *) &fhdr;
		// read file header
		fread(dummy, sizeof(char), sizeof(BITMAPFILEHEADER), fp);
		
		// read image header
		dummy = (char *) &Hdr;
		fread(dummy, sizeof(char), sizeof(BITMAPINFOHEADER), fp);

		if ( (dimX != Hdr.biWidth) || (dimY != Hdr.biHeight) ||
			((pixelType == YARP_PIXEL_RGB) && (Hdr.biBitCount != 24)) ||
			((pixelType == YARP_PIXEL_MONO) && (Hdr.biBitCount != 8)))
			{
				if (Hdr.biBitCount == 8)
					pixelType = YARP_PIXEL_MONO;
				else if (Hdr.biBitCount == 24)
					pixelType = YARP_PIXEL_RGB;
				else
					return false;

				dimX = Hdr.biWidth;
				dimY = Hdr.biHeight;
				
				_alloc_dib();
			}
		
		
		// copy header ...
		memcpy(bufDIB, &Hdr, sizeof(BITMAPINFOHEADER));

		// read the palette, if any
		if (pixelType == YARP_PIXEL_MONO) 
		{
			COLORREF* rgb = (COLORREF *)(bufDIB + sizeof(BITMAPINFOHEADER));
			fread(rgb, sizeof(RGBQUAD),256,fp);
		}

		// read bits ...
		fread(dataAreaDIB, sizeof(char), imageSize, fp);
				
		// close file
		fclose(fp);

		return true;

	};
	
	inline void const SaveDIB(const char *filename)
	{
		BITMAPFILEHEADER fhdr;
		LPBITMAPINFOHEADER lpbi;

		int	addedBytesDIB = _pad_bytes(dimX,4);
		int dibSize;
		int offset;

		lpbi = (LPBITMAPINFOHEADER) bufDIB;
		
		dibSize = imageSize + headerSize;
		offset = sizeof(BITMAPFILEHEADER) + headerSize;
				
		FILE  *fp    = fopen(filename, "wb");
		
		fhdr.bfType = 0x4d42;
		fhdr.bfSize = dibSize + sizeof(BITMAPFILEHEADER);
		fhdr.bfReserved1 = 0;
		fhdr.bfReserved2 = 0;
		fhdr.bfOffBits = (DWORD) offset;

		// write file header
		fwrite((char*) &fhdr, sizeof(char), sizeof(BITMAPFILEHEADER), fp);
		
		// write DIB header and Bits
		fwrite(bufDIB, sizeof(char), dibSize, fp);

		// close file
		fclose(fp);
	
	};

private:
	// prepare internal buffer
	void _alloc_dib();
	// destroy internal buffer
	void _free_dib();

	inline void _refresh_dib (const YARPGenericImage& img) 
	{
		// prepare internal dib
		// TODO need the whole data area be prepared ?
		
		if (dimX != img.GetWidth() || dimY != img.GetHeight() || pixelType != img.GetID())
			Resize(img);
		if (bufDIB  == NULL)
			_alloc_dib ();

		if (pixelType == YARP_PIXEL_HSV)
		{
			// special conversion is required ...
			unsigned char *dest = dataAreaDIB;
			unsigned char *src = (unsigned char *) img.GetRawBuffer();

			int addedBytesDIB = _pad_bytes(dimX*3,4);
			int numBytes = (dimX*3+addedBytesDIB)*dimY;

			memcpy(src, dest, numBytes);
		}
		else
			iplConvertToDIB((IplImage *)img, (BITMAPINFOHEADER*)bufDIB, IPL_DITHER_NONE, IPL_PALCONV_NONE);
	};

	inline int _pad_bytes(int linesize, int align)
	{
		int rem = linesize % align;
		return (rem != 0) ? (align - rem) : rem;
	}

	unsigned char *dataAreaDIB;			//pointer to data
	unsigned char *bufDIB;				//pointer to header
	int dimX;
	int dimY;
	int pixelType;
	int headerSize;
	int imageSize;
};

#endif	// __WIN__

#endif
