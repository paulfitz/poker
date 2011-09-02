//
// YARPTagFilter.h

//

#ifndef __YARPTagFilterh__
#define __YARPTagFilterh__

#include "YARPImage.h"
#include "YARPFilters.h"

//
// it works on YarpPixelMono only.
//
class YARPTagFilter : public YARPFilter 
{
private:
	YARPTagFilter (const YARPTagFilter&);
	void operator= (const YARPTagFilter&);

protected:
	YARPImageOf<YarpPixelMono> sTempImg;
	int m_ntags;

	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);
	void MergeRegions (IplImage *src, int index);

	bool m_exact;
	int m_value;

public:
	YARPTagFilter(void);
	YARPTagFilter(int necc, int nang);
	void Resize (int necc, int nang);

	virtual ~YARPTagFilter() { Cleanup(); } 
	virtual void Cleanup (void);
	virtual bool InPlace (void) { return false; }

	void Apply (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out);
	inline int GetTags (void) const { return m_ntags; }

	inline int GetWidth (void) const { return sTempImg.GetWidth(); }
	inline int GetHeight (void) const { return sTempImg.GetHeight(); }

	// there are 2 modes. Exact value checks for the exact pixel value (e.g. 255).
	// threshold fills the values above the threshold.
	inline void SetExact (int value) 
	{ 
		m_exact = true; 
		m_value = value;
	}

	inline void SetThreshold (int value) 
	{
		m_exact = false;
		m_value = value;
	}
};

#endif