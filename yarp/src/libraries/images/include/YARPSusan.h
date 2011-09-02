//
// YARPSusan.h
//

#ifndef __YARPSusanh__
#define __YARPSusanh__

//
//
// options for QNX.
//
//
#ifdef __QNX__
#define PPC 1

#endif

// stripped from original SUSAN code.
//
#ifndef PPC
typedef int        TOTAL_TYPE; /* this is faster for "int" but should be "float" for large d masks */
#else
typedef float      TOTAL_TYPE; /* for my PowerPC accelerator only */
#endif

/*#define FOPENB*/           /* uncomment if using djgpp gnu C for DOS or certain Win95 compilers */
#define SEVEN_SUPP           /* size for non-max corner suppression; SEVEN_SUPP or FIVE_SUPP */
#define MAX_CORNERS   1000   /* it was 15000 - max corners per frame */

/* ********** Leave the rest - but you may need to remove one or both of sys/file.h and malloc.h lines */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __QNX__
#include <sys/file.h>    /* may want to remove this line */
#include <malloc.h>      /* may want to remove this line */
#endif

#define  exit_error(IFB,IFC) { fprintf(stderr,IFB,IFC); exit(0); }
#define  FTOI(a) ( (a) < 0 ? ((int)(a-0.5)) : ((int)(a+0.5)) )

typedef  unsigned char uchar;
struct corner_piece 
{
  int x,y,info, dx, dy, I;
};

typedef corner_piece CORNER_LIST[MAX_CORNERS];

// global susan functions.
void susan_setup_brightness_lut(uchar **bp, int thresh, int form);
void susan_principle(uchar *in, int* r,uchar *bp, int max_no, int x_size, int y_size);
void susan_principle_small(uchar *in, int *r, uchar *bp, int max_no, int x_size, int y_size);
uchar susan_median(uchar *in, int i, int j, int x_size);
void susan_enlarge(uchar **in, uchar *tmp_image, int *x_size, int *y_size, int border);
void susan_smoothing(int three_by_three, uchar *in, float dt, int x_size, int y_size, uchar *bp, uchar *tmp_image, uchar *dp);
void susan_edge_draw(uchar *in, uchar *mid, int x_size, int y_size, int drawing_mode);
void susan_thin(int *r, uchar *mid, int x_size, int y_size);
void susan_edges(uchar *in, int *r, uchar *mid, uchar *bp, int max_no, int x_size, int y_size);
void susan_edges_small(uchar *in, int *r, uchar *mid, uchar *bp, int max_no, int x_size, int y_size);
void susan_corner_draw(uchar *in, CORNER_LIST corner_list, int x_size, int drawing_mode);
void susan_corners(uchar *in, int *r, uchar *bp, int max_no, CORNER_LIST corner_list, int x_size, int y_size, int *cgx, int *cgy);
void susan_corners_quick(uchar *in, int *r, uchar *bp, int max_no, CORNER_LIST corner_list, int x_size, int y_size);
void susan_int_to_uchar(int *r, uchar *in, int size);

// allocation...
uchar * susan_alloc_smooth_tmp_image (int three_by_three, float dt, int x_size, int y_size);
uchar * susan_alloc_smooth_mask (int three_by_three, float dt, int x_size, int y_size);
void susan_alloc_corners_tmp (int **cgx, int **cgy, int x_size, int y_size);


// std YARP stuff.
#include "YARPImage.h"
#include "YARPFilters.h"

enum YARPSusanMode
{
	SUSAN_SMOOTH = 0,
	SUSAN_EDGES = 1,
	SUSAN_CORNERS = 2
};

class YARPSusan : public YARPFilter
{
private:
	YARPSusan (const YARPSusan&);
	void operator= (const YARPSusan&);

protected:
	uchar *bp, *mid;
	uchar *tmp_image;
	uchar *dp;
	float dt;
	int *r;
	int bt;
	int principle;
	int thin_post_proc;
	int three_by_three;
	int drawing_mode;
	int susan_quick;
	int max_no_corners;
	int max_no_edges;
	int mode;
	int x_size, y_size;
	CORNER_LIST corner_list;
	int *cgx, *cgy;

public:
	YARPSusan (int sizex, int sizey)
	{
		// default.
		dt=4.0;
		bt=20;
		principle=0;
		thin_post_proc=1;
		three_by_three=0;
		drawing_mode=0;
		susan_quick=0;
		max_no_corners=1850;
		max_no_edges=2650;
		mode = 0;

		x_size = sizex;
		y_size = sizey;
		bp = NULL;
		mid = NULL;
		r = NULL;
		tmp_image = NULL;
		dp = NULL;
		cgx = NULL;
		cgy = NULL;
	}

	virtual ~YARPSusan () { Cleanup(); }

	virtual void Cleanup (void) { Uninitialize(); }
	virtual bool InPlace () const { return true; }

	void SetMainMode (YARPSusanMode smode) 
	{ 
		mode = (int)smode; 
		// mode = 0 smoothing
		// mode = 1 edges
		// mode = 2 corners
	}

	void SetPrinciple (void) { principle = 1; }
	void UnsetThinningPostprocessing (void) { thin_post_proc = 0; }
	void SetSimpleDrawingMode (void) { drawing_mode = 1; }
	void SetDrawEdgeOnly (void) { drawing_mode = 2; }
	void Set3by3 (void) { three_by_three = 1; }
	void SetQuickMode (void) { susan_quick = 1; }
	void SetDistanceThreshold (float thr) { dt = thr; if (dt < 0) three_by_three = 1; }
	void SetBrightnessThreshold (int thr) { bt = thr; }

	// call this after every params change.
	void InitializeFromParams (void)
	{
		//
		Uninitialize ();

		if ((principle==1) && (mode==0))
			mode = 1;

		switch (mode)
		{
		case 0:
			susan_setup_brightness_lut (&bp, bt, 2);
			tmp_image = susan_alloc_smooth_tmp_image (three_by_three, dt, x_size, y_size);
			dp = susan_alloc_smooth_mask (three_by_three, dt, x_size, y_size);
			break;

		case 1:
			r   = new int[x_size * y_size];
			assert (r != NULL);
			susan_setup_brightness_lut (&bp, bt, 6);

			if (!principle)
			{
				mid = new uchar[x_size * y_size];
				assert (mid != NULL);
			}
			break;

		case 2:
			r   = new int[x_size * y_size];
			assert (r != NULL);
			susan_setup_brightness_lut (&bp, bt, 6);

			susan_alloc_corners_tmp (&cgx, &cgy, x_size, y_size);
			break;
		}
	}

	void Uninitialize (void)
	{
		if (r != NULL)
			delete[] r;
		if (bp != NULL)
			delete[] bp;
		if (mid != NULL)
			delete[] mid;
		if (tmp_image != NULL)
			delete[] tmp_image;
		if (dp != NULL)
			delete[] dp;
		if (cgx != NULL)
			delete[] cgx;
		if (cgy != NULL)
			delete[] cgy;
	}

	virtual void Apply(const YARPImageOf<YarpPixelMono>& is, YARPImageOf<YarpPixelMono>& out)
	{
		assert (is.GetWidth () == x_size && is.GetHeight () == y_size);
		assert (is.GetPadding () == 0 && out.GetPadding() == 0);

		out = is;
		uchar *in = (uchar *)out.GetAllocatedArray();

		switch (mode)
		{
		case 0:
			susan_smoothing (three_by_three, in, dt, x_size, y_size, bp, tmp_image, dp);
			break;

		case 1:
			if (principle)
			{
				if (three_by_three)
					susan_principle_small (in, r, bp, max_no_edges, x_size, y_size);
				else
					susan_principle (in, r, bp, max_no_edges, x_size, y_size);

				susan_int_to_uchar(r,in,x_size*y_size);
			}
			else
			{
				memset (mid, 100, x_size * y_size); /* note not set to zero */

				if (three_by_three)
					susan_edges_small (in, r, mid, bp, max_no_edges, x_size, y_size);
				else
					susan_edges (in, r, mid, bp, max_no_edges, x_size, y_size);

				if (thin_post_proc)
					susan_thin (r, mid, x_size, y_size);

				susan_edge_draw (in, mid, x_size, y_size, drawing_mode);
			}
			break;

		case 2:
			if (principle)
			{
				susan_principle (in, r, bp, max_no_corners, x_size, y_size);
				susan_int_to_uchar (r, in, x_size * y_size);
			}
			else
			{
				if(susan_quick)
					susan_corners_quick (in, r, bp, max_no_corners,corner_list, x_size, y_size);
				else
					susan_corners (in, r, bp, max_no_corners, corner_list, x_size, y_size, cgx, cgy);

				susan_corner_draw (in, corner_list, x_size, drawing_mode);
			}
			break;
		}

		// the result is already in the dest image (I hope).
	}
};


#endif
