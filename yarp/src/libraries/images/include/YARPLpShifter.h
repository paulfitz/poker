//
// YARPLpShifter.h
//	shift a log polar image by a given x,y displacement vector.
//

#ifndef __YARPLpShifterh__
#define __YARPLpShifterh__


#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPlogpolar.h"

//
// Following the policy of having the image services as independent as
//	possible I chose not to define a Vector & Matrix type here.
//	- see use of double * below.

class YARPLpShifter : public YARPLogPolar
{
	class cv1 
	{
	public:
		int ecc;
		int ang;

		cv1 (void) { ecc = ang = -1; }
	};

protected:	
	cv1 ***lut;
	const int lut_size;

public:
	YARPLpShifter(int nEcc, int nAng, double rfmin, int size);
	virtual ~YARPLpShifter();

	void GetShiftedImage(const YARPImageOf<YarpPixelRGB>& src, YARPImageOf<YarpPixelRGB>& im1, double * shift);
	void GetShiftedImage(const YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& im1, double * shift);
	void GetShiftedImage(const YARPImageOf<YarpPixelMono>& src, YARPImageOf<YarpPixelMono>& im1, double * shift);
};

#endif
