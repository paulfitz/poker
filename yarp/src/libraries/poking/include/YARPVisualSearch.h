#ifndef YARPVisualSearch_INC
#define YARPVisualSearch_INC

#include "YARPImage.h"

class YARPVisualSearch
{
protected:
  void *system_resource;
public:
  YARPVisualSearch();
  virtual ~YARPVisualSearch();

  void Reset();

  int IsActive();

  // add a target; by default will be searched for
  void Add(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelFloat>& mask,
	   int dx = 0, int dy = 0);
  void Add(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelMono>& mask,
	   int dx = 0, int dy = 0);

  // add a target, specifying approximate area and position
  void Add(YARPImageOf<YarpPixelBGR>& src, float x, float y, float dx,
	   float dy);

  /*
  void BackProject(YARPImageOf<YarpPixelBGR>& src, 
		   YARPImageOf<YarpPixelFloat>& dest_aux,
		   YARPImageOf<YarpPixelBGR>& dest);
  */

  void BackProject(YARPImageOf<YarpPixelBGR>& src, 
		   YARPImageOf<YarpPixelFloat>& dest);

  // give initial localization in x and y if known
  float Localize(YARPImageOf<YarpPixelBGR>& src,
		 YARPImageOf<YarpPixelBGR>& dest,
		 float& x, float& y, float radius = 1000.0);

  // give initial localization in x and y, orientation in theta
  float Orient(YARPImageOf<YarpPixelBGR>& src,
	       YARPImageOf<YarpPixelBGR>& dest,
	       float& x, float& y, float& theta);

  // search for previously set target
  void Apply(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest);

  float Apply(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest,
	      float x, float y, float dx=-1, float dy=-1);

  float Compare(YARPVisualSearch& alt);

  void Opportunistic(YARPImageOf<YarpPixelBGR>& src, 
		     YARPImageOf<YarpPixelFloat>& mask, 
		     YARPImageOf<YarpPixelBGR>& dest);

  void Show();
};


#endif
