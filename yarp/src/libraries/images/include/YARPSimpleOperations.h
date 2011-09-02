//
// YARPSimpleOperations.h
//

#ifndef __YARPSimpleOperationsh__
#define __YARPSimpleOperationsh__


//
// add here simple image manipulation stuff. Scaling, sum, diff.
// It's not by any mean exhaustive...

#include "YARPImage.h"
#include "YARPFilters.h"

class YARPSimpleOperation
{
public:
	static void Scale (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out, double scale);
	
	inline static void Fill (YARPGenericImage &img, int value)
	{
		// image type is not checked!
		char *pointer = img.GetRawBuffer();
		assert (pointer != NULL);
		
		memset (pointer, value, img.GetAllocatedDataSize());
	};

	inline static void DrawCross(YARPImageOf<YarpPixelRGB> &img, double dx, double dy, const YarpPixelRGB &pixel)
	{
		// coordinate center is top-left
		int x = (int) (dx + 0.5);
		int y = (int) (dy + 0.5);

		for(int i = -2; i <= 2; i++)
			img.Pixel(x+i,y) = pixel;
		for (int j = -2; j <= 2; j++)
			img.Pixel(x,y+j) = pixel;
	};
};

#endif