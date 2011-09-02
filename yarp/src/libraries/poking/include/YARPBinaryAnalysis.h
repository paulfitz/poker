#ifndef YARPBINARYANALYSIS_INC
#define YARPBINARYANALYSIS_INC

#include "YARPImage.h"

#define YARPBINARYANALYSIS_INVARIANTS 4

class YARPBinaryAnalysis
{
public:

  // parameters set by a call to Apply()
  double active_area;
  double total_area;
  double x_centroid;
  double y_centroid;
  double principal_angle; // in radians
  double moment_of_inertia;
  double isotropic_measure;
  double moment_invariant[YARPBINARYANALYSIS_INVARIANTS];

  void Apply(YARPImageOf<YarpPixelMono>& src);

  // this does a cast and calls the Apply for YarpPixelMono
  void Apply(YARPImageOf<YarpPixelFloat>& src);

  double GetX() { return x_centroid; }
  double GetY() { return y_centroid; }
  double GetAngle() { return principal_angle; }
  double GetPointiness() { return isotropic_measure; } 

  double GetArea() { return active_area; }
  double GetMomentOfInertia() { return moment_of_inertia; }

  int GetInvariantCount() { return YARPBINARYANALYSIS_INVARIANTS; }
  double GetInvariant(int index) { return moment_invariant[index]; }
};


#endif

