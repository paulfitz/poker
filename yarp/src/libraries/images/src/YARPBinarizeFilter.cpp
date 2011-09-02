//
// YARPBinarizeFilter.cpp
//

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

#include "YARPBinarizeFilter.h"

// do not skip padding bytes. Not a big deal though.
void YARPBinarizeFilter::Apply(const YARPImageOf<YarpPixelMono>& is, YARPImageOf<YarpPixelMono>& id)
{
	assert (id.GetWidth() == is.GetWidth());
	assert (id.GetHeight() == is.GetHeight());

	unsigned char *src = (unsigned char *)is.GetAllocatedArray();
	unsigned char *dst = (unsigned char *)id.GetAllocatedArray();
	
	short lth,hth;

	lth=(short)(low +.5);
	hth=(short)(hi + .5);

	short tmp;
	const int size = id.GetIplPointer()->imageSize;

	if (notSingle)
	{
		if (lth<hth)
		{
			for (int i = 0; i < size; i++)
			{
				tmp=*src++;
				if (tmp>=lth && tmp<=hth)
					*dst++=(unsigned char)255;
				else
					*dst++=(unsigned char)0;
			}
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				tmp=*src++;
				if (tmp>=lth || tmp<=hth)
					*dst++=(unsigned char)255;
				else
					*dst++=(unsigned char)0;
			}
		}
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			if (*src++ < lth)
				*dst++=(unsigned char)0;
			else
				*dst++=(unsigned char)255;
		}
	}
}
