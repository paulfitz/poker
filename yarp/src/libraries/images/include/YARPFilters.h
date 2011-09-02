///////////////////////////////////////////////////////////////////////////////
//
// YARPfilters.h
//  minimal definition of filters types.
//	this is just to derive from a common base class.
// 

#ifndef __YARPFiltersh__
#define __YARPFiltersh__

#include <math.h>
#include <string.h>

#include "YARPImage.h"

//
//
// I haven't added an "apply" method because the parameters could
// be different for different filters.
// I'll try to use an Apply of the form Apply(const src, dest, params).
class YARPFilter
{
private:
	YARPFilter (const YARPFilter&);
	void operator= (const YARPFilter&);

public:
	YARPFilter () {}
	virtual ~YARPFilter () {}

	virtual void Cleanup () {}
	virtual bool InPlace () const { return false; }
};

//
// I'm trying to keep this class minimal and do not commit to any 
//	 style. I'll try to use "Apply" most of the time...
//
template <class T>
class YARPFilterOf : public YARPFilter
{
private:
	// filters are not supposed to be copied around.
	YARPFilterOf (const YARPFilterOf<T>&);
	void operator= (const YARPFilterOf<T>&);

protected:
	// support for logpolar border handling.
	void AddBorderLP(YARPImageOf<T>& id, const YARPImageOf<T>& is, int kLR, int kUD);

public:
	YARPFilterOf () : YARPFilter() {}
	virtual ~YARPFilterOf () {}
};

//
template <class T>
void YARPFilterOf<T>::AddBorderLP(YARPImageOf<T>& id, const YARPImageOf<T>& is, int kLR, int kUD)
{
	int nAng = is.GetHeight();
	int nEcc = is.GetWidth();
	
	unsigned char **dst = (unsigned char **) id.GetArray();
	unsigned char **src = (unsigned char **) is.GetArray();
	
	assert((nAng%2) == 0);
	
	const int d = sizeof(T);

	// optimize most common formats.
	switch (d)
	{
	case 1:
		{
			// left border
			for (int c = 0; c < kLR ; c++)
			{
				for (int r = 0; r < (nAng/2); r++)
					dst[kUD+nAng/2+r][(kLR-1-c)] = src[r][c];

				for (int r = nAng/2; r < nAng; r++)
					dst[r-nAng/2+kUD][(kLR-1-c)] = src[r][c];
			}

			// top & bottom borders.	
			for (int c = 0; c < nEcc+kLR; c++)
			{
				for (int r = 0; r < kUD; r++)
					dst[kUD+nAng+r][c] = dst[r+kUD][c];

				int t;
				int r;
				for (t = 0, r = nAng; r < nAng + kUD; t++, r++)
					dst[t][c*d] = dst[r][c*d];
			} 
		}
		break;

	case 3:
		{
			// left border
			for (int c = 0; c < kLR ; c++)
			{
				for (int r = 0; r < (nAng/2); r++)
				{
					dst[kUD+nAng/2+r][(kLR-1-c)*3] = src[r][c*3];
					dst[kUD+nAng/2+r][(kLR-1-c)*3+1] = src[r][c*3+1];
					dst[kUD+nAng/2+r][(kLR-1-c)*3+2] = src[r][c*3+2];
				}

				for (int r = nAng/2; r < nAng; r++)
				{
					dst[r-nAng/2+kUD][(kLR-1-c)*3] = src[r][c*3];
					dst[r-nAng/2+kUD][(kLR-1-c)*3+1] = src[r][c*3+1];
					dst[r-nAng/2+kUD][(kLR-1-c)*3+2] = src[r][c*3+2];
				}
			}

			// top & bottom borders.	
			for (int c=0; c<nEcc+kLR; c++)
			{
				for (int r = 0; r < kUD; r++)
				{
					dst[kUD+nAng+r][c*3] = dst[r+kUD][c*3];
					dst[kUD+nAng+r][c*3+1] = dst[r+kUD][c*3+1];
					dst[kUD+nAng+r][c*3+2] = dst[r+kUD][c*3+2];
				}

				int t;  int r;
				for (t = 0, r = nAng; r < nAng + kUD; t++, r++)
				{
					dst[t][c*3] = dst[r][c*3];
					dst[t][c*3+1] = dst[r][c*3+1];
					dst[t][c*3+2] = dst[r][c*3+2];
				}
			} 
		}
		break;

	default:
		{
			// left border
			for (int c = 0; c < kLR ; c++)
			{
				for (int r = 0; r < (nAng/2); r++)
				{
					memcpy (&dst[kUD+nAng/2+r][(kLR-1-c)*d], 
							&src[r][c*d],
							d);
				}

				for (int r = nAng/2; r < nAng; r++)
				{
					memcpy (&dst[r-nAng/2+kUD][(kLR-1-c)*d],
							&src[r][c*d],
							d);
				}
			}

			// top & bottom borders.	
			for (int c=0; c<nEcc+kLR; c++)
			{
				for (int r = 0; r < kUD; r++)
				{
					memcpy (&dst[kUD+nAng+r][c*d],
							&dst[r+kUD][c*d],
							d);
				}

				int t;  int r;
				for (t = 0, r = nAng; r < nAng + kUD; t++, r++)
				{
					memcpy (&dst[t][c*d],
							&dst[r][c*d],
							d);
				}
			} 
		}
		break;
	}
}



#endif
