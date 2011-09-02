//
// YARPTemporalSmooth.cpp
//

#include "YARPTemporalSmooth.h"

YARPTemporalSmooth::YARPTemporalSmooth(const YARPImageOf<YarpPixelMono>& first, double lambda) : YARPFilter()
{
	lCoeff = lambda;
	m_old = first;
}
	
YARPTemporalSmooth::YARPTemporalSmooth(void) : YARPFilter()
{
	lCoeff = 0.0;
}

void YARPTemporalSmooth::Resize(const YARPImageOf<YarpPixelMono>& first, double lambda)
{
	lCoeff = lambda;
	m_old = first;
}

void YARPTemporalSmooth::Init(const YARPImageOf<YarpPixelMono>& first, double lambda)
{
	Resize(first, lambda);
}

void YARPTemporalSmooth::Cleanup (void)
{
	m_old.Cleanup ();
}

void YARPTemporalSmooth::Apply(const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out)
{
	// res(t+1)=l*img(t)+(1-l)*res(t)

	const double ulambda = 1 - lCoeff;
	unsigned char * src = (unsigned char *)in.GetIplPointer()->imageData;
	unsigned char * dst = (unsigned char *)out.GetIplPointer()->imageData;
	unsigned char * f0 = (unsigned char *)m_old.GetIplPointer()->imageData;

	double tmp;
	const int size = out.GetIplPointer()->imageSize;
	for (int i = 0; i < size; i++)
	{
		 tmp = lCoeff * *src++ + ulambda * *f0;
		 *f0++ = *dst++ = (unsigned char)(tmp);
	}
}
