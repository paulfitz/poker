//
// YARPTagFilter.cpp
//

#include "YARPTagFilter.h"

YARPTagFilter::YARPTagFilter(void) : YARPFilter()
{
	m_ntags = 0;

	m_value = 255;
	m_exact = true;
}

YARPTagFilter::YARPTagFilter(int necc, int nang) : YARPFilter ()
{
	m_ntags = 0;
	_alloc_TempImg (necc, nang);
	m_value = 255;
	m_exact = true;
}

void YARPTagFilter::Resize (int necc, int nang)
{
	m_ntags = 0;
	_alloc_TempImg (necc, nang);
}

void YARPTagFilter::Cleanup (void)
{
	_free_TempImg();
}

void YARPTagFilter::Apply(const YARPImageOf<YarpPixelMono>& in, YARPImageOf<YarpPixelMono>& out)
{
	assert(out.GetHeight() == in.GetHeight() && out.GetWidth() == in.GetWidth());
	
	memcpy (sTempImg.GetIplPointer()->imageData, in.GetIplPointer()->imageData, in.GetIplPointer()->imageSize);

	IplImage *source = sTempImg.GetIplPointer();
	IplImage *dest = out.GetIplPointer();
	out.Zero();

	// .... implement tagging here.
	// returns the results in m_ntags. 

	unsigned char *src = (unsigned char *)source->imageData;
	unsigned char *dst = (unsigned char *)dest->imageData;

	int r, c;
	int last_tag = 1;
	int left_tag, up_tag;

	const int w = source->width;
	const int h = source->height;
	const int size = w * h;
	r = c = 0;
	for(int i = 0; i < size; i++)
	{
		if ((m_exact && src[i] == m_value) ||
			(!m_exact && src[i] >= m_value)) 
		{
			if (c > 0)
				left_tag = dst[i - 1];
			else
				left_tag = 0;

			if (r > 0)
				up_tag = dst[i - w];
			else
				up_tag = 0;

			if (left_tag)
			{
				if (up_tag)
				{
					// both left and up tagged, may need to merge 
					if(left_tag != up_tag)
						MergeRegions(dest, i);
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
					if (last_tag <= 255)
						dst[i] = last_tag;
					else
					{
						// everything under tag 255 (?).
						dst[i] = 255;
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

	m_ntags = last_tag;
}

void YARPTagFilter::_alloc_TempImg(int DimX, int DimY)
{
	sTempImg.Resize (DimX, DimY);
}

void YARPTagFilter::MergeRegions(IplImage *src, int index)
{
	// for now, no reuse of tags
	unsigned char *ind = (unsigned char *)src->imageData;
	unsigned char tag1 = ind[index - 1];
	unsigned char tag2 = ind[index - src->width];

	for (int i=0; i<index-1; i++)
	{
		if(ind[i] == tag2)
			ind[i] = tag1;
	}
	ind[index] = tag1;
}

void YARPTagFilter::_free_TempImg(void)
{
	sTempImg.Cleanup();
}
