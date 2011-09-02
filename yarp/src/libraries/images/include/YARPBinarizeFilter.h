//
// YARPBinarizeFilter.h
//

#ifndef __YARPBinarizeFilterh__
#define __YARPBinarizeFilterh__

#include "YARPFilters.h"

//
// binarize image.
//
class YARPBinarizeFilter : public YARPFilterOf<YarpPixelMono>
{
private:
	YARPBinarizeFilter (const YARPBinarizeFilter&);
	void operator= (const YARPBinarizeFilter&);

protected:
	int low,hi;
	bool notSingle;

public:
	YARPBinarizeFilter(void) : YARPFilterOf<YarpPixelMono>()
	{
		low = 0;
		hi = 0;
		notSingle=false;
	}

	YARPBinarizeFilter(int limit) : YARPFilterOf<YarpPixelMono>()
	{
		low = limit;
		hi = 0;
		notSingle = false;
	}

	YARPBinarizeFilter(int l,int h) : YARPFilterOf<YarpPixelMono>()
	{
		low = l;
		hi = h;
		notSingle = true;	
	}

	virtual ~YARPBinarizeFilter() {}
	virtual void Cleanup (void) {}

    void SetThresholds(int l,int h)
	{
		low=l;
		hi=h;
		notSingle=true;	
	}

	void SetThresholds(int limit)
	{
		low=limit;
		hi=0;
		notSingle=false;		
	}

	void Apply(const YARPImageOf<YarpPixelMono>& is, YARPImageOf<YarpPixelMono>& id);
	virtual bool InPlace () const { return true; }
};

#endif