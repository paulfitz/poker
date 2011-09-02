
#include "YARPImageHash.h"

// very very stupid hash generator.
NetInt32 YARPImageHash::GetHash(YARPImageOf<YarpPixelBGR>& src)
{
  NetInt32 key = 0;
  int y = src.GetHeight()/2;
  int ent = 0;
  for (int x=src.GetWidth()-10; x>=10 && ent<5; x--)
    {
      YarpPixelBGR& pix = src.SafePixel(x,y);
      if ((pix.r>=10 && pix.r<=200) || x<=15)
	{
	  key *= 17;
	  key += pix.r;
	  key *= 17;
	  key += pix.g;
	  key *= 17;
	  key += pix.b;
	  ent++;
	}
    }
  return key;
}

