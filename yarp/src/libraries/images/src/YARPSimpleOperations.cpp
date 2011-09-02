//
// YARPSimpleOperations.cpp
//

#include "YARPSimpleOperations.h"

void YARPSimpleOperation::Scale (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out, double scale)
{
	assert (in.GetIplPointer() != NULL && out.GetIplPointer() != NULL);
	assert (in.GetWidth() == out.GetWidth() && in.GetHeight() == out.GetHeight());
	assert (scale <= 1 && scale >= 0);

	const int csize = out.GetIplPointer()->imageSize;
	char *tmpo = out.GetIplPointer()->imageData;
	char *tmpi = in.GetIplPointer()->imageData;

	// not sure about the correctness of this.
	for (int i = 0; i < csize; i++, tmpo++, tmpi++)
	{
		*tmpo = char(*tmpi * scale);
	}
}
