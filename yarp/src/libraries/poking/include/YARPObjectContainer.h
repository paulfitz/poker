//
// YARPObjectContainer.h
//

#ifndef __YARPObjectContainerh__
#define __YARPObjectContainerh__

#include "YARPImage.h"
#include "YARPPort.h"
#include "YARPImagePort.h"
#include "YARPImageDraw.h"

#include "YARPCanonicalData.h"
#include "YARPSearchRotation.h"
#include "YARPTargetLocator.h"


class YARPObjectContainer
{
protected:
	bool m_active;

	int m_last_known_object;

	double m_displacement;
	double m_orientation;

	YARPImageOf<YarpPixelBGR> *m_backp;

	YARPCanonicalNeurons *m_canonical;
	YARPCanonicalStats *m_canonical_stats;
	YARPColorLocator *m_locator;

public:
	YARPObjectContainer ()
	{
		m_canonical = NULL;
		m_canonical_stats = NULL;
		m_active = false;
		m_locator = NULL;
		m_backp = NULL;
		m_last_known_object = -1;

		m_displacement = 0;
		m_orientation = 0;
	}

	~YARPObjectContainer ()
	{
		if (m_locator != NULL) delete[] m_locator;
		if (m_backp != NULL) delete[] m_backp;
	}

	inline void SetCanonical (YARPCanonicalNeurons *canonical) { m_canonical = canonical; }
	inline void SetCanonicalStats (YARPCanonicalStats *canonical) { m_canonical_stats = canonical; }
	
	inline YARPCanonicalNeurons *GetCanonical (void) { return m_canonical; }
	inline YARPCanonicalStats *GetCanonicalStats (void) { return m_canonical_stats; }

	inline void Activate (void) { m_active = true; }
	inline void Deactivate (void) { m_active = false; }
	inline bool IsActive (void) const { return m_active; }
	
	inline int GetLastKnownObject (void) const { return m_last_known_object; }	// returns -1 if no object.

	int GetLastKnownObjectPose (double& orientation, double& displacement) const;
	int SetLastKnownObjectPose (double orientation, double displacement);
	int ResetObject (void);

	int UpdateModels(void);
	int Find (YARPImageOf<YarpPixelBGR>& scan, YARPImageOf<YarpPixelBGR>& out, int& bestaction);
	int FindSimple (YARPImageOf<YarpPixelBGR>& scan, YARPImageOf<YarpPixelBGR>& out);
	int FindObject (YARPImageOf<YarpPixelBGR>& scan);
	int Segment (int object_no, YARPImageOf<YarpPixelBGR>& scan, YARPImageOf<YarpPixelBGR>& out, int& xx, int& yy);
};


#endif