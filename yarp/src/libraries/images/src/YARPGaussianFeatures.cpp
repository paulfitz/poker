//
// YARPGaussianFeatures.cpp
//

#include "YARPGaussianFeatures.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

//
//
YARPGaussianFeatures::YARPGaussianFeatures(int necc, int nang, double rfmin, int size) 
	: YARPFilter(),
	  YARPLogPolar(necc, nang, rfmin, size),
	  m_sigmas(8)
{
	// memalloc.
	m_ro2 = new double[necc];
	assert (m_ro2 != NULL);
	
	m_coeffs = new _YARPG[m_sigmas];
	assert (m_coeffs != NULL);

	for (int j = 0; j < m_sigmas; j++)
	{
		m_coeffs[j].Resize (necc, nang);
	}

	// build arrays for gaussian derivative.
	double tmp;
	for (int i = 0; i < necc; i++)
	{
		tmp = ro0 * pow(a, i/kxi);
		m_ro2[i] = tmp*tmp;
	}

	// loop over sigmas.
	int count;  int i;
	for (i = 1, count = 0; i <= 15; i+=2, count++)
	{
		ComputeCoefficients (double(i), m_coeffs[count]);
	}

	m_features.Resize (m_sigmas * 2);
	m_features = 0;

	FILE *fp = fopen ("pippo.txt", "w");
	assert (fp != NULL);

	for (i = 0; i < m_sigmas; i++)
	{
		for (int j = 0; j < nang; j++)
		{
			for (int k = 0; k < necc; k++)
				fprintf (fp, "%lf ", m_coeffs[i].m_co[j][k]);
			fprintf (fp, "\n");
		}
		fprintf (fp, "\n");
	}

	fclose (fp);
}

// compute gaussian coeffs for a particular sigma.
void YARPGaussianFeatures::ComputeCoefficients(double sigma, _YARPG& coeffs)
{
	const double normfactor = sigma * sigma * kxi / log(a) / q;
	const double denom = 2 * sigma * sigma;
	const double thr = 1e-3;

	// filter maxsize is necc * 2
	// negative part. filter is odd.

	coeffs.m_norm = 0;
	for (int j = 0; j < nAng; j++)
	{
		double ctheta = cos(j/q);
		for (int i = 0; i < nEcc; i++)
		{
			coeffs.m_co[j][i] = -m_ro2[i] * ctheta / normfactor * exp (-m_ro2[i] / denom);
			if (fabs(coeffs.m_co[j][i]) < thr)
				coeffs.m_co[j][i] = 0.0;
			else
				coeffs.m_norm += fabs(coeffs.m_co[j][i]);
		}
	}

	coeffs.m_norm /= 2;
}

// buffer is the whole image buffer.
double YARPGaussianFeatures::SpecialConvolveX (_YARPG& coeffs, const unsigned char *buffer)
{
	double accumulator = 0;

	const int size = nEcc * nAng;
	double *co = coeffs.m_co[0];

	for (int i = 0; i < size; i++)
	{
		if (*co != 0.0)
		{
			accumulator += (*co * *buffer);
		}

		co++;
		buffer++;
	}

	accumulator /= coeffs.m_norm;
	return accumulator;
}

// just shift the image ptr.
double YARPGaussianFeatures::SpecialConvolveY (_YARPG& coeffs, const unsigned char *buffer)
{
	double accumulator = 0;

	const int size = nEcc * nAng;
	double *co = coeffs.m_co[0];

	const int shift =  nEcc * nAng/4;
	unsigned char *ptr = (unsigned char *)buffer + shift;

	for (int i = 0; i < size-shift; i++)
	{
		if (*co != 0.0)
		{
			accumulator += (*co * *ptr);
		}

		co++;
		ptr++;
	}

	ptr = (unsigned char *)buffer;
	for (int i = 0; i < shift; i++)
	{
		if (*co != 0.0)
		{
			accumulator += (*co * *ptr);
		}

		co++;
		ptr++;
	}

	accumulator /= coeffs.m_norm;
	return accumulator;
}

YARPGaussianFeatures::~YARPGaussianFeatures()
{
	Cleanup ();
}

void YARPGaussianFeatures::Cleanup ()
{
	if (m_ro2 != NULL)
		delete[] m_ro2;

	if (m_coeffs != NULL)
	{
		delete[] m_coeffs;
	}
}

void YARPGaussianFeatures::Apply (const YARPImageOf<YarpPixelMono>& in)
{
	assert (in.GetPadding() == 0);

	for (int i = 1; i <= m_sigmas; i++)
	{
		m_features(i) = SpecialConvolveX (m_coeffs[i-1], (const unsigned char *)in.GetAllocatedArray());
		m_features(i+m_sigmas) = SpecialConvolveY (m_coeffs[i-1], (const unsigned char *)in.GetAllocatedArray());
	}
}
