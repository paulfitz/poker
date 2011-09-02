// YARPVelocityField.h: interface for the YARPVelocityField class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VELOCITYFIELD_H__B71BC5A3_08D2_11D5_A5B3_005004BD1DE8__INCLUDED_)
#define AFX_VELOCITYFIELD_H__B71BC5A3_08D2_11D5_A5B3_005004BD1DE8__INCLUDED_

#include "YARPImage.h"
#include "YARPFilters.h"

#include <VisMatrix.h>

// THIS WORKS IN RECTANGULAR (E.G. 128SQ) IMAGES. NO LOGPOLAR!


class YARPVelocityField 
{
private:
	// Velocity Components
	CVisDMatrix vx;
	CVisDMatrix vy;
	
	// Useful Images Dimensions and Padding Bytes
	int dimX;
	int dimY;
	int dxy;
	
	// Kernels required for static operators
	IplConvKernelFP *DerivX;    
	IplConvKernelFP *DerivY;
 	IplConvKernelFP *DerivXX;	
	IplConvKernelFP *DerivYY;
	IplConvKernelFP *Gauss;
		
	// Images required for filtering
	IplImage *DerX_img;
	IplImage *DerY_img;
	IplImage *DerT_img;
	IplImage *DerXX_img;
	IplImage *DerXY_img;
	IplImage *DerYY_img;
	IplImage *DerXT_img;
	IplImage *DerYT_img;
	
	IplImage *Smoothing_img;	// temporal smooth.


	IplImage *PreviousFrame;	// for dert
	IplImage *PrevXT;			// for derxt
	IplImage *PrevYT;			// for deryt

	
	// Temporary images 
	IplImage *Temp_img;
	
	// Helper methods
	void _alloc_kernels(void);
	void _alloc_temp_images(void);
	void _free_kernels(void);
	void _free_temp_images(void);
	
	void _char2float(IplImage *src, IplImage *dst);
	
	void _compute_smoothing(IplImage *img);
	void _compute_derT(IplImage *img, IplImage *dst, IplImage *prev);

	double _verify_conditioning(float der_xx, float der_yy, float der_xy);
	
	void _fancydisplay (YARPImageOf<YarpPixelBGR>& out); //, CVisDMatrix& vx, CVisDMatrix& vy);

public:
	YARPVelocityField(int sizeX, int sizeY);
	virtual ~YARPVelocityField();

	virtual void ForceFreeMem (void);

	void Init(YARPImageOf<YarpPixelMono>& first);
	void Apply(YARPImageOf<YarpPixelMono>& currImg);

	// direct access to arrays. 
	inline CVisDMatrix& GetVx(void) { return vx; }
	inline CVisDMatrix& GetVy(void) { return vy; }			

	inline void FancyDisplay (YARPImageOf<YarpPixelBGR>& out) { _fancydisplay (out); }
};

#endif // !defined(AFX_VELOCITYFIELDFP_H__B71BC5A3_08D2_11D5_A5B3_005004BD1DE8__INCLUDED_)

