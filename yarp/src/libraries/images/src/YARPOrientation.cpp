// yarporientation.cpp

#include "YARPOrientation.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

YARPOrientation::YARPOrientation()
: nEcc(0), nAng(0), rfMin(0.31)
{
    initialise() ;
}

YARPOrientation::YARPOrientation(int necc, int nang, double rfmin)
: nEcc(necc), nAng(nang), rfMin(rfmin)
{
    initialise() ;
}

YARPOrientation::YARPOrientation(const YARPImageOf<YarpPixelMono> &image)
{
    nAng = image.GetHeight() ;
    nEcc = image.GetWidth() ;
    rfMin = 0.31 ;
    initialise() ;
}

YARPOrientation::~YARPOrientation()
{
    cleanup() ;
}

int YARPOrientation::initialise()
{
	// hmm... rfMin is a challenge... we put it here to 1 for compatibility reasons
	// ask David for details
	rfMin = 1 ;
    qr = (double)nAng / (2 * pi) ;
    r0 = (nAng * rfMin) / (2 * pi) ;
    a  = (r0 + rfMin) / r0 ;

    jacobian = new double[nEcc] ;

    double
        arg = (r0 * r0) / qr ;

    for (int i = 0; i < nEcc; i++)
	{
        jacobian[i] = fabs(arg * pow(a, i * 2)) ;
	}

    power = new double[nEcc] ;
    for (int i = 0; i < nEcc; i++)
	{
        power[i] = r0 * pow(a,i) ;
	}

    as = new double[nAng] ;
    ac = new double[nAng] ;
    
    for (int i = 0; i < nAng; i++)
    {
        as[i] = sin((double)i/qr) ;
        ac[i] = cos((double)i/qr) ;
    }
    return true ;
}

int YARPOrientation::centroid(const YARPImageOf<YarpPixelMono> &image,
                              double &x, double &y)
{
    x = 0 ;
    y = 0 ;
    
    char**
        pixels = image.GetArray() ;

    double
        numeratorX = 0.0,
        numeratorY = 0.0,
        area = 0.0 ;
    
    for (int i = 0; i < nAng; i++)
    {
        for (int j = 0; j < nEcc; j++)
        {
            if ((unsigned char)pixels[i][j] > 250)
            {
                area += fabs(jacobian[j]) ;
                numeratorX += power[j] * ac[i] * jacobian[j] ;
                numeratorY += power[j] * as[i] * jacobian[j] ;
            }
        }
    }

    x = numeratorX / area ;
    y = numeratorY / area ;

    return true ;
}

int YARPOrientation::arguments(const YARPImageOf<YarpPixelMono> &image,
                               double x, double y,
                               double &arcsin, double &arccos)
{
    arcsin = 0.0 ;
    arccos = 0.0 ;
    
    char**
        pixels = image.GetArray() ;

    double
        A = 0.0,
        B = 0.0,
        C = 0.0 ;

    for (int i = 0; i < nAng; i++)
    {
        for (int j = 0; j < nEcc; j++)
        {
            if ((unsigned char)pixels[i][j] > 250)
            {
                A += (power[j] * ac[i] - x) * (power[j] * ac[i] - x) * jacobian[j] ;
				//Logfile::msg (Logfile::info, "A = %f\n", A) ;

                B += 2* (power[j] * ac[i] - x) * (power[j] * as[i] - y) * jacobian[j] ;
				//Logfile::msg (Logfile::info, "B = %f\n", B) ;

                C += (power[j] * as[i] - y) * (power[j] * as[i] - y) * jacobian[j] ;
				//Logfile::msg (Logfile::info, "C = %f\n", C) ;
            }
        }
    }
    
    arcsin = B / sqrt(B * B + (A - C) * (A - C)) ;
    arccos = (A - C) / sqrt(B * B + (A - C) * (A - C)) ;

    return true ;
}

int YARPOrientation::cleanup()
{
    delete[] jacobian ;
    delete[] power ;
    delete[] as ;
    delete[] ac ;
   
    return true ;
}

double YARPOrientation::Apply(const YARPImageOf<YarpPixelMono> &image)
{
    double
        x = 0.0,
        y = 0.0 ;

    centroid(image, x, y) ;
    
    double
        argsin = 0.0,
        argcos = 0.0 ;

    arguments(image, x, y, argsin, argcos) ;
	        
    double
        angle = 0.0 ;

    if (argsin > 0.0 && argcos > 0.0)
        angle = (asin(argsin)) / 2.0 ;
    else if (argsin > 0.0 && argcos < 0.0)
        angle = (acos(argcos)) / 2.0 ;
    else if (argsin < 0.0 && argcos < 0.0)
        angle = (-1.0) * (acos(argcos)) / 2.0;
    else if (argsin < 0.0 && argcos > 0.0)
        angle = (asin(argsin)) / 2.0 ;
    
#ifdef __WIN32__
    return angle == numeric_limits<double>::signaling_NaN() ? 0 : - angle ;
#else
	//should check for error using ERRNO kind of stuff.
	return -angle;
#endif
}

int YARPOrientation::Resize(const YARPImageOf<YarpPixelMono> &image)
{
    cleanup() ;

    nEcc = image.GetWidth() ;
    nAng = image.GetHeight() ;

    initialise() ;

    return true ;
}

int YARPOrientation::Resize(int necc, int nang, double rfmin)
{
    cleanup() ;

    nEcc = necc ;
    nAng = nang ;
    rfMin = rfmin ;
    
    initialise() ;

    return true ;
}







