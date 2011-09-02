//
// YARPColorConverter.cpp
//

#include "YARPColorConverter.h"

void YARPColorConverter::RGB2Grayscale (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out)
{
	assert (out.GetIplPointer() != NULL && in.GetIplPointer() != NULL);
	iplColorToGray(in.GetIplPointer(), out.GetIplPointer());
}

void YARPColorConverter::RGB2Grayscale (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out)
{
	assert (out.GetIplPointer() != NULL && in.GetIplPointer() != NULL);
	iplColorToGray(in.GetIplPointer(), out.GetIplPointer());
}

void YARPColorConverter::RGB2HSV (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelHSV>& out)
{
	assert (out.GetIplPointer() != NULL && in.GetIplPointer() != NULL);
	iplRGB2HSV(in.GetIplPointer(), out.GetIplPointer());
}

void YARPColorConverter::RGB2HSV (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelHSV>& out)
{
	assert (out.GetIplPointer() != NULL && in.GetIplPointer() != NULL);
	iplRGB2HSV(in.GetIplPointer(), out.GetIplPointer());
}