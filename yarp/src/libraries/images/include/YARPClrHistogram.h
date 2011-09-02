//
// YARPClrHistogram.h
//	- simple row/column color histogram.

#ifndef __YARPClrHistogramh__
#define __YARPClrHistogramh__

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPColorConverter.h"

// useful?
#include <VisMatrix.h>
#include <Models.h>

#ifdef __LINUX__
#include "StaticLut.h"
#endif

class YARPClrHistogram : public YARPFilter
{
private:
	YARPClrHistogram (const YARPClrHistogram&);
	void operator= (const YARPClrHistogram&);

protected:
	int m_ecc;
	int m_ang;

	YARPImageOf<YarpPixelHSV> m_hsv;
	CStaticLut<int> m_histo;

	CVisDVector m_rows;	// row histo
	CVisDVector m_cols;	// col histo
	double m_max_r, m_max_c;

	int m_saturation_int;
	int m_luma_int;

public:
	YARPClrHistogram (int necc, int nang, int hwidth, int hheight);
	virtual ~YARPClrHistogram () { Cleanup(); }
	virtual void Cleanup();
	virtual bool InPlace(void) const { return false; }

	void CumulateHistograms (const YARPImageOf<YarpPixelHSV>& src);
	void ConvertHistogramToImage (YARPImageOf<YarpPixelMono>& img);

	void Apply (const YARPImageOf<YarpPixelRGB>& is);

	CVisDVector& GetRowHistogram (void) { return m_rows; }
	CVisDVector& GetColHistogram (void) { return m_cols; }
	int GetMaxRow (void) const { return (int)(m_max_r+0.5); }
	int GetMaxCol (void) const { return (int)(m_max_c+0.5); }
};

#endif
