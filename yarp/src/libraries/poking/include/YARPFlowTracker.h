//
// YARPFlowTracker.h
//
//


#ifndef __YARPFlowTrackerh__
#define __YARPFlowTrackerh__

#include "VisDMatrix.h"

#include "YARPImage.h"
#include "YARPPort.h"
#include "YARPImagePort.h"
#include "YARPImageDraw.h"
#include "YARPImageFile.h"
#include "YARPOpticFlowBM.h"
#include "YARPColorConverter.h"

#include "conf/tx_data.h"

#include "YARPTracker.h"
#include "YARPVisualContact.h"
#include "YARPVisualSearch.h"
#include "YARPBinaryAnalysis.h"

// from motor control lib.
#include "CogGaze.h"
#include "YARPMapCameras.h"

//
// class YARPImageSequence.
// data storage.
//


class YARPImageSequence
{
protected:
	int m_size;

	YARPImageOf<YarpPixelBGR> *m_array;
	CVisDVector *m_head_pos;
	CVisDVector *m_arm_pos;
	CVisDVector *m_arm_error;

	int m_counter;

	int m_start_pushing_frame;
	int m_stop_pushing_frame;

	int m_action_number;
	double m_reinforcer;

public:
	YARPImageSequence (int size, int w, int h); 
	virtual ~YARPImageSequence ();

	inline void Reset () { m_counter = 0; }

	void AddFrame (YARPImageOf<YarpPixelBGR>& i, CVisDVector& hj, CVisDVector& aj, CVisDVector& ae);
	int Save(char *dname, char *base); 
	int Load(char *dname, char *base);

	inline int GetNumElements (void) const { return m_counter; }

	YARPImageOf<YarpPixelBGR>& GetImageRef(int i);
	void GetImage (int i, YARPImageOf<YarpPixelBGR>& img) {}

	inline CVisDVector * GetArmPositionPtr (void) { return m_arm_pos; }
	inline CVisDVector * GetHeadPositionPtr (void) { return m_head_pos; }

	inline void SetStartPushing (void) { m_start_pushing_frame = m_counter; }
	inline void SetStopPushing (void) { m_stop_pushing_frame = m_counter; }
	inline int GetStartPushing (void) const { return m_start_pushing_frame; }
	inline int GetStopPushing (void) const { return m_stop_pushing_frame; }
	inline void SetWholeSequenceProcessing (void)
	{
		m_start_pushing_frame = 0;
		m_stop_pushing_frame = m_counter-1;
	}

	inline void SetActionID (int id) { m_action_number = id; }
	inline int GetActionID (void) const { return m_action_number; }

	inline void SetReinforcer (double r) { m_reinforcer = r; }
	inline int GetReinforcer (void) const { return m_reinforcer; }
};

class YARPOutputImageSequence : public YARPImageSequence
{
protected:
	int m_frame_of_contact;
	bool m_uptodate;

public:
	YARPOutputImageSequence (int size, int w, int h) : YARPImageSequence (size, w, h)
	{
		m_frame_of_contact = 0;
		m_uptodate = false;
	}

	virtual ~YARPOutputImageSequence () {}

	inline void Reset () 
	{ 
		m_counter = 0; 
		m_frame_of_contact = 0; 
		m_uptodate = false; 
	}

	int Save(char *dname, char *base); 
	int Load(char *dname, char *base);

	int GetFrameOfContact (void) const { return m_frame_of_contact; }

	int IncrementCounter (void) { m_counter++; return m_counter; }

	bool& UpToDate (void) { return m_uptodate; }
};




const int		MAXFLOW = 4;
const int		BLOCKSIZE = 16;
const int		BLOCKINC = 8;
const int		FRAMES_TRACKED = 12;
const double	CLASSIFICATION_THR = 0.6;
const int		MAXNUMBEROFACTIONS = 4;
const int		TEMPSIZE = 33;
const int		RANGE = 15;


//
//
//
//
//
//
class YARPFlowTracker 
{
protected:
	YARPOpticFlowBM<YarpPixelMono>	of;
	YARPTracker<YarpPixelBGR>		tracker;
	YARPBinaryAnalysis				ba;
	YARPVisualContact				processor;

	const int OOVERFLOW;

	int m_maxsize;
	int m_width, m_height;

	int **vx, **vy;	
	int ox, oy;

	YARPImageSequence *seq;
	YARPOutputImageSequence oseq;

	YARPImageOf<YarpPixelMono> mono, outimage;
	YARPImageOf<YarpPixelMono> extended_segmentation;
	YARPImageOf<YarpPixelMono> segmentation_mask;
	
	YARPImageOf<YarpPixelMono> segmentation_mask_copy;
	YARPImageOf<YarpPixelBGR>  output_image;

	YARPImageOf<YarpPixelMono> flipper_segmentation_mask;
	YARPImageOf<YarpPixelMono> flipper_segmentation_mask_copy;
	
	YARPImageOf<YarpPixelMono> mask;

	CVisDVector *trsf;
	CVisDVector cumulative_trsf;

	int contact;
	int contact_frame;
	int howmanycontacts;
	int lack_of_vectors;

	int com_x, com_y;
	int final_x, final_y;
	float dirx, diry;

	int dispframe;		// where to start the display from (12 frames).
	int m_stepping;

	double orientation, orientation_quality;

	// helper functions.
	int GrowMask (const YARPImageOf<YarpPixelMono>& src, YARPImageOf<YarpPixelMono>& dest);
	int ComputeRotation (YARPImageOf<YarpPixelMono>& mask, int *vx, int *vy, int ox, int oy, CVisDVector& trsf, int thr = 10);
	int WriteMask (YARPImageOf<YarpPixelMono>& mask, YARPImageOf<YarpPixelBGR>& out, YarpPixelBGR *color = NULL);
	int ComputeCumulativeTrsf (void);
	int CenterOfMass (YARPImageOf<YarpPixelMono>& in, int& x, int& y);
	int GenerateAndSend(YARPOutputPortOf<YARPGenericImage>& port);

public:
	YARPFlowTracker (int size, int width, int height);
	~YARPFlowTracker ();

	inline void Initialize (YARPImageSequence *s) { seq = s; }

	int Apply (YARPOutputPortOf<YARPGenericImage>& port);

	YARPImageOf<YarpPixelMono>& GetSegmentationMaskRef (void) { return segmentation_mask_copy; }
	inline YARPOutputImageSequence& GetOutputSeqRef (void) { return oseq; }
	inline YARPImageSequence& GetSeqRef (void) { return *seq; }

	int GetObjectProperties (YARPImageOf<YarpPixelMono>& mask, YARPImageOf<YarpPixelBGR>& frame);
	int GetDisplacement (CVisDVector& v1, CVisDVector& v2, CVisDVector& i1, CVisDVector& i2);
	int GetPushingDirection (CVisDVector& p);
	int GetMotorData (int& start, int& stop, CVisDVector **head, CVisDVector **arm, int& contact);
	int GetActionID (int& id);
	int GetReinforcer (double& r);
	int GetOrientation (double& o, double& quality);

	inline int GetDisplayStep (void) const { return m_stepping; }
	inline int GetDisplayFrame (void) const { return dispframe; }
	inline YARPVisualContact& GetVisualContactInstance (void) { return processor; }
};



#endif