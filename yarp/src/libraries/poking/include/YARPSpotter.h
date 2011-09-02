
#ifndef YARPSPOTTER_INC
#define YARPSPOTTER_INC

#include <iostream.h>
#include "YARPImage.h"

class YARPSpotter
{
public:
  int use_graph;

  void *system_resource;

  YARPSpotter();

  void InitDefaults()
    {
      use_graph = 1;
    }


  int UseGraph(int v=-1)
    {
      if (v>=0) use_graph = v;
      return v;
    }

  virtual ~YARPSpotter();

  void AddItem(int id, const char *main_data_file, 
	      const char *prototype_file, 
	      const char *means_file);

  void AddItems(const char *file_name);

  void Train(YARPImageOf<YarpPixelBGR>& src,
	     YARPImageOf<YarpPixelMono>& mask,
	     YARPImageOf<YarpPixelBGR>& dest,
	     int px = -1, int py = -1);

  void WriteTrain(ostream& os, int all=0);

  void PruneOverlap();

  void CalibrateItems();

  void SetLabel(const char *str);

  void Test(YARPImageOf<YarpPixelBGR>& src,
	    YARPImageOf<YarpPixelBGR>& dest,
	    int training = 0);

  int Found();

  int GetX();
  
  int GetY();

  int GetR();

  int GetID();
  
  void SetTarget(int target = -1);

  void Add(YARPImageOf<YarpPixelBGR>& src,
	   YARPImageOf<YarpPixelMono>& mask);

  YARPImageOf<YarpPixelBGR>& GetPrecedent();

  YARPImageOf<YarpPixelMono>& GetPrecedentMask();
};

#endif
