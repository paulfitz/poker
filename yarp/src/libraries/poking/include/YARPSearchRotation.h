//
// YARPSearchRotation.h
//

#ifndef __YARPSearchRotationh__
#define __YARPSearchRotationh__

#include "stdio.h"
#include "VisMatrix.h"
#include "YARPImage.h"
#include "YARPImageFile.h"

#include "YARPCanonicalData.h"

#define GOODNESS (1.0)
// 1.5

class YARPSearchRotation
{
	int m_threshold;
	double m_quality_thr;

public:
	YARPSearchRotation (int thr, double quality_thr);
	~YARPSearchRotation ();
	
	double Compare (YARPImageOf<YarpPixelMono>& mask, YARPImageOf<YarpPixelBGR>& frame, int cx, int cy, int x, int y, YARPImageOf<YarpPixelBGR>& img);
	int Search (int neuron, YARPCanonicalNeurons& canonical, YARPImageOf<YarpPixelBGR>& img, int x, int y, double ave, double std, double& angle, double& angle2);
	int EstimateGoodness (YARPImageOf<YarpPixelMono>& mask, double ave, double std);
};

#endif
