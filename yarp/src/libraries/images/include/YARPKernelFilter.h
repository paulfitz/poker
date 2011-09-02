//
// YARPKernelFilter.h
//

#ifndef __YARPKernelFilterh__
#define __YARPKernelFilterh__

template <class T>
class YARPKernelFilter : public YARPFilterOf<T> 
{
private:
	YARPKernelFilter (const YARPKernelFilter<T>&);
	void operator= (const YARPKernelFilter<T>&);

protected:
	IplConvKernel *Kernel;
	int borderUD, borderLR;
	int Shift;

	void _free_Kernel(void);
	
public:
	YARPKernelFilter (void);
	YARPKernelFilter (int KernelSizeX, int KernelSizeY, int *KernelValues, int shift=0);
	virtual ~YARPKernelFilter() { Cleanup(); }

	virtual void Cleanup (void);
	virtual bool InPlace (void) const { return true; }

	void SetKernel(int KernelSizeX, int KernelSizeY, int *KernelValues, int shift=0);

	void Apply (const YARPImageOf<T>& in, YARPImageOf<T>& out);
	
	inline int GetBorderUD(void) const { return borderUD; }
	inline int GetBorderLR(void) const { return borderLR; }
};

template <class T>
YARPKernelFilter<T>::YARPKernelFilter(void) : YARPFilterOf<T>()
{
	Kernel = NULL;
	borderUD = borderLR = 0;
	Shift = 0;
}

template <class T>
YARPKernelFilter<T>::YARPKernelFilter(int KernelSizeX, int KernelSizeY, int *KernelValues, int shift) : YARPFilterOf<T>()
{
	int anchorX = (int)(KernelSizeX / 2);
	int anchorY = (int)(KernelSizeY / 2);
	borderUD = anchorY;
	borderLR = anchorX;
	Shift = shift;

	Kernel = iplCreateConvKernel(KernelSizeX,KernelSizeY,anchorX,anchorY,KernelValues,Shift);
	assert (Kernel != NULL);
}

template <class T>
void YARPKernelFilter<T>::Cleanup (void)
{
	_free_Kernel();
}

template <class T>
void YARPKernelFilter<T>::SetKernel(int KernelSizeX, int KernelSizeY, int *KernelValues, int shift)
{
	_free_Kernel();

	int anchorX = (int)(KernelSizeX / 2);
	int anchorY = (int)(KernelSizeY / 2);
	borderUD = anchorY;
	borderLR = anchorX;
	Shift = shift;

	Kernel = iplCreateConvKernel(KernelSizeX,KernelSizeY,anchorX,anchorY,KernelValues,Shift);
	assert (Kernel != NULL);
}

template <class T>
void YARPKernelFilter<T>::Apply(const YARPImageOf<T>& in, YARPImageOf<T>& out)
{
	IplImage* source = in.GetIplPointer();
	IplImage* dest = out.GetIplPointer();

	assert (source != NULL && dest != NULL);

	iplConvolve2D(source, dest, &Kernel, 1, IPL_SUM);	
}

template <class T>
void YARPKernelFilter<T>::_free_Kernel(void)
{
	if (Kernel != NULL)
	{
		iplDeleteConvKernel(Kernel);
		Kernel = NULL;
	}
}

#endif