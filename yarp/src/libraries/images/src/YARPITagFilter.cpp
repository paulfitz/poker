//
// YARPITagFilter.cpp
//

#include "YARPITagFilter.h"

YARPITagFilter::YARPITagFilter(void) : YARPFilter()
{
	m_ntags = 0;

	m_value = 255;
	m_exact = true;

	m_necc = m_nang = 0;
	sTempImg = NULL;
}

YARPITagFilter::YARPITagFilter(int necc, int nang) : YARPFilter ()
{
	m_ntags = 0;
	_alloc_TempImg (necc, nang);
	m_value = 255;
	m_exact = true;

	m_necc = necc;
	m_nang = nang;
}

void YARPITagFilter::Resize (int necc, int nang)
{
	m_ntags = 0;
	_alloc_TempImg (necc, nang);
	m_necc = necc;
	m_nang = nang;
}

void YARPITagFilter::Cleanup (void)
{
	_free_TempImg();
	m_necc = m_nang = 0;
	sTempImg = NULL;
}

void YARPITagFilter::Apply(const YARPImageOf<YarpPixelMono>& in)
{
	assert(m_nang == in.GetHeight() && m_necc == in.GetWidth());
	
	memset (sTempImg[0], 0, sizeof(int)*m_necc*m_nang);

	// .... implement tagging here.
	// returns the results in m_ntags. 

	unsigned char *src = (unsigned char *)in.GetAllocatedArray();
	int *dst = sTempImg[0];

	int r, c;
	int last_tag = 1;
	int left_tag, up_tag;

	const int w = in.GetWidth();
	const int h = in.GetHeight();
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
						MergeRegions(i);
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
					if (last_tag <= 65535)
						dst[i] = last_tag;
					else
					{
						// everything under tag 255 (?).
						dst[i] = 65535;
						last_tag = 65535;
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

void YARPITagFilter::_alloc_TempImg(int DimX, int DimY)
{
	//sTempImg.Resize (DimX, DimY);
	sTempImg = new int *[DimY];
	assert (sTempImg != NULL);
	sTempImg[0] = new int [DimX*DimY];
	assert (sTempImg[0] != NULL);

	for (int i = 1; i < DimY; i++)
	{
		sTempImg[i] = sTempImg[i-1]+DimX;
	}
}

void YARPITagFilter::MergeRegions(int index)
{
	// for now, no reuse of tags
	int *ind = sTempImg[0];
	unsigned char tag1 = ind[index - 1];
	unsigned char tag2 = ind[index - m_necc];

	for (int i=0; i<index-1; i++)
	{
		if(ind[i] == tag2)
			ind[i] = tag1;
	}
	ind[index] = tag1;
}

void YARPITagFilter::_free_TempImg(void)
{
	//sTempImg.Cleanup();

	if (sTempImg != NULL)
	{
		if (sTempImg[0] != NULL)
			delete[] sTempImg[0];

		delete[] sTempImg;
	}
}
