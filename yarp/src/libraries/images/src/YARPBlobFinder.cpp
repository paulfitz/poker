//
// YARPBlobFinder.cpp
//

#include "YARPBlobFinder.h"

#ifndef MaxBoxesBlobFinder
#define MaxBoxesBlobFinder 65000
#endif

#ifndef MaxBoxes
#define MaxBoxes 5
#endif

#ifndef MaxTags
#define MaxTags 65000
#endif

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

void YARPBlobFinder::MergeRegions(short *src, int index, int w)
{
	// for now, no reuse of tags
	//unsigned char *ind = (unsigned char *)src->imageData;
	short *ind = src;
	short tag1 = ind[index - 1];
	short tag2 = ind[index - w];

	for (int i=0; i<index-1; i++)
	{
		if(ind[i] == tag2)
			ind[i] = tag1;
	}
	ind[index] = tag1;
}

// uses short instead of a std image.
int YARPBlobFinder::SpecialTagging(short *tagged, YARPImageOf<YarpPixelHSV>& img)
{
	IplImage *source = img.GetIplPointer();

	unsigned char *src = (unsigned char *)source->imageData;
	short *dst = tagged;

	int r, c;
	int last_tag = 1;
	int left_tag, up_tag;

	const int w = source->width;
	const int h = source->height;
	const int size = w * h;
	memset (dst, 0, sizeof(short) * w * h);

	r = c = 0;
	for (int i = 0; i < size; i++)
	{
		unsigned char hue = img (c, r).h;
		unsigned char sat = img (c, r).s;

		if (sat != 0)
		{
			if (c > 0)
			{
				if (SimilarSaturation (sat, img(c-1, r).s) &&
					SimilarHue (hue, img(c-1, r).h))
					left_tag = dst[i - 1];
				else
					left_tag = 0;
			}
			else
				left_tag = 0;

			if (r > 0)
			{
				if (SimilarSaturation (sat, img(c, r-1).s) &&
					SimilarHue (hue, img(c, r-1).h))
					up_tag = dst[i - w];
				else
					up_tag = 0;
			}
			else
				up_tag = 0;

			if (left_tag)
			{
				if (up_tag)
				{
					// both left and up tagged, may need to merge 
					if(left_tag != up_tag)
					{
						MergeRegions(dst, i, w);
					}
					else
						dst[i] = left_tag;
				}	
				else
				{
					// inherit from the left 
					dst[i] = left_tag;
				}	
			}
			else
			{
				if (up_tag)
				{
					// inherit from the top 
					dst[i] = up_tag;
				}		
				else
				{
					// gets a new tag 
					last_tag++;
					if (last_tag <= MaxTags)
						dst[i] = last_tag;
					else
					{
						// everything under tag MaxTags (?).
						dst[i] = MaxTags;
					}
				}	
			}
		}
		else 
		{
			dst[i] = 0;
		}

		c++;
		if (c == w)
		{
			c = 0;
			r++;
		}
	}

	return last_tag;
}

YARPBlobFinder::YARPBlobFinder(int necc, int nang, double rfmin, int size) : 
	YARPFilter (), 
	m_lp (necc, nang, rfmin, size),
	m_saliency (necc, nang, 0)
{
	m_last_tag = 0;
	m_boxes = new YARPBox[MaxBoxesBlobFinder]; 
	assert (m_boxes != NULL);

	m_attn = new YARPBox[MaxTags];
	assert (m_attn != NULL);

	m_tagged = new short[necc * nang];
	m_hsv_enhanced.Resize (necc, nang);

	// defaults.

	m_sat_thr = 25;	// 0.1 is the best resolution in saturation
	m_hue_thr = 10; // 36 degrees is the resolution in hue

	m_saliency.SetSaturationThreshold (50);	// 0.3 of the max.
	m_saliency.SetLumaThreshold (50);
}

void YARPBlobFinder::Cleanup()
{
	if (m_tagged != NULL)
		delete[] m_tagged;
	m_tagged = NULL;

	m_saliency.Cleanup ();
	m_hsv_enhanced.Cleanup ();
}

void YARPBlobFinder::Apply(const YARPImageOf<YarpPixelBGR>& is, YARPImageOf<YarpPixelBGR>& id)
{
	// no padding enforced.
	assert (id.GetPadding() == 0);
	assert (is.GetPadding() == 0);
	assert (id.GetHeight() == is.GetHeight() && id.GetWidth() == is.GetWidth());

	// extract the saliency image.
	m_saliency.Apply (is, m_hsv_enhanced);

	_apply (is, id);
}

void YARPBlobFinder::Apply(const YARPImageOf<YarpPixelRGB>& is, YARPImageOf<YarpPixelRGB>& id)
{
	// no padding enforced.
	assert (id.GetPadding() == 0);
	assert (is.GetPadding() == 0);
	assert (id.GetHeight() == is.GetHeight() && id.GetWidth() == is.GetWidth());

	// extract the saliency image.
	m_saliency.Apply (is, m_hsv_enhanced);

	_apply (is, id);
}

void YARPBlobFinder::_apply(const YARPGenericImage& is, YARPGenericImage& id)
{
	// hsv_enhanced is already filled out.

	m_last_tag = SpecialTagging (m_tagged, m_hsv_enhanced);

	// I've got the tagged image now.
	assert (m_last_tag <= MaxBoxesBlobFinder);

	//printf ("last_tag is %d\n", m_last_tag);

	const int w = is.GetWidth ();
	const int h = is.GetHeight ();
	for(int i = 0; i <= m_last_tag; i++)
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

	// pixels are logpolar, averaging is done in cartesian.
	unsigned char *source = (unsigned char *)m_hsv_enhanced.GetArray()[0];	// Hue. 
	short *tmp = m_tagged;
	for(int r = 0; r < h; r++)
		for(int c = 0; c < w; c++)
		{
			short tag_index = *tmp++;
			if (tag_index != 0)
			{
				m_boxes[tag_index].total_pixels++;

				// the saliency here is the average hue.
				m_boxes[tag_index].total_sal += *source;
				source += 3;

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
	MergeBoxes (); //further clustering not needed.

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
			int area = TotalArea (m_boxes[i]);
			if (area > max_num && m_boxes[i].total_pixels > 0)
			//if(m_boxes[i].total_pixels > max_num)
			{
				max_num = area; //m_boxes[i].total_pixels;
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

	//PrintBoxes (m_attn, MaxBoxes);

	// I've here MaxBoxes boxes accounting for the largest 
	// regions in the image.
	// remember that there's a bias because of logpolar.

	// destination image is not changed.
	// should I store the result of the tagging process?
}

void YARPBlobFinder::PrintBoxes (YARPBox *m_boxes, int size)
{
	for (int i = 0; i < size; i++)
	{
		printf ("box : %d\n", i);

		if (m_boxes[i].valid)
		{
			printf ("area : %d\n", TotalArea (m_boxes[i]));

			printf ("cmin, cmax : %d %d\n", m_boxes[i].cmin, m_boxes[i].cmax);
			printf ("rmin, rmax : %d %d\n", m_boxes[i].rmin, m_boxes[i].rmax);
			printf ("xmin, xmax : %d %d\n", m_boxes[i].xmin, m_boxes[i].xmax);
			printf ("ymin, ymax : %d %d\n", m_boxes[i].ymin, m_boxes[i].ymax);
			printf ("total_sal : %d\n", m_boxes[i].total_sal);
			printf ("total_pix : %d\n", m_boxes[i].total_pixels);
			printf ("sx, sy : %d %d\n", m_boxes[i].xsum, m_boxes[i].ysum);
		}
	}

}


void YARPBlobFinder::RemoveNonValid (void)
{
	const int max_size = m_lp.GetSize() * m_lp.GetSize() / 8;
	const int min_size = 100;

	for (int i = 1; i < m_last_tag;)
	{
		int area = TotalArea (m_boxes[i]);
		if (m_boxes[i].valid == false || 
			m_boxes[i].total_pixels < 5 ||
			area < min_size ||
			area > max_size)
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
void YARPBlobFinder::MergeBoxes (void)
{
	// do not take i = 0 into account.
	//const double overlap = 1.5;
	for (int i = 1; i <= m_last_tag-1; i++)
	{
		int j = i + 1;
		while (j <= m_last_tag) 
		{
			//if (CenterDistance (i, j) < CombinedSize (i, j) * overlap &&
			if (Overlap (i, j) &&
				HueDistance (i, j) < m_hue_thr/2 /*4*/)
			{
				FuseBoxes (i, j);

				if (j == m_last_tag)
				{
					m_last_tag--;
				}
				else
				{
					memcpy (&(m_boxes[j]), &(m_boxes[j+1]), sizeof(YARPBox) * (m_last_tag - j /*- 1*/));
					m_last_tag--;
				}
			}
			else
				j++;
		}
	}
}

// draw a white box.
void YARPBlobFinder::DrawBox (IplImage *img, int x1, int y1, int x2, int y2)
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

// draw a colored box.
void YARPBlobFinder::DrawBox (IplImage *img, int x1, int y1, int x2, int y2, int r, int g, int b)
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
		*ptr1++ = r;
		*ptr1++ = g;
		*ptr1++ = b;
		*ptr2++ = r;
		*ptr2++ = g;
		*ptr2++ = b;
	}

	ptr1 = dst + y1 * w * 3 + x1 * 3;
	ptr2 = dst + y1 * w * 3 + x2 * 3;
	for (int i = 0; i <= y2-y1; i++)
	{
		*ptr1++ = r;
		*ptr1++ = g;
		*ptr1++ = b;
		ptr1 += (w * 3 - 3);
		*ptr2++ = r;
		*ptr2++ = g;
		*ptr2++ = b;
		ptr2 += (w * 3 - 3);
	}
}

void YARPBlobFinder::DrawBoxes (YARPImageOf<YarpPixelMono>& id)
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

void YARPBlobFinder::DrawBoxes (YARPImageOf<YarpPixelRGB>& id)
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

void YARPBlobFinder::DrawBoxes (YARPImageOf<YarpPixelBGR>& id)
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
double YARPBlobFinder::CenterDistance (int i, int j)
{
	const double xi = double(m_boxes[i].xsum) / double(m_boxes[i].total_pixels);
	const double yi = double(m_boxes[i].ysum) / double(m_boxes[i].total_pixels);
	const double xj = double(m_boxes[j].xsum) / double(m_boxes[j].total_pixels);
	const double yj = double(m_boxes[j].ysum) / double(m_boxes[j].total_pixels);

	const double dx = xi - xj;
	const double dy = yi - yj;

	return dx * dx + dy * dy;
}

double YARPBlobFinder::CombinedSize (int i, int j)
{
	const double sx_i = double(m_boxes[i].xmax - m_boxes[i].xmin);
	const double sy_i = double(m_boxes[i].ymax - m_boxes[i].ymin);
	const double sx_j = double(m_boxes[j].xmax - m_boxes[j].xmin);
	const double sy_j = double(m_boxes[j].ymax - m_boxes[j].ymin);

	const double size_i = (sx_i > sy_i) ? sx_i : sy_i;
	const double size_j = (sx_j > sy_j) ? sx_j : sy_j;

	return (size_i + size_j) * (size_i + size_j) / 4;
}

void YARPBlobFinder::FuseBoxes (int i, int j)
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

// un-defines.
#undef MaxTags
#undef MaxBoxes
#undef MaxBoxesBlobFinder
