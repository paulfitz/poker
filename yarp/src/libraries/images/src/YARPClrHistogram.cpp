//
// YARPClrHistogram.cpp
//

#include "YARPClrHistogram.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

//
// YARPClrHistogram. 1-dimensional color histogram.
//
YARPClrHistogram::YARPClrHistogram (int necc, int nang, int hwidth, int hheight) 
//	: m_histo (0.0, 180.0, 0.0, 255.0, hwidth, hheight)	
// apparently IPL v2.5 has got a different conversion. hue is normalized 255 rather than 180.
// LATER: to be changed also in color segmentation and in FakeIPL for compatibility.
	: m_histo (0.0, 255.0, 0.0, 255.0, hwidth, hheight)
{
	m_ecc = necc;
	m_ang = nang;

	m_hsv.Resize (m_ecc, m_ang);
	m_hsv.Zero();

	m_saturation_int = int(0.1 * 255);
	m_luma_int = int(0.2 * 255);

	m_rows.Resize (hheight);
	m_rows = 0;
	m_cols.Resize (hwidth);
	m_cols = 0;
}

void YARPClrHistogram::Cleanup()
{
	m_hsv.Cleanup();
}

void YARPClrHistogram::CumulateHistograms (const YARPImageOf<YarpPixelHSV>& src)
{
	m_histo = 0;

	unsigned char *hue2, *sat2, *val2;

	double hue, saturation;

	for (int i = 0; i < m_ang; i++)
	{
		hue2 = (unsigned char *)src.GetArray ()[i];
		sat2 = hue2 + 1;
		val2 = sat2 + 1;

		//for (int j = 0; j < m_ecc/2; j++)
		for (int j = 0; j < m_ecc; j++)
		{
			if (*val2 > m_luma_int && *sat2 > m_saturation_int)
			{
				hue = double (*hue2);
				saturation = double (*sat2);

				m_histo (hue, saturation) ++;
				if (m_histo (hue, saturation) > m_histo.GetMaximum ())
					m_histo.GetMaximum () = m_histo (hue, saturation);
			}

			hue2 += 3;
			sat2 += 3;
			val2 += 3;
		}
	}

	m_rows = 0;
	m_max_r = 0;

	// fill up rows and cols.
	for (int k = 1; k <= m_histo.GetHeight(); k++)
	{
		for (int i = 1; i <= m_histo.GetWidth(); i++)
		{
			m_rows(k) += m_histo (i-1, k-1);
		}

		if (m_rows(k) > m_max_r)
			m_max_r = m_rows(k);
	}

	m_cols = 0;
	m_max_c = 0;
	
	// fill up rows and cols.
	for (int k = 1; k <= m_histo.GetWidth(); k++)
	{
		for (int i = 1; i <= m_histo.GetHeight(); i++)
		{
			m_cols(k) += m_histo (k-1, i-1);
		}

		if (m_cols(k) > m_max_c)
			m_max_c = m_cols(k);
	}
}

void YARPClrHistogram::Apply (const YARPImageOf<YarpPixelRGB>& is)
{
	YARPColorConverter::RGB2HSV (is, m_hsv);
	CumulateHistograms(m_hsv);
}


//
// Convert a lut histogram in a mono image.
// Get a slightly bigger image (size is hwidth+1, hheight+1).
//
void YARPClrHistogram::ConvertHistogramToImage (YARPImageOf<YarpPixelMono>& img)
{
	int * ptr = m_histo.GetArrayPtr ()[0];

	unsigned char **ip = (unsigned char **)img.GetArray();

	const int width = m_histo.GetWidth ();
	const int height = m_histo.GetHeight ();
	double max = m_histo.GetMaximum();

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			int tmp = *ptr++;
			tmp = int (tmp / max * 255.0 + .5);
			if (tmp > 255) 
				tmp = 255;
			ip[i][j] = (unsigned char)tmp;
		}

	for (int i = 0; i < height; i++)
	{
		int tmp = int (m_rows(i+1) / m_max_r * 255.0 + .5);
		if (tmp > 255)
			tmp = 255;
		ip[i][width] = (unsigned char)tmp;
	}

	for (int i = 0; i < width; i++)
	{
		int tmp = int (m_cols(i+1) / m_max_c * 255.0 + .5);
		if (tmp > 255)
			tmp = 255;
		ip[height][i] = (unsigned char)tmp;
	}
}

