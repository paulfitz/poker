//
// YARPTemporalSmooth.h
//
// Causal temporal smoothing.
//

#ifndef __YARPTemporalSmoothh__
#define __YARPTemporalSmoothh__

#include "YARPImage.h"
#include "YARPFilters.h"

//
// temporal smoothing res(t+1)=l*img(t)+(1-l)*res(t).    l=lambda
//
class YARPTemporalSmooth : public YARPFilter
{
private:
	YARPTemporalSmooth (const YARPTemporalSmooth&);
	void operator= (const YARPTemporalSmooth&);

protected:
	double lCoeff;
	YARPImageOf<YarpPixelMono>  m_old;
	
public:
	YARPTemporalSmooth(void);
	YARPTemporalSmooth(const YARPImageOf<YarpPixelMono>& first, double lambda);

	virtual ~YARPTemporalSmooth(void) { Cleanup(); }
	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return true; }

	void Resize(const YARPImageOf<YarpPixelMono>& first, double lambda);
	void Init(const YARPImageOf<YarpPixelMono>& first, double lambda);

	void Apply(const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out);

	inline double GetLambda(void) { return lCoeff;}
};

#endif