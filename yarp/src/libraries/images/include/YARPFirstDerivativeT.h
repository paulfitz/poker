//
// YARPFirstDerivativeT.h
//
// Temporal derivative.
//

#ifndef __YARPFirstDerivativeTh__
#define __YARPFirstDerivativeTh__
 
#include "YARPImage.h"
#include "YARPFilters.h"

//
// out is: I(t)-I(t-1)
//
// this is just an example of temporal filter. Not really used in 
// optic flow stuff.
//
template <class T>
class YARPFirstDerivativeT : public YARPFilterOf<T>
{
protected:
	YARPImageOf<T> m_old;

public:
	YARPFirstDerivativeT (YARPImageOf<T>& first);
	YARPFirstDerivativeT (void);
	virtual ~YARPFirstDerivativeT (void) { Cleanup(); }

	virtual void Cleanup (void) { m_old.Cleanup(); }
	virtual bool InPlace (void) const { return true; }

	void Resize (YARPImageOf<T>&  first);
	void Init (YARPImageOf<T>& first);

	void Apply(const YARPImageOf<T>& is, YARPImageOf<T>& id);
};

template <class T>
YARPFirstDerivativeT<T>::YARPFirstDerivativeT (YARPImageOf<T>& first) 
	: YARPFilterOf<T> ()
{
	// allocated?
	m_old = first;
}

template <class T>	
YARPFirstDerivativeT<T>::YARPFirstDerivativeT (void) : YARPFilterOf<T>()
{
}

template <class T>
void YARPFirstDerivativeT<T>::Resize (YARPImageOf<T>& first)
{
	m_old = first;
}

template <class T>
void YARPFirstDerivativeT<T>::Init (YARPImageOf<T>& first)
{
	m_old = first;
}

template <class T>
void YARPFirstDerivativeT<T>::Apply (const YARPImageOf<T>& is, YARPImageOf<T>& id)
{
	assert (is.GetID() == YARP_PIXEL_MONO_SIGNED);

	char * src = is.GetIplPointer()->imageData;
	char * dst = id.GetIplPointer()->imageData;
	char * f0 = m_old.GetIplPointer()->imageData;

	assert (f0 != NULL && dst != NULL && src != NULL);

	char tmp;
	const int size = id.GetIplPointer()->imageSize;
	for (int i = 0; i < size; i++)
	{
		tmp = (*src - *f0) / 2;
		*f0++ = *src++;
		*dst++ = tmp;
	}
}


#endif
