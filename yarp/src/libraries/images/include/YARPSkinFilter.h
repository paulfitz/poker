//
// YARPSkinFilter.h
//		skin color detector filter.
//

#ifndef __YARPSkinFilterh__
#define __YARPSkinFilterh__

#include "YARPImage.h"
#include "YARPFilters.h"

//
// it's not a template because it works only with RGB/BGR images.
class YARPSkinFilter : public YARPFilter 
{
private:
	YARPSkinFilter (const YARPSkinFilter&);
	void operator= (const YARPSkinFilter&);

protected:
	float transformWeights[6];
	float transformDelta;
	float mask[6];

	inline unsigned char SkinOperatorRGB (unsigned char *bgr);
	inline unsigned char SkinOperatorBGR (unsigned char *bgr);

public:
	YARPSkinFilter(void);
	virtual ~YARPSkinFilter() { Cleanup(); }

	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return false; }

	void Apply(const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelMono>& out);
	void Apply(const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelMono>& out);
};

// argument is ptr to red.
inline unsigned char YARPSkinFilter::SkinOperatorRGB (unsigned char *bgr)
{
	unsigned char rawpixel;
	int mask2, r2, b2, g2, r, g, b;
	float judge;

	r = *bgr++;
	g = *bgr++;
	b = *bgr;

	// This mask works pretty well in general
	// mask = (r>g) && (r<3*g) && (r>0.9*b) && (r<3*b) && (r>70);
	// This mask takes advantage of indoors
	// mask = (r>g) && (r<2*g) && (r>0.9*b) && (r<2*b) && (r>70);

	// this is the original   mask = (r>1.1*g) && (r<2*g) && (r>0.9*b) && (r<2*b);
	// this is the original   mask = mask && (r>20) && (r<250);

	mask2 = ((r > mask[0] * g) &&
			(r < mask[1] * g) &&
			(r > mask[2] * b) &&
			(r < mask[3] * b) &&
			(r > mask[4]) &&
			(r < mask[5]));

	if (mask2)
	{
		r2 = r*r;
		g2 = g*g;
		b2 = b*b;
		judge = 
			r*transformWeights[0] +
			g*transformWeights[1] +
			b*transformWeights[2] +
			r2*transformWeights[3] +
			g2*transformWeights[4] +
			b2*transformWeights[5] +
			transformDelta;
		float tmp = judge * 1.5f * 5.0f;
		rawpixel = (tmp > 255) ? 255 : ((tmp < 0) ? 0 : (unsigned char)tmp);
		//rawpixel = Clamp8bit();
	}
	else
	{
		rawpixel = 0;
	}

	return rawpixel;
}

// argument is ptr to red.
inline unsigned char YARPSkinFilter::SkinOperatorBGR (unsigned char *bgr)
{
	unsigned char rawpixel;
	int mask2, r2, b2, g2, r, g, b;
	float judge;

	b = *bgr++;
	g = *bgr++;
	r = *bgr;

	// This mask works pretty well in general
	// mask = (r>g) && (r<3*g) && (r>0.9*b) && (r<3*b) && (r>70);
	// This mask takes advantage of indoors
	// mask = (r>g) && (r<2*g) && (r>0.9*b) && (r<2*b) && (r>70);

	// this is the original   mask = (r>1.1*g) && (r<2*g) && (r>0.9*b) && (r<2*b);
	// this is the original   mask = mask && (r>20) && (r<250);

	mask2 = ((r > mask[0] * g) &&
			(r < mask[1] * g) &&
			(r > mask[2] * b) &&
			(r < mask[3] * b) &&
			(r > mask[4]) &&
			(r < mask[5]));

	if (mask2)
	{
		r2 = r*r;
		g2 = g*g;
		b2 = b*b;
		judge = 
			r*transformWeights[0] +
			g*transformWeights[1] +
			b*transformWeights[2] +
			r2*transformWeights[3] +
			g2*transformWeights[4] +
			b2*transformWeights[5] +
			transformDelta;
		float tmp = judge * 1.5f * 5.0f;
		rawpixel = (tmp > 255) ? 255 : ((tmp < 0) ? 0 : (unsigned char)tmp);
		//rawpixel = Clamp8bit();
	}
	else
	{
		rawpixel = 0;
	}

	return rawpixel;
}

#endif