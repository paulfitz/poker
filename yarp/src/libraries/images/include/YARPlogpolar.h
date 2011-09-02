// YARPlogpolar.h: interface for the YARPLogPolar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YARPLOGPOLAR_H__9A2A9207_4883_11D2_8719_0060087AA09D__INCLUDED_)
#define AFX_YARPLOGPOLAR_H__9A2A9207_4883_11D2_8719_0060087AA09D__INCLUDED_

// 
// This is the software logpolar mapper, remapper, ecc.
//	it could be used also to convert a single pixel position, etc.
//	it maintains a representation of the lp mapping.

#include <YARPImage.h>
#include <math.h>

// locally defined - undef before leaving the file.
#ifndef pi
#define pi 3.14159265359
#endif

/////////////////////////////////////////////////////////////////////////
// Conversion equations.
//
// Start with:
//	nEcc, nAng, RFmin e Size.
//  where:
//    nEcc is the # of eccentricities.
//	  nAng is the # of angles.
//	  RFmin is the smallest receptive field size.
//	  Size is the size of the cartesian image.
//		
//		ro0 = nAng * RFmin / (2 * pi);
//		a = (ro0 + RFmin) / ro0;
//
//	this two equations take into account the fact that we do not
//	want to oversample in fovea.
//
//		q = nAng / (2 * pi);
//
//	this is the conversion factor for angles.
//
//		kxi = nEcc / (log((Size / 2 - 1) / ro0) / log(a));
//
//	this linear scaling factor is needed to fit the mapping into a cartesian
//	of a given size (Size in our case).
//
//	logpolar to cartesian (single pixel).
//		ro=ro0 * pow(a, xi / kxi);   
//		theta = eta / q;
//		xCoord=int(Size/2+ro*cos(theta)+.5);
//		yCoord=int(Size/2+ro*sin(theta)+.5);
//
//	cartesian to logpolar.
//		ro = sqrt ((x-x0) ^ 2 + (y-y0) ^ 2);  dove x0=y0=Size/2
//		if (ro < ro0) ro = ro0;
//		if (ro >= Size/2 - 1)  ro = Size/2 - 1; 
//		theta = atan2(dy,dx);
//		if (theta < 0) theta += 2 * pi;
//		xi = int(kxi * log(ro / ro0) / log(a) + .5); 
//		if (xi >= nEcc) xi = nEcc - 1;
//		eta = int(q * theta + .5);
//		if (eta >= nAng) eta = 0;
//
//

//
// It could not be a template... too complicated.
// For the sake of efficiency it's better to have it as a case.
// Logpolar mapping is used only for a few types (predefined).
//
class YARPLogPolar   
{
private:
	// avoid copy construction and op=.
	YARPLogPolar& operator=(const YARPLogPolar&);
	YARPLogPolar(const YARPLogPolar&);

protected:
	int nEcc;		// eccentricities
	int nAng;		// angles
	double RFmin;	// min receptive field
	int Size;		// cartesian size.
	int * xCoord;	// lut
	int * yCoord;	// lut
	

	double ro0;		// min radius
	double a;		// log basis
					
	double q;		// angular resolution
	double kxi;		// linear scaling

	int *l2cx;		// lut from lp to cartesian 
	int *l2cy;		//

	int ** x_ave;	// averaged version of the lp sampling.
	int ** y_ave;	// 
	bool use_new;	// not a parameter yet.

	void InitSampling(void);
	void InitNewSampling(void);

public:
	YARPLogPolar(void);
	YARPLogPolar(int necc,int nang,double rfmin,int size);

	virtual ~YARPLogPolar(void);

	void Resize(int necc,int nang,double rfmin,int size);
	
	inline int GetnEcc(void) const;
	inline int GetnAng(void) const;
	inline int GetSize(void) const;
	
	inline int *GetxLut(void) const { return xCoord; }
	inline int *GetyLut(void) const { return yCoord; }

	inline double GetA(void) const;
	inline double GetQ(void) const;
	inline double GetKXI(void) const;
	inline double GetRO0(void) const;

	inline void Lp2Cart(double,double,double&,double&) const;
	inline void Cart2Lp(double,double,double&,double&) const;
	inline void Lp2Cart_Exact(double,double,double&,double&) const;
	inline void Cart2Lp_Exact(double,double,double&,double&) const;

	inline double Ro2X(double) const;
	inline double X2Ro(double) const;

	void Lp2Cart(const YARPGenericImage& is, YARPGenericImage& id) const;
	void Cart2Lp(const YARPGenericImage& is, YARPGenericImage& id) const;
	void Cart2LpAverage(const YARPGenericImage& is, YARPGenericImage& id) const;
	void Cart2LpSwap(const YARPGenericImage& is, YARPGenericImage& id) const;
};



//////////////////////////////////////////////////////////////////////////////
//
// inlines.
//
inline int YARPLogPolar::GetnEcc(void) const { return nEcc; }
inline int YARPLogPolar::GetnAng(void) const { return nAng; }
inline int YARPLogPolar::GetSize(void) const { return Size; }

inline double YARPLogPolar::GetA(void) const { return a; }
inline double YARPLogPolar::GetQ(void) const { return q; }
inline double YARPLogPolar::GetKXI(void) const { return kxi; }
inline double YARPLogPolar::GetRO0(void) const { return ro0; }

inline void YARPLogPolar::Lp2Cart(double xi,double eta,double& x,double& y) const
{
	int index = int(eta*nEcc+xi);

	assert (eta >= 0 && eta <= nAng-1 && xi >= 0 && xi <= nEcc-1);
	if (eta < 0 || eta > nAng-1 || xi < 0 || xi > nEcc-1)
	{
		x = 0;
		y = 0;
		return;
	}

	x=xCoord[index];
	y=yCoord[index];
}

inline void YARPLogPolar::Lp2Cart_Exact(double xi,double eta,double& x,double& y) const
{
	int index = int(eta*nEcc+xi);

	assert (eta >= 0 && eta <= nAng-1 && xi >= 0 && xi <= nEcc-1);
	if (eta < 0 || eta > nAng-1 || xi < 0 || xi > nEcc-1)
	{
		x = 0;
		y = 0;
		return;
	}

	double ro=ro0 * pow(a, xi / kxi);
	double theta = eta / q;
	x=(Size/2+ro*cos(theta)+.5);
	y=(Size/2+ro*sin(theta)+.5);
}

// xi in the range 0 - nEcc-1
inline double YARPLogPolar::Ro2X (double xi) const
{
	int index = int(xi);

	assert (xi >= 0 && xi <= nEcc-1);
	return ro0 * pow(a, xi / kxi);
}

// x in the range 0 - Size-1
inline double YARPLogPolar::X2Ro (double x) const
{
	assert (x>=0 && x<Size);

	const double x0 = Size/2; // y0 e' uguale (img quadrata).
	const double dx = x - x0;

	double ro = sqrt (dx * dx);
	if (ro < ro0) ro = ro0;
	if (ro >= x0 - 1) ro = x0 - 1;

	double xi = kxi * log(ro / ro0) / log(a);
	if (xi >= nEcc) xi = nEcc - 1;

	return xi;
}

inline void YARPLogPolar::Cart2Lp(double x, double y, double& xi, double& eta) const
{
	assert (x>=0 && x<Size && y>=0 && y<Size);
	if (x < 0 || x >= Size || y < 0 || y >= Size)
	{
		xi = 0;
		eta = 0;
		return;
	}
	
	const double x0 = Size/2; // y0 e' uguale (img quadrata).
	const double dx = x - x0;
	const double dy = y - x0;

	double ro = sqrt (dx * dx + dy * dy);
	if (ro < ro0) ro = ro0;
	if (ro >= x0 - 1) ro = x0 - 1;

	double theta = 0;
	if (dy == 0 && dx == 0)
		theta = 0;
	else
	{
		theta = atan2(dy,dx);
		if (theta < 0) theta += 2 * pi;
	}

	if (theta >= 2 * pi)
		theta = 2 * pi - 1e-10;
	else
	if (theta < 0)
		theta = 0;

	//assert (theta < 2 * pi && theta >= 0);

	xi = int(kxi * log(ro / ro0) / log(a) + .5);
	if (xi >= nEcc) xi = nEcc - 1;
	eta = int(q * theta + .5);
	if (eta >= nAng) eta = 0;
}

inline void YARPLogPolar::Cart2Lp_Exact(double x, double y, double& xi, double& eta) const
{
	assert (x>=0 && x<Size && y>=0 && y<Size);
	if (x < 0 || x >= Size || y < 0 || y >= Size)
	{
		xi = 0;
		eta = 0;
		return;
	}
	
	const double x0 = Size/2; // y0 e' uguale (img quadrata).
	const double dx = x - x0;
	const double dy = y - x0;

	double ro = sqrt (dx * dx + dy * dy);
	if (ro < ro0) ro = ro0;
	if (ro >= x0 - 1) ro = x0 - 1;

	double theta = 0;
	if (dy == 0 && dx == 0)
		theta = 0;
	else
	{
		theta = atan2(dy,dx);
		if (theta < 0) theta += 2 * pi;
	}

	//assert (theta < 2 * pi && theta >= 0);
	if (theta >= 2 * pi)
		theta = 2 * pi - 1e-10;
	else
	if (theta < 0)
		theta = 0;

	xi = (kxi * log(ro / ro0) / log(a) + .5);
	if (xi >= nEcc) xi = nEcc - 1;
	eta = (q * theta + .5);
	if (eta >= nAng) eta = 0;
}

// un-definitions
#undef pi

#endif // !defined(AFX_LOGPOLAR_H__9A2A9207_4883_11D2_8719_0060087AA09D__INCLUDED_)
