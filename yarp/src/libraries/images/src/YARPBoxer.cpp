//
// YARPBoxer.cpp
//

#include "YARPBoxer.h"

//
#ifndef MaxBoxes
#define MaxBoxes 5
#endif

#ifndef MaxTags
#define MaxTags 1000
#endif

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

YARPLpBoxer::YARPLpBoxer(int necc, int nang, double rfmin, int size) : 
	YARPFilter (), 
	m_tagger (necc, nang), 
	m_lp (necc, nang, rfmin, size)
{
	m_last_tag = 0;
	m_boxes = new YARPBox[MaxTags]; 
	assert (m_boxes != NULL);

	m_attn = new YARPBox[MaxBoxes];
	assert (m_attn != NULL);

	_alloc_TempImg (necc, nang);

	m_tagger.SetExact (255);
}

void YARPLpBoxer::Cleanup (void)
{
	_free_TempImg();
	m_tagger.Cleanup ();
}

// the input image should be a saliency-like image.
// 
void YARPLpBoxer::Apply (const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out, bool saveout)
{
	assert (out.GetHeight() == in.GetHeight() && out.GetWidth() == in.GetWidth());
	assert (in.GetPadding() == 0);

	// no threshold on the tagger right now.
	//m_tagger.Apply (in, sTempImg);
	m_tagger.Apply (in);
	//if (saveout)
	//	out = sTempImg;
	if (saveout)
		out = in;

	// I've got the tagged image now.
	m_last_tag = m_tagger.GetTags () + 1;
	assert (m_last_tag <= MaxTags);
	//assert (m_last_tag <= 255);

	const int w = in.GetWidth ();
	const int h = in.GetHeight ();
	for(int i = 0; i < m_last_tag; i++)
	{
		m_boxes[i].cmax = m_boxes[i].rmax = 0;
		m_boxes[i].cmax = m_boxes[i].rmax = 0;
		m_boxes[i].cmin = w;
		m_boxes[i].rmin = h;

		m_boxes[i].xmax = m_boxes[i].ymax = 0;
		m_boxes[i].xmin = m_boxes[i].ymin = m_lp.GetSize();

		m_boxes[i].total_sal = 0;
		m_boxes[i].total_pixels = 0;
		m_boxes[i].xsum = m_boxes[i].ysum = 0;
		m_boxes[i].valid = false;
	}
  
	// special case for the null tag (0)
	m_boxes[0].rmax = m_boxes[0].rmin = h/2;
	m_boxes[0].cmax = m_boxes[0].cmin = w/2;
	m_boxes[0].xmax = m_boxes[0].xmin = m_lp.GetSize() / 2;
	m_boxes[0].ymax = m_boxes[0].ymin = m_lp.GetSize() / 2;
	m_boxes[0].valid = true;

	// build all possible bounding boxes out of the tagged image.
	// this is done for the global saliency image (sum of all partial sal).

	// pixels are logpolar, averaging is done in cartesian.
	unsigned char *source = (unsigned char *)in.GetIplPointer()->imageData;
	int *tmp = m_tagger.GetTagged ()[0];

	for(int r = 0; r < h; r++)
		for(int c = 0; c < w; c++)
		{
			int tag_index = *tmp++;
			if (tag_index != 0)
			{
				m_boxes[tag_index].total_pixels++;
				m_boxes[tag_index].total_sal += 255; // (*source++);
				m_boxes[tag_index].valid = true;

				// x,y.
				double x, y;
				m_lp.Lp2Cart (double(c), double(r), x, y);

				if (m_boxes[tag_index].ymax < int(y)) m_boxes[tag_index].ymax = int(y);
				if (m_boxes[tag_index].ymin > int(y)) m_boxes[tag_index].ymin = int(y);
				if (m_boxes[tag_index].xmax < int(x)) m_boxes[tag_index].xmax = int(x);
				if (m_boxes[tag_index].xmin > int(x)) m_boxes[tag_index].xmin = int(x);

				if (m_boxes[tag_index].rmax < r) m_boxes[tag_index].rmax = r;
				if (m_boxes[tag_index].rmin > r) m_boxes[tag_index].rmin = r;
				if (m_boxes[tag_index].cmax < c) m_boxes[tag_index].cmax = c;
				if (m_boxes[tag_index].cmin > c) m_boxes[tag_index].cmin = c;

				m_boxes[tag_index].ysum += int(y);
				m_boxes[tag_index].xsum += int(x);
			}
		}

	RemoveNonValid ();
	MergeBoxes ();

	// merge boxes which are too close.
	// statically. Clearly this procedure could be more effective 
	// if it takes time into account.
	// LATER: update also the logpolar coordinates during
	//	the merger of the boxes.

	int max_tag, max_num;

	// create now the subset of attentional boxes.
	for (int box_num = 0; box_num < MaxBoxes; box_num++)
    {
		// find the most frequent tag, zero does not count 
		max_tag = max_num = 0;
		for(int i = 1; i < m_last_tag; i++)
		{
			if(m_boxes[i].total_pixels > max_num)
			{
				max_num = m_boxes[i].total_pixels;
				max_tag = i;
			}
		}

		if (max_tag != 0)
		{
			// compute saliency of region.
			// it cannot be done here.
			m_attn[box_num] = m_boxes[max_tag];

			m_attn[box_num].valid = true;
			m_attn[box_num].centroid_y = m_boxes[max_tag].ysum / max_num;
			m_attn[box_num].centroid_x = m_boxes[max_tag].xsum / max_num;
			
			m_boxes[max_tag].total_pixels = 0;
		}
		else
		{
			// no motion, return the center 
			m_attn[box_num].valid = false;
			m_attn[box_num].centroid_x = m_lp.GetSize() / 2;
			m_attn[box_num].centroid_y = m_lp.GetSize() / 2;
		}
	}

	// I've here MaxBoxes boxes accounting for the largest 
	// regions in the image.
	// remember that there's a bias because of logpolar.

	// destination image is not changed.
	// should I store the result of the tagging process?
}

void YARPLpBoxer::Apply (const YARPImageOf<YarpPixelMono>& in)
{
	Apply (in, (YARPImageOf<YarpPixelMono>&)in, false);
}

void YARPLpBoxer::RemoveNonValid (void)
{
	for (int i = 1; i < m_last_tag;)
	{
		if (m_boxes[i].valid == false || m_boxes[i].total_pixels < 5)
		{
			if (i == (m_last_tag-1))
				m_last_tag--;
			else
			{
				memcpy (&(m_boxes[i]), &(m_boxes[i+1]), sizeof(YARPBox) * (m_last_tag - i - 1));
				m_last_tag--;
			}
		}
		else
			i++;
	}
}

// 
void YARPLpBoxer::MergeBoxes (void)
{
	// do not take i = 0 into account.
	const double overlap = 1.0;
	for (int i = 1; i < m_last_tag-1; i++)
	{
		int j = i + 1;
		while (j < m_last_tag) 
		{
			//if (CenterDistance (i, j) < CombinedSize (i, j) * overlap)
			if (Overlap (i, j))
			{
				FuseBoxes (i, j);

				if (j == (m_last_tag - 1))
				{
					m_last_tag--;
				}
				else
				{
					memcpy (&(m_boxes[j]), &(m_boxes[j+1]), sizeof(YARPBox) * (m_last_tag - j - 1));
					m_last_tag--;
				}
			}
			else
				j++;
		}
	}
}

// draw a white box.
void YARPLpBoxer::DrawBox (IplImage *img, int x1, int y1, int x2, int y2)
{
	assert (x1 <= x2 && y1 <= y2);
	assert (img->nChannels == 1);
	const int w = img->width;
	const int h = img->height;
	unsigned char *dst = (unsigned char *)img->imageData;

	unsigned char *ptr1 = dst + y1 * w + x1;
	unsigned char *ptr2 = dst + y2 * w + x1;
	for (int i = 0; i <= x2-x1; i++)
	{
		*ptr1++ = 255;
		*ptr2++ = 255;
	}

	ptr1 = dst + y1 * w + x1;
	ptr2 = dst + y1 * w + x2;
	for (int i = 0; i <= y2-y1; i++)
	{
		*ptr1 = 255;
		*ptr2 = 255;
		ptr1 += w;
		ptr2 += w;
	}
}

void YARPLpBoxer::DrawBox (IplImage *img, int x1, int y1, int x2, int y2, int r, int g, int b)
{
	assert (x1 <= x2 && y1 <= y2);
	assert (img->nChannels == 3);

	const int w = img->width;
	const int h = img->height;
	unsigned char *dst = (unsigned char *)img->imageData;

	unsigned char *ptr1 = dst + y1 * w * 3 + x1 * 3;
	unsigned char *ptr2 = dst + y2 * w * 3 + x1 * 3;
	for (int i = 0; i <= x2-x1; i++)
	{
		*ptr1++ = b;
		*ptr1++ = g;
		*ptr1++ = r;
		*ptr2++ = b;
		*ptr2++ = g;
		*ptr2++ = r;
	}

	ptr1 = dst + y1 * w * 3 + x1 * 3;
	ptr2 = dst + y1 * w * 3 + x2 * 3;
	for (int i = 0; i <= y2-y1; i++)
	{
		*ptr1++ = b;
		*ptr1++ = g;
		*ptr1++ = r;
		ptr1 += (w * 3 - 3);
		*ptr2++ = b;
		*ptr2++ = g;
		*ptr2++ = r;
		ptr2 += (w * 3 - 3);
	}
}

void YARPLpBoxer::DrawBoxes (YARPImageOf<YarpPixelMono>& id)
{
	for (int i = 0; i < MaxBoxes; i++)
	{
		if (m_attn[i].valid)
		{
			DrawBox (id.GetIplPointer(), 
				m_attn[i].xmin,
				m_attn[i].ymin,
				m_attn[i].xmax,
				m_attn[i].ymax);
		}
	}
}

void YARPLpBoxer::DrawBoxes (YARPImageOf<YarpPixelRGB>& id)
{
	for (int i = 0; i < MaxBoxes; i++)
	{
		if (m_attn[i].valid)
		{
			DrawBox (id.GetIplPointer(), 
				m_attn[i].xmin,
				m_attn[i].ymin,
				m_attn[i].xmax,
				m_attn[i].ymax,
				255, 0, 0);		// Use red.
		}
	}
}

void YARPLpBoxer::DrawBoxes (YARPImageOf<YarpPixelBGR>& id)
{
	for (int i = 0; i < MaxBoxes; i++)
	{
		if (m_attn[i].valid)
		{
			DrawBox (id.GetIplPointer(), 
				m_attn[i].xmin,
				m_attn[i].ymin,
				m_attn[i].xmax,
				m_attn[i].ymax,
				0, 0, 255);		// Use red.
		}
	}
}

// should be inlined.
double YARPLpBoxer::CenterDistance (int i, int j)
{
	const double xi = double(m_boxes[i].xsum) / double(m_boxes[i].total_pixels);
	const double yi = double(m_boxes[i].ysum) / double(m_boxes[i].total_pixels);
	const double xj = double(m_boxes[j].xsum) / double(m_boxes[j].total_pixels);
	const double yj = double(m_boxes[j].ysum) / double(m_boxes[j].total_pixels);

	const double dx = xi - xj;
	const double dy = yi - yj;

	return dx * dx + dy * dy;
}

double YARPLpBoxer::CombinedSize (int i, int j)
{
	const double sx_i = double(m_boxes[i].xmax - m_boxes[i].xmin);
	const double sy_i = double(m_boxes[i].ymax - m_boxes[i].ymin);
	const double sx_j = double(m_boxes[j].xmax - m_boxes[j].xmin);
	const double sy_j = double(m_boxes[j].ymax - m_boxes[j].ymin);

	const double size_i = (sx_i > sy_i) ? sx_i : sy_i;
	const double size_j = (sx_j > sy_j) ? sx_j : sy_j;

	return (size_i + size_j) * (size_i + size_j) / 4;
}

void YARPLpBoxer::FuseBoxes (int i, int j)
{
	const int max_x = (m_boxes[i].xmax > m_boxes[j].xmax) ? m_boxes[i].xmax : m_boxes[j].xmax;
	const int min_x = (m_boxes[i].xmin < m_boxes[j].xmin) ? m_boxes[i].xmin : m_boxes[j].xmin;
	const int max_y = (m_boxes[i].ymax > m_boxes[j].ymax) ? m_boxes[i].ymax : m_boxes[j].ymax;
	const int min_y = (m_boxes[i].ymin < m_boxes[j].ymin) ? m_boxes[i].ymin : m_boxes[j].ymin;

	m_boxes[i].xmax = max_x;
	m_boxes[i].xmin = min_x;
	m_boxes[i].ymax = max_y;
	m_boxes[i].ymin = min_y;

	m_boxes[i].xsum += m_boxes[j].xsum;
	m_boxes[i].ysum += m_boxes[j].ysum;
	m_boxes[i].total_pixels += m_boxes[j].total_pixels;
	m_boxes[i].total_sal += m_boxes[j].total_sal;

	m_boxes[j].valid = false;
}

void YARPLpBoxer::_free_TempImg()
{
	sTempImg.Cleanup();
}

void YARPLpBoxer::_alloc_TempImg(int DimX, int DimY)
{
	sTempImg.Resize (DimX, DimY);
}

#undef MaxBoxes
#undef MaxTags
