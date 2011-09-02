// YARPFirstOrderFlow.h
//

#if !defined(AFX_YARPFLOWFP_H__C98D5653_FCD8_11D4_A5B2_005004BD1DE8__INCLUDED_)
#define AFX_YARPFLOWFP_H__C98D5653_FCD8_11D4_A5B2_005004BD1DE8__INCLUDED_

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPlogpolar.h"

#include <VisMatrix.h>

// Note: Pasa 2001. This code needs to be cleaned up a little.
//	- optimize copies, type conversions, borders, etc.
//  - optimize filters, kernels, separability.
//
//
class YARPLpFirstOrderFlow : public YARPLogPolar
{
protected:
	int dxy;	// image size.

	//  first order flow decomposition [u0, v0, d, r, s1, s2]
    CVisDVector flowComp;
	int flowPoints;

	// const int pars
	//	int border;
	double inv_klog; 		

    //  arrays of coeffs for the least square solution
    CVisDVector rho;
    CVisDVector co;
    CVisDVector si;
	CVisDVector co2;
    CVisDVector si2;

	//  matrices for least square solution
    CVisDMatrix A_flow;
	CVisDMatrix A_trans;
	CVisDMatrix sqA;
	CVisDVector b_flow;
	CVisDVector sqB;

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
	void _float2char(IplImage *src, IplImage *dst, float scale, int border);
	void _rev_memcpy(float *dst, float *src, int nElem);
	void _compute_smoothing(IplImage *img);
	void _compute_derT(IplImage *img);

	inline void SetMatrix(double dxi, double deta, double dt, int item,int c, int r);

public:
	YARPLpFirstOrderFlow (int necc,int nang,double rfmin,int size);
	virtual ~YARPLpFirstOrderFlow() { Cleanup(); }
	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return false; }

	//  methods of flow class
	void Init(const YARPImageOf<YarpPixelMono>& first);
	void Apply(const YARPImageOf<YarpPixelMono>& currImg, const YARPImageOf<YarpPixelMono>& mask);

	inline CVisDVector GetFlowComp(void) const { return flowComp; }
	inline int GetFlowPoints(void) const { return flowPoints; }
};

inline void YARPLpFirstOrderFlow::SetMatrix(double dxi, double deta, double dt, int item,int c, int r)
{
	double gxi, geta;
	
	gxi = inv_klog * dxi;
	geta = q * deta;

	A_flow(item,1) = rho[c]*(gxi*co[r]-geta*si[r]);
	A_flow(item,2) = rho[c]*(gxi*si[r]+geta*co[r]);
	A_flow(item,3) = gxi;
	A_flow(item,4) = geta;
	A_flow(item,5) = gxi*co2[r]-geta*si2[r];
	A_flow(item,6) = gxi*si2[r]+geta*co2[r];

	b_flow(item)    = -dt;	// why there is no minus here ??
}



#endif // !defined(AFX_YARPFLOWFP_H__C98D5653_FCD8_11D4_A5B2_005004BD1DE8__INCLUDED_)
