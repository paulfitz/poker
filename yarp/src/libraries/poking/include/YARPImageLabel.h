#ifndef YARPIMAGELABEL_INC
#define YARPIMAGELABEL_INC

#include "YARPImage.h"

class YARPImageLabel
{
private:
  int alloc_len;
  int *xstack;
  int *ystack;
public:
  YARPImageLabel()
    {
      xstack = ystack = NULL;
      alloc_len = -1;
    }

  virtual ~YARPImageLabel()
    {
      Clean();
    }

  void Clean()
    {
      if (xstack!=NULL) delete[] xstack;
      if (ystack!=NULL) delete[] ystack;
      xstack = ystack = NULL;
    }

  int Apply(YARPImageOf<YarpPixelMono>& src, 
	    YARPImageOf<YarpPixelMono>& dest);


  int Apply(YARPImageOf<YarpPixelInt>& src, 
	    YARPImageOf<YarpPixelInt>& dest);


  int ApplySimilarity(YARPImageOf<YarpPixelInt>& src, 
		      YARPImageOf<YarpPixelInt>& dest);

  // answers
  int bestId;

  virtual void Notify(int id, int count, int finished)
    {
    }

  virtual void Notify(int x, int y)
    {
    }

  virtual int IsCompatible(int x1, int y1, int x2, int y2)
    {
      return 1;
    }

  // overload IsCompatible to use this
  int Apply(int x, int y, YARPImageOf<YarpPixelInt>& dest);
};


#endif
