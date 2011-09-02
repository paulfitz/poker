//
// YARPTargetLocator.h
//	pasa: Aug 2002
//

#ifndef __YARPTargetLocatorh__
#define __YARPTargetLocatorh__

#include "stdio.h"
#include "VisMatrix.h"
#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPColorConverter.h"
#include "YARPImageFile.h"
#include "Models.h"


// contains the histogram.
// histogram is built in the HS space. Of course this eliminates the possibility to
//	distinguish 'white' and 'dark' stuff. In a future version a better color space representation
//	should be used. This is ok for a quick test of the method.
class YARPColorLocator
{
protected:
	YARPImageOf<YarpPixelHSV> m_hsv;
	YARPImageOf<YarpPixelFloat> m_backprojected;
	YARPImageOf<YarpPixelFloat> m_integral;
	double m_norm_factor_backp;

	CStaticLut<int> m_histo;
	double m_luma_thr;
	double m_sat_thr;
	int m_luma_thr_int;
	int m_sat_thr_int;

	int m_nbinssat;
	int m_nbinshue;

	int m_numberofframes;
	int m_extx, m_exty;
	int m_numberofpoints;
	int m_numberofpoints_std;

protected:
	// just a convenient place to store per model data.
	double m_acc_thr;
	double m_acc_std;
	int m_count;

	double m_expected_thr;
	double m_expected_thr_std;

public:
	YARPColorLocator (int size, int nbinssat, int nbinshue, double luma_thr, double sat_thr);
	YARPColorLocator ();
	~YARPColorLocator ();

	void Resize (int size, int nbinssat, int nbinshue, double luma_thr, double sat_thr);

	void CumulateHistograms (const YARPImageOf<YarpPixelHSV>& src, const YARPImageOf<YarpPixelMono>& mask, bool reset=true);
	void Apply (const YARPImageOf<YarpPixelBGR>& img, const YARPImageOf<YarpPixelMono>& mask, bool reset=true);
	void BackProject (const YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest);

	void Reset ();
	int GetExtent (double& exx, double& exy);
	void GenerateSum(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest);
	int Find (double exx, double exy, double& x, double& y, double& quality);
	int GetNumberOfPoints (double& ave, double& std);
	int ImprovedSearch (YARPImageOf<YarpPixelBGR>& img, double exx, double exy, double x, double y, double& xx, double& yy);
	double CompareHisto (CStaticLut<int>& x1, CStaticLut<int>& x2);

	void AddExpectancy (YARPImageOf<YarpPixelBGR>& img);
	void GetExpectancy (double& mean, double& stddev);
};


#endif