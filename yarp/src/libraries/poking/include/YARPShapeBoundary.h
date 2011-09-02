#ifndef YARPShapeBoundary_INC
#define YARPShapeBoundary_INC

#include <vector>

#include "YARPImage.h"

class YARPShapeElement
{
public:
  float x, y;

  YARPShapeElement()
    { x = y = 0; }

  YARPShapeElement(float nx, float ny)
  {
    x = nx;
    y = ny;
  }

  YARPShapeElement(const YARPShapeElement& e)
  {
    x = e.x;
    y = e.y;
  }

  float GetX() { return x; }
  float GetY() { return y; }
};


class YARPShapeBoundary
{
public:
  vector<YARPShapeElement> edge;

  void Reset()
  { 
#ifndef __QNX__
    edge.clear(); 
#else
    edge.erase(edge.begin(),edge.end()); 
#endif
  }

  void Add(float x, float y);
      
  void Show(YARPImageOf<YarpPixelBGR>& img, int fill = 1);

  void Transform(double sx,	/* x-scale changes */
		 double sy,	/* y-scale changes */
		 double rt,	/* rotation changes */
		 double tx,	/* x-translation changes */
		 double ty,	/* y-translation changes */
		 double idx,    /* x-dilation changes */
		 double idy);   /* y-dilation changes */

  void Rotate(double angle)
    {
      Transform(1,1,angle,0,0,0,0);
    }

  void RotateAbout(double angle, double x, double y);

  void RotateAboutCenter(double angle);

  void Scale(double scale)
    {
      Transform(scale,scale,0,0,0,0,0);
    }

  void ScaleAbout(double scale, double x, double y);

  void ScaleAboutCenter(double scale);

  void Translate(double dx, double dy);

  void GetMask(YARPImageOf<YarpPixelMono>& img);

  void SetFromMask(YARPImageOf<YarpPixelMono>& img, int freq = 5);

  YARPShapeElement& GetPoint(int index)
    { return edge[index]; }

  int GetPointCount()
    { return edge.size(); }

  YARPShapeElement GetCenter();
};

#endif
