//
// YARPTargetLocator.cpp
//

#ifndef __LINUX__

#include "YARPTargetLocator.h"

// some kind of name conflict going on
#define ave aver
#ifdef std
#undef std
#endif
#define std stdr

YARPColorLocator::YARPColorLocator ()
{
	// luma and saturation are thresholded.
	m_luma_thr = 0;
	m_sat_thr = 0;

	// thr must be between 0 and 1.
	m_luma_thr_int = 0;
	m_sat_thr_int = 0;

	m_norm_factor_backp = 0;

	m_numberofframes = 0;
	m_extx = m_exty = 0;

	m_numberofpoints = 0;
	m_numberofpoints_std = 0;

	m_nbinssat = 0;
	m_nbinshue = 0;

	m_expected_thr = 0;
	m_expected_thr_std = 0;

	m_acc_thr = 0;
	m_acc_std = 0;
	m_count = 0;
}

YARPColorLocator::YARPColorLocator (int size, int nbinssat, int nbinshue, double luma_thr, double sat_thr) 
	: m_histo (0.0, 255.0, 0.0, 255.0, nbinssat, nbinshue)
{
	// luma and saturation are thresholded.
	m_luma_thr = luma_thr;
	m_sat_thr = sat_thr;

	// thr must be between 0 and 1.
	m_luma_thr_int = int(luma_thr * 255);
	m_sat_thr_int = int(sat_thr * 255);

	m_hsv.Resize (size, size);
	m_hsv.Zero ();

	m_backprojected.Resize (size, size);
	m_backprojected.Zero();

	m_integral.Resize (size, size);
	m_integral.Zero ();

	m_norm_factor_backp = 0;

	m_numberofframes = 0;
	m_extx = m_exty = 0;

	m_numberofpoints = 0;
	m_numberofpoints_std = 0;

	m_nbinssat = nbinssat;
	m_nbinshue = nbinshue;

	m_expected_thr = 0;
	m_expected_thr_std = 0;

	m_acc_thr = 0;
	m_acc_std = 0;
	m_count = 0;
}

YARPColorLocator::~YARPColorLocator ()
{
}

void YARPColorLocator::Resize (int size, int nbinssat, int nbinshue, double luma_thr, double sat_thr) 
{
	m_histo.Resize (0.0, 255.0, 0.0, 255.0, nbinssat, nbinshue);
	m_nbinssat = nbinssat;
	m_nbinshue = nbinshue;

	// luma and saturation are thresholded.
	m_luma_thr = luma_thr;
	m_sat_thr = sat_thr;

	// thr must be between 0 and 1.
	m_luma_thr_int = int(luma_thr * 255);
	m_sat_thr_int = int(sat_thr * 255);

	m_hsv.Resize (size, size);
	m_hsv.Zero ();

	m_backprojected.Resize (size, size);
	m_backprojected.Zero();

	m_integral.Resize (size, size);
	m_integral.Zero ();

	m_norm_factor_backp = 0;

	m_numberofframes = 0;
	m_extx = m_exty = 0;

	m_numberofpoints = 0;
	m_numberofpoints_std = 0;

	m_expected_thr = 0;
	m_expected_thr_std = 0;

	m_acc_thr = 0;
	m_acc_std = 0;
	m_count = 0;
}

void YARPColorLocator::Reset (void)
{
	m_histo = 0;
	m_numberofframes = 0;
	m_extx = m_exty = 0;
	m_numberofpoints = 0;
	m_numberofpoints_std = 0;
	m_expected_thr = 0;
	m_expected_thr_std = 0;
	m_acc_thr = 0;
	m_acc_std = 0;
	m_count = 0;

	m_hsv.Zero ();
	m_backprojected.Zero ();
	m_integral.Zero ();
}


void YARPColorLocator::CumulateHistograms (const YARPImageOf<YarpPixelHSV>& src, const YARPImageOf<YarpPixelMono>& mask, bool reset)
{
	if (reset)
	{
		Reset ();
	}

	unsigned char *hue2, *sat2, *val2;
	unsigned char *m = (unsigned char *)mask.GetRawBuffer();

	double hue, saturation;
	const int h = m_hsv.GetHeight();
	const int w = m_hsv.GetWidth();

	int minx = w, maxx = 0;
	int miny = h, maxy = 0;

	int counter = 0;
	int dropped = 0;

	for (int i = 0; i < h; i++)
	{
		hue2 = (unsigned char *)src.GetArray ()[i];
		sat2 = hue2 + 1;
		val2 = sat2 + 1;

		for (int j = 0; j < w; j++)
		{
			if (*m != 0)
			{
				if (j < minx) minx = j;
				else
				if (j > maxx) maxx = j;

				if (i < miny) miny = i;
				else
				if (i > maxy) maxy = i;
	
				counter ++;
			}

			if (*val2 > m_luma_thr_int && *sat2 > m_sat_thr_int && *m != 0)
			{
				hue = double (*hue2);
				saturation = double (*sat2);

				m_histo (hue, saturation) ++;
				if (m_histo (hue, saturation) > m_histo.GetMaximum ())
					m_histo.GetMaximum () = m_histo (hue, saturation);
			}
			else
				dropped ++;
				//m_histo.GetDropped()++;

			hue2 += 3;
			sat2 += 3;
			val2 += 3;
			m++;
		}
	}

	m_extx += (maxx - minx);
	m_exty += (maxy - miny);
	m_numberofpoints += counter;
	m_numberofpoints_std += (counter*counter);

	m_histo.GetDropped() += double(dropped);
	m_histo.GetDroppedStd() += (double(dropped)*double(dropped));
	
	m_numberofframes++;
}

int YARPColorLocator::GetExtent (double& exx, double& exy)
{
	if (m_numberofframes <= 0)
	{
		printf ("GetExtent: no data to return\n");
		exx = exy = 0;
		return -1;
	}

	exx = double(m_extx) / m_numberofframes;
	exy = double(m_exty) / m_numberofframes;

	return 0;
}

int YARPColorLocator::GetNumberOfPoints (double& ave, double& std)
{
	if (m_numberofframes <= 0)
	{
		printf ("GetNumberOfPoints: no data to return\n");
		ave = std = 0;
		return -1;
	}

	ave = double(m_numberofpoints) / m_numberofframes;
	std = sqrt(double(m_numberofpoints_std) / (m_numberofframes-1) - (double(m_numberofframes)/double(m_numberofframes-1) * ave * ave));

	return 0;
}

void YARPColorLocator::GenerateSum(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest)
{
  int i, j;
  float total, v;
  int h = src.GetHeight();
  int w = src.GetWidth();
 
  for (i=0; i<h; i++)
    {
      total = 0;
      for (j=0; j<w; j++)
		{
			v = src(j,i);
			total += v;
			dest(j,i) = total;
		}
    }

  for (j=0; j<w; j++)
    {
      total = 0;
      for (i=0; i<h; i++)
		{
		  total += dest(j,i);
		  dest(j,i) = total;
		}
    }
}


double YARPColorLocator::CompareHisto (CStaticLut<int>& x1, CStaticLut<int>& x2)
{
	int *p1 = x1.GetArrayPtr()[0];
	int *p2 = x2.GetArrayPtr()[0];
	const double m1 = x1.GetMaximum();
	const double m2 = x2.GetMaximum();

	//assert (m1 != 0 && m2 != 0);

	if (x1.GetMaximum() <= 0)
	{
		// big enough?
		return 1e50;
	}

	const int size = x1.GetWidth() * x1.GetHeight();
	double score = 0;
	
#if 0
	// LATER: to be optimized, don't need to be computed here.!!!!
	double mm1 = 0, mm2 = 0;
	for (int j = 0; j < size; j++)
	{
		mm1 += *p1++;
		mm2 += *p2++;
	}

	p1 = x1.GetArrayPtr()[0];
	p2 = x2.GetArrayPtr()[0];
#endif

	double mm1 = 1;
	double mm2 = 1; //m_numberofframes;

	for (int i = 0; i < size; i++)
	{
		const double p1n = *p1 / mm1;
		const double p2n = *p2 / mm2;
		score += ((p1n - p2n) * (p1n - p2n));	
		p1++;
		p2++;
	}

	// LATER: it would be nice to count also the number of pix discarded when building the histo.
	const double n = m_numberofframes;
	double ave = x2.GetDropped() / n;
	double arg = x2.GetDroppedStd()/(n-1) - (n/(n-1) * ave * ave);
	double std = sqrt (arg);

	if (fabs(double(x1.GetDropped()) - ave) > std * 100)
	{
		// 
		printf ("CompareHisto: dropped %f %f %f\n", x1.GetDropped(), ave, std);
		return 1e50;
	}

	return score / size;
}


int YARPColorLocator::ImprovedSearch (YARPImageOf<YarpPixelBGR>& img, double exx, double exy, double x, double y, double& xx, double& yy)
{
	YARPColorConverter::RGB2HSV (img, m_hsv);
	const int RANGE = 20;

	CStaticLut<int> comp (0.0, 255.0, 0.0, 255.0, m_nbinssat, m_nbinshue);

	const int w = img.GetWidth();
	const int h = img.GetHeight();

	const int ex = int(exx+.5);
	const int ey = int(exy+.5);

	int x1 = x - ex/2 - RANGE;
	if (x1 < 0) x1 = 0;
	int y1 = y - ey/2 - RANGE;
	if (y1 < 0) y1 = 0;
	int x2 = x - ex/2 + RANGE;
	if (x2+ex >= w) x2 = w-1-ex;
	int y2 = y - ey/2 + RANGE;
	if (y2+ey >= h) y2 = h-1-ey;

	unsigned char hue2, sat2, val2;
	double hue, saturation;

	double score = 0;
	double best_score = 1e50;

	printf ("searching in %d %d, to %d %d with size %d %d\n", x1, y1, x2, y2, ex, ey);
	for (int i = y1; i <= y2; i++)
	{
		// build the initial histo for the current row.
		comp = 0;

		for (int ii = i; ii <= i+ey; ii++)
		{
			for (int jj = x1; jj <= x1+ex; jj++)
			{
				hue2 = m_hsv (jj, ii).h;
				sat2 = m_hsv (jj, ii).s;
				val2 = m_hsv (jj, ii).v;

				if (val2 > m_luma_thr_int && sat2 > m_sat_thr_int)
				{
					hue = double (hue2);
					saturation = double (sat2);

					comp (hue, saturation) ++;
					if (comp (hue, saturation) > comp.GetMaximum ())
						comp.GetMaximum () = comp (hue, saturation);
				}
				else
					comp.GetDropped()+=1;
			}
		}

		//printf ("%d %d\n", i, x1);
		score = CompareHisto (comp, m_histo);
		if (score < best_score)
		{
			best_score = score;
			xx = x1;
			yy = i;
		}

		for (int j = x1+1; j <= x2; j++)
		{
			// for each possible initial point build the histogram.
			// subtract the previous column and add the new one on the right.
			for (int ii = i; ii <= i+ey; ii++)
			{
				hue2 = m_hsv (j-1, ii).h;
				sat2 = m_hsv (j-1, ii).s;
				val2 = m_hsv (j-1, ii).v;

				if (val2 > m_luma_thr_int && sat2 > m_sat_thr_int)
				{
					hue = double (hue2);
					saturation = double (sat2);

					comp (hue, saturation) --;
				}
				else
					comp.GetDropped()-=1;
			}

			// research max...
			const int size = comp.GetHeight() * comp.GetWidth();
			int *ptr = comp.GetArrayPtr()[0];
			int max = 0;
			for (int kk = 0; kk < size; kk++)
			{
				if (*ptr > max) max = *ptr;
				ptr++;
			}
			comp.GetMaximum() = max;

			for (int ii = i; ii <= i+ey; ii++)
			{
				hue2 = m_hsv (j+ex, ii).h;
				sat2 = m_hsv (j+ex, ii).s;
				val2 = m_hsv (j+ex, ii).v;

				if (val2 > m_luma_thr_int && sat2 > m_sat_thr_int)
				{
					hue = double (hue2);
					saturation = double (sat2);

					comp (hue, saturation) ++;
					if (comp (hue, saturation) > comp.GetMaximum ())
						comp.GetMaximum () = comp (hue, saturation);
				}
				else
					comp.GetDropped()+=1;
			}

			//printf ("%d %d\n", i, j);
			score = CompareHisto (comp, m_histo);
			if (score < best_score)
			{
				best_score = score;
				xx = j;
				yy = i;
			}
		}
	}

	xx += exx/2;
	yy += exy/2;

	return 0;
}

int YARPColorLocator::Find (double exx, double exy, double& x, double& y, double& quality)
{
	// look for the area with the max backp per template size.

	// compute integral image (backp is between 0, 255).
	m_integral.Zero ();
	GenerateSum (m_backprojected, m_integral);

	const int ix = int(exx+.5);
	const int iy = int(exy+.5);
	const int w = m_hsv.GetWidth();
	const int h = m_hsv.GetHeight();

	float maxsum = -1;
	int besti = 0;
	int bestj = 0;

	for (int i = 0; i < h-iy; i++)
	{
		for (int j = 0; j < w-ix; j++)
		{
			float sum = m_integral (j+ix-1, i+iy-1) + m_integral(j,i) - m_integral(j+ix-1,i) - m_integral(j,i+iy-1);
			if (sum > maxsum)
			{
				maxsum = sum;
				besti = i;
				bestj = j;
			}
		}
	}

	if (maxsum == -1)
	{
		printf ("Find: no max found\n");
		return -1;
	}

	y = besti + double(exy)/2;
	x = bestj + double(exx)/2;
	quality = maxsum;

	return 0;
}


void YARPColorLocator::Apply (const YARPImageOf<YarpPixelBGR>& img, const YARPImageOf<YarpPixelMono>& mask, bool reset)
{
	YARPColorConverter::RGB2HSV (img, m_hsv);
	CumulateHistograms(m_hsv, mask, reset);
}

// simple backprojection.
void YARPColorLocator::BackProject (const YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest)
{
	unsigned char *s = (unsigned char *)m_hsv.GetRawBuffer ();
	float *b =  (float *)m_backprojected.GetRawBuffer ();

	const int w = m_backprojected.GetWidth();
	const int h = m_backprojected.GetHeight();

	YARPColorConverter::RGB2HSV (src, m_hsv);
	m_norm_factor_backp = 0;

	int max = m_histo.GetMaximum();

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			double hue = double(*s);
			double sat = double(*(s+1));

			*b = float(m_histo (hue, sat)) / float(max) * 255.0;
			m_norm_factor_backp += double(*b);

			b++;
			s += 3;
		}
	}

	dest.CastCopy (m_backprojected);
}

// must be sure you call this with an image of the right object class (roughly).
void YARPColorLocator::AddExpectancy (YARPImageOf<YarpPixelBGR>& img)
{
	double x, y, quality;

	YARPImageOf<YarpPixelBGR> out;
	out.Resize (m_backprojected.GetWidth(), m_backprojected.GetHeight());
	BackProject (img, out);

	double ex, ey;
	GetExtent (ex, ey);
	
	Find (ex, ey, x, y, quality);

	m_count++;
	m_acc_thr += quality;
	m_acc_std += (quality*quality);
}

void YARPColorLocator::GetExpectancy (double& mean, double& stddev)
{
	m_expected_thr = mean = m_acc_thr / m_count;
	m_expected_thr_std = stddev = sqrt (m_acc_std / (m_count-1) - (m_count)/(m_count-1) * mean * mean);
}

#endif
