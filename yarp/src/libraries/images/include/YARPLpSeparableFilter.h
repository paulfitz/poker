//
//

#ifndef __YARPLpSeparableFilterh__
#define __YARPLpSeparableFilterh__

#include "YARPImage.h"
#include "YARPFilters.h"

template <class T>
class YARPLpSeparableFilter : public YARPFilterOf<T> 
{
private:
	YARPLpSeparableFilter (const YARPLpSeparableFilter<T>&);
	void operator= (const YARPLpSeparableFilter<T>&);

protected:
	IplConvKernel *KernelX;
	IplConvKernel *KernelY;

	int borderUD, borderLR;
	YARPImageOf<T> sTempImg;
	YARPImageOf<T> dTempImg;
	int Shift;

	void _free_Kernel (IplConvKernel **Kernel);
	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);

public:
	YARPLpSeparableFilter (void);
	YARPLpSeparableFilter (int KernelsizeX, int *KernelValues, int sizeX, int sizeY, int shift=0);

	virtual ~YARPLpSeparableFilter() { Cleanup(); }
	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return true; }

	void SetKernel (int KernelsizeX, int *KernelValues, int sizeX, int sizeY, int shift=0);

	void Apply(const YARPImageOf<T>& in, YARPImageOf<T>& out);

	inline int GetBorderUD(void) const { return borderUD; }
	inline int GetBorderLR(void) const { return borderLR; }
};

template <class T>
YARPLpSeparableFilter<T>::YARPLpSeparableFilter(void) : YARPFilterOf<T>()
{
	KernelX = KernelY = NULL;
	borderUD = borderLR = 0;
	Shift = 0;
}

template <class T>
YARPLpSeparableFilter<T>::YARPLpSeparableFilter(int KernelSizeX, int *KernelValues, int sizeX, int sizeY, int shift) : YARPFilterOf<T>()
{
	int anchorX = (int)(lenghtX / 2);
	borderUD = anchorX ;
	borderLR = anchorX ;
	Shift = shift;
	KernelX = iplCreateConvKernel(KernelSizeX, 1, anchorX, 0, KernelValues, Shift);
	KernelY = iplCreateConvKernel(1, KernelSizeX, 0, anchorX, KernelValues, Shift);
	assert (KernelX != NULL && KernelY != NULL);

	int tempSizeX, tempSizeY;
	tempSizeX = sizeX + borderLR;
	tempSizeY = sizeY + (2*borderUD);

	_alloc_TempImg(tempSizeX, tempSizeY);
}

template <class T>
void YARPLpSeparableFilter<T>::Cleanup (void)
{
	_free_Kernel (&KernelX);
	_free_Kernel (&KernelY);
	_free_TempImg ();
}

template <class T>
void YARPLpSeparableFilter<T>::Apply (const YARPImageOf<T>& in, YARPImageOf<T>& out)
{
	assert(out.GetHeight() == in.GetHeight() && out.GetWidth() == in.GetWidth());

	sTempImg.Paste ((YARPGenericImage&)in, borderLR, borderUD);
	AddBorderLP (sTempImg, in, borderLR, borderUD);

	IplImage* source = sTempImg.GetIplPointer();
	IplImage* dest = dTempImg.GetIplPointer();

	iplConvolveSep2D (source, dest, KernelX, KernelY);

	dTempImg.Crop (out, borderLR, borderUD);
}

template <class T>
void YARPLpSeparableFilter<T>::SetKernel(int KernelSizeX, int *KernelValues, int sizeX, int sizeY, int shift)
{
	_free_Kernel(&KernelX);
	_free_Kernel(&KernelY);
	_free_TempImg();

	const int anchorX = (int)(KernelSizeX / 2);
	borderUD = anchorX ;
	borderLR = anchorX ;
	Shift = shift;
	KernelX = iplCreateConvKernel(KernelSizeX, 1, anchorX, 0, KernelValues, Shift);
	KernelY = iplCreateConvKernel(1, KernelSizeX, 0, anchorX, KernelValues, Shift);

	assert (KernelX != NULL && KernelY != NULL);

	const int tempSizeX = sizeX + borderLR;
	const int tempSizeY = sizeY + (2*borderUD);

	_alloc_TempImg (tempSizeX, tempSizeY);
}

template <class T>
void YARPLpSeparableFilter<T>::_alloc_TempImg(int DimX, int DimY)
{
	sTempImg.Resize (DimX, DimY);
	dTempImg.Resize (DimX, DimY);
}

template <class T>
void YARPLpSeparableFilter<T>::_free_Kernel(IplConvKernel **Kernel)
{
	if (*Kernel != NULL)
	{
		iplDeleteConvKernel(*Kernel);
		*Kernel = NULL;
	}
}

template <class T>
void YARPLpSeparableFilter<T>::_free_TempImg(void)
{
	sTempImg.Cleanup();
	dTempImg.Cleanup();
}


#endif