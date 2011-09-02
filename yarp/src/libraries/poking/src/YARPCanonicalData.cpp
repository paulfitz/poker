//
// YARPCanonicalData.cc
//

#include "YARPCanonicalData.h"


void YARPCanonicalData::Create (int steps) 
{
	m_arm_trajectory = new CVisDVector[m_num_steps];
	m_head_trajectory = new CVisDVector[m_num_steps];

	assert (m_arm_trajectory != NULL);
	assert (m_head_trajectory != NULL);
}

void YARPCanonicalData::Destroy (void) 
{
	if (m_arm_trajectory != NULL) delete[] m_arm_trajectory;
	if (m_head_trajectory != NULL) delete[] m_head_trajectory;
}

void YARPCanonicalData::Resize (int steps)
{
	if (m_num_steps != steps)
	{
		if (m_arm_trajectory != NULL) delete[] m_arm_trajectory;
		if (m_head_trajectory != NULL) delete[] m_head_trajectory;

		m_num_steps = steps;

		m_arm_trajectory = new CVisDVector[m_num_steps];
		m_head_trajectory = new CVisDVector[m_num_steps];

		assert (m_arm_trajectory != NULL);
		assert (m_head_trajectory != NULL);

		m_sealed = false;
	}
	else
	{
		m_sealed = false;
	}
}

void YARPCanonicalData::Copy (const YARPCanonicalData& c) 
{
	for (int i = 0; i < m_num_steps; i++)
	{
		m_arm_trajectory[i] = c.m_arm_trajectory[i];
		m_head_trajectory[i] = c.m_head_trajectory[i];
	}
}

 
//
//
//
//
YARPCanonicalData::YARPCanonicalData(const YARPCanonicalData& c)
	: m_starting_position(3),
	  m_ending_position(3),
	  m_pushing_direction(2),
	  m_starting_position_image(2),
	  m_ending_position_image(2)
{
	m_reinforcer = c.m_reinforcer;
	m_contact_frame = c.m_contact_frame;

	m_frame = c.m_frame;
	m_mask = c.m_mask;

	m_num_steps = c.m_num_steps;
	Create (m_num_steps);
	Copy (c);

	m_starting_position = c.m_starting_position;
	m_ending_position = c.m_ending_position;
	m_pushing_direction = c.m_pushing_direction;
	m_starting_position_image = c.m_starting_position_image;
	m_ending_position_image = c.m_ending_position_image;

	m_action_number = c.m_action_number;

	m_object_orientation = c.m_object_orientation;
	m_object_orientation_quality = c.m_object_orientation_quality;

	m_sealed = c.m_sealed;
}

YARPCanonicalData::YARPCanonicalData(void)
	: m_starting_position(3),
	  m_ending_position(3),
	  m_pushing_direction(2),
	  m_starting_position_image(2),
	  m_ending_position_image(2)
{
	m_num_steps = 0;
	m_reinforcer = 0;
	m_contact_frame = 0;

	m_arm_trajectory = NULL;
	m_head_trajectory = NULL;

	m_starting_position = 0;
	m_ending_position = 0;
	m_pushing_direction = 0;
	m_starting_position_image = 0;
	m_ending_position_image = 0;

	m_action_number = -1;

	m_object_orientation = 0;
	m_object_orientation_quality = 0;

	m_sealed = false;
}

YARPCanonicalData::YARPCanonicalData(int steps)
	: m_starting_position(3),
	  m_ending_position(3),
	  m_pushing_direction(2),
	  m_starting_position_image(2),
	  m_ending_position_image(2)
{
	m_num_steps = steps;
	m_reinforcer = 0;
	m_contact_frame = 0;

	Create (m_num_steps);

	m_starting_position = 0;
	m_ending_position = 0;
	m_pushing_direction = 0;
	m_starting_position_image = 0;
	m_ending_position_image = 0;

	m_action_number = -1;

	m_object_orientation = 0;
	m_object_orientation_quality = 0;

	m_sealed = false;
}

void YARPCanonicalData::operator=(const YARPCanonicalData& c)
{
	m_reinforcer = c.m_reinforcer;
	m_contact_frame = c.m_contact_frame;

	m_frame = c.m_frame;
	m_mask = c.m_mask;

	Resize (c.m_num_steps);
	m_num_steps = c.m_num_steps;
	Copy (c);

	m_starting_position = c.m_starting_position;
	m_ending_position = c.m_ending_position;
	m_pushing_direction = c.m_pushing_direction;
	m_starting_position_image = c.m_starting_position_image;
	m_ending_position_image = c.m_ending_position_image;

	m_action_number = c.m_action_number;

	m_object_orientation = c.m_object_orientation;
	m_object_orientation_quality = c.m_object_orientation_quality;

	m_sealed = c.m_sealed;
}


YARPCanonicalData::~YARPCanonicalData()
{
	Destroy ();
}

int YARPCanonicalData::WriteV (FILE *fp, const CVisDVector& v)
{
	fprintf (fp, "%d ", v.Length());

	for (int i = 1; i <= v.Length(); i++)
	{
		fprintf (fp, "%lf ", v(i));
	}
	fprintf (fp, "\n");
	return 0;
}

int YARPCanonicalData::ReadV (FILE *fp, CVisDVector& v)
{
	int l = 0;
	double tmp;

	fscanf (fp, "%d ", &l);
	v.Resize (l);

	for (int i = 1; i <= l; i++)
	{
		fscanf (fp, "%lf ", &tmp);
		v(i) = tmp;
	}
	fscanf (fp, "\n");

	return 0;
}

int YARPCanonicalData::Save (FILE *fp, char *name)
{
	fprintf (fp, "trace with name %s\n", name);
	fprintf (fp, "m_starting_position : "); WriteV(fp, m_starting_position);
	fprintf (fp, "m_ending_position : ");	WriteV(fp, m_ending_position);
	fprintf (fp, "m_pushing_direction : "); WriteV(fp, m_pushing_direction);
	fprintf (fp, "m_starting_position_image : "); WriteV(fp, m_starting_position_image);
	fprintf (fp, "m_ending_position_image : ");	WriteV(fp, m_ending_position_image);
	fprintf (fp, "m_contact_frame : %d\n", m_contact_frame); 
	fprintf (fp, "m_num_steps : %d\n", m_num_steps); 

	fprintf (fp, "arm_trajectory :\n");
	for (int i = 0; i < m_num_steps; i++)
	{
		WriteV (fp, m_arm_trajectory[i]);
	}
	fprintf (fp, "head_trajectory :\n");
	for (int i = 0; i < m_num_steps; i++)
	{
		WriteV (fp, m_head_trajectory[i]);
	}
	fprintf (fp, "m_reinforcer : %lf\n", m_reinforcer);
	fprintf (fp, "m_action_number : %d\n", m_action_number);
	fprintf (fp, "m_object_orientation : %lf\n", m_object_orientation);
	fprintf (fp, "m_object_orientation_quality : %lf\n", m_object_orientation_quality);
	fprintf (fp, "m_sealed : %d\n", m_sealed);

	fprintf (fp, "width, height : %d %d\n", m_mask.GetWidth(), m_mask.GetHeight());

	char tname[512];
	sprintf (tname, "%s_mask.pgm\0", name); 		
	int ret = YARPImageFile::Write (tname, m_mask);
	if (ret < 0)
	{
		printf ("YARPCanonicalData: can't save frame %s\n", tname);
		return -1;
	}

	sprintf (tname, "%s_contact.ppm\0", name); 		
	ret = YARPImageFile::Write (tname, m_frame);
	if (ret < 0)
	{
		printf ("YARPCanonicalData: can't save frame %s\n", tname);
		return -1;
	}

	return 0;
}

int YARPCanonicalData::Load (FILE *fp, char *name)
{
	char buf[512];
	sprintf (buf, "trace with name %s\n\0", name);
	fscanf (fp, buf);

	fscanf (fp, "m_starting_position : "); ReadV(fp, m_starting_position);
	fscanf (fp, "m_ending_position : ");   ReadV(fp, m_ending_position);
	fscanf (fp, "m_pushing_direction : "); ReadV(fp, m_pushing_direction);
	fscanf (fp, "m_starting_position_image : "); ReadV(fp, m_starting_position_image);
	fscanf (fp, "m_ending_position_image : ");	ReadV(fp, m_ending_position_image);
	fscanf (fp, "m_contact_frame : %d\n", &m_contact_frame); 
	
	int tnumsteps = 0;
	fscanf (fp, "m_num_steps : %d\n", &tnumsteps); 

	Resize (tnumsteps);
	m_num_steps = tnumsteps;
	printf ("num_steps : %d\n", m_num_steps);

	fscanf (fp, "arm_trajectory :\n");
	for (int i = 0; i < m_num_steps; i++)
	{
		ReadV (fp, m_arm_trajectory[i]);
	}
	fscanf (fp, "head_trajectory :\n");
	for (int i = 0; i < m_num_steps; i++)
	{
		ReadV (fp, m_head_trajectory[i]);
	}
	fscanf (fp, "m_reinforcer : %lf\n", &m_reinforcer);
	fscanf (fp, "m_action_number : %d\n", &m_action_number);
	fscanf (fp, "m_object_orientation : %lf\n", &m_object_orientation);
	fscanf (fp, "m_object_orientation_quality : %lf\n", &m_object_orientation_quality);
	fscanf (fp, "m_sealed : %d\n", &m_sealed);

	int width, height;
	fscanf (fp, "width, height : %d %d\n", &width, &height);

	m_mask.Resize (width, height);
	m_frame.Resize (width, height);

	char tname[512];
	sprintf (tname, "%s_mask.pgm\0", name); 		
	int ret = YARPImageFile::Read (tname, m_mask);
	if (ret < 0)
	{
		printf ("YARPCanonicalData: can't load frame %s\n", tname);
		return -1;
	}

	sprintf (tname, "%s_contact.ppm\0", name); 		
	ret = YARPImageFile::Read (tname, m_frame);
	if (ret < 0)
	{
		printf ("YARPCanonicalData: can't load frame %s\n", tname);
		return -1;
	}

	return 0;
}

int YARPCanonicalNeuron::Save (FILE *fp, char *name)
{
	char tname[512];
	fprintf (fp, "neuron with name %s has %d elements\n", name, size());

	printf ("saving neuron %s of size %d\n", name, size());

	if (!empty())
	{
		POSITION index = begin();
		int counter = 0;

		while (index != end())
		{
			sprintf (tname, "%s_%04d\0", name, counter);
			int ret = (*index).Save (fp, tname); 

			if (ret < 0)
			{
				printf ("YARPCanonicalNeuron: can't save with name\n", tname);
				return -1;
			}

			counter ++;
			index ++;
		}
	}

	return 0;
}

int YARPCanonicalNeuron::Load (FILE *fp, char *name)
{
	char tname[512];
	
	int size = 0;
	char buf[512];
	sprintf (buf, "neuron with name %s has ", name);
	fscanf (fp, buf);
	fscanf (fp, "%d elements\n", &size); 

	printf ("loading neuron %s of size %d\n", name, size);

	erase (begin(), end());

	YARPCanonicalData u;
	
	// LATER: pass arguments width, height to load
	//u.m_mask.Resize (WIDTH, HEIGHT);
	//u.m_frame.Resize (WIDTH, HEIGHT);

	for (int i = 0; i < size; i++)
	{
		sprintf (tname, "%s_%04d\0", name, i);
		int ret = u.Load (fp, tname);

		if (ret < 0)
		{
			printf ("YARPCanonicalNeuron: can't load with name\n", tname);
			return -1;
		}

		Add(u);
	}

	return 0;
}


//
//
//
YARPCanonicalNeurons::YARPCanonicalNeurons (int size, int width, int height)
{
	m_size = size;
	m_width = width;
	m_height = height;
	
	m_units = new YARPCanonicalNeuron[m_size];
	assert (m_units != NULL);

	m_num_objects = 0;
}

YARPCanonicalNeurons::~YARPCanonicalNeurons ()
{
	if (m_units != NULL) delete[] m_units;
}

// returns the number of the added unit.
int YARPCanonicalNeurons::AddNewObject (void)
{
	m_num_objects ++;
	return m_num_objects-1;	
}

int YARPCanonicalNeurons::Save (char *name)
{
	char tname[512];
	sprintf (tname, "%s.txt", name);

	FILE *fp = fopen (tname, "w");
	if (fp == NULL)
	{
		printf ("can't save to %s\n", tname);
		return -1;
	}
			
	fprintf (fp, "Saving to name %s\n", tname);
	fprintf (fp, "m_size : %d\n", m_size);
	fprintf (fp, "m_width : %d\n", m_width);
	fprintf (fp, "m_height : %d\n", m_height);
	fprintf (fp, "m_num_objects : %d\n", m_num_objects);

	for (int i = 0; i < m_num_objects; i++)
	{
		sprintf (tname, "%s_%04d\0", name, i);
		m_units[i].Save (fp, tname);
	}

	fclose (fp);
	return 0;
}

int YARPCanonicalNeurons::Load (char *name)
{
	char tname[512];
	sprintf (tname, "%s.txt", name);

	FILE *fp = fopen (tname, "r");
	if (fp == NULL)
	{
		printf ("can't load from %s\n", tname);
		return -1;
	}
			
	char buf[512];
	sprintf (buf, "Saving to name %s\n", tname); 
	printf ("buf: %s\n", buf);

	fscanf (fp, buf);

	fscanf (fp, "m_size : %d\n", &m_size);
	fscanf (fp, "m_width : %d\n", &m_width);
	fscanf (fp, "m_height : %d\n", &m_height);
	fscanf (fp, "m_num_objects : %d\n", &m_num_objects);

	printf ("loading %d %d %d %d\n", m_size, m_width, m_height, m_num_objects);
	// memory is already allocated.

	for (int i = 0; i < m_num_objects; i++)
	{
		sprintf (tname, "%s_%04d\0", name, i);
		m_units[i].Load (fp, tname);
	}

	fclose (fp);
	return 0;
}




//
//
//
//
//
//
//


YARPCanonicalStats::YARPCanonicalStats (int size, int nbins)
{
	m_size = size;
	m_nbins = nbins;
	m_goodneurons = 0;
	m_threshold = 50;

	m_units = new YARPCanonicalNeuron *[m_size];
	assert (m_units != NULL);

	memset (m_units, 0, sizeof(YARPCanonicalNeuron *) * m_size);

	m_neuron_numbers.Resize (m_size);
	m_neuron_numbers = -1;

	m_pointiness.Resize (m_size);
	m_pointiness_std.Resize (m_size);
	m_pointiness_elements.Resize (m_size);

	m_probability.Resize (m_size, m_nbins);

	m_actions = new CVisDMatrix[m_size];
	assert (m_actions != NULL);
	m_actions_std = new CVisDMatrix[m_size];
	assert (m_actions_std != NULL);
	m_actions_elements = new CVisDMatrix[m_size];
	assert (m_actions_elements != NULL);

	for (int i = 0; i < m_size; i++)
	{	
		m_actions[i].Resize (ACTIONS, m_nbins);
		m_actions_std[i].Resize (ACTIONS, m_nbins);
		m_actions_elements[i].Resize (ACTIONS, m_nbins);
	}

	m_mirror_actions = new CVisDMatrix[m_size];
	m_mirror_tmp = new CVisDMatrix[ACTIONS];

	assert (m_mirror_actions != NULL);
	assert (m_mirror_tmp != NULL);

	for (int i = 0; i < m_size; i++) m_mirror_actions[i].Resize (m_nbins, m_nbins);
	for (int i = 0; i < ACTIONS; i++) m_mirror_tmp[i].Resize (m_nbins, m_nbins);

	m_directions.Resize (ACTIONS);
	m_directions_std.Resize (ACTIONS);
	m_directions = 0;
	m_directions_std = 0;

	m_directions_tmp.Resize (2, ACTIONS);
	m_directions_tmp_std.Resize (2, ACTIONS);
	m_directions_tmp = 0;
	m_directions_tmp_std = 0;
	m_total_samples = 0;
}

YARPCanonicalStats::~YARPCanonicalStats ()
{
	if (m_units != NULL) delete[] m_units;

	if (m_actions != NULL) delete[] m_actions;
	if (m_actions_std != NULL) delete[] m_actions_std;
	if (m_actions_elements != NULL) delete[] m_actions_elements;

	if (m_mirror_actions != NULL) delete[] m_mirror_actions;
	if (m_mirror_tmp != NULL) delete[] m_mirror_tmp;
}

int YARPCanonicalStats::UpdateStats (YARPCanonicalNeurons& canonical)
{
	if (canonical.Empty())
	{
		printf ("YARPCanonicalStats: nothing to stats from\n");
		return -1;
	}

	m_goodneurons = 0;
	m_pointiness = 0;
	m_pointiness_std = 0;
	m_pointiness_elements = 0;
	m_probability = 0;

	for (int z = 0; z < m_size; z++)
	{
		m_actions[z] = 0;
		m_actions_std[z] = 0;
		m_actions_elements[z] = 0;
	}

	CVisDVector v(2);
	v = 0;

	m_directions = 0;
	m_directions_std = 0;
	m_directions_tmp = 0;
	m_directions_tmp_std = 0;
	m_total_samples = 0;

	//
	//
	//
	for (int i = 0; i <= canonical.End(); i++)
	{
		YARPCanonicalNeuron& n = canonical[i];

		if (n.size() > m_threshold)
		{
			m_units[m_goodneurons] = &canonical[i];
			m_mirror_actions[m_goodneurons] = 0;

			m_goodneurons ++;

			m_neuron_numbers(m_goodneurons) = i;

			for (int l = 0; l < ACTIONS; l++) m_mirror_tmp[l] = 0;

			n.InitCursor();
			while (!n.EndCursor ())
			{
				YARPCanonicalData& data = n.Get();

				CVisDVector& s1 = data.m_starting_position_image;
				CVisDVector& s2 = data.m_ending_position_image;

				v = s2 - s1;
				double phase = atan2(v(2), v(1));
//				if (phase > pi/2) phase -= pi;
//				if (phase < pi/2) phase += pi;

				double diff = phase - data.m_object_orientation;
				diff = fabs (fmod (diff, pi));
				if (diff > pi/2) diff = pi - diff;

				// diff is the orientation difference.
				m_pointiness (m_goodneurons) += data.m_object_orientation_quality;
				m_pointiness_std (m_goodneurons) += (data.m_object_orientation_quality * data.m_object_orientation_quality);
				m_pointiness_elements (m_goodneurons) += 1;

				//
				int index = int(diff / (pi/2/m_nbins)) + 1;
				m_probability (m_goodneurons, index) += 1;

				//
				int index1 = int (fmod (data.m_object_orientation+pi, pi) / (pi / m_nbins)) + 1;
				int index2 = int (fmod (phase+pi, pi) / (pi / m_nbins)) + 1;
				
				assert (index1 >= 1 && index1 <= m_nbins && index2 >= 1 && index2 <= m_nbins);

				m_mirror_tmp[data.m_action_number] (index2, index1) += 1;

				m_directions_tmp (1, data.m_action_number+1) += cos (phase);
				m_directions_tmp (2, data.m_action_number+1) += sin (phase);
				m_directions_tmp_std (1, data.m_action_number+1) += (cos (phase) * cos(phase));
				m_directions_tmp_std (2, data.m_action_number+1) += (sin (phase) * sin(phase));
				m_total_samples ++;

				n.IncrementCursor ();
			}

			m_pointiness(m_goodneurons) /= m_pointiness_elements(m_goodneurons);
			m_pointiness_std(m_goodneurons) = sqrt(m_pointiness_std(m_goodneurons) / (m_pointiness_elements(m_goodneurons)-1) - (m_pointiness_elements(m_goodneurons))/(m_pointiness_elements(m_goodneurons)-1) * m_pointiness(m_goodneurons) * m_pointiness(m_goodneurons));

			double sum = 0;
			for (int k = 1; k <= m_nbins; k++)
				sum += m_probability (m_goodneurons, k);

			for (int k = 1; k <= m_nbins; k++)
				m_probability (m_goodneurons, k) /= sum;

			//
			//
			for (int k = 1; k <= m_nbins; k++)
			{
				for (int l = 1; l <= m_nbins; l++)
				{
					int maxa = 0;
					int max = -1;

					for (int a = 0; a < ACTIONS; a++)
						if (m_mirror_tmp[a] (l, k) > max)
						{
							max = m_mirror_tmp[a] (l, k);
							maxa = a;
						}

					if (max == 0)
						m_mirror_actions[m_goodneurons-1] (l, k) = -1;
					else
						m_mirror_actions[m_goodneurons-1] (l, k) = maxa;
				}
			}

			//
			//

			for (int action = 1; action <= ACTIONS; action++)
			{
				n.InitCursor();
				while (!n.EndCursor ())
				{
					YARPCanonicalData& data = n.Get();
					if (data.m_action_number == action-1)
					{
						const double direction = data.m_object_orientation;
						int index = int (fmod (direction+pi, pi) / (pi / m_nbins)) + 1;

						CVisDVector& s1 = data.m_starting_position_image;
						CVisDVector& s2 = data.m_ending_position_image;
						v = s2 - s1;
						double phase = atan2(v(2), v(1));
//						if (phase > pi/2) phase -= pi;
//						if (phase < pi/2) phase += pi;
						double diff = phase - data.m_object_orientation;
						diff = fabs (fmod (diff, pi));
						if (diff > pi/2) diff = pi - diff;

						int index2 = int (diff / (pi/2 / m_nbins)) + 1;
						double weight = v.norm2() * m_probability (m_goodneurons, index2);

						m_actions[m_goodneurons-1] (action, index) += weight;
						m_actions_std[m_goodneurons-1] (action, index) += (weight * weight);
						m_actions_elements[m_goodneurons-1] (action, index) += 1;
					}
					n.IncrementCursor ();
				}

				for (int i = 1; i <= m_nbins; i++)
				{
					const double n = m_actions_elements[m_goodneurons-1](action, i);
					if (n < 3)
					{
						m_actions[m_goodneurons-1](action, i) = 0;
						m_actions_std[m_goodneurons-1](action, i) = 0;
					}
					else
					{
						m_actions[m_goodneurons-1](action, i) /= n;
						m_actions_std[m_goodneurons-1](action, i) =
							sqrt (m_actions[m_goodneurons-1](action, i) / (n-1) - (n/(n-1) * m_actions[m_goodneurons-1](action,i) * m_actions[m_goodneurons-1](action,i)));
					}
				}
			}

			//
			//
			//

		} /* if threshold */
	} /* end for */


	//
	for (int i = 1; i <= ACTIONS; i++)
	{
		m_directions_tmp (1,i) /= m_total_samples;
		m_directions_tmp (2,i) /= m_total_samples;
		m_directions_tmp_std (1,i) = 
				sqrt(m_directions_tmp_std (1,i) / (m_total_samples-1) - 
				m_total_samples / (m_total_samples-1) * 
				m_directions_tmp(1,i) * m_directions_tmp(1,i));
		m_directions_tmp_std (2,i) = 
				sqrt(m_directions_tmp_std (2,i) / (m_total_samples-1) - 
				m_total_samples / (m_total_samples-1) * 
				m_directions_tmp(2,i) * m_directions_tmp(2,i));

		m_directions(i) = atan2 (m_directions_tmp(2,i), m_directions_tmp(1,i));
		m_directions_std(i) = atan2 (m_directions_tmp_std(2,i), m_directions_tmp_std(1,i));
	}

	printf ("There are %d objects in memory\n", m_goodneurons);
	return 0;
}

double YARPCanonicalStats::GetPrincipalAffordance (int object)
{
	object++;

	int index = 0;
	double max = -1;
	for (int i = 1; i <= m_nbins; i++)
	{
		if (m_probability(object, i) > max)
		{
			max = m_probability(object, i);
			index = i;
		}
	}
	
	if (index == 0)
	{
		printf ("can't find affordance for object %d\n", object);
		return -1.0;
	}

	return (index - 1) * pi/2 / m_nbins;
}


int YARPCanonicalStats::GetBestActionGeneric (double orientation, double& score)
{
	// orientation is between -pi/2 and pi/2
	// as returned by the search.
	
	// look for min distance (disregard std dev).

	double mindist = 10*pi;
	int action = -1;

	for (int i = 1; i <= ACTIONS; i++)
	{
		double comparewith = m_directions(i);
		if (m_directions(i) > pi/2)
			comparewith -= pi;
		else
		if (m_directions(i) < -pi/2)
			comparewith += pi;

		double diff = fabs (comparewith - orientation);
		if (diff < mindist)
		{
			mindist = diff;
			action = i;
		}
	}

	score = mindist;
	return action-1;
}

int YARPCanonicalStats::GetMirrorAction (int object, double orientation, double direction)
{
	if (object < 0 || object >= m_goodneurons)
	{
		printf ("*** YARPCanonicalStats, asking for the wrong object\n");
		return -1;
	}

	int index1 = int (fmod (orientation+pi, pi) / (pi / m_nbins)) + 1;
	int index2 = int (fmod (direction+pi, pi) / (pi / m_nbins)) + 1;
	return int(m_mirror_actions[object] (index2, index1));
}


// this is a little redundant. It might be useful to check the computation though.
//

int YARPCanonicalStats::Save (char *name)
{
	FILE *fp = fopen (name, "w");
	if (fp == NULL)
	{
		printf ("YARPCanonicalStats: can't save to file %s\n", name);
		return -1;
	}

	fprintf (fp, "YARPCanonicalStats: %s\n", name);
	fprintf (fp, "m_goodneurons: %d\n", m_goodneurons);
	fprintf (fp, "m_size: %d\n", m_size);
	fprintf (fp, "m_nbins: %d\n", m_nbins);
	fprintf (fp, "m_threshold: %d\n", m_threshold);

	fprintf (fp, "pointiness: ");
	for (int i = 1; i <= m_goodneurons; i++)
	{
		fprintf (fp, "%lf ", m_pointiness(i));
	}
	fprintf (fp, "\n");

	fprintf (fp, "probability:\n");
	for (int i = 1; i <= m_goodneurons; i++)
	{
		for (int j = 1; j <= m_nbins; j++)
		{
			fprintf (fp, "%lf ", m_probability(i, j));
		}
		fprintf (fp, "\n");
	}
	fprintf (fp, "\n");

	for (int k = 0; k < m_goodneurons; k++)
	{
		fprintf (fp, "object: %d\n", k);
		
		for (int i = 1; i <= ACTIONS; i++)
		{
			fprintf (fp, "action: %d\n", i);

			for (int j = 1; j <= m_nbins; j++)
			{
				fprintf (fp, "%lf ", m_actions[k](i,j));
			}

			fprintf (fp, "\n");
		}
		fprintf (fp, "\n");
	}

	for (int k = 0; k < m_goodneurons; k++)
	{
		fprintf (fp, "object: %d\n", k);

		for (int l = 1; l <= m_nbins; l++)
		{
			for (int m = 1; m <= m_nbins; m++)
			{
				fprintf (fp, "%lf ", m_mirror_actions[k] (l, m));
			}
		
			fprintf (fp, "\n");
		}

		fprintf (fp, "\n");
	}

	for (int k = 1; k <= ACTIONS; k++)
	{
		fprintf (fp, "action: %d, mean: %lf, std: %lf\n", k, m_directions(k), m_directions_std(k));
	}

	fclose (fp);

	return 0;
}

int YARPCanonicalStats::Load (char *name)
{
	return -1;
}

int YARPCanonicalStats::Print (void)
{
	printf ("YARPCanonicalStats: printing\n");
	printf ("goodneurons: %d\n", m_goodneurons);
	printf ("size: %d\n", m_size);
	printf ("nbins: %d\n", m_nbins);
	printf ("threshold: %d\n", m_threshold);

	printf ("pointiness: ");
	for (int i = 1; i <= m_goodneurons; i++)
	{
		printf ("%lf ", m_pointiness(i));
	}
	printf ("\n");

	printf ("probability:\n");
	for (int i = 1; i <= m_goodneurons; i++)
	{
		for (int j = 1; j <= m_nbins; j++)
		{
			printf ("%lf ", m_probability(i, j));
		}
		printf ("\n");
	}
	printf ("\n");

	for (int k = 0; k < m_goodneurons; k++)
	{
		printf ("object: %d\n", k);
		
		for (int i = 1; i <= ACTIONS; i++)
		{
			printf ("action: %d\n", i);

			for (int j = 1; j <= m_nbins; j++)
			{
				printf ("%lf ", m_actions[k](i,j));
			}

			printf ("\n");
		}
		printf ("\n");
	}


	return 0;
}
