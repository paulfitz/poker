//
// YARPFftFeatures.cpp
//

#include "YARPFftFeatures.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

YARPFftFeatures::YARPFftFeatures (int width, int height) 
	: YARPFilter(), 
	  m_fft (_max(width, height), _max(width, height))
{
	m_w = width;
	m_h = height;
	m_re = new double[m_w * m_h];
	m_im = new double[m_w * m_h];
	assert (m_re != NULL && m_im != NULL);

	m_real2 = new double[m_w * m_h];
	m_imag2 = new double[m_w * m_h];
	assert (m_real2 != NULL && m_imag2 != NULL);
	
}

YARPFftFeatures::~YARPFftFeatures ()
{
	Cleanup ();
}

void YARPFftFeatures::Cleanup ()
{
	if (m_re != NULL)
		delete[] m_re;
	if (m_im != NULL)
		delete[] m_im;
	if (m_real2 != NULL)
		delete[] m_real2;
	if (m_imag2 != NULL)
		delete[] m_imag2;
}

void YARPFftFeatures::Apply (const YARPImageOf<YarpPixelMono>& in)
{
	assert (in.GetPadding() == 0);

	// convert normalizes also to 1 (more precise in theory).
	YARPFft::Convert2DoubleScaled ((const unsigned char *)in.GetAllocatedArray(), m_re, m_w, m_h);
	memset (m_im, 0, sizeof(double) * m_w * m_h);

	int dims[2] = { m_w, m_h };
	m_fft.Fft (2, dims, m_re, m_im, 1, 0);	// direct fft!
}

// returns a vector with a particular subset of features.
void YARPFftFeatures::ZigZagScan (CVisDVector& streamlined)
{
	// not a real zigzag unless frequency ordering is required.

	// this is the size of the 1st quadrant.
	const int width = 4;
	const int height = 7;

	// the vector is [width * heigth + (width) * (height-1)] * 2.
	const int vectorsize = width * height * 2 + (width) * (height-1) * 2;
	const int skip = width * height + (width) * (height-1);
	if (streamlined.Length() != vectorsize)
		streamlined.Resize (vectorsize);

	// fourth quadrant. (here's where the DC is).
	int count = 1;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			streamlined(count) = m_re[i*m_w+j];
			streamlined(count+skip) = m_im[i*m_w+j];
			count++;
		}
	}

	for (int i = 0; i < height-1; i++)
	{
		for (int j = 0; j < width; j++)
		{
			streamlined(count) = m_re[(m_h-1-i)*m_w+j];
			streamlined(count+skip) = m_re[(m_h-1-i)*m_w+j];
			count++;
		}
	}

	// remove DC component.
	// THIS IS ONLY A TEST.
	streamlined(1) = 0;
	streamlined(skip) = 0;
}

// reconstruct using only a limited number of coeff.
void YARPFftFeatures::Reconstruct (YARPImageOf<YarpPixelMono>& out)
{
	assert (out.GetPadding() == 0);

	memcpy (m_real2, m_re, sizeof(double) * m_h * m_w);
	memcpy (m_imag2, m_im, sizeof(double) * m_h * m_w);

	const int width = 4;
	const int height = 7;

	// the vector is [width * heigth + (width) * (height-1)] * 2.
	const int vectorsize = width * height * 2 + (width) * (height-1) * 2;

	for (int j = 0; j < m_h; j++)
	{
		memset (m_real2+j*m_w+width, 0, sizeof(double) * (m_w - 2*width + 1));
		memset (m_imag2+j*m_w+width, 0, sizeof(double) * (m_w - 2*width + 1));
	}

	for (int j = height; j < m_h-height+1; j++)
	{
		memset (m_real2+j*m_w, 0, sizeof(double) * m_w);
		memset (m_imag2+j*m_w, 0, sizeof(double) * m_w);
	}

	// remove the DC component.
	m_real2[0] = m_imag2[0] = 0;

	// inverse FFT.
	int dims[2] = { m_w, m_h };
	m_fft.Fft (2, dims, m_real2, m_imag2, -1, 0);	// inverse fft!

	unsigned char *tmp = (unsigned char *)out.GetAllocatedArray();

	double max = m_real2[0];
	double min = m_real2[0];
	for (int j = 1; j < m_w * m_h; j++)
		if (m_real2[j] > max)
			max = m_real2[j];
		else
		if (m_real2[j] < min)
			min = m_real2[j];

	for (int j = 0; j < m_w * m_h; j++)
		*tmp++ = (unsigned char)((m_real2[j]-min)/(max-min)*255.0);
}

void YARPFftFeatures::GetFftPict (YARPImageOf<YarpPixelRGB>& out)
{
	assert (out.GetPadding() == 0);

	YARPFft::Fft2DShift (m_re, m_real2, m_w, m_h);
	YARPFft::Fft2DShift (m_im, m_imag2, m_w, m_h);

	// create a pseudo color image of the re im part.
	double rmax = fabs(m_real2[0]);
	double imax = fabs(m_imag2[0]);
	for (int i = 1; i < m_w * m_h; i++)
	{
		if (fabs(m_real2[i]) > rmax)
			rmax = fabs(m_real2[i]);
		if (fabs(m_imag2[i]) > imax)
			imax = fabs(m_imag2[i]);
	}

	double rmin = - rmax;
	double imin = - imax;
	double tmp;

	// it's rgb!
	char *op = out.GetAllocatedArray();

	for (int i = 0; i < m_w * m_h; i++)
	{
		tmp = (m_real2[i] - rmin) / (rmax - rmin) * 255.0;
		if (tmp > 255.0) 
			tmp = 255;
		else
		if (tmp < 0) 
			tmp = 0;

		*op++ = (char)tmp;

		tmp = (m_imag2[i] - imin) / (imax - imin) * 255.0;
		if (m_im[i] > 255.0) 
			m_im[i] = 255;
		else
		if (m_im[i] < 0) 
			m_im[i] = 0;

		*op++ = (char)tmp;

		*op++ = 0; // skip blue.
	}
}
