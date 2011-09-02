//
// YARPDisparity.cpp
//

#include "YARPDisparity.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YARPLpDisparity::YARPLpDisparity (int necc, int nang, double rfmin, int size) 
			 : YARPLpShifter (necc, nang, rfmin, size),
			   m_all_disp(2 * lut_size - 1)
{
	m_disparity_array = new double[m_all_disp];
	assert (m_disparity_array != NULL);
	memset (m_disparity_array, 0, sizeof(double) * m_all_disp);

	m_histogram_values.Resize(m_all_disp, 20);
	m_histogram_values.Zero();

	m_corrMap.Resize (necc, nang);
	m_corrMap.Zero();
	m_cumulativeMap.Resize (necc, nang);
	m_cumulativeMap.Zero();

	m_tmp.Resize (necc, nang);
	m_tmp.Zero();
}

double YARPLpDisparity::ShiftAndCorrelate (const YARPImageOf<YarpPixelMono>& l, const YARPImageOf<YarpPixelMono>& r, int shift)
{
	int i, j, i1, j1;
	int count = 0;
	float d1 = 0;
	int jt;

	unsigned char **src1p0 = (unsigned char **)l.GetArray();
	unsigned char **src2p0 = (unsigned char **)r.GetArray();

	int thetaint;
	if (shift >= 0)
		thetaint = 0;
	else
	{
		shift = -shift;
		thetaint = nAng / 2;
	}

	// use normalized cross correlation.
	double average_1 = 0;
	double average_2 = 0;
	double d_1 = 0;
	double d_2 = 0;

	// blind implementation.
	// there should be a smarter formula.
	for(i = 0; i < nEcc; i++)
		for(j = 0; j < nAng; j++)
		{
			jt = (j + thetaint) % nAng;
			i1 = lut[shift][i][jt].ecc;
			j1 = lut[shift][i][jt].ang;
			j1 = (j1 + nAng - thetaint) % nAng;

			if (i1 >= 0)
			{
				average_1 += src1p0[j][i];
				average_2 += src2p0[j1][i1];
				count++;
			}
		}

	average_1 /= count;
	average_2 /= count;
	double num = 0;
	double den_1 = 0;
	double den_2 = 0;

	for(i = 0; i < nEcc; i++)
		for(j = 0; j < nAng; j++)
		{
			jt = (j + thetaint) % nAng;
			i1 = lut[shift][i][jt].ecc;
			j1 = lut[shift][i][jt].ang;
			j1 = (j1 + nAng - thetaint) % nAng;

			if (i1 >= 0)
			{
				d_1 = src1p0[j][i] - average_1;
				d_2 = src2p0[j1][i1] - average_2;
				num += (d_1 * d_2);
				den_1 += (d_1 * d_1);
				den_2 += (d_2 * d_2);
			}
		}
	return (1.0 - (num * num) / (den_1 * den_2 + 0.00001));
}

int YARPLpDisparity::ComputeDisparity(const YARPImageOf<YarpPixelMono>& l, const YARPImageOf<YarpPixelMono>& r)
{
	const int limit = 3 * nEcc / 2 - 1;
	for (int i = -limit; i <= limit; i++)
	{
		m_disparity_array[i + limit] = ShiftAndCorrelate(l, r, i);
	}

	// update histo img;
	unsigned char **tmp = (unsigned char **)m_histogram_values.GetArray();
	
	int max_disp = 0;
	double max_value = m_disparity_array[0];

	for (int i = 0; i < m_all_disp; i++)
	{
		if (m_disparity_array[i] <= max_value)
		{
			max_disp = i;
			max_value = m_disparity_array[i];
		}

		for (int j = 0; j < 20; j++)
		{
			double value = m_disparity_array[i];
			assert (value > 0);

			//value *= 100;
			value *= 20;
			value = (value > 20) ? 20 : value;

			if (j <= value)
				tmp[j][i] = (unsigned char)255;
			else
				tmp[j][i] = (unsigned char)0;
		}
	}

	for (int j = 0; j < 20; j+=2)
	{
		double value = m_disparity_array[max_disp];
		assert (value > 0);

		//value *= 100;
		value *= 20;
		value = (value > 20) ? 20 : value;

		if (j <= value)
			tmp[j][max_disp] = (unsigned char)0;
		else
			tmp[j][max_disp] = (unsigned char)255;
	}
	
	return max_disp - limit;
}


void YARPLpDisparity::GetHistogramAsImage(YARPImageOf<YarpPixelMono>& histo)
{
	histo = m_histogram_values;
}


YARPImageOf<YarpPixelMono>& YARPLpDisparity::GetHistogramAsImage()
{
	return m_histogram_values;
}

double YARPLpDisparity::ShiftAndCorrelate (const YARPImageOf<YarpPixelBGR>& l, const YARPImageOf<YarpPixelBGR>& r, int shift)
{
	return ShiftAndCorrelate ((const YARPImageOf<YarpPixelRGB>&) l, (const YARPImageOf<YarpPixelRGB>&) r, shift);
}

double YARPLpDisparity::ShiftAndCorrelate (const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r, int shift)
{
	int i, j, i1, j1;
	int count = 0;
	float d1 = 0;
	int jt;

	unsigned char **src1p0 = (unsigned char **)l.GetArray();
	unsigned char **src2p0 = (unsigned char **)r.GetArray();

	int thetaint;
	if (shift >= 0)
		thetaint = 0;
	else
	{
		shift = -shift;
		thetaint = nAng / 2;
	}

	// use normalized cross correlation.
	double average_1r = 0;
	double average_1g = 0;
	double average_1b = 0;
	double average_2r = 0;
	double average_2g = 0;
	double average_2b = 0;
	
	double d_1 = 0;
	double d_2 = 0;

	// blind implementation.
	// there should be a smarter formula.
	for(i = 0; i < nEcc; i++)
		for(j = 0; j < nAng; j++)
		{
			jt = (j + thetaint) % nAng;
			i1 = lut[shift][i][jt].ecc;
			j1 = lut[shift][i][jt].ang;
			j1 = (j1 + nAng - thetaint) % nAng;

			if (i1 >= 0)
			{
				average_1r += src1p0[j][i*3];
				average_1g += src1p0[j][i*3+1];
				average_1b += src1p0[j][i*3+2];
				average_2r += src2p0[j1][i1*3];
				average_2g += src2p0[j1][i1*3+1];
				average_2b += src2p0[j1][i1*3+2];
				count++;
			}
		}

	average_1r /= count;
	average_1g /= count;
	average_1b /= count;
	average_2r /= count;
	average_2g /= count;
	average_2b /= count;

	double num = 0;
	double den_1 = 0;
	double den_2 = 0;

	for(i = 0; i < nEcc; i++)
		for(j = 0; j < nAng; j++)
		{
			jt = (j + thetaint) % nAng;
			i1 = lut[shift][i][jt].ecc;
			j1 = lut[shift][i][jt].ang;
			j1 = (j1 + nAng - thetaint) % nAng;

			if (i1 >= 0)
			{
				d_1 = src1p0[j][i*3] - average_1r;
				d_2 = src2p0[j1][i1*3] - average_2r;
				num += (d_1 * d_2);
				den_1 += (d_1 * d_1);
				den_2 += (d_2 * d_2);

				d_1 = src1p0[j][i*3+1] - average_1g;
				d_2 = src2p0[j1][i1*3+1] - average_2g;
				num += (d_1 * d_2);
				den_1 += (d_1 * d_1);
				den_2 += (d_2 * d_2);

				d_1 = src1p0[j][i*3+2] - average_1b;
				d_2 = src2p0[j1][i1*3+2] - average_2b;
				num += (d_1 * d_2);
				den_1 += (d_1 * d_1);
				den_2 += (d_2 * d_2);
			}
		}

	return (1.0 - (num * num) / (den_1 * den_2 + 0.00001));
}

int YARPLpDisparity::ComputeDisparity(const YARPImageOf<YarpPixelBGR>& l, const YARPImageOf<YarpPixelBGR>& r)
{
	return ComputeDisparity ((const YARPImageOf<YarpPixelRGB>&)l, (const YARPImageOf<YarpPixelRGB>&)r);
}

int YARPLpDisparity::ComputeDisparity(const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r)
{
	const int limit = 3 * nEcc / 2 - 1;
	for (int i = -limit; i <= limit; i++)
	{
		m_disparity_array[i + limit] = ShiftAndCorrelate(l, r, i);
	}

	// update histo img;
	unsigned char **tmp = (unsigned char **)m_histogram_values.GetArray();
	
	int max_disp = 0;
	double max_value = m_disparity_array[0];

	for (int i = 0; i < m_all_disp; i++)
	{
		if (m_disparity_array[i] <= max_value)
		{
			max_disp = i;
			max_value = m_disparity_array[i];
		}

		for (int j = 0; j < 20; j++)
		{
			double value = m_disparity_array[i];
			assert (value > 0);

			value *= 20;
			value = (value > 20) ? 20 : value;

			if (j <= value)
				tmp[j][i] = (unsigned char)255;
			else
				tmp[j][i] = (unsigned char)0;
		}
	}

	for (int j = 0; j < 20; j+=2)
	{
		double value = m_disparity_array[max_disp];
		assert (value > 0);

		value *= 20;
		value = (value > 20) ? 20 : value;

		if (j <= value)
			tmp[j][max_disp] = (unsigned char)0;
		else
			tmp[j][max_disp] = (unsigned char)255;
	}
	
	return max_disp - limit;
}

//
// computes also the global correlation (not used!)
//
double YARPLpDisparity::CorrelationMap(const YARPImageOf<YarpPixelMono>& im1, const YARPImageOf<YarpPixelMono>& im2)
{
	const int border = 2;

	int i,j,pos_i;
	int xi = 0, eta = 0;

	double correl;
	float Cmedia1 = .0f, Cmedia2 = .0f;

	float Cnumeratore = .0f, Cdenom1 = .0f, Cdenom2 = .0f;
	float numeratore = .0f, denom1 = .0f, denom2 = .0f;

	float d1 = .0f, d2 = .0f;

	int count=0;
	int Ccount=0;

	unsigned char **src1p = (unsigned char **)im1.GetArray();
	unsigned char **src2p = (unsigned char **)im2.GetArray();

	for(eta=0; eta<nAng; eta++)
		for(xi=0; xi<nEcc-border; xi++)
		{
			Cmedia1+=src1p[eta][xi];
			Cmedia2+=src2p[eta][xi];
			Ccount++;
		}

	Cmedia1 /= float(Ccount);
	Cmedia2 /= float(Ccount);

	float media1, media2;

	for(eta=0; eta<nAng; eta++)
		for(xi=border; xi<nEcc-border ; xi++)
		{
			media1 = .0; media2 = .0;
			for(j=-border;j<=border;j++)
				for(i=-border;i<=border;i++)
				{
					if (eta + i >= nAng)
						pos_i = eta + i - nAng ;
					else
					if (eta + i <0)
						pos_i = nAng + eta + i;
					else
						pos_i = eta + i;

					media1 += src1p[pos_i][xi+j];
					media2 += src2p[pos_i][xi+j];
				}

			media1 /= _sqr(2*border+1);
			media2 /= _sqr(2*border+1);

			numeratore = .0f;
			denom1 = .0f; 
			denom2 = .0f;

			for(j=-border;j<=border;j++)
				for(i=-border;i<=border;i++)
				{
					if (eta + i >= nAng)
						pos_i = eta +i - nAng;
					else
					if (eta + i <0)
						pos_i = nAng + eta + i;
					else
						pos_i = eta + i;

					d1=src1p[pos_i][xi+j]-media1;
					d2=src2p[pos_i][xi+j]-media2;
					numeratore += (d1*d2);
					denom1 += (d1*d1);
					denom2 += (d2*d2);

					if (i==0 && j==0)
					{
						d1=src1p[pos_i][xi+j]-Cmedia1;
						d2=src2p[pos_i][xi+j]-Cmedia2;
						Cnumeratore += (d1*d2);
						Cdenom1 += (d1*d1);
						Cdenom2 += (d2*d2);
					}
				}

			//
			// Correlation could be 1 for uniform areas.
			//
			correl = 1.0-(double(numeratore)/(denom1+0.00001))*(double(numeratore)/(denom2+0.00001));

			if (correl < 0.0) 
				correl = 0;
			else
			if (correl > 1.0)
				correl = 1;

		    m_corrMap(xi,eta) = (unsigned char)(correl*255.0);
			count++;
		}

	return 1.0-(double(Cnumeratore)/(Cdenom1+0.00001))*(double(Cnumeratore)/(Cdenom2+0.00001));
}

double YARPLpDisparity::CorrelationMap(const YARPImageOf<YarpPixelBGR>& im1, const YARPImageOf<YarpPixelBGR>& im2)
{
	return CorrelationMap ((const YARPImageOf<YarpPixelRGB>&)im1, (const YARPImageOf<YarpPixelRGB>&)im2);
}

//
// computes also the global correlation (not used!)
//
double YARPLpDisparity::CorrelationMap(const YARPImageOf<YarpPixelRGB>& im1, const YARPImageOf<YarpPixelRGB>& im2)
{
	const int border = 2;

	int i,j,pos_i;
	int xi = 0, eta = 0;

	double correl;
	float Cmedia1r = .0f, Cmedia2r = .0f;
	float Cmedia1g = .0f, Cmedia2g = .0f;
	float Cmedia1b = .0f, Cmedia2b = .0f;

	float Cnumeratore = .0f, Cdenom1 = .0f, Cdenom2 = .0f;
	float numeratore = .0f, denom1 = .0f, denom2 = .0f;

	float d1 = .0f, d2 = .0f;

	int count=0;
	int Ccount=0;

	unsigned char **src1 = (unsigned char **)im1.GetArray();
	unsigned char **src2 = (unsigned char **)im2.GetArray();

	// global average.
	for(eta=0; eta<nAng; eta++)
		for(xi=0; xi<nEcc-border; xi++)
		{
			Cmedia1r += src1[eta][xi*3];
			Cmedia2r += src2[eta][xi*3];
			Cmedia1g += src1[eta][xi*3+1];
			Cmedia2g += src2[eta][xi*3+1];
			Cmedia1b += src1[eta][xi*3+2];
			Cmedia2b += src2[eta][xi*3+2];
			Ccount++;
		}

	Cmedia1r /= float(Ccount);
	Cmedia2r /= float(Ccount);
	Cmedia1g /= float(Ccount);
	Cmedia2g /= float(Ccount);
	Cmedia1b /= float(Ccount);
	Cmedia2b /= float(Ccount);

	float media1r, media2r;
	float media1g, media2g;
	float media1b, media2b;

	// 5 x 5 average and cross-correlation.
	for(eta=0; eta<nAng; eta++)
		for(xi=border; xi<nEcc-border ; xi++)
		{
			media1r = .0; media2r = .0;
			media1g = .0; media2g = .0;
			media1b = .0; media2b = .0;
			for(j=-border;j<=border;j++)
				for(i=-border;i<=border;i++)
				{
					if (eta + i >= nAng)
						pos_i = eta + i - nAng ;
					else
					if (eta + i <0)
						pos_i = nAng + eta + i;
					else
						pos_i = eta + i;

					media1r += src1[pos_i][(xi+j)*3];
					media2r += src2[pos_i][(xi+j)*3];
					media1g += src1[pos_i][(xi+j)*3+1];
					media2g += src2[pos_i][(xi+j)*3+1];
					media1b += src1[pos_i][(xi+j)*3+2];
					media2b += src2[pos_i][(xi+j)*3+2];
				}

			media1r /= _sqr(2*border+1);
			media2r /= _sqr(2*border+1);
			media1g /= _sqr(2*border+1);
			media2g /= _sqr(2*border+1);
			media1b /= _sqr(2*border+1);
			media2b /= _sqr(2*border+1);

			numeratore = .0f;
			denom1 = .0f; 
			denom2 = .0f;

			for(j=-border;j<=border;j++)
				for(i=-border;i<=border;i++)
				{
					if (eta + i >= nAng)
						pos_i = eta +i - nAng;
					else
					if (eta + i <0)
						pos_i = nAng + eta + i;
					else
						pos_i = eta + i;

					d1 = src1[pos_i][(xi+j)*3] - media1r;
					d2 = src2[pos_i][(xi+j)*3] - media2r;
					d1 += src1[pos_i][(xi+j)*3+1] - media1g;
					d2 += src2[pos_i][(xi+j)*3+1] - media2g;
					d1 += src1[pos_i][(xi+j)*3+2] - media1b;
					d2 += src2[pos_i][(xi+j)*3+2] - media2b;
					numeratore += (d1*d2);
					denom1 += (d1*d1);
					denom2 += (d2*d2);

					if (i==0 && j==0)
					{
						d1 = src1[pos_i][(xi+j)*3] - Cmedia1r;
						d2 = src2[pos_i][(xi+j)*3] - Cmedia2r;
						d1 += src1[pos_i][(xi+j)*3+1] - Cmedia1g;
						d2 += src2[pos_i][(xi+j)*3+1] - Cmedia2g;
						d1 += src1[pos_i][(xi+j)*3+2] - Cmedia1b;
						d2 += src2[pos_i][(xi+j)*3+2] - Cmedia2b;
						Cnumeratore += (d1*d2);
						Cdenom1 += (d1*d1);
						Cdenom2 += (d2*d2);
					}
				}

			//
			// Correlation could be 1 for uniform areas.
			//
			correl = 1.0-(double(numeratore)/(denom1+0.00001))*(double(numeratore)/(denom2+0.00001));

			if (correl < 0.0) 
				correl = 0;
			else
			if (correl > 1.0)
				correl = 1;

			if (correl < 80.0 / 255.0)
				//m_corrMap(xi,eta) = (unsigned char)(correl*255.0);
				m_corrMap(xi, eta) = 255;
			else
				m_corrMap(xi, eta) = 0;

			count++;
		}

	return 1.0-(double(Cnumeratore)/(Cdenom1+0.00001))*(double(Cnumeratore)/(Cdenom2+0.00001));
}

//
// this is a test of a N-disparity filter.
double YARPLpDisparity::CorrelationMap (const YARPImageOf<YarpPixelRGB>& l, const YARPImageOf<YarpPixelRGB>& r, int disp, int bias)
{
	double shift[2];
	shift[0] = 0;
	shift[1] = 0;

	const int size = nEcc * nAng;

	m_cumulativeMap.Zero();

	const int limit = 3 * nEcc / 2 - 1;
	if (bias < -limit + disp)
		bias = -limit + disp;
	else
	if (bias > limit - disp)
		bias = limit - disp;

	for (int d = -disp + bias; d <= disp + bias; d++)
	{
		shift[0] = d;
		if (d >= 0)
			shift[1] = 0;
		else
		{
			shift[0] = -shift[0];
			shift[1] = nAng / 2;
		}

		if (d != 0)
		{
			GetShiftedImage (r, m_tmp, shift);
			CorrelationMap (l, m_tmp);
		}
		else
			CorrelationMap (l, r);

		// result is in m_corrMap
		unsigned char *cumul = (unsigned char *)m_cumulativeMap.GetAllocatedArray();
		unsigned char *map = (unsigned char *)m_corrMap.GetAllocatedArray();

		for (int i = 0; i < size; i++)
		{
			if (*map++ == 255)
				*cumul++ = 255;
			else
				cumul++;
		}
	}

	m_corrMap = m_cumulativeMap;
	return 0;
}
