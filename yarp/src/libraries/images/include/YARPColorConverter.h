//
// YARPColorConverter.h
//

//
// a few color space conversion methods.

#ifndef __YARPColorConverterh__
#define __YARPColorConverterh__

#include "YARPImage.h"
#include "YARPFilters.h"

//
class YARPColorConverter
{
public:
	static void RGB2HSV (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelHSV>& out);
	static void RGB2HSV (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelHSV>& out);
	static void RGB2Grayscale (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out);
	static void RGB2Grayscale (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out);
};

#endif