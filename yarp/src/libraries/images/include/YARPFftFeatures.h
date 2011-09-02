//
// YARPFftFeatures.h
//

//
// EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL 
//

#ifndef __YARPFftFeaturesh__
#define __YARPFftFeaturesh__

#include "VisMatrix.h"

#include "YARPImage.h"
#include "YARPFilters.h"

//
//
// WARNING: because of the fft implementation (tmp buffer stuff)
//	the filter can only be allocated once (to the max size).
//
// WARNING: this class is very experimental.
//	- if you want to get a 2D FFT of an image better use YARPFft directly.
//
class YARPFftFeatures : public YARPFilter
{
private:
	YARPFftFeatures (const YARPFftFeatures&);
	void operator= (const YARPFftFeatures&);

	inline int _max (int a, int b) { return (a>b)?a:b; }

protected:
	double *m_re;
	double *m_im;
	double *m_real2;
	double *m_imag2;

	int m_w;
	int m_h;

	YARPFft m_fft;

public:
	YARPFftFeatures (int width, int height);
	virtual ~YARPFftFeatures ();

	virtual void Cleanup ();
	virtual bool InPlace (void) const { return true; }

	void Apply (const YARPImageOf<YarpPixelMono>& in);
	void ZigZagScan (CVisDVector& streamlined);
	void GetFftPict (YARPImageOf<YarpPixelRGB>& out);

	void Reconstruct (YARPImageOf<YarpPixelMono>& out);
};

#endif
