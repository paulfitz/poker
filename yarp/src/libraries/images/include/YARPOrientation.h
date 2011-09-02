// YARPOrientation.h
// 
// 
// by Davide Zanardi and Wouter
// 

#ifndef _YARPORIENTATION_H_
#define _YARPORIENTATION_H_

//#include <Services.h>
#include <YARPImage.h>
#include <VisMatrix.h>

#ifdef __QNX__
#include <limits>
#endif

class YARPOrientation
{
    public:
	YARPOrientation() ;
	YARPOrientation(int necc, int nang, double rfmin) ;
	YARPOrientation(const YARPImageOf<YarpPixelMono> &image) ;
	~YARPOrientation() ;

	double Apply(const YARPImageOf<YarpPixelMono> &image) ;
	int Resize(const YARPImageOf<YarpPixelMono> &image) ;
	int Resize(int necc, int nang, double rfmin) ;

    private:
	int
        nEcc,
        nAng ;
	double
        qr, r0, a,
        rfMin ;
    double 
        *jacobian,
        *power,
        *as, *ac ;

        int initialise() ;
        int cleanup() ;
        int centroid(const YARPImageOf<YarpPixelMono> &image,
                     double &x, double &y) ;
        int arguments(const YARPImageOf<YarpPixelMono> &image,
                      double x, double y,
                      double &arcsin, double &arccos) ;
} ;

#endif
