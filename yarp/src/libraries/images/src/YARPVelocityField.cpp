#ifndef __LINUX__

// YARPVelocityField.cpp: implementation of the YARPVelocityField class.
//
//////////////////////////////////////////////////////////////////////

#include "YARPVelocityField.h"



#define YARP_SmoothFactor		0.35		// lambda for temporal smoothing 
#define YARP_CondThreshold		0.95		// Threshold on [A] conditioning number
#define YARP_DetThreshold		0.1			// Threshold on det[AtA] 
#define YARP_Border				5
#define YARP_MaxVelThreshold	5.0			// Max Velocity (if bigger it's noise)
#define YARP_DisplayThreshold   0.1			//

YARPVelocityField::YARPVelocityField(int sizeX, int sizeY)
{
	assert (sizeX > 0 && sizeY > 0);

	// image size
    dxy = sizeX * sizeY;

	dimX = sizeX;
	dimY = sizeY;
	
	// alloc useful kernels
	_alloc_kernels();

	// alloc temporany images
	_alloc_temp_images();

	// prepare velocity images
	vx.Resize(dimY, dimX);
	vy.Resize(dimY, dimX);
	vx = 0.0;
	vy = 0.0;
}

YARPVelocityField::~YARPVelocityField()
{
	ForceFreeMem();
}

void YARPVelocityField::ForceFreeMem (void)
{
	_free_kernels();
	_free_temp_images();
}

void YARPVelocityField::_alloc_kernels()
{
	// LATER: optimization required.

	// defines gauss matrix required for filtering
	// this has to be reimplemented completely.
	// -the double precision values make no sense.
	// -gaussian filtering is separable.
	// -I feel ashamed I haven't checked this piece of code before.

	float Values[25];
	Values[0] = Values[4] = Values[20] = Values[24] = (float)(3.50262697022767075306479859894e-3);
	Values[1] = Values[3] = Values[5] = Values[9] = Values[15] = Values[19] = Values[21] = Values[23] = (float)(1.22591943957968476357267950963e-2);
	Values[2] = Values[10] = Values[14] = Values[22] = (float)(2.10157618213660245183887915936e-2);
	Values[6] = Values[8] = Values[16] = Values[18] = (float)(5.42907180385288966725043782837e-2);
	Values[7] = Values[11] = Values[13] = Values[17] = (float)(9.10683012259194395796847635726e-2);
	Values[12] = (float)(0.22241681260945709281961471103327);

	// Gaussian Kernel
	Gauss = iplCreateConvKernelFP (5, 5, 2, 2, Values);
	assert (Gauss != NULL);
	
	// defines first derivative kernel 
	float KernelValues_FirstDer[7];
	KernelValues_FirstDer[0] = 1.0f;
	KernelValues_FirstDer[1] = -9.0f;
	KernelValues_FirstDer[2] = 45.0f;
	KernelValues_FirstDer[3] = 0.0f;
	KernelValues_FirstDer[4] = -45.0f;
	KernelValues_FirstDer[5] = 9.0f;
	KernelValues_FirstDer[6] = -1.0f;
	
	for (int i=0;i<7;i++)
		KernelValues_FirstDer[i] = KernelValues_FirstDer[i]/60;
	
	// defines second derivative kernel
	float KernelValues_SecondDer[7];
	KernelValues_SecondDer[0] = 2.0f;
	KernelValues_SecondDer[1] = -27.0f;
	KernelValues_SecondDer[2] = 270.0f;
	KernelValues_SecondDer[3] = -490.0f;
	KernelValues_SecondDer[4] = 270.0f;
	KernelValues_SecondDer[5] = -27.0f;
	KernelValues_SecondDer[6] = 2.0f;

	for (int i=0;i<7;i++)
		KernelValues_SecondDer[i] = KernelValues_SecondDer[i]/180;
	
	// X Derivative Kernel
	DerivX = iplCreateConvKernelFP(7, 1, 3, 0, KernelValues_FirstDer);
	// Y Derivative Kernel
	DerivY = iplCreateConvKernelFP(1, 7, 0, 3, KernelValues_FirstDer);

	// XX Derivative Kernel
	DerivXX = iplCreateConvKernelFP(7, 1, 3, 0, KernelValues_SecondDer);
	// YY Derivative Kernel
	DerivYY = iplCreateConvKernelFP(1, 7, 0, 3, KernelValues_SecondDer);
}

//
//
//
void YARPVelocityField::_alloc_temp_images()
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
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerY_img, 0, 0.0);

	DerYY_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerYY_img, 0, 0.0);

	DerX_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerX_img, 0, 0.0);

	DerXX_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerXX_img, 0, 0.0);

	DerXY_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerXY_img, 0, 0.0);
	
	DerT_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerT_img, 0, 0.0);

	DerXT_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerXT_img, 0, 0.0);

	DerYT_img = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (DerYT_img, 0, 0.0);
	
	PreviousFrame = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
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
					dimX,
					dimY,
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
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (Temp_img, 0, 0.0);

	PrevXT = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,	 
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (PrevXT, 0, 0.0);

	PrevYT = iplCreateImageHeader(
					1,
					0,
					IPL_DEPTH_32F,			
					"GRAY",
					"GRAY",
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_BL,			
					IPL_ALIGN_QWORD,		
					dimX,
					dimY,
					NULL,
					NULL,
					NULL,
					NULL);

	iplAllocateImageFP (PrevYT, 0, 0.0);
}


void YARPVelocityField::_free_kernels(void)
{
	iplDeleteConvKernelFP(DerivX);
	iplDeleteConvKernelFP(DerivY);
	iplDeleteConvKernelFP(DerivXX);
	iplDeleteConvKernelFP(DerivYY);
}


void YARPVelocityField::_free_temp_images(void)
{
	iplDeallocate (DerX_img, IPL_IMAGE_ALL);
	iplDeallocate (DerY_img, IPL_IMAGE_ALL);
	iplDeallocate (DerT_img, IPL_IMAGE_ALL);
	iplDeallocate (DerXX_img, IPL_IMAGE_ALL);
	iplDeallocate (DerXY_img, IPL_IMAGE_ALL);
	iplDeallocate (DerYY_img, IPL_IMAGE_ALL);
	iplDeallocate (DerXT_img, IPL_IMAGE_ALL);
	iplDeallocate (DerYT_img, IPL_IMAGE_ALL);
	iplDeallocate (PreviousFrame, IPL_IMAGE_ALL);
	iplDeallocate (Smoothing_img, IPL_IMAGE_ALL);
	iplDeallocate (Temp_img, IPL_IMAGE_ALL);

	iplDeallocate (PrevXT, IPL_IMAGE_ALL);
	iplDeallocate (PrevYT, IPL_IMAGE_ALL);
}


double YARPVelocityField::_verify_conditioning(float der_xx, float der_yy, float der_xy)
{
	double n1,d1,d2;

	d1 = sqrt((der_xx * der_xx) + (der_xy * der_xy));
	d2 = sqrt((der_yy * der_yy) + (der_xy * der_xy));
	
	n1 = der_xy * (der_xx + der_yy);
	return (fabs(n1/(d1*d2)));
}


//
//	convert from char to float.
//
void YARPVelocityField::_char2float (IplImage *src, IplImage *dst)
{
	//
	float *currentFloat;
	char *lineFloat, *lineChar;
	unsigned char *currentChar;

	lineFloat = dst->imageData;
	lineChar = src->imageData ;

	for(int r = 0; r < dimY; r++)
	{
		currentChar = (unsigned char *)lineChar;
		currentFloat = (float *)lineFloat;
		
		for(int c = 0; c < dimX; c++)
			(*currentFloat++) = (float)(*currentChar++);
		
		lineChar += src->widthStep;
		lineFloat += dst->widthStep;
	}
}


//
// temporal smoothing.
//
void YARPVelocityField::_compute_smoothing(IplImage *img)
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

	// result is in Smoothing_img.
}


//
//
//
void YARPVelocityField::_compute_derT(IplImage *img, IplImage *dst, IplImage *prev)
{
	float *src = (float *)img->imageData;
	float *dt = (float *)dst->imageData;
	float *copied = (float *)prev->imageData;

	const int size = img->width * img->height;
	for (int i = 0; i < size; i++)
	{
		*dt++ = *src - *copied;
		*copied++ = *src++;
	}

	// result in DerT_img
	// a copy of the original in PreviousFrame;
}


void YARPVelocityField::Init(YARPImageOf<YarpPixelMono>& first)
{
	assert (first.GetPadding() == 0);

	_char2float(first.GetIplPointer(), Temp_img);
	_char2float(first.GetIplPointer(), Smoothing_img);

	//Smoothing_img = Temp_img;
	//iplConvolve2DFP(Temp_img, Smoothing_img, &Gauss, 1, IPL_SUM); 
	iplConvolve2DFP(Temp_img, PreviousFrame, &Gauss, 1, IPL_SUM); 

	// X der
	iplConvolve2DFP(PreviousFrame, PrevXT, &DerivX, 1, IPL_SUM);
		
	// Y der
	iplConvolve2DFP(PreviousFrame, PrevYT, &DerivY, 1, IPL_SUM);
}


void YARPVelocityField::Apply(YARPImageOf<YarpPixelMono>& currImg)
{
	assert (currImg.GetPadding() == 0);

	//
	//
	//

	// convert to float.
	_char2float(currImg.GetIplPointer(), Temp_img); 

	// temporal smoothing.
	// result in Smoothing_img.
	_compute_smoothing(Temp_img);

	// gaussian spatial filtering
	// LATER: Use separable kernel!!!!!!!
	iplConvolve2DFP(Smoothing_img, Temp_img, &Gauss, 1, IPL_SUM); 


	// XX derivative	
	iplConvolve2DFP(Temp_img, DerXX_img, &DerivXX, 1, IPL_SUM);

	// YY derivative
	iplConvolve2DFP(Temp_img, DerYY_img, &DerivYY, 1, IPL_SUM);

	// dt
	_compute_derT(Temp_img, DerT_img, PreviousFrame);

	// X der
	iplConvolve2DFP(Temp_img, DerX_img, &DerivX, 1, IPL_SUM);
		
	// Y der
	iplConvolve2DFP(Temp_img, DerY_img, &DerivY, 1, IPL_SUM);

	// XY der
	iplConvolve2DFP(DerX_img, DerXY_img, &DerivY, 1, IPL_SUM);


	// TX
	_compute_derT(DerX_img, DerXT_img, PrevXT);

	// TY
	_compute_derT(DerY_img, DerYT_img, PrevYT);


	//
	//
	//
	const int maxx = dimX - YARP_Border;
	const int maxy = dimY - YARP_Border;

	float *dxx = (float *)DerXX_img->imageData + YARP_Border * dimX + YARP_Border;
	float *dxy = (float *)DerXY_img->imageData + YARP_Border * dimX + YARP_Border;
	float *dyy = (float *)DerYY_img->imageData + YARP_Border * dimX + YARP_Border;
	float *dx = (float *)DerX_img->imageData + YARP_Border * dimX + YARP_Border;
	float *dy = (float *)DerY_img->imageData + YARP_Border * dimX + YARP_Border;

	float *dxt = (float *)DerXT_img->imageData + YARP_Border * dimX + YARP_Border;
	float *dyt = (float *)DerYT_img->imageData + YARP_Border * dimX + YARP_Border;
	float *dt = (float *)DerT_img->imageData + YARP_Border * dimX + YARP_Border;

	//
	//
	//
	for(int r = YARP_Border; r < maxy; r++)
	{
		for(int c = YARP_Border; c < maxx; c++)
		{			
			double cond_number = _verify_conditioning(*dxx, *dyy, *dxy);

			if (cond_number <= YARP_CondThreshold)
			{
				// compute determinat of matrix AtA
				double den1 = (*dx * *dx) + (*dxx * *dxx) + (*dxy * *dxy);
				double den2 = (*dx * *dy) + (*dxx * *dxy) + (*dyy * *dxy);
				double den3 = (*dy * *dy) + (*dyy * *dyy) + (*dxy * *dxy);
				double den = (den1 * den3) - (den2 * den2);

				if (fabs(den) > YARP_DetThreshold)
				{
					double tn1 = (*dx * *dt) + (*dxx * *dxt) + (*dxy * *dyt);
					double tn2 = (*dy * *dt) + (*dxy * *dxt) + (*dyy * *dyt);
					double numx = (den2 * tn2) - (den3 * tn1);
					double numy = (den2 * tn1) - (den1 * tn2);
					
					double Vel_x, Vel_y;
					Vel_x = (numx / den);
					Vel_y = (numy / den);

					if ((fabs(Vel_x) <= YARP_MaxVelThreshold) && (fabs(Vel_y) <= YARP_MaxVelThreshold))
					{
						// good (?) flow.
						vx (c+1, r+1) = Vel_x;
						vy (c+1, r+1) = Vel_y;
					}
					else
					{
						// flow = 0.
						vx (c+1, r+1) = 1001.0;
						vy (c+1, r+1) = 0;
					}
				}
				else
				{
					// flow = 0.
					vx (c+1, r+1) = 1001.0;
					vy (c+1, r+1) = 0;
				}
			}
			else
			{
				// flow = 0.
				vx (c+1, r+1) = 1001.0;
				vy (c+1, r+1) = 0;
			}

			dxx ++;
			dyy ++;
			dxy ++;
			dx ++;
			dy ++;
			dxt ++;
			dyt ++;
			dt ++;
		}

		// next row.
		const int a = YARP_Border * 2;
		dxx += a;
		dyy += a;
		dxy += a;
		dx += a;
		dy += a;
		dxt += a;
		dyt += a;
		dt += a;
	}
}


void YARPVelocityField::_fancydisplay (YARPImageOf<YarpPixelBGR>& out) //, CVisDMatrix& vx, CVisDMatrix& vy)
{
	YarpPixelBGR pix;
	memset (&pix, 0, sizeof (YarpPixelBGR));
	YarpPixelBGR white;
	memset (&white, 255, sizeof(YarpPixelBGR));
	YarpPixelBGR red;
	memset (&red, 0, sizeof(YarpPixelBGR));
	red.r = 255;

	const int step = 3;
	const int scale = 2;

	for (int j = 0; j < dimY; j += step)
	{
		for (int i = 0; i < dimX; i += step)
		{
			int cx = i + step/2;
			int cy = j + step/2;

			if (vx(i+1,j+1) <= YARP_MaxVelThreshold)
			{
				if (vx(i+1,j+1) >= YARP_DisplayThreshold || vy(i+1,j+1) >= YARP_DisplayThreshold)
				{
					// draw arrow.
					const int RES = YARP_MaxVelThreshold;
					const double inc = 1.0 / RES;
					int x,y;
					for (int k = 0; k < RES; k++)
					{
						x = int(cx + scale * vx(i+1, j+1) * k * inc + .5);
						y = int(cy + scale * vy(i+1, j+1) * k * inc + .5);
						if (x >= 0 && x < dimX && y >= 0 && y < dimY)
							out (x,y) = pix;
					}
				}

				out(cx,cy) = white;
			}
			else
			{
				out(cx,cy) = red;
			}
		}
	}
}


#undef YARP_SmoothFactor	
#undef YARP_CondThreshold	
#undef YARP_DetThreshold	
#undef YARP_Border			
#undef YARP_MaxVelThreshold
#undef YARP_DisplayThreshold

#endif

