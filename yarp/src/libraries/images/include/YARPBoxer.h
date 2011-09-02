//
// YARPBoxer.h
//

#ifndef __YARPLpBoxerh__
#define __YARPLpBoxerh__

#include "YARPImage.h"
#include "YARPFilters.h"
//#include "YARPTagFilter.h"
#include "YARPITagFilter.h"
#include "YARPBlobFinder.h"

// create boxes on logpolar image, store them internally for future use.
class YARPLpBoxer : public YARPFilter 
{
private:
	YARPLpBoxer (const YARPLpBoxer& x);
	void operator= (const YARPLpBoxer& x);

protected:
	YARPITagFilter m_tagger;
	YARPBox *m_boxes;
	YARPBox *m_attn;

	YARPLogPolar m_lp;

	YARPImageOf<YarpPixelMono> sTempImg;

	int m_last_tag;

	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);

	void MergeBoxes (void);
	double CenterDistance (int i, int j);
	double CombinedSize (int i, int j);
	void FuseBoxes (int i, int j);
	void DrawBox (IplImage *img, int x1, int y1, int x2, int y2);
	void DrawBox (	IplImage *img, 
					int x1, int y1, 
					int x2, int y2,
					int r, int g, int b);
	void RemoveNonValid (void);

	inline bool Overlap (int i, int j);

public:
	YARPLpBoxer (int necc, int nang, double rfmin, int size);
	virtual ~YARPLpBoxer () { Cleanup(); }
	virtual void Cleanup (void);

	virtual bool InPlace (void) const { return true; }

	void Apply(const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out, bool saveout=true);
	void Apply(const YARPImageOf<YarpPixelMono>& in);

	inline int GetWidth (void) const { return m_tagger.GetWidth(); }
	inline int GetHeight (void) const { return m_tagger.GetHeight(); }

	YARPBox * GetBoxes(void) { return m_attn; }

	void DrawBoxes (YARPImageOf<YarpPixelRGB>& id);
	void DrawBoxes (YARPImageOf<YarpPixelBGR>& id);
	void DrawBoxes (YARPImageOf<YarpPixelMono>& id);

	inline void SetExact (int value) { m_tagger.SetExact (value); }
	inline void SetThreshold (int value) { m_tagger.SetThreshold (value); }
};

//
// overlap is in pixels.
inline bool YARPLpBoxer::Overlap (int i, int j)
{
	YARPBox* in1 = m_boxes + i;
	YARPBox* in2 = m_boxes + j;

	const int overlap = 3;

	if (in2->xmin < in1->xmax + overlap &&
		in2->xmin > in1->xmin - overlap &&
		in2->ymin < in1->ymax + overlap &&
		in2->ymin > in1->ymin - overlap)
		return true;
	else
	if (in2->xmin < in1->xmax + overlap &&
		in2->xmin > in1->xmin - overlap &&
		in2->ymax < in1->ymax + overlap &&
		in2->ymax > in1->ymin - overlap)
		return true;
	else
	if (in2->xmax < in1->xmax + overlap &&
		in2->xmax > in1->xmin - overlap &&
		in2->ymax < in1->ymax + overlap &&
		in2->ymax > in1->ymin - overlap)
		return true;
	else
	if (in2->xmax < in1->xmax + overlap &&
		in2->xmax > in1->xmin - overlap &&
		in2->ymin < in1->ymax + overlap &&
		in2->ymin > in1->ymin - overlap)
		return true;

	return false;
}

#endif