//
// YARPConverter.cpp
//

#ifdef __WIN__

#include "YARPDIBConverter.h"


YARPDIBConverter::YARPDIBConverter()
{
	bufDIB = NULL;
	dataAreaDIB = NULL;
	dimX = 0;
	dimY = 0;
	pixelType = 0;
	headerSize = 0;
	imageSize = 0;
}

YARPDIBConverter::YARPDIBConverter(const YARPGenericImage &img)
{
	assert (img.GetIplPointer != NULL);

	dimX = img.GetWidth();
	dimY = img.GetHeight();
	pixelType = img.GetID();
	_alloc_dib();
}

YARPDIBConverter::~YARPDIBConverter()
{
	_free_dib();
};

void YARPDIBConverter::Resize(const YARPGenericImage &img)
{
	assert (img.GetIplPointer != NULL);

	dimX = img.GetWidth();
	dimY = img.GetHeight();
	pixelType = img.GetID();

	_alloc_dib();
}

void YARPDIBConverter::_alloc_dib()
{
	// alloc internal dib
	if (bufDIB != NULL)
		_free_dib();

	switch (pixelType)
	{
		case YARP_PIXEL_MONO_SIGNED:
		case YARP_PIXEL_MONO:
			{
				// MONO
				if ( (dimX > 0) && (dimY > 0) )
				{
					int	addedBytesDIB = _pad_bytes(dimX,4);
					bufDIB = new unsigned char[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*256 + (dimX+addedBytesDIB)*dimY];
					assert(bufDIB != NULL);

					BITMAPINFOHEADER* dibhdr = (BITMAPINFOHEADER*)bufDIB;

					COLORREF* rgb = (COLORREF *)(bufDIB + sizeof(BITMAPINFOHEADER));

					headerSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*256;
					imageSize = (dimX + addedBytesDIB) * dimY;
					dataAreaDIB = bufDIB + headerSize;

					for (int i = 0; i < 256; i++)
						rgb[i] = RGB(i,i,i); // prepare the palette
		
					dibhdr->biSize = sizeof(BITMAPINFOHEADER);
					dibhdr->biWidth = dimX;
					dibhdr->biHeight = dimY;
					dibhdr->biPlanes = 1;
					dibhdr->biBitCount = 8; // TODO ...
					dibhdr->biCompression = BI_RGB;
					dibhdr->biSizeImage = imageSize;
		
					dibhdr->biXPelsPerMeter = 0;
					dibhdr->biYPelsPerMeter = 0;
					dibhdr->biClrUsed = 256;
					dibhdr->biClrImportant = 256;
				}
			}
			break;
		case YARP_PIXEL_HSV:
		case YARP_PIXEL_BGR:
		case YARP_PIXEL_RGB:
			{
				// RGB
				if ( (dimX > 0) && (dimY > 0) )
				{
					int	addedBytesDIB = _pad_bytes(dimX*3,4);
					bufDIB = new unsigned char[sizeof(BITMAPINFOHEADER) + (dimX*3+addedBytesDIB)*dimY];
					assert(bufDIB != NULL);

					headerSize = sizeof(BITMAPINFOHEADER);
					imageSize = (dimX * 3 + addedBytesDIB) * dimY;

					BITMAPINFOHEADER* dibhdr = (BITMAPINFOHEADER*)bufDIB;

					dataAreaDIB = bufDIB + headerSize;
		
					dibhdr->biSize = sizeof(BITMAPINFOHEADER);
					dibhdr->biWidth = dimX;
					dibhdr->biHeight = dimY;
					dibhdr->biPlanes = 1;
					dibhdr->biBitCount = 24; 
					dibhdr->biCompression = BI_RGB;
					dibhdr->biSizeImage = imageSize;
		
					dibhdr->biXPelsPerMeter = 0;
					dibhdr->biYPelsPerMeter = 0;
					dibhdr->biClrUsed = 0;
					dibhdr->biClrImportant = 0;
				}
			}
			break;
		case YARP_PIXEL_RGB_SIGNED:
		case YARP_PIXEL_MONO_FLOAT:
		case YARP_PIXEL_RGB_FLOAT:
		case YARP_PIXEL_HSV_FLOAT:
		case YARP_PIXEL_INVALID:
		default:
			// not supported !
			printf ("*** Sorry, convertion to DIB not supported\n");
			exit(1);
			assert (0);
			break;
	}

}

void YARPDIBConverter::_free_dib()
{
	if (bufDIB != NULL)
	{
		delete [] bufDIB;
		bufDIB = NULL;
	};
	dataAreaDIB = NULL;
}


#endif	// __WIN__