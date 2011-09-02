//

#ifndef __YARPColorSaliencyh__
#define __YARPColorSaliencyh__

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPColorConverter.h"

// in RGB image.
// out saliency as a function of saturation.

class YARPColorSaliencyFilter : public YARPFilter
{
private:
	int m_ecc, m_ang;
	int m_threshold;
	int m_luma_thr;

public:
	YARPColorSaliencyFilter (int necc, int nang, int thr)
	{
		m_ecc = necc;
		m_ang = nang;
		m_threshold = thr;
		m_luma_thr = thr;
	}

	virtual ~YARPColorSaliencyFilter () 
		{ Cleanup(); }

	virtual void Cleanup (void) {}
	virtual bool InPlace (void) const { return true; }

	void Apply(const YARPImageOf<YarpPixelBGR>& in, YARPImageOf<YarpPixelHSV>& out);
	void Apply(const YARPImageOf<YarpPixelRGB>& in, YARPImageOf<YarpPixelHSV>& out);

	inline int GetSaturationThreshold (void) const { return m_threshold; }
	inline void SetSaturationThreshold (int thr) { m_threshold = thr; }
	inline int GetLumaThreshold (void) const { return m_luma_thr; }
	inline void SetLumaThreshold (int thr) { m_luma_thr = thr; }

	inline int GetWidth () const { return m_ecc; }
	inline int GetHeight () const { return m_ang; }
};

#endif