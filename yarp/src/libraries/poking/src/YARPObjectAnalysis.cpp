
#include "YARPObjectAnalysis.h"
#include "YARPShapeBoundary.h"
#include "YARPNetworkTypes.h"
#include "YARPImageDraw.h"
#include "YARPImageLabel.h"

#include <map>

#ifdef __QNX__
#define for if (1) for
#endif

#define DEFAULT_SHRINK (0.75)

typedef map<NetInt32, int, less<NetInt32> > ColorMap;


static NetInt32 HashColor(const YarpPixelBGR& pix)
{
  NetInt32 x = 256;
  return (pix.r*x+pix.g)*x+pix.b;
}



extern int segm_apply(YARPImageOf<YarpPixelBGR>& src,YARPImageOf<YarpPixelBGR>& dest);

void YARPObjectAnalysis::ColorSegment(YARPImageOf<YarpPixelBGR>& src,
				      YARPImageOf<YarpPixelBGR>& dest)
{
  segm_apply(src,dest);
}

YARPObjectAnalysis::YARPObjectAnalysis()
{
  shrink = DEFAULT_SHRINK;
}

void YARPObjectAnalysis::SetShrinkFactor(float factor)
{
  if (factor < 0)
    {
      shrink = DEFAULT_SHRINK;
    }
  else
    {
      shrink = factor;
    }
}

float YARPObjectAnalysis::SpotInternals(YARPImageOf<YarpPixelBGR>& src_seg,
					YARPImageOf<YarpPixelMono>& in_mask,
					YARPImageOf<YarpPixelMono>& out_mask,
					YARPImageOf<YarpPixelBGR>& dest)
{
  YARPShapeBoundary sb;
  sb.SetFromMask(in_mask);
  sb.ScaleAboutCenter(shrink);
  YARPImageOf<YarpPixelMono> mask_small;
  SatisfySize(src_seg,mask_small);
  sb.GetMask(mask_small);
  ColorMap color_map, color_map2, color_id;
  YARPImageOf<YarpPixelInt> cluster0, cluster1;
  SatisfySize(src_seg,cluster0);
  SatisfySize(src_seg,cluster1);

  int iid = 1;
  IMGFOR(src_seg,x,y)
    {
      NetInt32 addr = HashColor(src_seg(x,y));
      ColorMap::iterator it = color_id.find(addr);
      
      if (it == color_id.end())
	{
	  color_id[addr] = iid;
	  it = color_id.find(addr);
	  iid++;
	}
      cluster0(x,y) = (*it).second;
    }
  YARPImageLabel label;
  label.ApplySimilarity(cluster0,cluster1);

  int ref_ct = 0;
  float in_quality = 0;
  IMGFOR(src_seg,x,y)
    {
      if (in_mask(x,y))
	{
	  in_quality++;
	  NetInt32 addr = cluster1(x,y);
	  ColorMap::iterator it = color_map.find(addr);
	  
	  if (it == color_map.end())
	    {
	      color_map[addr] = 0;
	      color_map2[addr] = 0;
	      it = color_map.find(addr);
	    }
	  (*it).second++;
	  if (!mask_small(x,y))
	    {
	      ref_ct++;
	      color_map2[addr]++;
	    }
	}
    }
  YARPImageOf<YarpPixelMono> mono;
  mono.CastCopy(dest);

  SatisfySize(in_mask,out_mask);
  out_mask.Zero();

  IMGFOR(dest,x,y)
    {
      dest(x,y).r = dest(x,y).g = dest(x,y).b = mono(x,y)/2;
    }
  float quality = 0;
  for (ColorMap::iterator it = color_map.begin(); it!=color_map.end(); it++)
    {
      NetInt32 key = (*it).first;
      int all = (*it).second;
      int outer = color_map2[key];
      //printf("for color %ld get %d/%d ", key, all, outer);
      if (outer<5 && all>40)
	{
	  //printf(" ***");
	  IMGFOR(dest,x,y)
	    {
	      if (cluster1(x,y)==key)
		{
		  dest(x,y).g = 255;
		  out_mask(x,y) = 255;
		  quality++;
		}
	    }
	}
      //printf("\n");
    }
  //printf("reference count is %d\n",ref_ct);
  sb.Show(dest,0);

  if (in_quality<10) in_quality = 10;
  quality /= in_quality;

  return quality;
}


