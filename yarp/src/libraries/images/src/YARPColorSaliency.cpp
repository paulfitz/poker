//
// YARPColorSaliency.cpp
//

#include "YARPColorSaliency.h"

void YARPColorSaliencyFilter::Apply (const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelHSV>& out)
{
	assert (in.GetWidth() == m_ecc && in.GetHeight() == m_ang);

	YARPColorConverter::RGB2HSV (in, out);

	const int size = m_ecc * m_ang;
	unsigned char *sat = (unsigned char *)out.GetArray()[0] + 1;
	unsigned char *y = (unsigned char *)out.GetArray()[0] + 2;

	for (int i = 0; i < size; i++)
	{
		double tmp = *sat;
		if (tmp < m_threshold || *y < m_luma_thr)
			*sat = 0;
		else
		{
			// saliency is 0 for thr and 255 for sat=1.
			//*sat = (unsigned char)((tmp - m_threshold) / (255.0 - m_threshold) * 255);
			// leave as it is.
		}
		sat += 3;
		y += 3;
	}
}

void YARPColorSaliencyFilter::Apply (const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelHSV>& out)
{
	assert (in.GetWidth() == m_ecc && in.GetHeight() == m_ang);

	YARPColorConverter::RGB2HSV (in, out);

	const int size = m_ecc * m_ang;
	unsigned char *sat = (unsigned char *)out.GetArray()[0] + 1;
	unsigned char *y = (unsigned char *)out.GetArray()[0] + 2;

	for (int i = 0; i < size; i++)
	{
		double tmp = *sat;
		if (tmp < m_threshold || *y < m_luma_thr)
			*sat = 0;
		else
		{
			// saliency is 0 for thr and 255 for sat=1.
			//*sat = (unsigned char)((tmp - m_threshold) / (255.0 - m_threshold) * 255);
			// leave as it is.
		}
		sat += 3;
		y += 3;
	}
}

