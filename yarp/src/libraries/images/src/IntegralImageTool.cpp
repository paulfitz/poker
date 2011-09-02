
#include <math.h>
#include <stdlib.h>

#include "YARPImage.h"
#include "YARPImageDraw.h"
#include "IntegralImageTool.h"

// Sweep summation horizontally and vertically
void GenerateSum(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest)
{
  int i, j;
  float total, v;
  int h = src.GetHeight();
  int w = src.GetWidth();
 
  for (i=0; i<h; i++)
    {
      total = 0;
      for (j=0; j<w; j++)
		{
			v = src(j,i);
			total += v;
			dest(j,i) = total;
		}
    }

  for (j=0; j<w; j++)
    {
      total = 0;
      for (i=0; i<h; i++)
		{
		  total += dest(j,i);
		  dest(j,i) = total;
		}
    }
}

void GenerateSum2(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, 
		  YARPImageOf<YarpPixelFloat>& dest2)
{
  int i, j;
  float total, total2, v;
  int h = src.GetHeight();
  int w = src.GetWidth();

  for (i=0; i<h; i++)
    {
      total = 0;
      total2 = 0;
      for (j=0; j<w; j++)
	{
	  v = src(j,i);
	  total += v;
	  total2 += v*v;
	  dest(j,i) = total;
	  dest2(j,i) = total2;
	}
    }

  for (j=0; j<w; j++)
    {
      total = 0;
      total2 = 0;
      for (i=0; i<h; i++)
	{
	  total += dest(j,i);
	  total2 += dest2(j,i);
	  dest(j,i) = total;
	  dest2(j,i) = total2;
	}
    }
}

/* Optimized, deals with src==dest and dx or dy == 0 case */

template <class T>
void Offset(YARPImageOf<T>& src, YARPImageOf<T>& dest, 
	    int dx, int dy, int clean, const T& zero)
{
  int w, h, i, j;
  T *p, *p2;
  int H = src.GetHeight();
  int W = src.GetWidth();
  w = W-abs(dx);
  h = H-abs(dy);
  if (dx<0)
    {
      if (dy>0)
	{
	  for (i=dy; i<H; i++)
	    {
	      p = &src(-dx,i);
	      p2 = &dest(0,i-dy);
	      for (j=-dx; j<W; j++)
		{
		  *p2 = *p;
		  p++;
		  p2++;
		}
	    }
	}
      else
	{
	  for (i=H+dy-1; i>=0; i--)
	    {
	      p = &src(-dx,i);
	      p2 = &dest(0,i-dy);
	      for (j=-dx; j<W; j++)
		{
		  *p2 = *p;
		  p++;
		  p2++;
		}
	    }
	}
    }
  else
    {
      if (dy>0)
	{
	  for (i=dy; i<H; i++)
	    {
	      p = &src(W-dx-1,i);
	      p2 = &dest(W-1,i-dy);
	      for (j=dx; j<W; j++)
		{
		  *p2 = *p;
		  p--;
		  p2--;
		}
	    }
	}
      else
	{
	  for (i=H+dy-1; i>=0; i--)
	    {
	      p = &src(W-dx-1,i);
	      p2 = &dest(W-1,i-dy);
	      for (j=dx; j<W; j++)
		{
		  *p2 = *p;
		  p--;
		  p2--;
		}
	    }
	}      
    }
  if (clean)
    {
      if (dx>0)
	{
	  for (int i=0;i<dx;i++)
	    {
	      for (int j=0; j<H; j++)
		{
		  dest(i,j) = zero;
		}
	    }
	}
      else
	{
	  for (int i=W+dx; i<W; i++)
	    {
	      for (int j=0; j<H; j++)
		{
		  dest(i,j) = zero;
		}
	    }
	}
      if (dy<0)
	{
	  for (int j=0;j<-dy;j++)
	    {
	      for (int i=0; i<W; i++)
		{
		  dest(i,j) = zero;
		}
	    }
	}
      else
	{
	  for (int j=H-dy;j<H;j++)
	    {
	      for (int i=0; i<W; i++)
		{
		  dest(i,j) = zero;
		}
	    }
	}
    }
}

void IntegralImageTool::Offset(YARPImageOf<YarpPixelFloat>& src, 
			       YARPImageOf<YarpPixelFloat>& dest, 
			       int dx, int dy, int clean, float nulval)
{
  SatisfySize(src,dest);	
  ::Offset(src,dest,dx,dy,clean,(YarpPixelFloat)nulval);
}


void IntegralImageTool::Offset(YARPImageOf<YarpPixelMono>& src, 
			       YARPImageOf<YarpPixelMono>& dest, 
			       int dx, int dy, int clean, int nulval)
{
  SatisfySize(src,dest);	
  ::Offset(src,dest,dx,dy,clean,(YarpPixelMono)nulval);
}


void IntegralImageTool::Offset(YARPImageOf<YarpPixelBGR>& src, 
			       YARPImageOf<YarpPixelBGR>& dest, 
			       int dx, int dy, int clean)
{
  SatisfySize(src,dest);
  YarpPixelBGR zero(0,0,0);
  ::Offset(src,dest,dx,dy,clean,zero);
}


void AddOffset(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, 
	       int dx, int dy)
{
  int w, h, i, j;
  float *p, *p2;
  int H = src.GetHeight();
  int W = src.GetWidth();
  w = W-abs(dx);
  h = H-abs(dy);
  if (dx<0)
    {
      if (dy>0)
	{
	  for (i=dy; i<H; i++)
	    {
	      p = &src(-dx,i);
	      p2 = &dest(0,i-dy);
	      for (j=-dx; j<W; j++)
		{
		  *p2 += *p;
		  p++;
		  p2++;
		}
	    }
	}
      else
	{
	  for (i=H+dy-1; i>=0; i--)
	    {
	      p = &src(-dx,i);
	      p2 = &dest(0,i-dy);
	      for (j=-dx; j<W; j++)
		{
		  *p2 += *p;
		  p++;
		  p2++;
		}
	    }
	}
    }
  else
    {
      if (dy>0)
	{
	  for (i=dy; i<H; i++)
	    {
	      p = &src(W-dx-1,i);
	      p2 = &dest(W-1,i-dy);
	      for (j=dx; j<W; j++)
		{
		  *p2 += *p;
		  p--;
		  p2--;
		}
	    }
	}
      else
	{
	  for (i=H+dy-1; i>=0; i--)
	    {
	      p = &src(W-dx-1,i);
	      p2 = &dest(W-1,i-dy);
	      for (j=dx; j<W; j++)
		{
		  *p2 += *p;
		  p--;
		  p2--;
		}
	    }
	}      
    }
}

void SubOffset(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, 
	       int dx, int dy)
{
  int w, h, i, j;
  float *p, *p2;
  int H = src.GetHeight();
  int W = src.GetWidth();
  w = W-abs(dx);
  h = H-abs(dy);
  if (dx<0)
    {
      if (dy>0)
	{
	  for (i=dy; i<H; i++)
	    {
	      p = &src(-dx,i);
	      p2 = &dest(0,i-dy);
	      for (j=-dx; j<W; j++)
		{
		  *p2 -= *p;
		  p++;
		  p2++;
		}
	    }
	}
      else
	{
	  for (i=H+dy-1; i>=0; i--)
	    {
	      p = &src(-dx,i);
	      p2 = &dest(0,i-dy);
	      for (j=-dx; j<W; j++)
		{
		  *p2 -= *p;
		  p++;
		  p2++;
		}
	    }
	}
    }
  else
    {
      if (dy>0)
	{
	  for (i=dy; i<H; i++)
	    {
	      p = &src(W-dx-1,i);
	      p2 = &dest(W-1,i-dy);
	      for (j=dx; j<W; j++)
		{
		  *p2 -= *p;
		  p--;
		  p2--;
		}
	    }
	}
      else
	{
	  for (i=H+dy-1; i>=0; i--)
	    {
	      p = &src(W-dx-1,i);
	      p2 = &dest(W-1,i-dy);
	      for (j=dx; j<W; j++)
		{
		  *p2 -= *p;
		  p--;
		  p2--;
		}
	    }
	}      
    }
}

void MaskOffset(YARPImageOf<YarpPixelFloat>& dest, int delta)
{
  int i, j;
  int H = dest.GetHeight();
  int W = dest.GetWidth();
  for (i=0; i<delta; i++)
    {
      for (j=0; j<W; j++)
	{
	  dest(j,i) = 0;
	  dest(j,H-i-1) = 0;
	}
    }
  for (j=0; j<delta; j++)
    {
      for (i=0; i<H; i++)
	{
	  dest(j,i) = 0;
	  dest(W-j-1,i) = 0;
	}
    }
}

template <class T>
void MaskOffsetT(YARPImageOf<T>& dest, int delta, const T& zero)
{
  int i, j;
  int H = dest.GetHeight();
  int W = dest.GetWidth();
  for (i=0; i<delta; i++)
    {
      for (j=0; j<W; j++)
	{
	  dest(j,i) = zero;
	  dest(j,H-i-1) = zero;
	}
    }
  for (j=0; j<delta; j++)
    {
      for (i=0; i<H; i++)
	{
	  dest(j,i) = zero;
	  dest(W-j-1,i) = zero;
	}
    }
}


// Should optimize this
void AssignDelta(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, 
		 int dx, int dy)
{
  int i, j;
  int H = dest.GetHeight();
  int W = dest.GetWidth();
  for (i=0; i<H; i++)
    {
      for (j=0; j<W; j++)
	{
	  dest(j,i) = 0;
	}
    }
  Offset(src,dest,dx,dy,0,(YarpPixelFloat)0);
}

void TotalFromSum(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, 
		  int delta)
{
  AssignDelta(src,dest,-delta,delta);
  AddOffset(src,dest,delta+1,-delta-1);
  SubOffset(src,dest,-delta,-delta-1);
  SubOffset(src,dest,delta+1,delta);
  MaskOffset(dest,delta);
}



void GenerateVariance(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, 
		      int delta)
{
  float factor = (delta*2+1)*(delta*2+1);
  static YARPImageOf<YarpPixelFloat> sum1;
  static YARPImageOf<YarpPixelFloat> sum2;
  static YARPImageOf<YarpPixelFloat> tot1;
  static YARPImageOf<YarpPixelFloat> tot2;
  SatisfySize(src,sum1);
  SatisfySize(src,sum2);
  SatisfySize(src,tot1);
  SatisfySize(src,tot2);
  GenerateSum2(src,sum1,sum2);
  TotalFromSum(sum1,tot1,delta);
  TotalFromSum(sum2,tot2,delta);
  IMGFOR(dest,j,i)	
	{
	  dest(j,i) = (tot2(j,i)-tot1(j,i)*(tot1(j,i)/factor))/factor;
	}
  //  MakeLog(dest,dest,1);
  //  ApplyThreshold(dest,dest,3000);
}


void GenerateMean(YARPImageOf<YarpPixelFloat>& src, YARPImageOf<YarpPixelFloat>& dest, int delta)
{
  float factor = (delta*2+1)*(delta*2+1);
  static YARPImageOf<YarpPixelFloat> sum1;
  static YARPImageOf<YarpPixelFloat> sum2;
  static YARPImageOf<YarpPixelFloat> tot1;
  static YARPImageOf<YarpPixelFloat> tot2;
  SatisfySize(src,sum1);
  SatisfySize(src,sum2);
  SatisfySize(src,tot1);
  SatisfySize(src,tot2);
  GenerateSum(src,sum1);
  TotalFromSum(sum1,tot1,delta);
  IMGFOR(dest,j,i)
  {
	  dest(j,i) = tot1(j,i)/factor;
  }
  //  MakeLog(dest,dest,1);
  //  ApplyThreshold(dest,dest,3000);
}



void IntegralImageTool::GetMean(YARPImageOf<YarpPixelFloat>& src, 
				YARPImageOf<YarpPixelFloat>& dest, 
				int size)
{
  SatisfySize(src,dest);
  GenerateMean(src,dest,size);
}


void IntegralImageTool::GetVariance(YARPImageOf<YarpPixelFloat>& src, 
				    YARPImageOf<YarpPixelFloat>& dest, 
				    int size)
{
  SatisfySize(src,dest);
  GenerateVariance(src,dest,size);
}


void IntegralImageTool::MaskOffset(YARPImageOf<YarpPixelFloat>& dest, int delta, float zero)
{
  ::MaskOffsetT(dest,delta,zero);
}

void IntegralImageTool::MaskOffset(YARPImageOf<YarpPixelMono>& dest, int delta, int zero)
{
  ::MaskOffsetT(dest,delta,(YarpPixelMono)zero);
}

void IntegralImageTool::MaskOffset(YARPImageOf<YarpPixelBGR>& dest, int delta)
{
  YarpPixelBGR zero(0,0,0);
  ::MaskOffsetT(dest,delta,zero);
}

