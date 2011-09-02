//
// YARPSkinFilter.cpp
//

#include "YARPSkinFilter.h"

YARPSkinFilter::YARPSkinFilter(void) : YARPFilter()
{
	transformWeights[0] = 0.21742f;     // r
	transformWeights[1] = -0.36386f;    // g 
	transformWeights[2] = 0.90572f;     // b
	transformWeights[3] = 0.00096756f;  // r*r
	transformWeights[4] = -0.00050073f; // g*g
	transformWeights[5] = -0.00287f;    // b*b
	transformDelta = -50.1255f;
	mask[0] = 1.05f; // r > M*g
	mask[1] = 2.0f;   // r < M*g
	mask[2] = 0.9f; // r > M*g
	mask[3] = 2.0f;   // r < M*b
	mask[4] = 20.0f;  // r > M
	mask[5] = 250.0f; // r < M      
}

void YARPSkinFilter::Cleanup (void)
{
}

void YARPSkinFilter::Apply(const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out)
{
	assert(out.GetHeight() == in.GetHeight() && out.GetWidth() == in.GetWidth());

	IplImage *source = in.GetIplPointer();
	IplImage *dest = out.GetIplPointer();

	unsigned char *src = (unsigned char *)source->imageData;
	unsigned char *dst = (unsigned char *)dest->imageData;

	const int size = source->width * source->height;
	for (int i = 0; i < size; i++)
	{
		*dst++ = SkinOperatorRGB(src);
		src += 3;
	}
}

void YARPSkinFilter::Apply(const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out)
{
	assert(out.GetHeight() == in.GetHeight() && out.GetWidth() == in.GetWidth());

	IplImage *source = in.GetIplPointer();
	IplImage *dest = out.GetIplPointer();

	unsigned char *src = (unsigned char *)source->imageData;
	unsigned char *dst = (unsigned char *)dest->imageData;

	const int size = source->width * source->height;
	for (int i = 0; i < size; i++)
	{
		*dst++ = SkinOperatorBGR(src);
		src += 3;
	}
}

