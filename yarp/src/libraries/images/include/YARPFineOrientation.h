
#ifndef YARPFineOrientation_INC
#define YARPFineOrientation_INC

#include "YARPImage.h"

#define FINE_ORIENTATION_SHIFTS (16)

class YARPFineOrientation
{
protected:

  static YARPImageOf<YarpPixelBGR> shifts[FINE_ORIENTATION_SHIFTS], mean;

public:
  int use_democracy;
  int use_luminance_filter;
  int use_quotient;

  YARPFineOrientation()
    {
      use_democracy = 1;
      use_luminance_filter = 10;
      use_quotient = -1;
    }

  static void Init(const char *fname = NULL);

  void Apply(YARPImageOf<YarpPixelBGR>& src,
	     YARPImageOf<YarpPixelFloat>& delx,
	     YARPImageOf<YarpPixelFloat>& dely,
	     YARPImageOf<YarpPixelFloat>& strength);

  void Apply(YARPImageOf<YarpPixelBGR>& src,
	     YARPImageOf<YarpPixelBGR>& dest,
	     YARPImageOf<YarpPixelFloat>& delx,
	     YARPImageOf<YarpPixelFloat>& dely,
	     YARPImageOf<YarpPixelFloat>& strength);

  int SetDemocracy(int d)
    {
      if (d>=0) { use_democracy = d; }
      return use_democracy;
    }

  int SetLuminanceFilter(int d)
    {
      if (d>=0) { use_luminance_filter = d; }
      return use_luminance_filter;
    }

  int SetQuotient(int d)
    {
      if (d>=0) { use_quotient = d; }
      return use_quotient;
    }
};

#endif



