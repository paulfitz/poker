//
// YARPLpShifter.cpp
//

#include "YARPLpShifter.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YARPLpShifter::YARPLpShifter(int necc, int nang, double rfmin, int acqres) 
			 : YARPLogPolar (necc, nang, rfmin, acqres),
			   lut_size(3 * necc / 2)
{
	double x, y, nx, id1, jd1;
	int d, i, j, i1, j1;

	// LUT mem alloc.
	lut = new cv1**[lut_size];	// all possible disparities.
	assert(lut != NULL);

	for (i = 0; i < lut_size; i++)
	{
		lut[i] = new cv1*[nEcc];	// all possible eccentricities.
		assert(lut[i] != NULL);
	}

	for (i = 0; i < lut_size; i++)
	{
		for (j = 0; j < nEcc; j++)
		{
			lut[i][j]= new cv1[nAng];	// all possible angles.
			assert(lut[i][j] != NULL);
		}
	}
	// it should be lut[4*nEcc][nEcc][nAng];
	
	for (d = 0; d < lut_size; d++)
	{
		for (i = 0; i < nEcc; i++)
		{
			for (j = 0; j < nAng; j++)
			{
				Lp2Cart((double)i, (double)j, x, y);
				nx = x + d; 
				if (nx > 0 && 
					nx < Size && 
					sqrt((nx - Size / 2) * (nx - Size / 2) +
					(y - Size / 2) * (y - Size / 2)) <= Size / 2)
				{
					Cart2Lp(nx, y, id1, jd1);
					i1 = (int)id1; 
					j1 = (int)jd1;
				} 
				else
				{ 
					i1 = -1; 
					j1 = -1;	
				}

				lut[d][i][j].ecc = i1;
				lut[d][i][j].ang = j1;
			}
		}
	}
}

YARPLpShifter::~YARPLpShifter()
{
	if (lut!=NULL) 
	{
		for (int i = 0; i < lut_size; i++)
			for (int j = 0; j < nEcc; j++)
				delete[] lut[i][j];

		for (int i = 0; i < lut_size; i++)
			delete[] lut[i];

		delete[] lut;
	}
}

void YARPLpShifter::GetShiftedImage(const YARPImageOf<YarpPixelRGB>& im1, YARPImageOf<YarpPixelRGB>& dst, double * shift)
{
	unsigned char **src1p0 = (unsigned char **)im1.GetArray();
	unsigned char **src2p0 = (unsigned char **)dst.GetArray();

	// shift (xi, eta);
	int xi = int (shift[0] + .5);
	int eta = int (shift[1] + .5);
	assert (xi >= 0 && xi < lut_size && eta >= 0 && eta < nAng);

	int i,j,i1,j1,jt;
	for(i=0; i<nEcc; i++)
		for(j=0; j<nAng; j++)
		{
			jt = (j + eta) % nAng;
		   	i1 = lut[xi][i][jt].ecc;
			j1 = lut[xi][i][jt].ang;
			j1=(j1 + nAng - eta) % nAng;
			if (i1>=0) 
			{
				src2p0[j][i*3] = src1p0[j1][i1*3];
				src2p0[j][i*3+1] = src1p0[j1][i1*3+1];
				src2p0[j][i*3+2] = src1p0[j1][i1*3+2];
			}
			else 
			{
				src2p0[j][i*3] = 0;
				src2p0[j][i*3+1] = 0;
				src2p0[j][i*3+2] = 0;
			}
		}
}

void YARPLpShifter::GetShiftedImage(const YARPImageOf<YarpPixelBGR>& im1, YARPImageOf<YarpPixelBGR>& dst, double * shift)
{
	GetShiftedImage ((const YARPImageOf<YarpPixelRGB>&)im1, (YARPImageOf<YarpPixelRGB>&)dst, shift);
}

void YARPLpShifter::GetShiftedImage(const YARPImageOf<YarpPixelMono>& im1, YARPImageOf<YarpPixelMono>& dst, double * shift)
{
	unsigned char **src1p0 = (unsigned char **)im1.GetArray();
	unsigned char **src2p0 = (unsigned char **)dst.GetArray();

	// shift (xi, eta);
	int xi = int (shift[0] + .5);
	int eta = int (shift[1] + .5);
	assert (xi >= 0 && xi < lut_size && eta >= 0 && eta < nAng);

	int i,j,i1,j1,jt;
	for(i=0; i<nEcc; i++)
		for(j=0; j<nAng; j++)
		{
			jt = (j + eta) % nAng;
		   	i1 = lut[xi][i][jt].ecc;
			j1 = lut[xi][i][jt].ang;
			j1=(j1 + nAng - eta) % nAng;
			if (i1>=0) 
			{
				src2p0[j][i]=src1p0[j1][i1];
			}
			else 
			{
				src2p0[j][i]=0;
			}
		}
}

