//
// YARPGaussianFeatures.h
//

#ifndef __YARPGaussianFeaturesh__
#define __YARPGaussianFeaturesh__

#ifdef __QNX__
#include "YARPSafeNew.h"
#endif

#include "YARPImage.h"
#include "YARPFilters.h"
#include "YARPlogpolar.h"

#include "VisMatrix.h"

//
//
class _YARPG 
{
private:
	void _alloc (int necc, int nang)
	{
		m_co = new double *[nang];
		m_co[0] = new double[nang * necc];
		assert (m_co != NULL && m_co[0] != NULL);
		memset (m_co[0], 0, sizeof(double) * necc * nang);
		for (int i = 1; i < nang; i++)
			m_co[i] = m_co[i-1] + necc;
	}

	void _free (void)
	{
		if (m_co != NULL)
		{
			if (m_co[0] != NULL)
				delete[] m_co[0];
			delete[] m_co;
		}
	}

public:
	double **m_co;
	double m_norm;
	int m_necc;
	int m_nang;

	_YARPG() { m_co = NULL; }
	void Resize (int necc, int nang) 
	{
		if (m_co != NULL)
		{
			_free ();
		}

		m_necc = necc;
		m_nang = nang;
		_alloc (necc, nang);
	}
	~_YARPG() { _free(); }
};

//
class YARPGaussianFeatures : public YARPFilter, public YARPLogPolar
{
private:
	YARPGaussianFeatures (const YARPGaussianFeatures&);
	void operator= (const YARPGaussianFeatures&);

protected:
	CVisDVector m_features;
	const int m_sigmas;

	double *m_ro2;
	_YARPG *m_coeffs;
	double *m_norm;

	void ComputeCoefficients(double sigma, _YARPG& coeffs);
	double SpecialConvolveX (_YARPG& coeffs, const unsigned char *buffer);
	double SpecialConvolveY (_YARPG& coeffs, const unsigned char *buffer);

public:
	YARPGaussianFeatures (int necc, int nang, double rfmin, int size);
	virtual ~YARPGaussianFeatures ();

	void Apply (const YARPImageOf<YarpPixelMono>& in);

	virtual void Cleanup();
	virtual bool InPlace () const { return true; }

	CVisDVector& GetFeatures (void) { return m_features; }
	int GetFeatureSize (void) const { return m_sigmas * 2; }
};



#endif