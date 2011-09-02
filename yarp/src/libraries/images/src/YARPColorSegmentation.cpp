//
// YARPColorSegmentation.cpp
//
#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

#include "YARPColorSegmentation.h"

YARPLpClrSegmentation::YARPLpClrSegmentation (int necc, int nang, double rfmin, int size) :
		m_smoothing_delta (0.5),
		m_dtl (),
		m_dtr (),
		m_bin (10.0),
		m_lp (necc, nang, rfmin, size),
		// histo size modified due to change in IPL 2.5.
		m_motionhisto (0.0, 255.0, 0.0, 255.0, 15, 10),
		m_wholehisto (0.0, 255.0, 0.0, 255.0, 15, 10),
		m_dummy (),
		m_gauss (necc, nang)
{
	m_dummy.Resize (15, 10);
	m_dummy.Zero ();

	m_monoleft.Resize (necc, nang);
	m_convertedL.Resize (necc, nang);
	m_segleft.Resize (necc, nang);
	m_monoright.Resize (necc, nang);
	m_convertedR.Resize (necc, nang);
	m_segright.Resize (necc, nang);
	m_tmp.Resize (necc, nang);

	m_sl.Resize (necc, nang);
	m_sr.Resize (necc, nang);

	m_background_l.Resize (necc, nang);
	m_background_r.Resize (necc, nang);

	m_segleft.Zero();
	m_segright.Zero();

	m_reset = true;

	m_motionhisto = 0;
	m_wholehisto = 0;
	m_npixelleft = 0;
	m_npixelright = 0;

	// target (cartesian 0-acqRes range)
	m_targetleft.Resize (2);
	m_targetleft = size / 2;
	m_targetright.Resize (2);
	m_targetright = size / 2;

	m_old_left.Resize (2);
	m_old_left = size / 2;
	m_delta_left.Resize (2);
	m_delta_left = 0;

	// target (log-polar 0-necc, 0,nang range);
	m_lptargetleft.Resize (2);
	m_lptargetleft = 0;

	m_old_right.Resize (2);
	m_old_right = size / 2;
	m_delta_right.Resize (2);
	m_delta_right = 0;

	// target (log-polar 0-necc, 0,nang range);
	m_lptargetright.Resize (2);
	m_lptargetright = 0;

	m_valid = false;
	m_old_valid = false;
	m_speed_valid = false;

	m_locked = false;

	InitDefault ();
}

void YARPLpClrSegmentation::Cleanup (void)
{
	m_dummy.Cleanup ();

	m_sl.Cleanup ();
	m_sr.Cleanup ();

	m_monoleft.Cleanup ();
	m_convertedL.Cleanup ();
	m_segleft.Cleanup ();
	m_monoright.Cleanup ();
	m_convertedR.Cleanup ();
	m_segright.Cleanup ();
	m_tmp.Cleanup ();

	m_background_l.Cleanup ();
	m_background_r.Cleanup ();

	m_dtl.Cleanup ();
	m_dtr.Cleanup ();

	m_smoothl.Cleanup ();
	m_smoothr.Cleanup ();
}


bool YARPLpClrSegmentation::InitialDetection (const YARPImageOf<YarpPixelRGB>& m_left,
											  const YARPImageOf<YarpPixelRGB>& m_right,
										      YARPImageOf<YarpPixelMono>& m_store_left,
											  YARPImageOf<YarpPixelMono>& m_store_right)
{
	bool segmentation_init = false;

	// convert from RGB to HSV. 
	YARPColorConverter::RGB2HSV (m_left, m_convertedL);
	YARPColorConverter::RGB2HSV (m_right, m_convertedR);

	// get 2 mono images.
	YARPColorConverter::RGB2Grayscale (m_left, m_monoleft);
	YARPColorConverter::RGB2Grayscale (m_right, m_monoright);
	
	if (!m_segm_only)
	{
		// just added this to store the actual image.
		m_store_left = m_monoleft;
		YARPSimpleOperation::Scale (m_store_left, m_store_left, 0.9);
		m_store_right = m_monoright;
		YARPSimpleOperation::Scale (m_store_right, m_store_right, 0.9);
	}

	// uses the internal reset parameter.
	// modifies m_monoleft.
	MotionDetection2 (m_reset, true);

	if (m_reset == true)
	{
		// reset of motion detection.
		// Restart motion detection. Filters have been reset in MotionDet.
		m_reset = false;

		m_targetleft = m_lp.GetSize() / 2;
		m_lptargetleft = 0;
		m_targetright = m_lp.GetSize() / 2;
		m_lptargetright = 0;
		m_valid = false;

		if (m_segm_only)
		{
			m_store_left.Zero();
			m_store_right.Zero();
		}
	}
	else
	{
		if (m_segm_only)
		{
			m_store_left = m_monoleft;
			m_store_right = m_monoright;
		}

		// std processing.
		// No-reset -> extract target (motion area COG).
		bool validleft, validright;
		TargetLocation (m_targetleft, m_lptargetleft, validleft, m_npixelleft, m_monoleft);
		TargetLocation (m_targetright, m_lptargetright, validright, m_npixelleft, m_monoright);

		m_motionhisto = 0;
		if (validleft && validright)
		{
			BuildHsHistogram (m_motionhisto, m_convertedL, m_monoleft);
			BuildHsHistogram (m_motionhisto, m_convertedR, m_monoright);
			m_wholehisto /= 1.1;
		}
		else
		{
			BuildHsHistogramNoMask (m_wholehisto, m_convertedL);
			BuildHsHistogramNoMask (m_wholehisto, m_convertedR);
		}

		m_valid = validleft && validright;

		if (!m_valid)
		{
			m_targetleft = m_lp.GetSize() / 2;
			m_targetright = m_lp.GetSize() / 2;
#ifdef VERBOSE
			printf ("Not a valid motion target\n");
#endif
		}
		else
		{
			segmentation_init = true;
		}
	}

	EstimateTargetSpeed ();

	return segmentation_init;
	//return false;
}

bool YARPLpClrSegmentation::StartupSegmentation (const YARPImageOf<YarpPixelRGB>& m_left,
											     const YARPImageOf<YarpPixelRGB>& m_right,
										         YARPImageOf<YarpPixelMono>& m_store_left,
											     YARPImageOf<YarpPixelMono>& m_store_right)
{
	bool segmentation_init = false; // return to non segmenting status.

	// Algorithm.
	// Build histogram in the HS space for the moving pixels.

	// convert from RGB to HSV. 
	YARPColorConverter::RGB2HSV (m_left, m_convertedL);
	YARPColorConverter::RGB2HSV (m_right, m_convertedR);

	if (m_segm_only)
	{
		YARPColorConverter::RGB2Grayscale (m_left, m_monoleft);
		YARPColorConverter::RGB2Grayscale (m_right, m_monoright);
	}
	else
	{
		YARPColorConverter::RGB2Grayscale (m_left, m_segleft);
		YARPColorConverter::RGB2Grayscale (m_right, m_segright);
	}

	// Threshold the histogram and get the max over H and S.
	double h, s;
	ThresholdHistogram (h, s, m_motionhisto, 0);

	// Do a region growing subtracting the whole img histogram.
	const int thr = int (.5 * m_histothreshold * m_motionhisto.GetMaximum () + .5);
	const int thr_whole = int (m_histothreshold * m_wholehisto.GetMaximum () + .5);

	if (thr > 1)
	{
		ConditionedRegionGrowing (h, s, m_motionhisto, thr, m_wholehisto, thr_whole);

		bool validleft, validright;
		SegmentTarget (m_segleft, m_targetleft, m_lptargetleft, validleft, m_motionhisto, m_convertedL);
		SegmentTarget (m_segright, m_targetright, m_lptargetright, validright, m_motionhisto, m_convertedR);
		//SegmentTarget3 (m_segleft, m_targetleft, m_lptargetleft, validleft, m_motionhisto, m_convertedL);		

		m_store_left = m_segleft;
		m_store_right = m_segright;

		bool goodl = EvaluateQuality (m_segleft);
		bool goodr = EvaluateQuality (m_segright);
		if (validright && validleft && goodl && goodr)
		{
			m_valid = true;
			m_reset = true; // ?? Does it need to be reset?
			
			segmentation_init = true;
		}
		else
		{
			m_valid = false;
			m_targetleft = m_lp.GetSize() / 2;
			m_targetright = m_lp.GetSize() / 2;
			m_reset = true;

			segmentation_init = false;
#ifdef VERBOSE
			if (validleft)
				printf ("Target of poor quality (size not correct)\n");
			else
			if (goodl)
				printf ("Target has too few pixels\n");
			else
				printf ("Target has both few pixels and wrong size\n");

			if (validright)
				printf ("Target of poor quality (size not correct)\n");
			else
			if (goodr)
				printf ("Target has too few pixels\n");
			else
				printf ("Target has both few pixels and wrong size\n");
#endif
			if (m_segm_only)
			{
				m_store_left.Zero();
				m_store_right.Zero();
			}
		}
	}
	else
	{
		// not enough pixels for region growing.
		m_valid = false;
		m_targetleft = m_lp.GetSize() / 2;
		m_targetright = m_lp.GetSize() / 2;
		m_reset = true;

		segmentation_init = false;

#ifdef VERBOSE
		printf ("Apparently the histogram is wrong\n");
#endif
		if (m_segm_only)
		{
			m_store_left.Zero();
			m_store_right.Zero();
		}
	}

	EstimateTargetSpeed ();

	return segmentation_init;
}

bool YARPLpClrSegmentation::ColorSegmentation (const YARPImageOf<YarpPixelRGB>& m_left,
											   const YARPImageOf<YarpPixelRGB>& m_right,
									           YARPImageOf<YarpPixelMono>& m_store_left,
											   YARPImageOf<YarpPixelMono>& m_store_right)
{
	bool segmentation_can_continue = false;

	// use stored histograms for segmentation.
	// convert from RGB to HSV. 
	YARPColorConverter::RGB2HSV (m_left, m_convertedL);
	YARPColorConverter::RGB2HSV (m_right, m_convertedR);

	if (m_segm_only)
	{
		YARPColorConverter::RGB2Grayscale (m_left, m_monoleft);
		YARPColorConverter::RGB2Grayscale (m_right, m_monoright);
	}
	else
	{
		YARPColorConverter::RGB2Grayscale (m_left, m_segleft);
		m_monoleft = m_segleft;
		YARPColorConverter::RGB2Grayscale (m_right, m_segright);
		m_monoright = m_segright;
	}

	bool validleft, validright;

	// it was segmentTarget2
	SegmentTarget       (m_segleft, 
						 m_targetleft, 
						 m_lptargetleft, 
						 validleft, 
						 m_motionhisto, 
						 m_convertedL);

	SegmentTarget       (m_segright, 
						 m_targetright, 
						 m_lptargetright, 
						 validright, 
						 m_motionhisto, 
						 m_convertedR);

	m_store_left = m_segleft;
	m_store_right = m_segright;

	m_valid = validleft && validright;
	if (!m_valid)
	{
		m_targetleft = m_lp.GetSize() / 2;
		m_targetright = m_lp.GetSize() / 2;
	}

	// locked means that segmentation is never 
	// reinitiated.
	if (!m_locked)
	{
		// Motion evaluation.
		MotionDetection (m_reset);
		//m_store_left = m_monoleft;
		//m_store_right = m_monoright;

		bool motion = true;
		if (m_reset == true)
			m_reset = false;
		else
			motion = IsMoving (m_monoleft) && IsMoving (m_monoright);

		// good has been added to m_valid.
		bool goodl = EvaluateQuality (m_segleft);
		bool goodr = EvaluateQuality (m_segright);
		m_valid = m_valid && goodl && goodr;

		if (!validleft ||
			!validright ||
			!motion || 
			!goodl ||
			!goodr)
		{
			segmentation_can_continue = false;
#ifdef VERBOSE
			if (!validleft)
				printf ("Taget has too few pixels\n");
			else
			if (!motion)
				printf ("There's no motion!\n");
			else
				printf ("Target of bad size\n");
			if (!validright)
				printf ("Taget has too few pixels\n");
			else
			if (!motion)
				printf ("There's no motion!\n");
			else
				printf ("Target of bad size\n");
#endif
		}
		else
		{
			segmentation_can_continue = true;
		}
	}
	else
		segmentation_can_continue = true;

	EstimateTargetSpeed ();

	return segmentation_can_continue;
}

void YARPLpClrSegmentation::AbortSegmentation (void)
{
	m_targetleft = m_lp.GetSize() / 2;
	m_targetright = m_lp.GetSize() / 2;
	m_valid = false;

	m_delta_left = 0;
	m_speed_valid = false;

	m_reset = true;
}


