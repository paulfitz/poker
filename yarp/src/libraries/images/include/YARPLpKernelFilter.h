//

#ifndef __YARPLpKernelFilterh__
#define __YARPLpKernelFilterh__

#include "YARPImage.h"
#include "YARPFilters.h"

template <class T>
class YARPLpKernelFilter : public YARPFilterOf<T> 
{
private:
	YARPLpKernelFilter (const YARPLpKernelFilter&);
	void operator= (const YARPLpKernelFilter&);

private:
	IplConvKernel *Kernel;
	int borderUD, borderLR;
	YARPImageOf<T> sTempImg;
	YARPImageOf<T> dTempImg;
	int Shift;

	void _free_Kernel (void);
	void _alloc_TempImg (int DimX, int DimY);
	void _free_TempImg (void);

public:
	YARPLpKernelFilter (void);
	YARPLpKernelFilter (int KernelsizeX, int KernelsizeY, int *KernelValues, int sizeX, int sizeY, int shift=0);

	virtual ~YARPLpKernelFilter() { Cleanup(); }
	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return true; }

	void SetKernel(int KernelsizeX, int KernelsizeY, int *KernelValues, int sizeX, int sizeY, int shift=0);

	void Apply (const YARPImageOf<T>& in, YARPImageOf<T>& out);

	inline int GetBorderUD(void) const { return borderUD; }
	inline int GetBorderLR(void) const { return borderLR; }
};

template <class T>
YARPLpKernelFilter<T>::YARPLpKernelFilter(void) : YARPFilterOf<T>()
{
	Kernel = NULL;
	borderUD = borderLR = 0;
	Shift = 0;
}

template <class T>
YARPLpKernelFilter<T>::YARPLpKernelFilter(int KernelSizeX, int KernelSizeY, int *KernelValues, int sizeX, int sizeY, int shift) : YARPFilterOf<T>()
{
	int anchorX = (int)(KernelSizeX / 2);
	int anchorY = (int)(KernelSizeY / 2);
	borderUD = anchorY ;
	borderLR = anchorX ;
	Shift = shift;
	Kernel = iplCreateConvKernel(KernelSizeX,KernelSizeY,anchorX,anchorY,KernelValues,Shift);
	assert (Kernel != NULL);

	int tempSizeX = sizeX + borderLR;
	int tempSizeY = sizeY + (2*borderUD);

	_alloc_TempImg (tempSizeX, tempSizeY);
}

template <class T>
void YARPLpKernelFilter<T>::Cleanup (void)
{
	_free_Kernel ();
	_free_TempImg ();
}

template <class T>
void YARPLpKernelFilter<T>::Apply (const YARPImageOf<T>& in, YARPImageOf<T>& out)
{
	assert(out.GetHeight() == in.GetHeight() && out.GetWidth() == in.GetWidth());

	sTempImg.Paste ((YARPGenericImage&)in, borderLR, borderUD);
	AddBorderLP(sTempImg, in, borderLR, borderUD);

	IplImage* source = sTempImg.GetIplPointer();
	IplImage* dest = dTempImg.GetIplPointer();
	
	iplConvolve2D (source, dest, &Kernel, 1, IPL_SUM);	

	dTempImg.Crop (out, borderLR, borderUD);
}

template <class T>
void YARPLpKernelFilter<T>::SetKernel(int KernelSizeX, int KernelSizeY, int *KernelValues, int sizeX, int sizeY, int shift)
{
	_free_Kernel();
	_free_TempImg();

	int anchorX = (int)(KernelSizeX / 2);
	int anchorY = (int)(KernelSizeY / 2);
	borderUD = anchorY ;
	borderLR = anchorX ;
	Shift = shift;

	Kernel = iplCreateConvKernel(KernelSizeX,KernelSizeY,anchorX,anchorY,KernelValues,Shift);
	assert (Kernel != NULL);

	int tempSizeX = sizeX + borderLR;
	int tempSizeY = sizeY + (2*borderUD);

	_alloc_TempImg (tempSizeX, tempSizeY);
}

template <class T>
void YARPLpKernelFilter<T>::_alloc_TempImg(int DimX, int DimY)
{
	_free_TempImg();

	sTempImg.Resize (DimX, DimY);
	dTempImg.Resize (DimX, DimY);
}

template <class T>
void YARPLpKernelFilter<T>::_free_Kernel(void)
{
	if (Kernel != NULL)
	{
		iplDeleteConvKernel(Kernel);
		Kernel = NULL;
	}
}

template <class T>
void YARPLpKernelFilter<T>::_free_TempImg(void)
{
	sTempImg.Cleanup();
	dTempImg.Cleanup();
}

#endif