//
// YARPCanonicalData.h
//
//


#ifndef __YARPCanonicalDatah__
#define __YARPCanonicalDatah__

#ifdef __QNX__NONONO
#include "vector.h"
#else
#include <vector>
#ifndef __QNX__
using namespace std;
#endif
#endif

#include "stdio.h"
#include "VisMatrix.h"
#include "YARPImage.h"
#include "YARPImageFile.h"

class YARPCanonicalData
{
protected:
	int WriteV (FILE *fp, const CVisDVector& v);
	int ReadV (FILE *fp, CVisDVector& v);

public:
	YARPImageOf<YarpPixelMono>	m_mask;
	YARPImageOf<YarpPixelBGR>	m_frame;
	CVisDVector					m_starting_position;
	CVisDVector					m_ending_position;
	CVisDVector					m_pushing_direction;
	CVisDVector					m_starting_position_image;
	CVisDVector					m_ending_position_image;
	int							m_contact_frame;
	int							m_num_steps;
	CVisDVector				   *m_arm_trajectory;
	CVisDVector				   *m_head_trajectory;
	double						m_reinforcer;
	int							m_action_number;
	
	double						m_object_orientation;
	double						m_object_orientation_quality;

	bool						m_sealed;

	YARPCanonicalData (int steps);
	YARPCanonicalData (void);
	YARPCanonicalData (const YARPCanonicalData& c);
	void operator= (const YARPCanonicalData& c);

	~YARPCanonicalData (void);

	void Create (int steps);
	void Destroy (void); 
	void Resize (int steps);
	void Copy (const YARPCanonicalData& c);

	void Seal (void) { m_sealed = true; }

	int Save (FILE *fp, char *name);
	int Load (FILE *fp, char *name);
};

//
//
//
class YARPCanonicalNeuron : public vector<YARPCanonicalData>
{
protected:
	typedef vector<YARPCanonicalData>::iterator POSITION;
	POSITION cursor;

public:
	YARPCanonicalNeuron () {}
	~YARPCanonicalNeuron () {}


	inline void Add (YARPCanonicalData& d) { push_back(d); }
	inline void Reset (void) { erase(begin(), end()); }
	inline bool Empty (void) { return empty(); }
	
	// to scan the list.
	inline void InitCursor (void) { cursor = begin(); }
	inline YARPCanonicalData& Get(void) { return *cursor; }
	inline bool EndCursor() { return (cursor == end()) ? true : false; }
	inline void IncrementCursor (void) { cursor++; }

	inline vector<YARPCanonicalData>::iterator GetCursor (void) const { return cursor; }
	inline void SetCursor (vector<YARPCanonicalData>::iterator c) { cursor = c; }

	int Save (FILE *fp, char *name);
	int Load (FILE *fp, char *name);
};

//
//
//
class YARPCanonicalNeurons
{
protected:
	YARPCanonicalNeuron *m_units;

	int m_size, m_width, m_height;
	int m_num_objects;

public:
	YARPCanonicalNeurons (int size, int width, int height);
	~YARPCanonicalNeurons ();

	int AddNewObject (void);

	inline int GetNumObjects(void) const { return m_num_objects; }
	inline int GetWidth(void) const { return m_width; }
	inline int GetHeight(void) const { return m_height; }

	inline int Begin (void) const { return 0; }
	inline int End (void) const { return m_num_objects-1; }
	inline bool Empty (void) const { return (m_num_objects == 0) ? true : false; }

	inline YARPCanonicalNeuron& operator[](int i) { return m_units[i]; }

	int Save (char *name);
	int Load (char *name);
};


//
//
//
class YARPCanonicalStats
{
public:
	enum { ACTIONS = 4 };

public:
	YARPCanonicalNeuron **m_units;
	CVisDVector m_neuron_numbers;

	CVisDVector m_pointiness;
	CVisDVector m_pointiness_std;
	CVisDVector m_pointiness_elements;

	CVisDMatrix m_directions_tmp;
	CVisDMatrix m_directions_tmp_std;
	CVisDVector m_directions;
	CVisDVector m_directions_std;
	int m_total_samples;

	int m_nbins;
	int m_size;
	int m_goodneurons;
	int m_threshold;

	CVisDMatrix m_probability;
	CVisDMatrix *m_actions;
	CVisDMatrix *m_actions_std;
	CVisDMatrix *m_actions_elements;

	CVisDMatrix *m_mirror_tmp;
	CVisDMatrix *m_mirror_actions;

	YARPCanonicalStats (int size, int nbins);
	~YARPCanonicalStats ();

	int UpdateStats (YARPCanonicalNeurons& canonical);
	int GetMirrorAction (int object, double orientation, double displacement);
	int GetBestActionGeneric (double orientation, double& score);
	double GetPrincipalAffordance (int object);

	int Save (char *name);
	int Load (char *name);

	int Print (void);
};

#endif
