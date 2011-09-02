//
// YARPImageUtils.h 
//

//
// Utilities. I'm not trying to be exhaustive. When we need them we can 
//	simply add them here.
//
#ifndef __YARPImageUtilsh__
#define __YARPImageUtilsh__

#include "YARPImage.h"

//
class YARPImageUtils 
{
public:
	static void GetRed (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetRed (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetGreen (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetGreen (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetBlue (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetBlue (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out);

	static void SetRed (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelRGB>& out);
	static void SetRed (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelBGR>& out);
	static void SetGreen (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelRGB>& out);
	static void SetGreen (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelBGR>& out);
	static void SetBlue (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelRGB>& out);
	static void SetBlue (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelBGR>& out);

	static void GetValue (const YARPImageOf<YarpPixelHSV>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetSaturation (const YARPImageOf<YarpPixelHSV>& in, YARPImageOf<YarpPixelMono>& out);
	static void GetHue (const YARPImageOf<YarpPixelHSV>& in, YARPImageOf<YarpPixelMono>& out);

	static void PasteInto (const YARPImageOf<YarpPixelMono>& src, int x, int y, int zoom, YARPImageOf<YarpPixelMono>& dst);
};

#endif