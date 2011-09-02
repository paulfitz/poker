#ifndef YARPVisualContact_INC
#define YARPVisualContact_INC

#include "YARPImage.h"

class YARPVisualContact
{
protected:
  void *system_resource;
public:
  YARPVisualContact();
  virtual ~YARPVisualContact();

  void UseSegmentation(int flag=1);

  void Reset();

  // Default operation -- identify point of contact
  int Apply(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest);
  
  // returns an image where non-zero values represent motion
  // only valid after Apply() is called and a point-of-contact is identified
  int GetSegmentedImage(YARPImageOf<YarpPixelMono>& dest);

  // returns an image where non-zero values represent possible presence
  // of robot arm/flipper
  // only valid after Apply() is called and a point-of-contact is identified
  int GetFlipper(YARPImageOf<YarpPixelMono>& dest);

  int GetPokeDirection(float& x, float &y);

  // apply pixel-level motion modeling
  float GetMotion(YARPImageOf<YarpPixelBGR>& src, 
		  YARPImageOf<YarpPixelBGR>& dest,
		  YARPImageOf<YarpPixelFloat>& response);

  void GroupMotion(YARPImageOf<YarpPixelBGR>& src, 
		   YARPImageOf<YarpPixelBGR>& src_prev,
		   float dx, float dy,
		   YARPImageOf<YarpPixelMono>& dest_mask,
		   YARPImageOf<YarpPixelBGR>& dest);
};



#endif
