// DisparitySSD.h: interface for the CDisparitySSD class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YARPLpDisparity_H__30671FB3_DD3B_11D3_A532_00A0CC2C26FF__INCLUDED_)
#define AFX_YARPLpDisparity_H__30671FB3_DD3B_11D3_A532_00A0CC2C26FF__INCLUDED_

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPLpShifter.h"

//
// Multiple channel disparity over logpolar images.
// Shifts LP images by different disparity values, computes SSD correlation, and finds the maximum.
//
class YARPLpDisparity : public YARPLpShifter
{
protected:
	YARPImageOf<YarpPixelMono> m_histogram_values;
	const int m_all_disp;
	double *m_disparity_array;

	// maximum shift is 1.5 * nEcc.
	//enum { m_nshifts = 11 };
	//int shifts[] = { /* list all disp values here */ };

	double ShiftAndCorrelate (const YARPImageOf<YarpPixelMono>& l, const YARPImageOf<YarpPixelMono>& r, int shift);
	double ShiftAndCorrelate (const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r, int shift);
	double ShiftAndCorrelate (const YARPImageOf<YarpPixelBGR>& l, const YARPImageOf<YarpPixelBGR>& r, int shift);
	inline float _sqr(float x) const { return x*x; }

	YARPImageOf<YarpPixelMono> m_corrMap;
	YARPImageOf<YarpPixelMono> m_cumulativeMap;
	YARPImageOf<YarpPixelRGB>  m_tmp;

public:
	YARPLpDisparity (int, int, double, int);
	virtual ~YARPLpDisparity() { Cleanup(); }

	virtual void Cleanup(void) 
	{
		m_histogram_values.Cleanup();
		m_corrMap.Cleanup();
		m_cumulativeMap.Cleanup();
		m_tmp.Cleanup();

		if (m_disparity_array != NULL)
			delete[] m_disparity_array;
	}

	int ComputeDisparity(const YARPImageOf<YarpPixelMono>& l, const YARPImageOf<YarpPixelMono>& r);
	int ComputeDisparity(const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r);
	int ComputeDisparity(const YARPImageOf<YarpPixelBGR>& l, const YARPImageOf<YarpPixelBGR>& r);

	void GetHistogramAsImage(YARPImageOf<YarpPixelMono>& histo);
	YARPImageOf<YarpPixelMono>& GetHistogramAsImage();

	double CorrelationMap (const YARPImageOf<YarpPixelMono>& l, const YARPImageOf<YarpPixelMono>& r);
	double CorrelationMap (const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r);
	double CorrelationMap (const YARPImageOf<YarpPixelBGR>& l, const YARPImageOf<YarpPixelBGR>& r);
	inline YARPImageOf<YarpPixelMono>& GetCorrelationMap () { return m_corrMap; }

	double CorrelationMap (const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r, int disp, int bias);
};

#endif // !defined(AFX_YARPLpDisparity_H__30671FB3_DD3B_11D3_A532_00A0CC2C26FF__INCLUDED_)
