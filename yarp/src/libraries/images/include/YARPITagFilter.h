//
// YARPITagFilter.h

//

#ifndef __YARPITagFilterh__
#define __YARPITagFilterh__

#include "YARPImage.h"
#include "YARPFilters.h"

//
// it works on YarpPixelMono only.
//	- call GetTagged to access the internal tagged image (integer).
//
class YARPITagFilter : public YARPFilter 
{
private:
	YARPITagFilter (const YARPITagFilter&);
	void operator= (const YARPITagFilter&);

protected:
	int ** sTempImg;
	int m_ntags;

	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);
	void MergeRegions (int index);

	bool m_exact;
	int m_value;

	int m_necc, m_nang;

public:
	YARPITagFilter(void);
	YARPITagFilter(int necc, int nang);
	void Resize (int necc, int nang);

	virtual ~YARPITagFilter() { Cleanup(); } 
	virtual void Cleanup (void);
	virtual bool InPlace (void) { return false; }

	void Apply (const YARPImageOf<YarpPixelMono>& in);
	inline int ** GetTagged (void) const { return sTempImg; }
	inline int GetTags (void) const { return m_ntags; }

	inline int GetWidth (void) const { return m_necc; }
	inline int GetHeight (void) const { return m_nang; }

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