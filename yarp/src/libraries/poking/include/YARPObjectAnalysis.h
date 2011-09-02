#ifndef YARPObjectAnalysis_INC
#define YARPObjectAnalysis_INC

#include "YARPImage.h"

class YARPObjectAnalysis
{
public:
  float shrink;
  
  YARPObjectAnalysis();
  
  void SetShrinkFactor(float factor = -1);
  
  void ColorSegment(YARPImageOf<YarpPixelBGR>& src,
		    YARPImageOf<YarpPixelBGR>& dest);

  float SpotInternals(YARPImageOf<YarpPixelBGR>& src_seg,
		      YARPImageOf<YarpPixelMono>& in_mask,
		      YARPImageOf<YarpPixelMono>& out_mask,
		      YARPImageOf<YarpPixelBGR>& dest);

};


#endif
