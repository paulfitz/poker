// ColorSegmentation.h: interface for the CColorSegmentation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YARPLpClrSegmentation_H__71DD7F89_D0DB_11D4_90F4_00500463430C__INCLUDED_)
#define AFX_YARPLpClrSegmentation_H__71DD7F89_D0DB_11D4_90F4_00500463430C__INCLUDED_

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPlogpolar.h"
#include "YARPTemporalSmooth.h"
#include "YARPFirstDerivativeT.h"
#include "YARPBlurFilter.h"
#include "YARPBinarizeFilter.h"
#include "YARPColorConverter.h"
#include "YARPSimpleOperations.h"

#include <VisMatrix.h>
#include <Models.h>

#ifndef __QNX__
#include "StaticLut.h"
#endif

// input: color images.
// output: segmented 1 bit images, target position, target speed, bounding box.
//
// TODO LATER:
// WARNING: the saturation and the intensity have to be thresholded appropriately.
//
//
#ifndef VERBOSE
//#define VERBOSE 1	// remove to avoid printing messages.
#endif
#ifdef VERBOSE
#include <stdio.h>
#endif

const int gx2[] = {2, 28, 124, 204, 124, 28, 2};
// gauss (7, (int *)gx, necc, nang, IT_Mono, 9),

class YARPLpClrSegmentation 
{
protected:
	YARPImageOf<YarpPixelMono> m_monoleft;
	YARPImageOf<YarpPixelMono> m_segleft;
	YARPImageOf<YarpPixelMonoSigned> m_sl;
	YARPImageOf<YarpPixelMono> m_monoright;
	YARPImageOf<YarpPixelMono> m_segright;
	YARPImageOf<YarpPixelMonoSigned> m_sr;

	YARPImageOf<YarpPixelHSV> m_convertedL;
	YARPImageOf<YarpPixelHSV> m_convertedR;

	YARPImageOf<YarpPixelMono> m_background_l;
	YARPImageOf<YarpPixelMono> m_background_r;
	YARPImageOf<YarpPixelMono> m_tmp;

	// filters.
	YARPTemporalSmooth m_smoothl;
	YARPTemporalSmooth m_smoothr;
	double m_smoothing_delta;
	YARPFirstDerivativeT<YarpPixelMonoSigned> m_dtl;
	YARPFirstDerivativeT<YarpPixelMonoSigned> m_dtr;
	//CLpGenericSepFilter m_gauss;
	YARPLpBlurFilter<YarpPixelMono> m_gauss;

	// log-polar converter.
	YARPLogPolar m_lp;

	bool m_reset;

	// histograms.
	CStaticLut<int> m_motionhisto;
	CStaticLut<int> m_wholehisto;
	YARPImageOf<YarpPixelMono> m_dummy;

	int m_npixelleft;
	int m_npixelright;

	double m_bin_thr;
	YARPBinarizeFilter m_bin;
	
	// threshold on histo region growing.
	double m_histothreshold;

	// threshold for histo difference.
	double m_diffthreshold;

	// threshold for target quality.
	double m_areathreshold;

	// remove colors below given thresholds.
	double m_saturation_thr;
	double m_luma_thr;
	int m_int_saturation;
	int m_int_luma;

	// threshold for the correlation measure [0,1].
	double m_corrindexthr;

	// segmentation results. i.e. the position of the object.
	// it would be nice to provide also the bounding box.
	// this could be updated on line for attentional mechanisms support...
	CVisDVector m_targetleft;
	CVisDVector m_targetright;
	
	// used to compute target speed in the image plane.
	CVisDVector m_old_left;
	CVisDVector m_delta_left;

	CVisDVector m_lptargetleft;

	CVisDVector m_old_right;
	CVisDVector m_delta_right;

	CVisDVector m_lptargetright;

	bool m_valid;		// target validity.
	bool m_old_valid;	// target validity (prev. step).
	bool m_speed_valid;	// target speed validity.

	// locked mode.
	bool m_locked;

	// out image segm only?
	bool m_segm_only;

	enum { motionDetectionThreshold = 80 };
	enum { colorDetectionThreshold = 20 };

public:
	YARPLpClrSegmentation (int necc, int nang, double rfmin, int size);
	virtual ~YARPLpClrSegmentation() { Cleanup (); }
	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return false; }

	// initialize default parameters. Only parameters are initialized here.
	inline void InitDefault (void)
	{
		m_bin_thr = 15;			// binarize threshold.
		
		m_histothreshold = 0.7;	// region growing. (default was 0.5)
								// changed on Feb 2000.

		m_diffthreshold = 36;	// histo difference.
		
		m_areathreshold = 100;	// threshold on area.
		
		m_corrindexthr = 1.0;	// 

		m_smoothing_delta = 0.5;

		// limit the color space (using luminance also).
		m_saturation_thr = 0.1;
		m_luma_thr = 0.2;

		// integer thresholds.
		m_int_saturation = int (m_saturation_thr * 255.0);
		m_int_luma = int (m_luma_thr * 255.0);

		m_segm_only = false;
	}

protected:
	inline void MotionDetection (bool reset);
	inline void MotionDetection2 (bool reset, bool binarize);
	inline void TargetLocation (CVisDVector& cart_res, CVisDVector& lp_res,	bool& valid, int& pixels, YARPImageOf<YarpPixelMono>& src_img);
	inline void BuildHsHistogram (CStaticLut<int>& lut, const YARPImageOf<YarpPixelHSV>& src, const YARPImageOf<YarpPixelMono>& mask);
	inline void ThresholdHistogram (double& h, double& s, CStaticLut<int>& lut,	int pixels);
	inline void BuildHsHistogramNoMask (CStaticLut<int>& lut, const YARPImageOf<YarpPixelHSV>& src);
	inline void RegionGrowing (double h, double s, CStaticLut<int>& lut, int thr);
	inline void RecursiveRegionGrowing (int x, int y, CStaticLut<int>& lut,	int thr);
	inline void ConditionedRegionGrowing (double h, double s, CStaticLut<int>& lut, int thr, CStaticLut<int>& exclude, int thr_e);
	inline void RecursiveConditionedRegionGrowing (int x, int y, CStaticLut<int>& lut, int thr, CStaticLut<int>& exclude, int thr_e);
	inline void SegmentTarget (YARPImageOf<YarpPixelMono>& mask, CVisDVector& cart_res,	CVisDVector& lp_res, bool& valid, CStaticLut<int>& lut,	const YARPImageOf<YarpPixelHSV>& src);
	inline void SegmentTargetMasked (YARPImageOf<YarpPixelMono>& mask, CVisDVector& cart_res, CVisDVector& lp_res, bool& valid, CStaticLut<int>& lut, const YARPImageOf<YarpPixelHSV>& src, const YARPImageOf<YarpPixelMono>& mask2);
	inline bool IsMoving (const YARPImageOf<YarpPixelMono>& left);
	inline bool EvaluateQuality (const YARPImageOf<YarpPixelMono>& left);
	inline bool IsOutOfFovea (const CVisDVector& tl);
	inline void EstimateTargetSpeed (void);

	inline void ConvertHistogramToImage (CStaticLut<int>& lut, YARPImageOf<YarpPixelMono>& img, bool only1=false);
	inline void FillRegion (const CVisDVector& t, YARPImageOf<YarpPixelMono>& img, int size);
	inline void BackgroundDifference (const YARPImageOf<YarpPixelMono>& bg, YARPImageOf<YarpPixelMono>& src);

public:
	// Segmentation steps helper functions.
	bool InitialDetection (const YARPImageOf<YarpPixelRGB>& m_left, const YARPImageOf<YarpPixelRGB>& m_right, YARPImageOf<YarpPixelMono>& m_store_left, YARPImageOf<YarpPixelMono>& m_store_right);
	bool StartupSegmentation (const YARPImageOf<YarpPixelRGB>& m_left, const YARPImageOf<YarpPixelRGB>& m_right, YARPImageOf<YarpPixelMono>& m_store_left, YARPImageOf<YarpPixelMono>& m_store_right);
	bool ColorSegmentation (const YARPImageOf<YarpPixelRGB>& m_left, const YARPImageOf<YarpPixelRGB>& m_right, YARPImageOf<YarpPixelMono>& m_store_left, YARPImageOf<YarpPixelMono>& m_store_right);
	void AbortSegmentation (void);

	inline void GetTargetPosition (CVisDVector& l, CVisDVector& r, bool& valid) const;
	inline void GetTargetPositionLp (CVisDVector& l, CVisDVector& r, bool& valid) const;
	inline void GetTargetSpeed (CVisDVector& l, CVisDVector& r, bool& valid) const;

	inline bool& GetReset (void) { return m_reset; }
	inline void Reset (void) { m_reset = true; }
	inline void ResetHistograms (void) { m_wholehisto = 0; m_motionhisto = 0; }

	inline void SetBinaryThreshold (double t) { m_bin_thr = t; m_bin.SetThresholds (t); }
	inline double GetBinaryThreshold (void) const { return m_bin_thr; }
	inline void SetDifferenceThreshold (double t) { m_diffthreshold = t; }
	inline double GetDifferenceThreshold (void) const { return m_diffthreshold; }
	inline void SetQualityThreshold (double t) { m_areathreshold = t; }
	inline double GetQualityThreshold (void) const { return m_areathreshold; }
	inline void SetLockStatus (bool l) { m_locked = l; }
	inline bool GetLockStatus (void) const { return m_locked; }

	inline void SetLumaThreshold (double t);
	inline double GetLumaThreshold (void) const;
	inline void SetSaturationThreshold (double t);
	inline double GetSaturationThreshold (void) const;

	inline int GetHistogramWidth (void) const { return m_motionhisto.GetWidth (); }
	inline int GetHistogramHeight (void) const { return m_motionhisto.GetHeight (); }

	inline void GetMotionHistogramAsImage (YARPImageOf<YarpPixelMono>& h) { ConvertHistogramToImage (m_motionhisto, h, true); }
	inline void GetMotionHistogramAsImage (unsigned char *h) 
	{ 
		ConvertHistogramToImage (m_motionhisto, m_dummy, true); 
		memcpy (h, m_dummy.GetAllocatedArray(), m_dummy.GetIplPointer()->imageSize);
	}
	inline void GetWholeHistogramAsImage (YARPImageOf<YarpPixelMono>& h) { ConvertHistogramToImage (m_wholehisto, h); } 
	inline void GetWholeHistogramAsImage (unsigned char *h) 
	{ 
		ConvertHistogramToImage (m_wholehisto, m_dummy);
		memcpy (h, m_dummy.GetAllocatedArray(), m_dummy.GetIplPointer()->imageSize);
	}
};

//
// Method: Motion detection.
// Purpose: detect motion while the robot is stationary.
//	Work on mono images (left and right channels together).
//	Applies a cascade of filters to detect moving regions in the images.
//	After the processing the images are binary (either 0 or 255).
//
inline void YARPLpClrSegmentation::MotionDetection (bool reset)
{
	// extracts motion area location..
	if (reset)
	{
		// intialize filters.
		m_sl.CastCopy(m_monoleft);
		m_dtl.Init (m_sl);
		m_smoothl.Init (m_monoleft, m_smoothing_delta);

		m_sr.CastCopy(m_monoright);
		m_dtr.Init (m_sr);
		m_smoothr.Init (m_monoright, m_smoothing_delta);
	}
	else
	{
		if (m_bin_thr == 0)
		{
			m_smoothl.Apply (m_monoleft, m_monoleft);
			m_sl.CastCopy(m_monoleft);
			m_dtl.Apply (m_sl, m_sl);
			iplAbs (m_sl, m_sl);
			m_monoleft.CastCopy(m_sl);

			m_smoothr.Apply (m_monoright, m_monoright);
			m_sr.CastCopy(m_monoright);
			m_dtr.Apply (m_sr, m_sr);
			iplAbs (m_sr, m_sr);
			m_monoright.CastCopy(m_sr);
		}
		else
		{
			m_smoothl.Apply (m_monoleft, m_monoleft);
			
			m_sl.CastCopy(m_monoleft);
			m_dtl.Apply (m_sl, m_sl);
			iplAbs (m_sl, m_sl);
			m_monoleft.CastCopy(m_sl);

			iplThreshold (m_monoleft, m_monoleft, int (m_bin_thr / 2 + 128.5));

			m_smoothr.Apply (m_monoright, m_monoright);
			
			m_sr.CastCopy(m_monoright);
			m_dtr.Apply (m_sr, m_sr);
			iplAbs (m_sr, m_sr);
			m_monoright.CastCopy(m_sr);

			iplThreshold (m_monoright, m_monoright, int (m_bin_thr / 2 + 128.5));
		}
	}
}

inline void YARPLpClrSegmentation::BackgroundDifference (const YARPImageOf<YarpPixelMono>& bg, YARPImageOf<YarpPixelMono>& src)
{
	unsigned char *bck = (unsigned char *)bg.GetIplPointer()->imageData;
	unsigned char *img = (unsigned char *)src.GetIplPointer()->imageData;

	int tmp;
	const int size = src.GetIplPointer()->imageSize;
	for (int i = 0; i < size; i++)
	{
		tmp = abs (*img - *bck++);
		*img++ = tmp;
	}
}

inline void YARPLpClrSegmentation::MotionDetection2 (bool reset, bool binarize)
{
	if (reset)
	{
		m_gauss.Apply (m_monoleft, m_background_l);
		m_gauss.Apply (m_monoright, m_background_r);
	}
	else
	{
		m_gauss.Apply (m_monoleft, m_monoleft);
		m_gauss.Apply (m_monoright, m_monoright);

		BackgroundDifference (m_background_l, m_monoleft);
		BackgroundDifference (m_background_r, m_monoright);

		// LATER: this could be improved. Remove temporary.
		//m_tmp = m_background_l;
		//m_tmp -= m_monoleft;
		//m_monoleft -= m_background_l;
		//m_monoleft += m_tmp;

		//m_tmp = m_background_r;
		//m_tmp -= m_monoright;
		//m_monoright -= m_background_r;
		//m_monoright += m_tmp;

		if (binarize)
		{
			m_bin.Apply (m_monoleft, m_monoleft);
			m_bin.Apply (m_monoright, m_monoright);
		}
	}
}

inline void YARPLpClrSegmentation::FillRegion (const CVisDVector& t, YARPImageOf<YarpPixelMono>& img, int size)
{
	img.Zero();

	const int x = int (t(1)+.5);	// necc.
	const int y = int (t(2)+.5);	// nang.
	const int nEcc = m_lp.GetnEcc();
	const int nAng = m_lp.GetnAng();

	int sx = x - size;
	if (sx < 0) sx = 0;
	int ex = x + size;	
	if (ex >= nEcc) ex = nEcc - 1;

	bool split = false;
	int sy = y - size;
	int ey = y + size;
	if (sy < 0 || ey >= nAng)
	{
		// split region.
		split = true;
		if (sy < 0)	sy = nAng + sy;
		if (ey >= nAng) ey = ey - nAng;
	}
	else
	{
		split = false;
	}

	if (!split)
		for (int i = sy; i <= ey; i++)
			for (int j = sx; j <= ex; j++)
				img(j, i) = 255;
	else
	{
		for (int i = sy; i < nAng; i++)
			for (int j = sx; j <= ex; j++)
				img(j, i) = 255;
		for (int i = 0; i <= ey; i++)
			for (int j = sx; j <= ex; j++)
				img(j, i) = 255;
	}
}

//
// TargetLocation.
// Purpose: given a binary image, computes the COM of the "white" region both 
//	in logpolar and cartesian and returns a judgement about the quality of the 
//	segmented region. The number of valid pixels must be greater than a threshold.
//
inline void YARPLpClrSegmentation::TargetLocation (CVisDVector& cart_res,
												   CVisDVector& lp_res,
												   bool& valid,
												   int& pixels,
												   YARPImageOf<YarpPixelMono>& src_img)
{
	unsigned char **ddata = (unsigned char **)src_img.GetArray();

	// look for the maximum.
	cart_res = 0;
	lp_res = 0;

	// simple averaging .. target location.
	double x, y;
	int pixCount = 0;

	cart_res = 0;
	lp_res = 0;

	for (int j = 0; j < m_lp.GetnAng(); j++)
		for (int i = 0; i < m_lp.GetnEcc(); i++)
		{
			if (ddata[j][i] == 255)
			{
				m_lp.Lp2Cart (i, j, x, y);
				cart_res(1) += x;
				cart_res(2) += y;
				pixCount ++;
			}
		}

	if (pixCount > motionDetectionThreshold)
	{
		valid = true;
		cart_res(1) /= pixCount;
		cart_res(2) /= pixCount;
		m_lp.Cart2Lp (cart_res(1), cart_res(2), lp_res(1), lp_res(2));
	}
	else
	{
		valid = false;
		cart_res = m_lp.GetSize() / 2;
		lp_res = 0;
	}

	pixels = pixCount;
}

//
// IsMoving.
//	Purpose: Decides whether there's something moving in the images worth to be followed.
//	It simply counts the number of pixels in the binary images.
//
inline bool YARPLpClrSegmentation::IsMoving (const YARPImageOf<YarpPixelMono>& left)
{
	unsigned char *imgl = (unsigned char *)left.GetArray()[0];
	int pixCount = 0;

	for (int i = 0; i < m_lp.GetnAng(); i++)
	{
		imgl = (unsigned char *)left.GetArray()[i];

		for (int j = 0; j < m_lp.GetnEcc(); j++)
		{
			if (*imgl++ == 255)
				pixCount++;
		}
	}

	return 	(pixCount > motionDetectionThreshold) ? true : false;
}

// LATER: I should try something more reliable.
// For example: evaluate shape and area/pixels ratio.
//
// EvaluateQuality.
//	Purpose: evaluate the reliability of the segmented region. Presently this is
//	done simply by computing the area of the enclosing rectangle of the segmented
//	region(s).
//
inline bool YARPLpClrSegmentation::EvaluateQuality (const YARPImageOf<YarpPixelMono>& left)
{
#if 0
	unsigned char **imgl = (unsigned char **)left.GetArray();

	int pixCountL = 0;
	double minxl = m_lp.GetSize(), maxxl = 0, minyl = m_lp.GetSize(), maxyl = 0;
	double x, y;

	// try area evaluation.
	for (int j = 0; j < m_lp.GetnAng(); j++)
		for (int i = 0; i < m_lp.GetnEcc(); i++)
		{
			if (imgl[j][i] == 255)
//			if (imgl[j][i] > 0) // > 0 if using SegmentTarget3
			{
				m_lp.Lp2Cart (i, j, x, y);
				if (x < minxl) minxl = x;
				if (x > maxxl) maxxl = x;
				if (y < minyl) minyl = y;
				if (y > maxyl) maxyl = y;
				pixCountL ++;
			}
		}

	// quality is good if target fits into m_areathreshold square.

	if (maxxl - minxl < m_areathreshold && 
		maxyl - minyl < m_areathreshold)
		return true;
	else
		return false;
#endif

	return true;
}

//
// BuildHsHistogram.
//	Purpose: given an HSV image and a mask (i.e. motion segmented pixels)
//	builds an histogram in HS space. It uses the lut class defined in Models lib.
//
inline void YARPLpClrSegmentation::BuildHsHistogram (CStaticLut<int>& lut,
												     const YARPImageOf<YarpPixelHSV>& src,
												     const YARPImageOf<YarpPixelMono>& mask)
{
	// just store each pixel in the histogram.
	unsigned char *fdataH, *fdataS, *fdataY, *fmask;

	double hue, saturation;

	for (int i = 0; i < m_lp.GetnAng(); i++)
	{
		fdataH = (unsigned char *)src.GetArray()[i];
		fdataS = fdataH + 1;
		fdataY = fdataS + 1;

		fmask = (unsigned char *)mask.GetArray()[i];

		for (int j = 0; j < m_lp.GetnEcc(); j++)
		{
			if (*fmask == 255 && *fdataY > m_int_luma && *fdataS > m_int_saturation)  //limits on segmentation must be checked.
			{
				// LATER: check limits on saturation... 
				// segmentation must be done only if saturation and brightness are above 
				// given thresholds.
				hue = double (*fdataH);
				saturation = double (*fdataS);

				lut (hue, saturation) ++;
				if (lut (hue, saturation) > lut.GetMaximum ())
					lut.GetMaximum () = lut (hue, saturation);
			}

			fdataH+=3;
			fdataS+=3;
			fdataY+=3;
			fmask++;
		}
	}
}

//
// BuildHsHistogramNoMask.
//	Purpose: build an histogram of all pixels in a HSV image taking into account
//	only HS planes. The lut class is defined in Models library.
//	This is used to build the histogram of the background.
//
inline void YARPLpClrSegmentation::BuildHsHistogramNoMask (CStaticLut<int>& lut,
														   const YARPImageOf<YarpPixelHSV>& src)
{
	// just store each pixel in the histogram.
	unsigned char *fdataH, *fdataS, *fdataY;
	double hue, saturation;

	for (int i = 0; i < m_lp.GetnAng(); i++)
	{
		fdataH = (unsigned char *)src.GetArray()[i];
		fdataS = fdataH + 1;
		fdataY = fdataS + 1;

		for (int j = 0; j < m_lp.GetnEcc(); j++)
		{
			if (*fdataY > m_int_luma && *fdataS > m_int_saturation) // (*fdataH != -1) check limits...
			{
				// as before limit must be checked to assure proper segmentation.
				hue = double(*fdataH);
				saturation = double(*fdataS);
				lut (hue, saturation) ++;
				if (lut (hue, saturation) > lut.GetMaximum ())
					lut.GetMaximum () = lut (hue, saturation);
			}
			fdataH+=3;
			fdataS+=3;
			fdataY+=3;
		}
	}
}

//
// ThresholdHistogram.
//	Purpose: get the mximum of all histo bins. This is used to find the principal 
//  color of the object of interests. 
//
inline void YARPLpClrSegmentation::ThresholdHistogram (double& h,
													   double& s,
													   CStaticLut<int>& lut,
													   int pixels)
{
	// max over the lut (pixels is not used).
	int mx = 0, my = 0;
	int max = lut (mx, my);

	for (int j = 0; j < lut.GetHeight (); j++)
		for (int i = 0; i < lut.GetWidth (); i++)
		{
			if (lut (i, j) > max)
			{
				max = lut (i, j);
				mx = i;
				my = j;
			}
		}

	h = lut.GetX (mx);
	s = lut.GetY (my);
}

//
// RegionGrowing.
//	Purpose: recursive region growing algorithm. It is applied to the histogram to
//	find all bins (they need to be adiacent) belonging to a particular moving object
//	The limit of the recursion is determined by a threshold on the height of the 
//	bins.
//
inline void YARPLpClrSegmentation::RegionGrowing (double h,
								  			      double s,
											      CStaticLut<int>& lut,
											      int thr)
{
	//
	int x, y;
	x = lut.GetIndexX (h);
	y = lut.GetIndexY (s);

	RecursiveRegionGrowing (x, y, lut, thr);
}

// the actual region growing procedure.
inline void YARPLpClrSegmentation::RecursiveRegionGrowing (int x,
														   int y,
														   CStaticLut<int>& lut,
														   int thr)
{
	int& value = lut (x, y);
	if (value == -1 || value < thr) return;
	if (value != 0) value = -1;

	if (x + 1 < lut.GetWidth ())
	{
		RecursiveRegionGrowing (x + 1, y, lut, thr);
		if (y - 1 >= 0)
		{
			RecursiveRegionGrowing (x + 1, y - 1, lut, thr);
			RecursiveRegionGrowing (x, y - 1, lut, thr);
		}

		if (y + 1 < lut.GetHeight ())
		{
			RecursiveRegionGrowing (x + 1, y + 1, lut, thr);
			RecursiveRegionGrowing (x, y + 1, lut, thr);
		}
	}
	else
	{
		RecursiveRegionGrowing (0, y, lut, thr);
		if (y - 1 >= 0)
		{
			RecursiveRegionGrowing (0, y - 1, lut, thr);
			RecursiveRegionGrowing (x, y - 1, lut, thr);
		}
		if (y + 1 < lut.GetHeight ())
		{
			RecursiveRegionGrowing (0, y + 1, lut, thr);
			RecursiveRegionGrowing (x, y + 1, lut, thr);
		}
	}

	if (x - 1 >= 0)
	{
		RecursiveRegionGrowing (x - 1, y, lut, thr);
		if (y - 1 >= 0)
			RecursiveRegionGrowing (x - 1, y - 1, lut, thr);

		if (y + 1 < lut.GetHeight ())
			RecursiveRegionGrowing (x - 1, y + 1, lut, thr);
	}
	else
	{
		RecursiveRegionGrowing (lut.GetWidth() - 1, y, lut, thr);

		if (y - 1 >= 0)
			RecursiveRegionGrowing (lut.GetWidth() - 1, y - 1, lut, thr);

		if (y + 1 < lut.GetHeight ())
			RecursiveRegionGrowing (lut.GetWidth() - 1, y + 1, lut, thr);
	}
}

//
// ConditionedRegionGrowing.
//	Purpose: to do a region growing but excluding the part of the color space
//	belonging to the background. It is used to detect the target without 
//	segmenting the background also.
//
inline void YARPLpClrSegmentation::ConditionedRegionGrowing (
									  double h,
									  double s,
									  CStaticLut<int>& lut,
									  int thr,
									  CStaticLut<int>& exclude,
									  int thr_e)
{
	//
	int x, y;
	x = lut.GetIndexX (h);
	y = lut.GetIndexY (s);

	RecursiveConditionedRegionGrowing (x, y, lut, thr, exclude, thr_e);
}

inline void YARPLpClrSegmentation::RecursiveConditionedRegionGrowing (
											   int x,
											   int y,
											   CStaticLut<int>& lut,
											   int thr,
											   CStaticLut<int>& exclude,
											   int thr_e)
{
	int& value = lut (x, y);
	if (value == -1 || 
		value < thr ||
		exclude (x, y) > thr_e) 
		return;

	if (value != 0) value = -1;

	if (x + 1 < lut.GetWidth ())
	{
		RecursiveConditionedRegionGrowing (x + 1, y, lut, thr, exclude, thr_e);
		if (y - 1 >= 0)
		{
			RecursiveConditionedRegionGrowing (x + 1, y - 1, lut, thr, exclude, thr_e);
			RecursiveConditionedRegionGrowing (x, y - 1, lut, thr, exclude, thr_e);
		}

		if (y + 1 < lut.GetHeight ())
		{
			RecursiveConditionedRegionGrowing (x + 1, y + 1, lut, thr, exclude, thr_e);
			RecursiveConditionedRegionGrowing (x, y + 1, lut, thr, exclude, thr_e);
		}
	}
	else
	{
		RecursiveConditionedRegionGrowing (0, y, lut, thr, exclude, thr_e);
		if (y - 1 >= 0)
		{
			RecursiveConditionedRegionGrowing (0, y - 1, lut, thr, exclude, thr_e);
			RecursiveConditionedRegionGrowing (x, y - 1, lut, thr, exclude, thr_e);
		}
		if (y + 1 < lut.GetHeight ())
		{
			RecursiveConditionedRegionGrowing (0, y + 1, lut, thr, exclude, thr_e);
			RecursiveConditionedRegionGrowing (x, y + 1, lut, thr, exclude, thr_e);
		}
	}

	if (x - 1 >= 0)
	{
		RecursiveConditionedRegionGrowing (x - 1, y, lut, thr, exclude, thr_e);
		if (y - 1 >= 0)
			RecursiveConditionedRegionGrowing (x - 1, y - 1, lut, thr, exclude, thr_e);

		if (y + 1 < lut.GetHeight ())
			RecursiveConditionedRegionGrowing (x - 1, y + 1, lut, thr, exclude, thr_e);
	}
	else
	{
		RecursiveConditionedRegionGrowing (lut.GetWidth() - 1, y, lut, thr, exclude, thr_e);

		if (y - 1 >= 0)
			RecursiveConditionedRegionGrowing (lut.GetWidth() - 1, y - 1, lut, thr, exclude, thr_e);

		if (y + 1 < lut.GetHeight ())
			RecursiveConditionedRegionGrowing (lut.GetWidth() - 1, y + 1, lut, thr, exclude, thr_e);
	}
}

inline void YARPLpClrSegmentation::SegmentTarget(
											  YARPImageOf<YarpPixelMono>& mask,
											  CVisDVector& cart_res,
											  CVisDVector& lp_res,
											  bool& valid,
											  CStaticLut<int>& lut,
											  const YARPImageOf<YarpPixelHSV>& src)
{
	cart_res = 0;
	lp_res = 0;

	double x, y;
	int pixCount = 0;

	unsigned char **fdataH = (unsigned char **)src.GetArray();
	unsigned char **fdataS = (unsigned char **)src.GetArray();
	unsigned char **fdataY = (unsigned char **)src.GetArray();

	unsigned char **fmask = (unsigned char **)mask.GetArray();

	double hue, saturation;
	const int ang = m_lp.GetnAng();
	const int ecc = m_lp.GetnEcc();

	for (int j = 0; j < ang; j++)
		for (int i = 0; i < ecc; i++)
		{
			if (fdataY[j][i*3+2] > m_int_luma && fdataS[j][i*3+1] > m_int_saturation)
			{
				hue = double (fdataH[j][i*3]);
				saturation = double (fdataS[j][i*3+1]);
				if (lut (hue, saturation) == -1)
				{
					fmask[j][i] = 255;		
				}
				else
				{
					if (!m_segm_only)
						fmask[j][i] = char (fmask[j][i] * .9);
					else
						fmask[j][i] = 0;
				}
			}
			else
			{
				if (!m_segm_only)
					fmask[j][i] = char (fmask[j][i] * .9);
				else
					fmask[j][i] = 0;
			}
		}
	
	for (int j = 0; j < ang; j++)
		for (int i = 0; i < ecc; i++)
		{
			if (fmask[j][i] == 255)
			{
				m_lp.Lp2Cart (i, j, x, y);
				cart_res(1) += x;
				cart_res(2) += y;
				pixCount ++;
			}
		}

	if (pixCount > colorDetectionThreshold)
	{
		valid = true;
		cart_res(1) /= pixCount;
		cart_res(2) /= pixCount;
		m_lp.Cart2Lp (cart_res(1), cart_res(2), lp_res(1), lp_res(2));
	}
	else
	{
		valid = false;
		cart_res = m_lp.GetSize() / 2;
		lp_res = 0;
	}
}

//
// Convert a lut histogram in a mono image.
//
inline void YARPLpClrSegmentation::ConvertHistogramToImage (CStaticLut<int>& lut, YARPImageOf<YarpPixelMono>& img, bool only1)
{
	int * ptr = lut.GetArrayPtr ()[0];
	unsigned char **ip = (unsigned char **)img.GetArray();
	const int width = lut.GetWidth ();
	const int height = lut.GetHeight ();
	double max = lut.GetMaximum();

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			int tmp = *ptr++;
			if (tmp < 0) 
				tmp = 255;
			else
			{
				if (!only1)
				{
					tmp = int (tmp / max * 255.0 + .5);
					if (tmp > 255) 
					tmp = 255;
				}
			}
			ip[i][j] = (unsigned char)tmp;
		}
}

inline bool YARPLpClrSegmentation::IsOutOfFovea (const CVisDVector& tl)
{
	const double xl = tl(1) - m_lp.GetSize() / 2;
	const double yl = tl(2) - m_lp.GetSize() / 2;

	double dst = sqrt (xl * xl + yl * yl);
	return (dst > 1.1 * m_lp.GetSize() / 4) ? true : false;
}

inline void YARPLpClrSegmentation::EstimateTargetSpeed (void)
{
	// estimate target speed by differentiation.
	if (m_valid)
	{
		if (m_old_valid)
		{
			// compute target speed. (0.04 means 40 ms period)
			m_delta_left = (m_targetleft - m_old_left) / 0.04;
			m_speed_valid = true;
		}
		else
		{
			m_delta_left = 0;
			m_speed_valid = false;
		}

		// store old values.
		m_old_left = m_targetleft;
		m_old_valid = true;
	}
	else
	{
		// reset old values.
		m_old_valid = false;
		m_speed_valid = false;
	}
}

inline void YARPLpClrSegmentation::GetTargetPosition (CVisDVector& l, CVisDVector& r, bool& valid) const
{
	l = m_targetleft;
	r = m_targetright;
	valid = m_valid;
}

inline void YARPLpClrSegmentation::GetTargetPositionLp (CVisDVector& l, CVisDVector& r, bool& valid) const
{
	l = m_lptargetleft;
	r = m_lptargetright;
	valid = m_valid;
}

inline void YARPLpClrSegmentation::GetTargetSpeed (CVisDVector& l, CVisDVector& r, bool& valid) const
{
	l = m_delta_left;
	r = m_delta_right;
	valid = m_speed_valid;
}

inline void YARPLpClrSegmentation::SetLumaThreshold (double t)
{
	m_luma_thr = t;
	m_int_luma = int (m_luma_thr * 255.0);
}

inline double YARPLpClrSegmentation::GetLumaThreshold (void) const
{
	return m_luma_thr;
}

inline void YARPLpClrSegmentation::SetSaturationThreshold (double t)
{
	m_saturation_thr = t;
	m_int_saturation = int (m_saturation_thr * 255.0);
}

inline double YARPLpClrSegmentation::GetSaturationThreshold (void) const
{
	return m_saturation_thr;
}


#endif // !defined(AFX_YARPLpClrSegmentation_H__71DD7F89_D0DB_11D4_90F4_00500463430C__INCLUDED_)
