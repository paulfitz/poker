//
// YARPTracker.h
//	= simple and dummy cross-correlation based tracker.
//

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPBlurFilter.h"

template <class T>
class YARPTracker : public YARPFilter
{
private:
	YARPTracker (const YARPTracker&);
	void operator= (const YARPTracker&);

protected:
	enum ETrackerStatus { YARP_Tracking, YARP_NoTracking };
	enum { STOPTHR = 50 };

	YARPImageOf<T> m_template;		// blurred template.
	YARPImageOf<T> m_prev_frame;	// previous frame.
	YARPImageOf<T> m_tmp_frame;

	YARPBlurFilter<T> m_blur;

	int m_sizex, m_sizey;			// size of the image.
	int m_tsizex, m_tsizey;			// size of the template.

	int m_searchrangex;				// search range.
	int m_searchrangey;

	bool m_smooth;					// spatial smooth.
	bool m_tempsmooth;				// temporal smooth.

	double m_lambda;				// t smooth lambda.
	int m_zerothr;					// no motion.
	double m_varthr;				// no enough variance to compute thr.

	int m_px;						// recovered/previous position.
	int m_py;
	int m_oldx;
	int m_oldy;

	double m_variance;				// the variance of the template.
	int m_stopcnt;					// counter for stopping tracker.
	
	ETrackerStatus m_status;		// internal status.

	double _cmpblock (unsigned char *p1, unsigned char *p2);
	double _cmpblock2 (unsigned char *p1, unsigned char *p2);
	void _tfilter (void);
	void _computetrack (const YARPImageOf<T>& curframe, bool update);
	double _variance (unsigned char *b);

public:
	YARPTracker (int x, int y, int tsizex, int tsizey, int rangex, int rangey, int zt);
	virtual ~YARPTracker ();

	virtual void Cleanup (void) {}
	virtual bool InPlace () const { return true; }

	int Initialize (const YARPImageOf<T>& firstframe, int x, int y);
	void Uninitialize (void);

	void SetSpatialSmooth (void) { m_smooth = true; }
	void SetTemporalSmooth (double l) { m_tempsmooth = true; m_lambda = l; }
	void SetVarianceThr (double t) { m_varthr = t; }

	bool IsTracking (void) const { return (m_status == YARP_Tracking)?true:false; }

	void Apply (const YARPImageOf<T>& curframe, bool update, int& ovx, int& ovy, bool& valid);
};

template <class T>
YARPTracker<T>::YARPTracker (int x, int y, int tsizex, int tsizey, int rangex, int rangey, int zt) 
	: YARPFilter(),
	  m_blur (x, y)
{
	m_lambda = 0.7;
	m_sizex = x;
	m_sizey = y;

	m_tsizex = tsizex;
	m_tsizey = tsizey;

	m_smooth = false;		// spat smooth.
	m_tempsmooth = false;	// temp smooth.

	m_prev_frame.Resize (m_sizex, m_sizey);
	m_tmp_frame.Resize (m_sizex, m_sizey);
	
	m_template.Resize (m_tsizex, m_tsizey);
	m_template.Zero();

	m_searchrangex = rangex;
	m_searchrangey = rangey;
	m_zerothr = zt;
	m_varthr = 0.0;

	m_px = m_py = m_sizex/2;
	m_oldx = m_oldy = m_sizex/2;

	m_status = YARP_NoTracking;
	m_variance = 0;
	m_stopcnt = 0;
}

template <class T>
YARPTracker<T>::~YARPTracker ()
{
	Cleanup ();
}

template <class T>
void YARPTracker<T>::Cleanup ()
{
	m_prev_frame.Cleanup ();
	m_tmp_frame.Cleanup ();
	m_template.Cleanup ();
}

// x,y are the coordinates of the center of the template.
// internally px,py are the upper-left point.
template <class T>
int YARPTracker<T>::Initialize (const YARPImageOf<T>& templateframe, int x, int y)
{
	SatisfySize ((YARPImageOf<T>&)templateframe, m_prev_frame);

	if (m_smooth)
	{
		m_blur.Apply (templateframe, m_prev_frame);
	}
	else
	{
		m_prev_frame = templateframe;
	}

	// extract template from frame and copy it into m_template.
	// use crop or paste?
	unsigned char *c = (unsigned char *)templateframe.GetAllocatedArray();
	m_px = x - m_tsizex/2;
	m_py = y - m_tsizey/2;

	if (m_px < 0)
	{
		m_px = 0;
		printf("warning : initialize was outside the image\n");
	}
	if (m_py < 0)
	{
		m_py = 0;
		printf("warning : initialize was outside the image\n");
	}
	if (m_px + m_tsizex >= m_sizex)
	{
		m_px = m_sizex - m_tsizex - 1;
		printf("warning : initialize was outside the image\n");
	}
	if (m_py + m_tsizey >= m_sizey)
	{
		m_py = m_sizey - m_tsizey - 1;
		printf("warning : initialize was outside the image\n");
	}

	m_variance = _variance(c+(m_px + m_py * m_sizex) * sizeof(T));

	m_oldx = m_px;	// used to estimate future position...
	m_oldy = m_py;

	if (m_variance <= m_varthr)
	{
		printf ("YARPTracker - not enough variance to track %f in %d %d!\n", m_variance, x, y);
		m_status = YARP_NoTracking;
		m_px = m_py = m_sizex / 2;
		m_template.Zero();
		m_variance = 0;
		return -1;
	}

	m_status = YARP_Tracking;
	m_prev_frame.Crop (m_template, m_px, m_py);
	m_stopcnt = 0;

	return 0;
}

template <class T>
void YARPTracker<T>::Uninitialize ()
{
}

template <class T>
void YARPTracker<T>::_tfilter (void)
{
	// frames are m_tmp_frame and m_prev_frame.
	const int size = m_tmp_frame.GetWidth() * m_tmp_frame.GetHeight();
	unsigned char *c = (unsigned char *)m_tmp_frame.GetAllocatedArray();
	unsigned char *o = (unsigned char *)m_prev_frame.GetAllocatedArray();

	for (int i = 0; i < size; i++)
	{
		*c = (unsigned char)(m_lambda * *c + (1 - m_lambda)* *o++);
		c++;
	}
}


template <class T>
double YARPTracker<T>::_cmpblock (unsigned char *p1, unsigned char *p2)
{
	// SSD?
	// p1 is an image.
	// p2 is the template. 
	double accu = 0;
	for (int i = 0; i < m_tsizey; i++)
	{
		for (int j = 0; j < m_tsizex * sizeof(T); j++, p1++, p2++)
		{
			accu += ((*p1 - *p2) * (*p1 - *p2));
		}

		p1 += ((m_sizex - m_tsizex) * sizeof(T));
		// p2 is not incremented because it's already tsizex, tsizey.
	}

	return accu / (m_tsizex * m_tsizey * sizeof(T));
}

// weight window.
template <class T>
double YARPTracker<T>::_cmpblock2 (unsigned char *p1, unsigned char *p2)
{
	// SSD?
	// p1 is an image.
	// p2 is the template. 
	double accu = 0;
	for (int i = 0; i < m_tsizey; i++)
	{
		for (int j = 0; j < m_tsizex * sizeof(T); j++, p1++, p2++)
		{
			accu -= (*p1 * *p2);
		}

		p1 += ((m_sizex - m_tsizex) * sizeof(T));
		// p2 is not incremented because it's already tsizex, tsizey.
	}

	return accu / (m_tsizex * m_tsizey * sizeof(T));
}

template <class T>
void YARPTracker<T>::Apply (const YARPImageOf<T>& curframe, bool update, int& ovx, int& ovy, bool& valid)
{
	_computetrack (curframe, update);

	if (IsTracking())
	{
		ovx = m_px + m_tsizex/2;
		ovy = m_py + m_tsizey/2;
		valid = true;
	}
	else
	{
		ovx = m_sizex / 2;
		ovy = m_sizey / 2;
		valid = false;
	}
}

// it's not ok to compute the variance of the template with this.
// there should be a -1 somewhere to make the estimate unbiased, although
// it doesn't really make a difference here.

template <class T>
double YARPTracker<T>::_variance (unsigned char *b)
{
	double average[3];
	double variance[3];
	double ret = 0;
	unsigned char *p1 = b;

	memset (average, 0, 3 * sizeof(double));
	memset (variance, 0, 3 * sizeof(double));

	switch (sizeof(T))
	{
	case 1:
		{
			for (int i = 0; i < m_tsizey; i++)
			{
				for (int j = 0; j < m_tsizex; j++, p1++)
				{
					average[0] += *p1;
					variance[0] += (*p1 * *p1);
				}

				p1 += ((m_sizex - m_tsizex));
			}
			average[0] /= (m_tsizex * m_tsizey);
			variance[0] /= (m_tsizex * m_tsizey);
			ret = variance[0] - average[0] * average[0];
		}
		break;

	case 3:
		{
			for (int i = 0; i < m_tsizey; i++)
			{
				for (int j = 0; j < m_tsizex; j++)
				{
					for (int k = 0; k < 3; k++)
					{
						average[k] += *p1;
						variance[k] += (*p1 * *p1);
						p1++;
					}
				}

				p1 += ((m_sizex - m_tsizex) * 3);
			}

			double v;
			ret = 0;
			for (int k = 0; k < 3; k++)
			{
				average[k] /= (m_tsizex * m_tsizey);
				variance[k] /= (m_tsizex * m_tsizey);
				v = variance[k] - average[k] * average[k];
				ret += v;
				//if (v > ret)
				//	ret = variance[k];
			}		
			ret /= 3;
		}
		break;
	}

	//printf ("%f\n", variance - average * average);
	return ret;
}

template <class T>
void YARPTracker<T>::_computetrack (const YARPImageOf<T>& curframe, bool update)
{
	assert (curframe.GetWidth () == m_sizex && curframe.GetHeight() == m_sizey);
	assert (curframe.GetPadding () == 0);
	SatisfySize ((YARPImageOf<T>&)curframe, m_prev_frame);


	// NEED TO BE TRACKING ALREADY.
	if (m_status != YARP_Tracking)
	{
		printf ("YARPTracker - probably not initialized or already stopped\n");
		return;
	}

	// FILTERING.
	if (m_smooth)
	{
		m_blur.Apply (curframe, m_tmp_frame);
	}
	else
	{
		m_tmp_frame = curframe;
	}

	// result in m_tmp_frame anyway.
	if (m_tempsmooth)
	{
		_tfilter ();
	}
	// results here is always in m_tmp_frame.


	// SEARCH USING CURRENT TEMPLATE.
	// block matching between curframe and prevframe.
	// tmp contains the current filtered frame.
	// prev is the old one.
	unsigned char *c = (unsigned char *)m_tmp_frame.GetAllocatedArray();
	unsigned char *t = (unsigned char *)m_template.GetAllocatedArray();
	int pixx, pixy;
	int minx, maxx;
	int miny, maxy;

	// search starting point is always the old px, py.
	// at this point it's not changed yet.
	// px, py represent the upper-left corner of the block.

	c += ((m_px + m_py * m_sizex) * sizeof(T));		// current image.

	// actual displacement of template region from px, py.
	int vx = 0;
	int vy = 0;

	// check within range.
	pixx = m_px; // + (m_px-m_oldx);
	pixy = m_py; // + (m_py-m_oldy);
	m_oldx = m_px;
	m_oldy = m_py;
	if (pixx - m_searchrangex < 0)
	{
		minx = pixx;
		maxx = m_searchrangex;
	}
	else
	if (pixx + m_searchrangex > m_sizex - m_tsizex)
	{
		minx = m_searchrangex;
		maxx = m_sizex - pixx - m_tsizex;
	}
	else
	{
		minx = m_searchrangex;
		maxx = m_searchrangex;
	}

	if (pixy - m_searchrangey < 0)
	{
		miny = pixy;
		maxy = m_searchrangey;
	}
	else
	if (pixy + m_searchrangey > m_sizey - m_tsizey)
	{
		miny = m_searchrangey;
		maxy = m_sizey - pixy - m_tsizey;
	}
	else
	{
		miny = m_searchrangey;
		maxy = m_searchrangey;
	}

	double tmpssd;
	double min_ssd = _cmpblock (c, t);
	for (int dy = -miny; dy <= maxy; dy++)
		for (int dx = -minx; dx <= maxx; dx++)
		{
			// limits are correct.
			tmpssd = _cmpblock (c + dx * sizeof(T) + dy * m_sizex * sizeof(T), t);
			if (tmpssd < min_ssd)
			{
				min_ssd = tmpssd;
				vx = dx;
				vy = dy;
			}
		}


	// UPDATE TEMPLATE (AND POSITION) OR TERMINATE TRACKING.
	// check also the variance of the destination block.
	int newpx = m_px + vx;
	int newpy = m_py + vy;
	double variance = _variance (c + (newpx + newpy * m_sizex) * sizeof(T));

	if (variance <= m_varthr)
	{
		printf ("YARPTracker - not enough variance to continue tracking %f\n", variance);
		m_template.Zero();
		m_px = m_py = m_sizex / 2;
		m_status = YARP_NoTracking;
		return;
	}

	// if variance changed more than a given threshold 
	// we might want to stop the tracker?

	// if new position didn't change...
	// count a few consecutive cycles and then stop the tracker?
	if (vx == 0 && vy == 0)
	{
		m_stopcnt++;
		if (m_stopcnt > STOPTHR)
		{
			printf ("YARPTracker - no movement for %d steps, stopping\n", STOPTHR);
			m_template.Zero();
			m_px = m_py = m_sizex / 2;
			m_status = YARP_NoTracking;
			return;
		}
	}
	else
	{
		m_stopcnt = 0;
	}

	m_status = YARP_Tracking;
	m_px = newpx;
	m_py = newpy;

	// update template...
	if (update)
		m_tmp_frame.Crop (m_template, m_px, m_py);

	m_prev_frame = m_tmp_frame;
}
