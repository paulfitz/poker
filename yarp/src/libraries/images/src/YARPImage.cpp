#define __YARPSafeNewh__
#include <iostream>
using namespace std;

#include "YARPTime.h"

#include <stdio.h>
#include <string.h>
#include "YARPImage.h"

#include "YARPRefCount.h"

#define DBGPF1 if (0)

void SatisfySize(YARPGenericImage& src, YARPGenericImage& dest)
{
  if (dest.GetHeight()!=src.GetHeight() ||
      dest.GetWidth() !=src.GetWidth())
    {
      dest.Resize(src.GetWidth(),src.GetHeight());
    }
}


// Default copy mechanism
template <class T1, class T2>
inline void CopyPixel(const T1 *src, T2 *dest)
{
	*dest = *src;
}

typedef YarpPixelMono Def_YARP_PIXEL_MONO;
typedef YarpPixelRGB Def_YARP_PIXEL_RGB;
typedef YarpPixelHSV Def_YARP_PIXEL_HSV;
typedef YarpPixelBGR Def_YARP_PIXEL_BGR;
typedef YarpPixelMonoSigned Def_YARP_PIXEL_MONO_SIGNED;
typedef YarpPixelRGBSigned Def_YARP_PIXEL_RGB_SIGNED;
typedef YarpPixelFloat Def_YARP_PIXEL_MONO_FLOAT;
typedef YarpPixelRGBFloat Def_YARP_PIXEL_RGB_FLOAT;
typedef YarpPixelHSVFloat Def_YARP_PIXEL_HSV_FLOAT;
typedef YarpPixelInt Def_YARP_PIXEL_INT;

#define VALID_PIXEL(x) ((x>255)?255:((x<0)?0:x))
#define SPECIAL_COPY_BEGIN void YARPDummyCopyPixel() {
#define SPECIAL_COPY(id1,id2) } inline void CopyPixel(const Def_##id1 *src, Def_##id2 *dest) {
#define SPECIAL_COPY_END }

static int implemented_yet = 1;

SPECIAL_COPY_BEGIN

SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_RGB)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_BGR)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_HSV)
  dest->v = *src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_RGB_SIGNED)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_RGB_FLOAT)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_HSV_FLOAT)
  dest->v = *src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_MONO_SIGNED)
  *dest = *src >> 1;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_INT)
  *dest = *src;
SPECIAL_COPY(YARP_PIXEL_MONO,YARP_PIXEL_MONO_FLOAT)
  *dest = *src;

SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_MONO)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_INT)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_HSV)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_MONO_SIGNED)
  *dest = (char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_RGB_SIGNED)
  dest->r = src->r; dest->g = src->g; dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_MONO_FLOAT)
  *dest = ((src->r + src->g + src->b)/3.0f);
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_RGB_FLOAT)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_BGR)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_RGB,YARP_PIXEL_HSV_FLOAT)
  assert(implemented_yet == 0);


SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_MONO)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_RGB)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_BGR)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_MONO_SIGNED)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_RGB_SIGNED)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_MONO_FLOAT)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_RGB_FLOAT)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV,YARP_PIXEL_HSV_FLOAT)
  assert(implemented_yet == 0);


SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_MONO)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_INT)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_HSV)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_MONO_SIGNED)
  *dest = (char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_RGB_SIGNED)
  dest->r = src->r; dest->g = src->g; dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_MONO_FLOAT)
  *dest = ((src->r + src->g + src->b)/3.0f);
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_RGB_FLOAT)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_RGB)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_BGR,YARP_PIXEL_HSV_FLOAT)
  assert(implemented_yet == 0);


SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_RGB)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_BGR)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_HSV)
  dest->v = *src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_RGB_SIGNED)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_RGB_FLOAT)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_HSV_FLOAT)
  dest->v = *src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_MONO)
  *dest = *src + 128;
SPECIAL_COPY(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_INT)
  *dest = *src;

SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_MONO)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_INT)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_HSV)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_MONO_SIGNED)
  *dest = (char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_RGB)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_BGR)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_MONO_FLOAT)
  *dest = ((src->r + src->g + src->b)/3.0f);
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_RGB_FLOAT)
  dest->r = src->r;
  dest->g = src->g;
  dest->b = src->b;
SPECIAL_COPY(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_HSV_FLOAT)
  assert(implemented_yet == 0);

SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_MONO)
  *dest = (unsigned char)*src;
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_INT)
  *dest = (unsigned char)*src;
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_MONO_SIGNED)
  *dest = (char)*src;
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_RGB)
  dest->r = dest->g = dest->b = (unsigned char)(*src);
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_BGR)
  dest->r = dest->g = dest->b = (unsigned char)(*src);
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_HSV)
  dest->v = (unsigned char)*src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_RGB_SIGNED)
  dest->r = dest->g = dest->b = (signed char) *src;
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_RGB_FLOAT)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_HSV_FLOAT)
  dest->v = *src;
  dest->h = dest->s = 0;

SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_MONO)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_INT)
  *dest = (unsigned char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_HSV)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_MONO_SIGNED)
  *dest = (char)((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_RGB)
  dest->r = (unsigned char) src->r;
  dest->g = (unsigned char) src->g;
  dest->b = (unsigned char) src->b;
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_BGR)
  dest->r = (unsigned char) src->r;
  dest->g = (unsigned char) src->g;
  dest->b = (unsigned char) src->b;
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_MONO_FLOAT)
  *dest = ((src->r + src->g + src->b)/3);
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_RGB_SIGNED)
  dest->r = (signed char) src->r;
  dest->g = (signed char) src->g;
  dest->b = (signed char) src->b;
SPECIAL_COPY(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_HSV_FLOAT)
  assert(implemented_yet == 0);

SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_MONO)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_RGB)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_BGR)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_MONO_SIGNED)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_RGB_SIGNED)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_MONO_FLOAT)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_RGB_FLOAT)
  assert(implemented_yet == 0);
SPECIAL_COPY(YARP_PIXEL_HSV_FLOAT,YARP_PIXEL_HSV)
  assert(implemented_yet == 0);

SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_RGB)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_BGR)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_HSV)
  dest->v = *src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_RGB_SIGNED)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_RGB_FLOAT)
  dest->r = dest->g = dest->b = *src;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_HSV_FLOAT)
  dest->v = *src;
  dest->h = dest->s = 0;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_MONO_SIGNED)
  *dest = *src >> 1;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_MONO)
  *dest = *src;
SPECIAL_COPY(YARP_PIXEL_INT,YARP_PIXEL_MONO_FLOAT)
  *dest = *src;

SPECIAL_COPY_END


template <class T1, class T2>
void CopyPixels(const T1 *src, T2 *dest, int len)
{
	for (int i=0; i<len; i++)
	{
		CopyPixel(src,dest);
		src++;
		dest++;
	}
}


#define HASH(id1,id2) ((id1)*256+(id2))
#define HANDLE_CASE(len,x1,T1,x2,T2) CopyPixels((T1*)x1,(T2*)x2,len);
#define MAKE_CASE(id1,id2) case HASH(id1,id2): HANDLE_CASE(len,src,Def_##id1,dest,Def_##id2); break;
#define MAKE_2CASE(id1,id2) MAKE_CASE(id1,id2); MAKE_CASE(id2,id1);

// More elegant ways to do this, but needs to be efficient at pixel level
void YARPPixelCopier(const char *src, int id1, 
		 char *dest, int id2, int len)
{
  switch(HASH(id1,id2))
    {
      // Macros rely on len, x1, x2 variable names

      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_RGB);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_HSV);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_BGR);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_MONO_SIGNED);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_RGB_SIGNED);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_MONO_FLOAT);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_MONO,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_HSV);
      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_BGR);
      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_MONO_SIGNED);
      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_RGB_SIGNED);
      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_MONO_FLOAT);
      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_RGB,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_HSV,YARP_PIXEL_BGR);
      MAKE_2CASE(YARP_PIXEL_HSV,YARP_PIXEL_MONO_SIGNED);
      MAKE_2CASE(YARP_PIXEL_HSV,YARP_PIXEL_RGB_SIGNED);
      MAKE_2CASE(YARP_PIXEL_HSV,YARP_PIXEL_MONO_FLOAT);
      MAKE_2CASE(YARP_PIXEL_HSV,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_HSV,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_BGR,YARP_PIXEL_MONO_SIGNED);
      MAKE_2CASE(YARP_PIXEL_BGR,YARP_PIXEL_RGB_SIGNED);
      MAKE_2CASE(YARP_PIXEL_BGR,YARP_PIXEL_MONO_FLOAT);
      MAKE_2CASE(YARP_PIXEL_BGR,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_BGR,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_RGB_SIGNED);
      MAKE_2CASE(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_MONO_FLOAT);
      MAKE_2CASE(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_MONO_SIGNED,YARP_PIXEL_HSV_FLOAT);


      MAKE_2CASE(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_MONO_FLOAT);
      MAKE_2CASE(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_RGB_SIGNED,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_RGB_FLOAT);
      MAKE_2CASE(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_RGB_FLOAT,YARP_PIXEL_HSV_FLOAT);

      MAKE_2CASE(YARP_PIXEL_BGR,YARP_PIXEL_INT);
      MAKE_2CASE(YARP_PIXEL_MONO_FLOAT,YARP_PIXEL_INT);

    default:
      printf("*** Tried to copy type %d to %d\n", id1, id2);
      exit(1);
    }
}

static int _GetPixelSize(int pixel_type)
{
  int result = 0;
  switch (pixel_type)
    {
    case YARP_PIXEL_MONO:
      result = sizeof(YarpPixelMono);
      break;
    case YARP_PIXEL_RGB:
      result = sizeof(YarpPixelRGB);
      break;
    case YARP_PIXEL_HSV:
      result = sizeof(YarpPixelHSV);
      break;
    case YARP_PIXEL_BGR:
      result = sizeof(YarpPixelBGR);
      break;
    case YARP_PIXEL_MONO_SIGNED:
      result = sizeof(YarpPixelMonoSigned);
      break;
    case YARP_PIXEL_RGB_SIGNED:
      result = sizeof(YarpPixelRGBSigned);
      break;
    case YARP_PIXEL_MONO_FLOAT:
      result = sizeof(YarpPixelFloat);
      break;
    case YARP_PIXEL_RGB_FLOAT:
      result = sizeof(YarpPixelRGBFloat);
      break;
    case YARP_PIXEL_HSV_FLOAT:
      result = sizeof(YarpPixelHSVFloat);
      break;
    default:
      // only other acceptable possibility is that the size is being supplied
      // for an unknown type
      //assert (pixel_type<0);
      result = -pixel_type;
      break;
    }
  //printf("Getting pixel size for %d (%d)\n", pixel_type, result);
  return result;
}

int YARPGenericImage::GetPixelSize() const
{
  return _GetPixelSize(type_id);
}

// need alternate definitions of the following functions if the IPL
// library is being used

YARPGenericImage::YARPGenericImage()
{ 
	type_id = YARP_PIXEL_INVALID;
	pImage = NULL;
	Data = NULL;
	buffer_references = NULL;
	is_owner = 1;
}

YARPGenericImage::YARPGenericImage (const YARPGenericImage &i)
{
	type_id = YARP_PIXEL_INVALID;
	pImage = NULL;
	Data = NULL;
	buffer_references = NULL;
	is_owner = 1;

	if (i.GetWidth()>0)
	  {
	    YARPGenericImage::CastCopy (i);
	  }
}

YARPGenericImage::~YARPGenericImage()
{
	_free_complete();
}

void YARPGenericImage::operator= (const YARPGenericImage &i)
{
  if (i.GetWidth()>0)
    {
	YARPGenericImage::CastCopy (i);
    }
}

// these are needed to allow easy manipulation of some logpolar stuff.
// note the padding stuff.
void YARPGenericImage::Crop (YARPGenericImage& id, int startX, int startY)
{
	assert (id.GetID() == GetID());
	assert (pImage != NULL && id.pImage != NULL);
	assert ((startX + id.GetWidth()) <= pImage->width && (startY + id.GetHeight()) <= pImage->height);
			
	int sAlignLineSize = pImage->widthStep;
	int dAlignLineSize = id.pImage->widthStep;

	unsigned char *dst = (unsigned char *)id.GetAllocatedArray();
	unsigned char *src = (unsigned char *)pImage->imageData;
	
	src += ((startY * sAlignLineSize) + startX * pImage->nChannels);

	const int nrows = id.GetHeight();
	for (int i = 0; i < nrows; i++)
	{
		memcpy (dst, src, dAlignLineSize);
		dst += dAlignLineSize;
		src += sAlignLineSize;
	}
}

void YARPGenericImage::Paste (YARPGenericImage& is, int startX, int startY)
{
	assert (is.GetID() == GetID());
	assert (pImage != NULL && is.pImage != NULL);
	assert ((startX + is.GetWidth()) <= pImage->width && (startY + is.GetHeight()) <= pImage->height);
		
	int dAlignLineSize = pImage->widthStep;
	int sAlignLineSize = is.pImage->widthStep;

	unsigned char *src = (unsigned char *)is.GetAllocatedArray();
	unsigned char *dst = (unsigned char *) pImage->imageData;

	dst += ((startY * dAlignLineSize) + startX * pImage->nChannels);

	const int nrows = is.GetHeight();
	for (int i = 0; i < nrows; i++)
	{
		memcpy (dst, src, sAlignLineSize);
		dst += dAlignLineSize;
		src += sAlignLineSize;
	}
}

// allocates an empty image.
void YARPGenericImage::_alloc (void)
{
  assert (pImage != NULL);
  
  if (pImage != NULL)
    if (pImage->imageData != NULL)
      _free(); // was iplDeallocateImage(pImage); but that won't work with refs

  iplAllocateImage (pImage, 0, 0);
  iplSetBorderMode (pImage, IPL_BORDER_CONSTANT, IPL_SIDE_ALL, 0);
}

// installs an external buffer as the image data
void YARPGenericImage::_alloc_extern (void *buf)
{
  assert (pImage != NULL);
  assert(Data==NULL);

  if (pImage != NULL)
    if (pImage->imageData != NULL)
      iplDeallocateImage (pImage);
  //iplAllocateImage (pImage, 0, 0);
  pImage->imageData = (char*)buf;
  // HIT probably need to do more for real IPL
  
  //iplSetBorderMode (pImage, IPL_BORDER_CONSTANT, IPL_SIDE_ALL, 0);
}

// allocates the Data pointer.
void YARPGenericImage::_alloc_data (void)
{
	DBGPF1 printf("alloc_data1\n"), fflush(stdout);
	assert (pImage != NULL);

	assert(Data==NULL);
	/*
	if (Data != NULL)
	{
	  DBGPF1 printf("HIT Deleting Data\n"), fflush(stdout);
	  //delete[] Data; //HIT2
	  free(Data);
	  Data = NULL;
	}
	*/

	DBGPF1 printf("alloc_data1b\n"), fflush(stdout);
	DBGPF1 YARPTime::DelayInSeconds(0.1);

	int hh = pImage->height * sizeof(char *);
        DBGPF1 printf("height is %d (%d)\n", pImage->height, hh);

	DBGPF1 printf("alloc_data1c\n"), fflush(stdout);
	DBGPF1 YARPTime::DelayInSeconds(0.1);

	char **ptr = new char *[pImage->height];

	DBGPF1 printf("alloc_data2\n"), fflush(stdout);
	DBGPF1 YARPTime::DelayInSeconds(0.1);
	Data = ptr;

	assert (Data != NULL);

	assert (pImage->imageData != NULL);

	int nPlanes = pImage->nChannels;
	int width = pImage->width;
    int height = pImage->height;
	DBGPF1 printf("alloc_data3\n");

    char * DataArea = pImage->imageData;
    
	for (int r = 0; r < height; r++)
	{
		Data[r] = DataArea;
		DataArea += pImage->widthStep;
	}
	DBGPF1 printf("alloc_data4\n");
}

void YARPGenericImage::_free (void)
{
//	assert (pImage != NULL);
  if (pImage != NULL)
    if (pImage->imageData != NULL)
    {
      int zero_count = 1;
      if (buffer_references != NULL)
	{
	  DBGPF1 if (buffer_references!=NULL) cout << "HIT B Buffer references " << buffer_references->GetReferenceCount() << endl;
	  buffer_references->RemoveRef();
	  int ref = buffer_references->GetReferenceCount();
	  DBGPF1 if (buffer_references!=NULL) cout << "HIT C Buffer references " << buffer_references->GetReferenceCount() << endl;
	  if (ref<=0)
	    {
	      DBGPF1 cout << "HIT deleting buffer_references" << endl;
	      delete buffer_references;
	    }
	  else
	    {
	      zero_count = 0;
	    }
	  buffer_references = NULL;
	}
      if (zero_count)
	{
	  //cout << "HIT maybe deleting ipl image" << endl;
	  if (is_owner)
	    {
	      DBGPF1 cout << "HIT really truly deleting ipl image" << endl;
	      iplDeallocateImage (pImage);
	      if (Data!=NULL)
		{
		  delete[] Data;
		}
	    }
	  else
	    {
	      if (Data!=NULL)
		{
		  delete[] Data;
		}
	    }
	}
      is_owner = 1;
      Data = NULL;
      pImage->imageData = NULL;
    }
}

void YARPGenericImage::_free_data (void)
{
  assert(Data==NULL); // Now always free Data at same time
                      // as image buffer, for correct refcounting
  /*
	if (Data != NULL)
	{
	  delete[] Data;
	}
	Data = NULL;
  */
}

// This reflects the fact that under QNX we do not need any padding.
// LATER: implement for LINUX.
int YARPGenericImage::_pad_bytes (int linesize, int align) const
{
#ifdef __QNX__
	return 0;
#else
#ifdef __USING_IPL__
	int rem = linesize % align;
	return (rem != 0) ? (align - rem) : rem;
#else
	return 0;
#endif
#endif
}

void YARPGenericImage::_set_ipl_header(int x, int y, int pixel_type)
{
	// used to allocate the ipl header.
	switch (pixel_type)
	{
	case YARP_PIXEL_MONO:
		pImage = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_8U,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
		DBGPF1 printf("Set pImage to %ld\n", (long int) pImage);
		DBGPF1 printf("Set init h to %d\n", (long int) pImage->height);
		break;

	case YARP_PIXEL_RGB:
		pImage = iplCreateImageHeader(
					3,
					0,
					IPL_DEPTH_8U,			
					"RGB",
					"RGB",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
		break;

	case YARP_PIXEL_HSV:
		pImage = iplCreateImageHeader(
					3,
					0,
					IPL_DEPTH_8U,			
					"HSV",
					"HSV",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
		break;

	case YARP_PIXEL_BGR:
		pImage = iplCreateImageHeader(
					3,
					0,
					IPL_DEPTH_8U,			
					"RGB",
					"BGR",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
		break;

	case YARP_PIXEL_MONO_SIGNED:
		pImage = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_8S,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
		break;

	case YARP_PIXEL_RGB_SIGNED:
		assert (implemented_yet == 0);
		break;

	case YARP_PIXEL_MONO_FLOAT:
             	  pImage = iplCreateImageHeader(
					  1,
					  0,
					  IPL_DEPTH_32F,
					  "GRAY",
					  "GRAY",
					  IPL_DATA_ORDER_PIXEL,
					  IPL_ORIGIN_BL,
					  IPL_ALIGN_QWORD,
					  x,
					  y,
					  NULL,
					  NULL,
					  NULL,
					  NULL);
		break;

	case YARP_PIXEL_RGB_FLOAT:
		pImage = iplCreateImageHeader(
					3,
					0,
					IPL_DEPTH_32F,			
					"RGB",
					"RGB",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
		//assert (implemented_yet == 0);
		break;

	case YARP_PIXEL_HSV_FLOAT:
		assert (implemented_yet == 0);
		break;

	case YARP_PIXEL_INT:
             	  pImage = iplCreateImageHeader(
					  1,
					  0,
					  IPL_DEPTH_32S,
					  "GRAY",
					  "GRAY",
					  IPL_DATA_ORDER_PIXEL,
					  IPL_ORIGIN_BL,
					  IPL_ALIGN_QWORD,
					  x,
					  y,
					  NULL,
					  NULL,
					  NULL,
					  NULL);
		break;

	case YARP_PIXEL_INVALID:
		// not a type!
		printf ("*** Trying to allocate an invalid pixel type image\n");
		exit(1);
		break;
	  
	case -2:
	  pImage = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_16S,
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_BL,
					IPL_ALIGN_QWORD,
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
	  break;

	case -4:
	  pImage = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32S,
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_BL,
					IPL_ALIGN_QWORD,
					x,
					y,
					NULL,
					NULL,
					NULL,
					NULL);
	  break;

	default:
		// unknown pixel type. Should revert to a non-IPL mode... how?
		// LATER: implement this.
		assert (implemented_yet == 0);
		break;
	}

	type_id = pixel_type;
}

void YARPGenericImage::_free_ipl_header()
{
  if (pImage!=NULL)
    {
      iplDeallocate (pImage, IPL_IMAGE_HEADER);
    }
  pImage = NULL;
}

void YARPGenericImage::_make_independent()
{
  // needs to be filled out once references are permitted -paulfitz
  // actually I think this isn't really needed -paulfitz
}

void YARPGenericImage::_alloc_complete(int x, int y, int pixel_type)
{
	_make_independent();
	_free_complete();
	_set_ipl_header(x, y, pixel_type);
	_alloc ();
	_alloc_data ();
}

void YARPGenericImage::_alloc_complete_extern(void *buf, int x, int y, 
					      int pixel_type)
{
  _make_independent();
  _free_complete();
  _set_ipl_header(x, y, pixel_type);
  Data = NULL;
  _alloc_extern (buf);
  _alloc_data ();
  is_owner = 0;
}


void YARPGenericImage::_free_complete()
{
	_free();
	_free_data();
	_free_ipl_header();
}

void YARPGenericImage::Clear()
{
	_make_independent();
	_free();
	_free_data();
}

void YARPGenericImage::Zero ()
{
	assert (pImage != NULL);
	memset (pImage->imageData, 0, pImage->imageSize);
}

void YARPGenericImage::Resize(int x, int y, int pixel_type)
{
	int need_recreation = 1;
	if (pImage != NULL)
	{
		if (x == pImage->width && y == pImage->height)
		{
			if (x == pImage->width && y == pImage->height &&
				pImage->imageSize == x * y * _GetPixelSize(pixel_type) &&
				pImage->imageData != NULL && Data != NULL)
			{
				need_recreation = 0;
			}
		}
	}

	if (need_recreation)
	{
	  _free_complete();
	  DBGPF1 printf("HIT recreation for %ld %ld: %d %d %d\n", (long int) this, (long int) pImage, x, y, pixel_type);
	  _alloc_complete (x, y, pixel_type);
	}
}

void YARPGenericImage::Refer(YARPGenericImage& src)
{
  assert (src.pImage != NULL); // should we allow copying from a non-allocated image?
  
  int my_id = GetID();
  int other_id = src.GetID();
  
  if (Data == NULL && my_id == YARP_PIXEL_INVALID)
    {
      SetID (other_id);
      my_id = GetID();
    }
  
  if (my_id != other_id)
    {
      printf("*** Tried to Refer() to an incompatible image type.\n");
      printf("*** Please copy instead, using CastCopy().\n");
      exit(1);
    }
  
  DBGPF1 cout << "HIT Starting refer" << endl;
  _make_independent();
  DBGPF1 cout << "HIT step 1" << endl;
  _free_complete();
  DBGPF1 cout << "HIT step 2" << endl;
  DBGPF1 if (buffer_references!=NULL) cout << "HIT X Buffer references " << buffer_references->GetReferenceCount() << endl;
  buffer_references = AddYarpRefCount(src.buffer_references);
  DBGPF1 if (buffer_references!=NULL) cout << "HIT Y Buffer references " << buffer_references->GetReferenceCount() << endl;
  DBGPF1 cout << "HIT step 3 // " 
	      << src.GetWidth() << " "
	      << src.GetHeight() << " "
	      << src.GetID() << " "
	      << endl;
  _set_ipl_header(src.GetWidth(), src.GetHeight(), src.GetID());
  DBGPF1 cout << "HIT step 4" << endl;
  _alloc_extern ((void*)src.GetRawBuffer());
  is_owner = src.is_owner;
  DBGPF1 cout << "HIT step 5" << endl;
  assert(Data==NULL);
  Data = src.Data;
  //_alloc_data ();
  DBGPF1 if (buffer_references!=NULL) cout << "HIT Z Buffer references " << buffer_references->GetReferenceCount() << endl;
  DBGPF1 cout << "HIT Ending refer" << endl;
}

// if not allocated... fill it out.
void YARPGenericImage::PeerCopy(const YARPGenericImage& img)
{
	assert (img.pImage != NULL); // should we allow copying from a non-allocated image?

	int my_id = GetID();
	int other_id = img.GetID();

	if (Data == NULL && my_id == YARP_PIXEL_INVALID)
	{
		SetID (other_id);
		my_id = GetID();
	}

	if (my_id != other_id)
	{
		printf("*** Tried to copy between incompatible image types.\n");
		printf("*** Please use CastCopy() to do this.\n");
		exit(1);
	}

	Resize(img.pImage->width, img.pImage->height, my_id);

	if (pImage != NULL && img.pImage != NULL)
	{
		if (pImage->imageData != NULL && img.pImage->imageData != NULL)
		{
			assert(pImage->imageSize == img.pImage->imageSize);
			memcpy(pImage->imageData, img.pImage->imageData, pImage->imageSize);
		}
	}
}

void YARPGenericImage::CastCopy(const YARPGenericImage& img)
{
	int my_id = GetID();
	int other_id = img.GetID();
	Resize(img.pImage->width, img.pImage->height, my_id);

	if (my_id == YARP_PIXEL_INVALID || other_id == YARP_PIXEL_INVALID || my_id != other_id)
	{
		const char *src = img.GetRawBuffer();
		char *dest = GetRawBuffer();
		const int len = pImage->height * pImage->width;
		assert(src!=NULL && dest!=NULL);
		YARPPixelCopier (src, other_id, dest, my_id, len);
	}
	else
	{
		PeerCopy (img);
	}
}

void YARPGenericImage::ReferOrCopy(YARPGenericImage& image)
{
  if (image.GetID() == GetID())
    {
      Refer(image);
    }
  else
    {
      CastCopy(image);
    }
}

void YARPGenericImage::ScaledCopy(const YARPGenericImage& img, int nx, int ny)
{
  if (GetWidth()!=nx || GetHeight()!=ny || GetID() != img.GetID())
    {
      Resize(nx, ny, img.GetID());
    }
  ScaledCopy(img);
}

void YARPGenericImage::ScaledCopy(const YARPGenericImage& img)
{
  int i, j;
  int i0 = 0, j0 = 0;
  float di, dj;
  
  int h = img.GetHeight(), w = img.GetWidth();
  int nh = GetHeight(), nw = GetWidth();
  int d = GetPixelSize();
  
  assert(img.GetID() == GetID());
  assert(h>0&&w>0&&nh>0&&nw>0);
  assert(GetPadding()==0);
  assert(img.GetPadding()==0);

  char *mem = img.GetRawBuffer();
  char *mem2 = GetRawBuffer();
  assert(mem!=NULL && mem2!=NULL);
  
  di = ((float)h)/nh;
  dj = ((float)w)/nw;
  
  for (i=0; i<nh; i++)
    {
      i0 = (int)(di*i);
      for (j=0; j<nw; j++)
	{
	  j0 = (int)(dj*j);
	  memcpy(RawPixel(j,i),img.RawPixel(j0,i0),d);
	}
    }
}
 
