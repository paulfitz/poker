//
// YARPOpticFlowBM.h
//


#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPBlurFilter.h"


#define OOVERFLOW 100000


template <class T>
class YARPOpticFlowBM : public YARPFilter
{
private:
	YARPOpticFlowBM (const YARPOpticFlowBM&);
	void operator= (const YARPOpticFlowBM&);

protected:
	YARPImageOf<T> m_prev_frame;
	YARPImageOf<T> m_tmp_frame;
	YARPBlurFilter<T> m_blur;

	int m_sizex, m_sizey;

	int m_blocksize;
	int m_blockincrement;

	int m_nblocksx;
	int m_nblocksy;

	int m_searchrange;

	bool m_smooth;
	bool m_tempsmooth;

	double m_lambda;
	int m_zerothr;
	double m_varthr;

	int **vx;
	int **vy;

	double _cmpblock (unsigned char *p1, unsigned char *p2);
	void _tfilter (void);
	void _fancydisplay (YARPImageOf<T>& out, int **vx, int **vy);
	void _computeflow (const YARPImageOf<T>& curframe);
	void _computeflow (const YARPImageOf<T>& curframe, const YARPImageOf<YarpPixelMono>& mask, int px, int py);
	double _variance (unsigned char *b);

public:
	YARPOpticFlowBM (int x, int y, int blocksize, int range, int zt, int binc = 0);
	virtual ~YARPOpticFlowBM ();

	virtual void Cleanup (void) {}
	virtual bool InPlace () const { return true; }

	void Initialize (const YARPImageOf<T>& firstframe);
	void Uninitialize (void);

	void GetNBlocks (int& x, int& y) const { x = m_nblocksx; y = m_nblocksy; }
	void SetSpatialSmooth (void) { m_smooth = true; }
	void SetTemporalSmooth (double l) { m_tempsmooth = true; m_lambda = l; }
	void SetVarianceThr (double t) { m_varthr = t; }

	void Apply (const YARPImageOf<T>& curframe, int *ovx, int *ovy);
	void Apply (const YARPImageOf<T>& curframe, YARPImageOf<T>& out, int *ovx, int *ovy);
	void Apply (const YARPImageOf<T>& curframe, const YARPImageOf<YarpPixelMono>& mask, int px, int py, YARPImageOf<T>& out, int *ovx, int *ovy);

	int L(void) const;

	void DrawFlow (YARPImageOf<YarpPixelBGR>& out);
	void DrawFlow (YARPImageOf<YarpPixelRGB>& out);
	void DrawFlow (YARPImageOf<YarpPixelMono>& out);
};


template <class T>
void YARPOpticFlowBM<T>::DrawFlow (YARPImageOf<YarpPixelBGR>& out)
{
	YarpPixelBGR pix;
	memset (&pix, 0, sizeof (YarpPixelBGR));
	YarpPixelBGR white;
	memset (&white, 255, sizeof(YarpPixelBGR));
	YarpPixelBGR red;
	red.g=red.b=0;
	red.r = 255;

	for (int j = 0; j < m_nblocksy; j++)
	{
		for (int i = 0; i < m_nblocksx; i++)
		{
			int cx = i * m_blockincrement + m_blocksize/2;
			int cy = j * m_blockincrement + m_blocksize/2;

			// don't draw if it's outside the specified range.
			if (vx[j][i] <= OOVERFLOW &&
				vy[j][i] <= OOVERFLOW)
			{
				// draw arrow.
				const int RES = m_searchrange;
				const double inc = 1.0 / RES;
				int x,y;
				for (int k = 0; k < RES; k++)
				{
					x = int(cx + vx[j][i] * k * inc + .5);
					y = int(cy + vy[j][i] * k * inc + .5);
					if (x >= 0 && x < m_sizex && y >= 0 && y < m_sizey)
						out (x,y) = pix;
				}

				out(cx,cy) = white;
			}
			else
			{
				if (vx[j][i] == OOVERFLOW+2 ||
					vy[j][i] == OOVERFLOW+2)
				{
					//for (int k = -1; k <= 1; k++)
					//	for (int l = -1; l <= 1; l++)
					out(cx,cy) = red;
				}
			}
		}
	}
}


template <class T>
void YARPOpticFlowBM<T>::DrawFlow (YARPImageOf<YarpPixelRGB>& out)
{
	YarpPixelRGB pix;
	memset (&pix, 0, sizeof (YarpPixelRGB));
	YarpPixelRGB white;
	memset (&white, 255, sizeof(YarpPixelRGB));

	for (int j = 0; j < m_nblocksy; j++)
	{
		for (int i = 0; i < m_nblocksx; i++)
		{
			int cx = i * m_blockincrement + m_blocksize/2;
			int cy = j * m_blockincrement + m_blocksize/2;

			// don't draw if it's outside the specified range.
			if (vx[j][i] <= OOVERFLOW &&
				vy[j][i] <= OOVERFLOW)
			{
				// draw arrow.
				const int RES = m_searchrange;
				const double inc = 1.0 / RES;
				int x,y;
				for (int k = 0; k < RES; k++)
				{
					x = int(cx + vx[j][i] * k * inc + .5);
					y = int(cy + vy[j][i] * k * inc + .5);
					if (x >= 0 && x < m_sizex && y >= 0 && y < m_sizey)
						out (x,y) = pix;
				}
			}
			else
			{
				if (vx[j][i] == OOVERFLOW+2 ||
					vy[j][i] == OOVERFLOW+2)
				{
					for (int k = -2; k <= 2; k++)
						for (int l = -2; l <= 2; l++)
							out(cx+l,cy+k) = pix;
				}
			}

			out(cx,cy) = white;
		}
	}
}


template <class T>
void YARPOpticFlowBM<T>::DrawFlow (YARPImageOf<YarpPixelMono>& out)
{
	YarpPixelMono pix = 0;
	YarpPixelMono white = 255;

	for (int j = 0; j < m_nblocksy; j++)
	{
		for (int i = 0; i < m_nblocksx; i++)
		{
			int cx = i * m_blockincrement + m_blocksize/2;
			int cy = j * m_blockincrement + m_blocksize/2;

			// don't draw if it's outside the specified range.
			if (vx[j][i] <= OOVERFLOW &&
				vy[j][i] <= OOVERFLOW)
			{
				// draw arrow.
				const int RES = m_searchrange;
				const double inc = 1.0 / RES;
				int x,y;
				for (int k = 0; k < RES; k++)
				{
					x = int(cx + vx[j][i] * k * inc + .5);
					y = int(cy + vy[j][i] * k * inc + .5);
					if (x >= 0 && x < m_sizex && y >= 0 && y < m_sizey)
						out (x,y) = pix;
				}
			}
			else
			{
				if (vx[j][i] == OOVERFLOW+2 ||
					vy[j][i] == OOVERFLOW+2)
				{
					for (int k = -2; k <= 2; k++)
						for (int l = -2; l <= 2; l++)
							out(cx+l,cy+k) = pix;
				}
			}

			out(cx,cy) = white;
		}
	}
}


template <class T>
YARPOpticFlowBM<T>::YARPOpticFlowBM (int x, int y, int blocksize, int range, int zt, int binc) 
	: YARPFilter(),
	  m_blur (x, y)
{
	assert ((x % blocksize) == 0 && (y % blocksize) == 0);
	assert ((x % binc) == 0 && (y % binc) == 0);
	assert (blocksize >= binc);
	assert ((blocksize % binc) == 0);

	m_lambda = 0.7;
	m_sizex = x;
	m_sizey = y;

	m_blocksize = blocksize;

	if (binc == 0)
		m_blockincrement = m_blocksize;
	else
		m_blockincrement = binc;
	
	//m_nblocksx = x / m_blockincrement;
	//m_nblocksy = y / m_blockincrement;

	//
	int blockinsize = blocksize / binc;
	m_nblocksx = x / m_blockincrement - blockinsize + 1;
	m_nblocksy = y / m_blockincrement - blockinsize + 1;

	m_smooth = false;
	m_tempsmooth = false;

	vx = new int *[m_nblocksy];
	vy = new int *[m_nblocksy];
	assert (vx != NULL && vy != NULL);
	vx[0] = new int[m_nblocksy * m_nblocksx];
	vy[0] = new int[m_nblocksy * m_nblocksx];
	assert (vx[0] != NULL && vy[0] != NULL);
	for (int i = 1; i < m_nblocksy; i++)
	{
		vx[i] = vx[i-1] + m_nblocksx;
		vy[i] = vy[i-1] + m_nblocksx;
	}

	m_prev_frame.Resize (m_sizex, m_sizey);
	m_tmp_frame.Resize (m_sizex, m_sizey);

	m_searchrange = range;
	m_zerothr = zt;
	m_varthr = 0.0;
}

template <class T>
YARPOpticFlowBM<T>::~YARPOpticFlowBM ()
{
	Cleanup ();
}

template <class T>
void YARPOpticFlowBM<T>::Cleanup ()
{
	m_prev_frame.Cleanup ();
	m_tmp_frame.Cleanup ();

	if (vx != NULL)
	{
		if (vx[0] != NULL)
			delete[] vx[0];
		delete[] vx;
	}
	if (vy != NULL)
	{
		if (vy[0] != NULL)
			delete[] vy[0];
		delete[] vy;
	}
}

template <class T>
void YARPOpticFlowBM<T>::Initialize (const YARPImageOf<T>& firstframe)
{
	if (m_smooth)
	{
		m_blur.Apply (firstframe, m_prev_frame);
	}
	else
	{
		m_prev_frame = firstframe;
	}
}

template <class T>
void YARPOpticFlowBM<T>::Uninitialize ()
{
}

template <class T>
void YARPOpticFlowBM<T>::_tfilter (void)
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
double YARPOpticFlowBM<T>::_cmpblock (unsigned char *p1, unsigned char *p2)
{
	// SSD?
	double accu = 0;
	for (int i = 0; i < m_blocksize; i++)
	{
		for (int j = 0; j < m_blocksize * sizeof(T); j++, p1++, p2++)
		{
			accu += ((*p1 - *p2) * (*p1 - *p2));
		}

		p1 += ((m_sizex - m_blocksize) * sizeof(T));
		p2 += ((m_sizex - m_blocksize) * sizeof(T));
	}

	return accu / (m_blocksize * m_blocksize * sizeof(T));
}

template <class T>
void YARPOpticFlowBM<T>::Apply (const YARPImageOf<T>& curframe, int *ovx, int *ovy)
{
	_computeflow (curframe);

	// copy results.
	memcpy (ovx, vx[0], sizeof(int)*m_nblocksx*m_nblocksy);
	memcpy (ovy, vy[0], sizeof(int)*m_nblocksx*m_nblocksy);

	//
	m_prev_frame = m_tmp_frame;
}

template <class T>
void YARPOpticFlowBM<T>::_fancydisplay (YARPImageOf<T>& out, int **vx, int **vy)
{
	T pix;
	memset (&pix, 0, sizeof (T));
	T white;
	memset (&white, 255, sizeof(T));

	for (int j = 0; j < m_nblocksy; j++)
	{
		for (int i = 0; i < m_nblocksx; i++)
		{
			int cx = i * m_blockincrement + m_blocksize/2;
			int cy = j * m_blockincrement + m_blocksize/2;

			// don't draw if it's outside the specified range.
			if (vx[j][i] <= m_searchrange &&
				vy[j][i] <= m_searchrange)
			{
				// draw arrow.
				const int RES = m_searchrange;
				const double inc = 1.0 / RES;
				int x,y;
				for (int k = 0; k < RES; k++)
				{
					x = int(cx + vx[j][i] * k * inc + .5);
					y = int(cy + vy[j][i] * k * inc + .5);
					if (x >= 0 && x < m_sizex && y >= 0 && y < m_sizey)
						out (x,y) = pix;
				}
			}

			out(cx,cy) = white;
		}
	}
}

// this can be optimized by using the integral image.
template <class T>
double YARPOpticFlowBM<T>::_variance (unsigned char *b)
{
	double average = 0;
	double variance = 0;
	unsigned char *p1 = b;

	for (int i = 0; i < m_blocksize; i++)
	{
		for (int j = 0; j < m_blocksize * sizeof(T); j++, p1++)
		{
			average += *p1;
			variance += (*p1 * *p1);
		}

		p1 += ((m_sizex - m_blocksize) * sizeof(T));
	}

	const int s = m_blocksize * m_blocksize;
	average /= s;
	variance /= (s-1);

	//printf ("%f\n", variance - average * average);
	return variance - s / (s-1) * average * average;
}

#define FAST 1

template <class T>
void YARPOpticFlowBM<T>::_computeflow (const YARPImageOf<T>& curframe)
{
	assert (curframe.GetWidth () == m_sizex && curframe.GetHeight() == m_sizey);
	assert (curframe.GetPadding () == 0);

	//
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

	// block matching between curframe and prevframe.
	// tmp contains the current frame.
	// prev is the old one.
	unsigned char *c = (unsigned char *)m_tmp_frame.GetAllocatedArray();
	unsigned char *o = (unsigned char *)m_prev_frame.GetAllocatedArray();
	int pixx, pixy;
	int minx, maxx;
	int miny, maxy;
	double min_ssd;
	double variance;

	for (int j = 0; j < m_nblocksy; j++)
	{
		for (int i = 0; i < m_nblocksx; i++)
		{
			min_ssd = _cmpblock (c, o);
			variance = _variance (o); // more correct to check o rather than c.
			vx[j][i] = 0;
			vy[j][i] = 0;

#ifdef FAST
			if (min_ssd > double(m_zerothr) && variance > m_varthr)
			{
#endif
				// check within range.
				pixx = i * m_blockincrement;
				pixy = j * m_blockincrement;
				if (pixx - m_searchrange < 0)
				{
					minx = pixx;
					maxx = m_searchrange;
				}
				else
				if (pixx + m_searchrange >= m_sizex - m_blocksize)
				{
					minx = m_searchrange;
					maxx = m_sizex - pixx - m_blocksize;
				}
				else
				{
					minx = m_searchrange;
					maxx = m_searchrange;
				}

				if (pixy - m_searchrange < 0)
				{
					miny = pixy;
					maxy = m_searchrange;
				}
				else
				if (pixy + m_searchrange >= m_sizey - m_blocksize)
				{
					miny = m_searchrange;
					maxy = m_sizey - pixy - m_blocksize;
				}
				else
				{
					miny = m_searchrange;
					maxy = m_searchrange;
				}

//				printf ("pixx, pixy %d %d, search x %d %d search y %d %d\n", pixx, pixy, minx, maxx, miny, maxy);

				double tmpssd;
				for (int dy = -miny; dy <= maxy; dy++)
					for (int dx = -minx; dx <= maxx; dx++)
					{
						// limits are correct.
						tmpssd = _cmpblock (c + dx * sizeof(T) + dy * m_sizex * sizeof(T), o);
						if (tmpssd < min_ssd)
						{
							min_ssd = tmpssd;
							vx[j][i] = dx;
							vy[j][i] = dy;
						}
					}
#ifdef FAST
			}
			else
			{
				if (variance <= m_varthr)
				{
					// below the variance threshold.
					vx[j][i] = OOVERFLOW + 1;
					vy[j][i] = OOVERFLOW + 1;
				}
			}
#else
			if (min_ssd <= double(m_zerothr))
			{
				vx[j][i] = 0;
				vy[j][i] = 0;
			}

			if (variance <= m_varthr)
			{
				// below the variance threshold.
				vx[j][i] = OOVERFLOW + 1;
				vy[j][i] = OOVERFLOW + 1;
			}
#endif

			// next block.
			c += (m_blockincrement * sizeof(T));
			o += (m_blockincrement * sizeof(T));
		}

		// next line block (blocksize-1 lines).
		c = (unsigned char *)m_tmp_frame.GetAllocatedArray();
		o = (unsigned char *)m_prev_frame.GetAllocatedArray();
		c += ((m_blockincrement * j) * m_sizex * sizeof(T));
		o += ((m_blockincrement * j) * m_sizex * sizeof(T));
	}
}

template <class T>
void YARPOpticFlowBM<T>::Apply (const YARPImageOf<T>& curframe, YARPImageOf<T>& out, int *ovx, int *ovy)
{
	_computeflow (curframe);

	// copy results.
	memcpy (ovx, vx[0], sizeof(int)*m_nblocksx*m_nblocksy);
	memcpy (ovy, vy[0], sizeof(int)*m_nblocksx*m_nblocksy);

	// display.
	out = m_tmp_frame;
	_fancydisplay (out, vx, vy);

	//
	m_prev_frame = m_tmp_frame;
}


template <class T>
void YARPOpticFlowBM<T>::Apply (const YARPImageOf<T>& curframe, const YARPImageOf<YarpPixelMono>& mask, int px, int py, YARPImageOf<T>& out, int *ovx, int *ovy)
{
	_computeflow (curframe, mask, px, py);

	// copy results.
	memcpy (ovx, vx[0], sizeof(int)*m_nblocksx*m_nblocksy);
	memcpy (ovy, vy[0], sizeof(int)*m_nblocksx*m_nblocksy);

	// display.
	out = m_tmp_frame;
	_fancydisplay (out, vx, vy);

	//
	m_prev_frame = m_tmp_frame;
}

template <class T>
int YARPOpticFlowBM<T>::L(void) const { return OOVERFLOW; }

template <class T>
void YARPOpticFlowBM<T>::_computeflow (const YARPImageOf<T>& curframe, const YARPImageOf<YarpPixelMono>& mask, int px, int py)
{
	assert (curframe.GetWidth () == m_sizex && curframe.GetHeight() == m_sizey);
	assert (curframe.GetPadding () == 0);

	//
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

	// block matching between curframe and prevframe.
	// tmp contains the current frame.
	// prev is the old one.
	unsigned char *c = (unsigned char *)m_tmp_frame.GetAllocatedArray();
	unsigned char *o = (unsigned char *)m_prev_frame.GetAllocatedArray();
	unsigned char *m = (unsigned char *)mask.GetAllocatedArray();

	int pixx, pixy;
	int minx, maxx;
	int miny, maxy;
	double min_ssd;
	double variance;
	const int shift = m_blocksize/2 + m_blocksize/2 * m_sizex;

	for (int j = 0; j < m_nblocksy; j++)
	{
		for (int i = 0; i < m_nblocksx; i++)
		{
			if (*(m + j * m_blockincrement * m_sizex + i * m_blockincrement + shift) != 0)
			{
				unsigned char *c1 = c + px * sizeof(T) + py * m_sizex * sizeof(T);				
		
				min_ssd = _cmpblock (c1, o);
				variance = _variance (o); // correct to check o rather than c.
				vx[j][i] = 0;
				vy[j][i] = 0;

				if (min_ssd > double(m_zerothr) && variance > m_varthr)
				{
					// check within range and shift by px, py.
					pixx = i * m_blockincrement + px;
					pixy = j * m_blockincrement + py;
					if (pixx - m_searchrange < 0)
					{
						minx = pixx;
						maxx = m_searchrange;
					}
					else
					if (pixx + m_searchrange >= m_sizex - m_blocksize)
					{
						minx = m_searchrange;
						maxx = m_sizex - pixx - m_blocksize;
					}
					else
					{
						minx = m_searchrange;
						maxx = m_searchrange;
					}

					if (pixy - m_searchrange < 0)
					{
						miny = pixy;
						maxy = m_searchrange;
					}
					else
					if (pixy + m_searchrange >= m_sizey - m_blocksize)
					{
						miny = m_searchrange;
						maxy = m_sizey - pixy - m_blocksize;
					}
					else
					{
						miny = m_searchrange;
						maxy = m_searchrange;
					}

					double tmpssd;
					for (int dy = -miny; dy <= maxy; dy++)
						for (int dx = -minx; dx <= maxx; dx++)
						{
							// limits are correct.
							tmpssd = _cmpblock (c1 + dx * sizeof(T) + dy * m_sizex * sizeof(T), o);
							if (tmpssd < min_ssd)
							{
								min_ssd = tmpssd;
								vx[j][i] = dx + px;
								vy[j][i] = dy + py;
							}
						}
				}
				else
				{
					if (variance <= m_varthr)
					{
						// below the variance threshold.
						vx[j][i] = OOVERFLOW+1;
						vy[j][i] = OOVERFLOW+1;
					}
				}

				if (min_ssd <= double(m_zerothr))
				{
					vx[j][i] = 0;
					vy[j][i] = 0;
				}

				if (variance <= m_varthr)
				{
					// below the variance threshold.
					vx[j][i] = OOVERFLOW+1;
					vy[j][i] = OOVERFLOW+1;
				}
			}	/* end if mask != 0 */
			else
			{
				// masked out.
				vx[j][i] = OOVERFLOW+2;
				vy[j][i] = OOVERFLOW+2;
			}

			// next block.
			c += (m_blockincrement * sizeof(T));
			o += (m_blockincrement * sizeof(T));
			//m += m_blockincrement;

		}	/* end of for i (columns) */

		// next line block (blocksize-1 lines).
		c = (unsigned char *)m_tmp_frame.GetAllocatedArray();
		o = (unsigned char *)m_prev_frame.GetAllocatedArray();
		//m = (unsigned char *)mask.GetAllocatedArray();

		c += ((m_blockincrement * j) * m_sizex * sizeof(T));
		o += ((m_blockincrement * j) * m_sizex * sizeof(T));
		//m += ((m_blockincrement * j) * m_sizex);

	}	/* end of for j (rows) */

}

#undef OOVERFLOW

#undef FAST