/*********************************************************************
 * pyramid.h
 *********************************************************************/

#ifndef _KLT_PYRAMID_H_
#define _KLT_PYRAMID_H_

#include "KLTklt_util.h"

typedef struct  {
  int subsampling;
  int nLevels;
  _KLT_FloatImage *img;
  int *ncols, *nrows;
}  _KLT_PyramidRec, *_KLT_Pyramid;


_KLT_Pyramid _KLTCreatePyramid(
  int ncols,
  int nrows,
  int subsampling,
  int nlevels);

void _KLTComputePyramid(
  _KLT_FloatImage floatimg, 
  _KLT_Pyramid pyramid,
  float sigma_fact);

void _KLTFreePyramid(
  _KLT_Pyramid pyramid);

#endif
