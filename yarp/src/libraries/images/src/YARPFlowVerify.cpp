//
// YARPFlowVerify.cpp
//
//

#include "YARPFlowVerify.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

#define YARP_SmoothFactor 0.5f
#define YARP_GaussVar 1.0f
#define YARP_FuseThr 70
#define YARP_MinPoints 25
#define YARP_Border 4
#define YARP_Threshold 2.0
#define YARP_ThreshLow 1.0

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YARPLpFlowVerify::YARPLpFlowVerify (int necc, int nang, double rfmin, int size) 
	: YARPLogPolar (necc, nang, rfmin, size)
{
	// image dimensions
    dxy=(nEcc*nAng);

	// alloc kernels
	_alloc_kernels();

	//alloc temporany images
	_alloc_temp_images();

	// matrix for storing error computation.
	error_img.Resize (nEcc, nAng);
	error_img = 0;

	// array needed for system solution
	rho.Resize(nEcc);
	co.Resize(nAng);
	si.Resize(nAng);
	co2.Resize(nAng);
	si2.Resize(nAng);

	// init co, si, co2, si2 data structures for LSQ sol.
	double delta = 2.0 * pi / ((double)nAng); // 1/q ??
	double tmp=0.0; //delta/2.0; LATER: check this change!
	for(int aa=1; aa<=nAng; aa++)
	{
		co(aa)=cos(tmp);
		co2(aa)=cos(2.0*tmp);
		si(aa)=sin(tmp);
		si2(aa)=sin(2.0*tmp);
		tmp+=delta;
	}

	// void init_rho data structure for LSQ sol
	int r;
	for(r=1;r<=necc;r++)
		rho(r) = ro0 * pow(a, (r-1) / kxi);
 
	for(r=1;r<=necc;r++)
		rho(r) = 1 / rho(r);

	// init inv_klog par.
	inv_klog = kxi / log(a);
}

void YARPLpFlowVerify::Cleanup (void)
{
	_free_kernels();
	_free_temp_images();
}

void YARPLpFlowVerify::_alloc_kernels(void)
{
	// See comments in FlowFP.

	// defines gauss matrix required for filtering
	float Values[25];
	Values[0] = Values[4] = Values[20] = Values[24] = (float)(3.50262697022767075306479859894e-3);
	Values[1] = Values[3] = Values[5] = Values[9] = Values[15] = Values[19] = Values[21] = Values[23] = (float)(1.22591943957968476357267950963e-2);
	Values[2] = Values[10] = Values[14] = Values[22] = (float)(2.10157618213660245183887915936e-2);
	Values[6] = Values[8] = Values[16] = Values[18] = (float)(5.42907180385288966725043782837e-2);
	Values[7] = Values[11] = Values[13] = Values[17] = (float)(9.10683012259194395796847635726e-2);
	Values[12] = (float)(0.22241681260945709281961471103327);

	// Gaussian Kernel
	Gauss = iplCreateConvKernelFP (5, 5, 2, 2, Values);

	float KernelValues_Five[5];

	KernelValues_Five[0] = (float)-0.0833333;
	KernelValues_Five[1] = (float)0.6666667;
	KernelValues_Five[2] = 0.0;
	KernelValues_Five[3] = (float)-0.6666667;
	KernelValues_Five[4] = (float)0.0833333;
	
	// X Derivative Kernel
	DerivX = iplCreateConvKernelFP (5, 1, 2, 0, KernelValues_Five);
	
	// Y Derivative Kernel
	DerivY = iplCreateConvKernelFP (1, 5, 0, 2, KernelValues_Five);
}

void YARPLpFlowVerify::_alloc_temp_images(void)
{
	DerY_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);
	iplAllocateImageFP (DerY_img, 0, 0.0);
	
	DerX_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);
	iplAllocateImageFP (DerX_img, 0, 0.0);
	
	DerT_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);
	iplAllocateImageFP (DerT_img, 0, 0.0);
	
	PreviousFrame = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);
	iplAllocateImageFP (PreviousFrame, 0, 0.0);
	
	Smoothing_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (Smoothing_img, 0, 0.0);
	
	Temp_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (Temp_img, 0, 0.0);

	A_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (A_img, 0, 0.0);

	B_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					nEcc+YARP_Border,
					nAng+(2*YARP_Border),
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (B_img, 0, 0.0);
}

void YARPLpFlowVerify::_free_kernels(void)
{
	iplDeleteConvKernelFP(DerivX);
	iplDeleteConvKernelFP(DerivY);
	iplDeleteConvKernelFP(Gauss);
}

void YARPLpFlowVerify::_free_temp_images(void)
{
	iplDeallocate (DerX_img, IPL_IMAGE_ALL);
	iplDeallocate (DerY_img, IPL_IMAGE_ALL);
	iplDeallocate (DerT_img, IPL_IMAGE_ALL);
	iplDeallocate (PreviousFrame, IPL_IMAGE_ALL);
	iplDeallocate (Smoothing_img, IPL_IMAGE_ALL);
	iplDeallocate (Temp_img, IPL_IMAGE_ALL);
	iplDeallocate (A_img, IPL_IMAGE_ALL);
	iplDeallocate (B_img, IPL_IMAGE_ALL);
}

void YARPLpFlowVerify::_char2float (IplImage *src, IplImage *dst)
{
	float *currentFloat;
	char *lineFloat, *lineChar;
	unsigned char *currentChar;

	lineFloat = dst->imageData;
	lineFloat += (dst->widthStep * YARP_Border); 
	lineChar = src->imageData ;

	for(int r=0; r<nAng; r++)
	{
		currentChar = (unsigned char *)lineChar;
		currentFloat = (float*)lineFloat;
		currentFloat += YARP_Border;
		
		for(int c=0; c<nEcc; c++)
			(*currentFloat++) = (float)(*currentChar++);
		
		lineChar += src->widthStep;
		lineFloat += dst->widthStep;
	}

	// Left Border
	char *lineFloat_dst = dst->imageData;
	char *lineFloat_src = dst->imageData;
	float *srcFloat;
	float *dstFloat;
	lineFloat_dst += YARP_Border * dst->widthStep;
	lineFloat_src += (dst->height / 2) * dst->widthStep;
	for (int r=0; r<nAng/2; r++)
	{
		srcFloat = (float*)lineFloat_src;
		dstFloat = (float *)lineFloat_dst;
		srcFloat += YARP_Border;
		
		_rev_memcpy(dstFloat,srcFloat,YARP_Border);

		lineFloat_src += dst->widthStep;
		lineFloat_dst += dst->widthStep;
	}

	lineFloat_dst = dst->imageData;
	lineFloat_src = dst->imageData;
	lineFloat_dst += (dst->height / 2) * dst->widthStep;
	lineFloat_src += YARP_Border * dst->widthStep;
	for (int r=0; r<nAng/2; r++)
	{
		srcFloat = (float*)lineFloat_src;
		dstFloat = (float *)lineFloat_dst;
		srcFloat += YARP_Border;
		
		_rev_memcpy(dstFloat,srcFloat,YARP_Border);

		lineFloat_src += dst->widthStep;
		lineFloat_dst += dst->widthStep;
	}

	// Upper and lower borders

	lineFloat_dst = dst->imageData;
	lineFloat_src = dst->imageData;
	
	lineFloat_src += dst->widthStep * nAng;

	memcpy(lineFloat_dst,lineFloat_src,(dst->widthStep * YARP_Border));
	
	lineFloat_src = dst->imageData;
	lineFloat_src += (dst->widthStep * YARP_Border);
	lineFloat_dst = dst->imageData;
	lineFloat_dst += dst->widthStep * (nAng + YARP_Border);

	memcpy(lineFloat_dst,lineFloat_src,(dst->widthStep * YARP_Border));
}

void YARPLpFlowVerify::_rev_memcpy(float *dst, float *src, int nElem)
{
	float *temp_src = src + (nElem-1);
	for (int i=0; i<nElem;i++)
		*dst++ = *temp_src--;
}

void YARPLpFlowVerify::_compute_smoothing(IplImage *img)
{
	float *src = (float *)img->imageData;
	float *smooth = (float *)Smoothing_img->imageData;

	const int size = img->width * img->height;
	const float ulambda = 1 - YARP_SmoothFactor;
	for (int i = 0; i < size; i++)
	{
		*smooth = YARP_SmoothFactor * *src++ + ulambda * *smooth;
		smooth++;
	}
}

void YARPLpFlowVerify::_compute_derT(IplImage *img)
{
	float *src = (float *)img->imageData;
	float *dt = (float *)DerT_img->imageData;
	float *copied = (float *)PreviousFrame->imageData;

	const int size = img->width * img->height;
	for (int i = 0; i < size; i++)
	{
		*dt++ = *src - *copied;
		*copied++ = *src++;
	}
}

void YARPLpFlowVerify::Init(const YARPImageOf<YarpPixelMono>& first)
{
	_char2float(first.GetIplPointer(), PreviousFrame); 
	_char2float(first.GetIplPointer(), Smoothing_img);
}

void YARPLpFlowVerify::Apply (const YARPImageOf<YarpPixelMono>& currImg, YARPImageOf<YarpPixelMono>& mask, CVisDVector& flow)
{
	error_img = 0;

	// Image FP conversion and log-polar border handling
	_char2float(currImg.GetIplPointer(), Temp_img); //ok tested

	// Apply Smoothing Filter 
	_compute_smoothing(Temp_img); //ok tested
	
	// Apply Gaussian Filter
	iplConvolve2DFP(Smoothing_img, Temp_img, &Gauss, 1, IPL_SUM); 
	
	// Apply Temporal Derivative Filter 
	_compute_derT(Temp_img); //ok tested
		
	// Apply X Derivative Filter
	iplConvolve2DFP(Temp_img, DerX_img, &DerivX, 1, IPL_SUM); //ok tested
		
	// Apply Y Derivative Filter
	iplConvolve2DFP(Temp_img, DerY_img, &DerivY, 1, IPL_SUM); //ok tested

	// passing image array pointers
	float *p_dx, *p_dy, *p_dt;
	char *p_lineDerX, *p_lineDerY, *p_lineDerT;

	p_lineDerX = DerX_img->imageData;
	p_lineDerY = DerY_img->imageData;
	p_lineDerT = DerT_img->imageData;

	double rad_quad;

	int c, r;
	int limCol = nEcc - YARP_Border;
		
	double dx, dy, dt;
	
	// Skipping upper border
	p_lineDerX += (DerX_img->widthStep * YARP_Border);
	p_lineDerY += (DerY_img->widthStep * YARP_Border);
	p_lineDerT += (DerT_img->widthStep * YARP_Border);

	// running maximum.
	double max = 0;	// always >0

	unsigned char *p_maskVal;
	char *p_lineMask;
	IplImage *p_mask;
	p_mask = mask.GetIplPointer();
	p_lineMask = p_mask->imageData;

	for (r=0; r<nAng; r++)
	{
		p_dx = (float *)p_lineDerX;
		p_dy = (float *)p_lineDerY;
		p_dt = (float *)p_lineDerT;

		//skipping left border
		p_dx += (YARP_Border);
		p_dy += (YARP_Border);
		p_dt += (YARP_Border);

		p_maskVal = (unsigned char *)p_lineMask;
		for (c=0; c<limCol; c++)
		{
			if (((int)*p_maskVal) < YARP_FuseThr)
			{
				dx  = (double)*p_dx;
				dy = (double)*p_dy;
				dt   = (double)*p_dt;
							
				rad_quad = sqrt(dx*dx+dy*dy);

				// Modified by Pasa (|| instead of &&).
				if ((rad_quad > YARP_Threshold) || (fabs(dt) > YARP_ThreshLow))
				{
					error_img(c+1, r+1) = ComputeHorn (dx, dy, dt, c, r, flow);
					if (error_img(c+1, r+1) > max)
						max = error_img(c+1,r+1);
				}
			}

			p_dx++;
			p_dy++;
			p_dt++;
			p_maskVal++;
		}

		p_lineDerX += DerX_img->widthStep;
		p_lineDerY += DerY_img->widthStep;
		p_lineDerT += DerT_img->widthStep;

		p_lineMask += p_mask->widthStep;
	}

	// 30 should be the intrinsic error of the OF comp.
	// it is the LSquare error. Could it be estimated?
	if (max < 30) 
	{
		p_lineMask = p_mask->imageData;
		for (r=0; r<nAng; r++)
		{
			p_maskVal = (unsigned char *)p_lineMask;
			for (c=0; c<limCol; c++)
			{
				double dtmp = ((YARPImageOf<YarpPixelMono>&)currImg)(c, r) * .9;
				*p_maskVal++ = char(dtmp);
				//*p_maskVal++ = 0;
			}
			p_lineMask += p_mask->widthStep;
		}
	}
	else
	{
		p_lineMask = p_mask->imageData;
		for (r=0; r<nAng; r++)
		{
			p_maskVal = (unsigned char *)p_lineMask;
			for (c=0; c<limCol; c++)
			{
				// put 100 instead of max.
				double dtmp = error_img(c+1, r+1) / max * 255.0;
				if (dtmp > 255.0) dtmp = 255.0;

				if (dtmp > 100) dtmp = 255.0;
				else
					dtmp = ((YARPImageOf<YarpPixelMono>&)currImg)(c, r) * .9;
				*p_maskVal++ = char(dtmp);
			}
			p_lineMask += p_mask->widthStep;
		}
	}
}

#undef YARP_SmoothFactor
#undef YARP_GaussVar
#undef YARP_FuseThr
#undef YARP_MinPoints
#undef YARP_Border
#undef YARP_Threshold
#undef YARP_ThreshLow
