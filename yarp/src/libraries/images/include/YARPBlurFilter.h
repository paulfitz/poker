//
// YARPBlurFilter.h
//

#ifndef __YARPBlurFilterh__
#define __YARPBlurFilterh__

#include "YARPImage.h"
#include "YARPFilters.h"

//
// cartesian blur filter.

template <class T>
class YARPBlurFilter : public YARPFilterOf<T> 
{
private:
	YARPBlurFilter (const YARPBlurFilter<T>&);
	void operator= (const YARPBlurFilter<T>&);

protected:
	int borderUD, borderLR;
	YARPImageOf<T> sTempImg;

	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);

public:
	YARPBlurFilter(int x, int y);
	virtual ~YARPBlurFilter() { Cleanup(); } 

	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return true; }

	virtual void Apply(const YARPImageOf<T>& is, YARPImageOf<T>& id);
	inline int GetBorderUD(void) const { return borderUD; }
	inline int GetBorderLR(void) const { return borderLR; }
};

template <class T>
YARPBlurFilter<T>::YARPBlurFilter(int x, int y) : YARPFilterOf<T>()
{
	borderUD = 1;
	borderLR = 1;
	_alloc_TempImg (x, y);
}

template <class T>
void YARPBlurFilter<T>::Cleanup (void)
{
	_free_TempImg();
}

// there could be an IPL version.
template <class T>
void YARPBlurFilter<T>::Apply(const YARPImageOf<T>& is, YARPImageOf<T>& id)
{
	assert (id.GetHeight() == is.GetHeight() && id.GetWidth() == is.GetWidth());
	//assert (is.GetPadding() == 0 && id.GetPadding() == 0);

	switch (is.GetID())
	{
	case YARP_PIXEL_MONO:
		{
			sTempImg = is;

			IplImage* source;
			IplImage* dest;
			source = sTempImg.GetIplPointer();
			dest = id.GetIplPointer();
			const int width = sTempImg.GetWidth();
			const int height = sTempImg.GetHeight();
			const int row_width = source->widthStep;

			unsigned char *asrc = (unsigned char *)source->imageData;
			unsigned char *adst = (unsigned char *)dest->imageData;
			unsigned char *row_s = asrc;
			unsigned char *row_d = adst;

			int tmp;
			for (int i = 0; i < height-2; i++)
			{
				row_s = asrc;
				row_d = adst;
				for (int j = 0; j < width-2; j++)
				{
					tmp = *(row_s);
					tmp += *(row_s + 1);
					tmp += *(row_s + 2);
					tmp += *(row_s + row_width);
					tmp += *(row_s +1 + row_width);
					tmp += *(row_s +2 + row_width);
					tmp += *(row_s + 2 * row_width);
					tmp += *(row_s +1 + 2 * row_width);
					tmp += *(row_s +2 + 2 * row_width);
					*(row_d + 1 + row_width) = tmp / 9;
					row_d ++;
					row_s ++;
				}

				asrc += row_width;
				adst += row_width;
			}
		}
		break;

	default:
		assert (1 == 0);
		break;
	}
}

template <class T>
void YARPBlurFilter<T>::_alloc_TempImg(int DimX, int DimY)
{
	sTempImg.Resize (DimX, DimY);
}

template <class T>
void YARPBlurFilter<T>::_free_TempImg()
{
	sTempImg.Cleanup ();
}


//
// logpolar blur filter.

template <class T>
class YARPLpBlurFilter : public YARPFilterOf<T> 
{
private:
	YARPLpBlurFilter (const YARPLpBlurFilter<T>&);
	void operator= (const YARPLpBlurFilter<T>&);

protected:
	int borderUD, borderLR;
	YARPImageOf<T> sTempImg;
	YARPImageOf<T> dTempImg;

	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);

public:
	YARPLpBlurFilter(int x, int y);
	virtual ~YARPLpBlurFilter() { Cleanup(); } 

	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return true; }

	virtual void Apply(const YARPImageOf<T>& is, YARPImageOf<T>& id);
	inline int GetBorderUD(void) const { return borderUD; }
	inline int GetBorderLR(void) const { return borderLR; }
};

template <class T>
YARPLpBlurFilter<T>::YARPLpBlurFilter(int x, int y) : YARPFilterOf<T>()
{
	borderUD = 1;
	borderLR = 1;
	int tempSizeX, tempSizeY;
	tempSizeX = x + borderLR;
	tempSizeY = y + (2*borderUD);

	_alloc_TempImg (tempSizeX, tempSizeY);
}

template <class T>
void YARPLpBlurFilter<T>::Cleanup (void)
{
	_free_TempImg();
}

// there could be an IPL version.
template <class T>
void YARPLpBlurFilter<T>::Apply(const YARPImageOf<T>& is, YARPImageOf<T>& id)
{
	assert (id.GetHeight() == is.GetHeight() && id.GetWidth() == is.GetWidth());
	//assert (is.GetPadding() == 0); safe alignment.

	switch (is.GetID())
	{
	case YARP_PIXEL_MONO:
		{
			sTempImg.Paste ((YARPGenericImage&)is, borderLR, borderUD);
			AddBorderLP(sTempImg, is, borderLR, borderUD);

			IplImage* source;
			IplImage* dest;
			source = sTempImg.GetIplPointer();
			dest = dTempImg.GetIplPointer();
			const int width = sTempImg.GetWidth();
			const int height = sTempImg.GetHeight();
			const int row_width = source->widthStep;

			unsigned char *asrc = (unsigned char *)source->imageData;
			unsigned char *adst = (unsigned char *)dest->imageData;
			unsigned char *row_s = asrc;
			unsigned char *row_d = adst;

			int tmp;
			for (int i = 0; i < height-2; i++)
			{
				row_s = asrc;
				row_d = adst;
				for (int j = 0; j < width-2; j++)
				{
					tmp = *(row_s);
					tmp += *(row_s + 1);
					tmp += *(row_s + 2);
					tmp += *(row_s + row_width);
					tmp += *(row_s +1 + row_width);
					tmp += *(row_s +2 + row_width);
					tmp += *(row_s + 2 * row_width);
					tmp += *(row_s +1 + 2 * row_width);
					tmp += *(row_s +2 + 2 * row_width);
					*(row_d + 1 + row_width) = tmp / 9;
					row_d ++;
					row_s ++;
				}

				asrc += row_width;
				adst += row_width;
			}
			dTempImg.Crop(id, borderLR, borderUD);
		}
		break;

	default:
		assert (1 == 0);
		break;
	}
}

template <class T>
void YARPLpBlurFilter<T>::_alloc_TempImg(int DimX, int DimY)
{
	sTempImg.Resize (DimX, DimY);
	dTempImg.Resize (DimX, DimY);
}

template <class T>
void YARPLpBlurFilter<T>::_free_TempImg()
{
	sTempImg.Cleanup ();
	dTempImg.Cleanup ();
}

#endif