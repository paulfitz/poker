// FlowFP.h: interface for the CVerifyFlowFP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YARPVERIFYFLOW_H__C98D5653_FCD8_11D4_A5B2_005004BD1DE8__INCLUDED_)
#define AFX_YARPVERIFYFLOW_H__C98D5653_FCD8_11D4_A5B2_005004BD1DE8__INCLUDED_

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPlogpolar.h"

#include <VisMatrix.h>

//
// See comments about improvement in YARPFirstOrderFlow.h
//
class YARPLpFlowVerify : public YARPLogPolar  
{
	// error image temp.
	CVisDMatrix error_img;

	// image dimensions
	int dxy;

	// const int pars
	//	int border;
	double inv_klog; 		

    //  arrays of coeffs for the least square solution
    CVisDVector rho;
    CVisDVector co;
    CVisDVector si;
	CVisDVector co2;
    CVisDVector si2;

	// Kernels required for image filtering
	IplConvKernelFP *DerivX;
	IplConvKernelFP *DerivY;
	IplConvKernelFP *Gauss;
	
	//  images required for filtering
	IplImage *DerX_img;
	IplImage *DerY_img;
	IplImage *DerT_img;
	IplImage *Smoothing_img;
	IplImage *PreviousFrame;
	
    //  temporary image 
	IplImage *Temp_img;
	IplImage *A_img;
	IplImage *B_img;

	// private methods.
	void _alloc_kernels(void);
	void _alloc_temp_images(void);
	void _free_kernels(void);
	void _free_temp_images(void);
	void _char2float(IplImage *src, IplImage *dst); // Converts a logpolar image from char to float adding borders
	void _rev_memcpy(float *dst, float *src, int nElem);
	void _compute_smoothing(IplImage *img);
	void _compute_derT(IplImage *img);

	inline void SetMatrix(double dxi, double deta, double dt, int item,int c, int r);

public:
	YARPLpFlowVerify (int necc, int nang, double rfmin, int size);
	virtual ~YARPLpFlowVerify() { Cleanup(); }
	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return false; }

	//  methods of flow class
	void Init(const YARPImageOf<YarpPixelMono>& first);

	// mask returns the segmentation.
	void Apply(const YARPImageOf<YarpPixelMono>& currImg, YARPImageOf<YarpPixelMono>& mask, CVisDVector& flow);

	double ComputeHorn (double dxi, double deta, double dt, int c, int r, CVisDVector& flow);
};

inline double YARPLpFlowVerify::ComputeHorn(double dxi, double deta, double dt, int c, int r, CVisDVector& flow)
{
	const double u0 = flow(1);
	const double v0 = flow(2);
	const double D = flow(3);
	const double R = flow(4);
	const double S1 = flow(5);
	const double S2 = flow(6);

	double x = (1.0 / rho[c]) * co[r];
	double y = (1.0 / rho[c]) * si[r];
	double u = u0 + (D+S1) *  x + (S2-R) * y;
	double v = v0 + (R+S2) *  x + (D-S1) * y;

	double gxi, geta;
	
	gxi = inv_klog * dxi;
	geta = q * deta;

	// Questi li ho verificati.
	double error = 
			rho[c]*(gxi*co[r]-geta*si[r]) * u +
			rho[c]*(gxi*si[r]+geta*co[r]) * v +
			dt;

	// 1-norm.
	error = fabs(error);

	return error;
}

#endif // !defined(AFX_YARPVERIFYFLOW_H__C98D5653_FCD8_11D4_A5B2_005004BD1DE8__INCLUDED_)
