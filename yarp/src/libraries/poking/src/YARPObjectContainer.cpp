//#ifndef __LINUX__
//#define __LINUX__
//#endif

#ifndef __LINUX__


//
// YARPObjectContainer.cpp
//

#include "YARPObjectContainer.h"

#ifdef std
#undef std
#endif


//
//
//
#define SCALE 1.0

//
//
//
//
//
int YARPObjectContainer::UpdateModels()
{
	//m_canonical = canonical;
	if (m_canonical == NULL || m_canonical_stats == NULL)
	{
		printf ("YARPObjectContainer: data is not allocated yet\n");
		return -1;
	}

	m_canonical_stats->UpdateStats(*m_canonical);
	m_canonical_stats->Print ();

	printf ("there are %d objects to process\n", m_canonical_stats->m_goodneurons);
	const int NOBJ = m_canonical_stats->m_goodneurons;

	// build a locator for each object.
	// 0.1 means 10% of the max (255).
	if (m_locator != NULL) delete[] m_locator;

	m_locator = new YARPColorLocator[NOBJ];
	for (int obj = 0; obj < NOBJ; obj++)
		m_locator[obj].Resize (m_canonical->GetWidth(), 15, 15, 0.3, 0.3);

	// building model out of many frames.
	printf ("building models\n");
	for (int obj = 0; obj < NOBJ; obj++)
	{
		YARPCanonicalNeuron& n = (*m_canonical_stats->m_units[obj]);

		printf ("neuron number: %d\n", obj);
		printf ("number of frames: %d\n", n.size());

		n.InitCursor ();
		m_locator[obj].Reset();
		while (!n.EndCursor ())
		{
			YARPCanonicalData& data = n.Get();

			YARPImageOf<YarpPixelBGR>& model = data.m_frame;
			YARPImageOf<YarpPixelMono>& mask = data.m_mask;

			m_locator[obj].Apply (model, mask, false);			
			n.IncrementCursor ();
		}

		double ex,ey;
		m_locator[obj].GetExtent(ex, ey);
		printf ("extent of model x y: %lf %lf\n", ex, ey);
	}

	if (m_backp != NULL) delete[] m_backp;
	m_backp = new YARPImageOf<YarpPixelBGR>[NOBJ];
	for (int obj = 0; obj < NOBJ; obj++)
		m_backp[obj].Resize (m_canonical->GetWidth(), m_canonical->GetHeight());



	printf ("building model thresholds, it may take a while...\n");
	for (int obj = 0; obj < NOBJ; obj++)
	{
		YARPCanonicalNeuron& n = (*m_canonical_stats->m_units[obj]);

		printf ("neuron number: %d\n", obj);
		printf ("number of frames: %d\n", n.size());

		n.InitCursor ();
		while (!n.EndCursor ())
		{
			YARPCanonicalData& data = n.Get();

			YARPImageOf<YarpPixelBGR>& model = data.m_frame;
			m_locator[obj].AddExpectancy (model);			

			n.IncrementCursor ();
		}

		double thr,std;
		m_locator[obj].GetExpectancy(thr, std);
		printf ("mean and stddev: %lf %lf\n", thr, std);
	}



	// models are up to date.
	Activate();

	return 0;
}



//
//
//
//
//  there's messy code here. e.g. angles are represented in two different ranges (+-pi/2, and +-pi).


int YARPObjectContainer::Find (YARPImageOf<YarpPixelBGR>& scan, YARPImageOf<YarpPixelBGR>& out, int& bestaction)
{
	if (!m_active)
	{
		printf ("YARPObjectContainer: need to update stats first\n");
		bestaction = -1;
		out.PeerCopy(scan);
		m_last_known_object = -1;
		m_orientation = 0;
		m_displacement = 0;
		return -1;
	}

	YarpPixelBGR red;
	red.r = 255;
	red.g = red.b = 0;

	double x, y, quality;
	double max_quality = 0; 
	int max_obj = -1;
	double max_x = 0, max_y = 0;

	const int NOBJ = m_canonical_stats->m_goodneurons;

	for (int obj = 0; obj < NOBJ; obj++)
	{
		m_locator[obj].BackProject (scan, m_backp[obj]);

		double ex, ey;
		m_locator[obj].GetExtent (ex, ey);

		ex *= SCALE;
		ey *= SCALE;

		if (m_locator[obj].Find (ex, ey, x, y, quality) >= 0)
		{
			double mean = 0, stddev = 0;
			const double THR = 2.0;
			m_locator[obj].GetExpectancy (mean, stddev);

			bool thr_cond = (fabs(quality - mean) < stddev * THR) ? true : false;

			printf ("object: %d location: %lf %lf q: %lf\n", obj, x, y, quality);

			if (quality > max_quality && thr_cond)
			{
				max_quality = quality;
				max_obj = obj;
				max_x = x;
				max_y = y;
			}
		}
	}

	m_last_known_object = max_obj;

	if (max_obj == -1)
	{
		printf ("I (COG) don't know about this object, if there's one at all!\n");
		bestaction = -1;
		out.PeerCopy(scan);
		m_orientation = 0;
		m_displacement = 0;
		return -1;
	}

	// if max_quality is not good enough then skip the following search.
	// if pointiness is not good enough then skip the following search.
	printf ("object is %d, improving position\n", max_obj);

	double ex, ey;
	m_locator[max_obj].GetExtent (ex, ey);
	ex *= SCALE;
	ey *= SCALE;

	// try to improve the position estimation.
	double betterx = 0, bettery = 0;
	m_locator[max_obj].ImprovedSearch (scan, ex, ey, max_x, max_y, betterx, bettery);

	printf ("object is %d, looking for orientation\n", max_obj);

	double angle = -1;
	double angle2 = -1;
	double ave = 0;
	double std = 0;
	m_locator[max_obj].GetNumberOfPoints (ave, std);

	// need to add a threshold on pointiness!
	YARPSearchRotation sr (50, 0.0);
	sr.Search (int(m_canonical_stats->m_neuron_numbers(max_obj+1)), *m_canonical, scan, betterx, bettery, ave, std, angle, angle2);
	
	// store for future use.
	m_orientation = angle;
	m_displacement = 0;

	printf ("orientation: %lf\n", angle * radToDeg);
	int x1 = betterx + cos(angle) * 40;
	int y1 = bettery + sin(angle) * 40;
	int x2 = betterx - cos(angle) * 40;
	int y2 = bettery - sin(angle) * 40;
	AddSegment (m_backp[max_obj], red, x1, y1, x2, y2);
	AddRectangle (m_backp[max_obj], red, int(betterx+.5), int(bettery+.5), int (ex/2+.5), int (ey/2+.5));

	AddCircleOutline (m_backp[max_obj], red, int(max_x+.5), int(max_y+.5), 10);
	AddCircle (m_backp[max_obj], red, int(betterx+.5), int(bettery+.5), 5);

	// return processed image.
	out.PeerCopy (m_backp[max_obj]);

	// angle is the current orientation.
	// search for the best affordance.
	const double affordance = m_canonical_stats->GetPrincipalAffordance (max_obj);
	double o1 = angle+affordance;
	double o2 = angle-affordance;
	printf ("---- object: %d, affordance: %lf, test1: %lf, test2: %lf\n", max_obj, affordance*radToDeg, o1*radToDeg, o2*radToDeg);

	if (o1 > pi/2) o1 -= pi;
	else
	if (o1 < -pi/2) o1 += pi;
	
	if (o2 > pi/2) o2 -= pi;
	else
	if (o2 < -pi/2) o2 += pi;

	double score1 = 0;
	double score2 = 0;
	int bestaction1 = m_canonical_stats->GetBestActionGeneric (o1, score1);
	int bestaction2 = m_canonical_stats->GetBestActionGeneric (o2, score2);

	printf ("---- score(s) %lf %lf\n", score1, score2);

	if (score1 < score2)
		bestaction = bestaction1;
	else
		bestaction = bestaction2;

	printf ("the best action for this object in this position is %d\n", bestaction);

	return 0;
}


//
// only find object and orientation.
//
int YARPObjectContainer::FindSimple (YARPImageOf<YarpPixelBGR>& scan, YARPImageOf<YarpPixelBGR>& out)
{
	if (!m_active)
	{
		printf ("YARPObjectContainer: need to update stats first\n");
		out.PeerCopy(scan);
		m_last_known_object = -1;
		m_orientation = 0;
		m_displacement = 0;
		return -1;
	}

	YarpPixelBGR red;
	red.r = 255;
	red.g = red.b = 0;

	double x, y, quality;
	double max_quality = 0; 
	int max_obj = -1;
	double max_x = 0, max_y = 0;

	const int NOBJ = m_canonical_stats->m_goodneurons;

	for (int obj = 0; obj < NOBJ; obj++)
	{
		m_locator[obj].BackProject (scan, m_backp[obj]);

		double ex, ey;
		m_locator[obj].GetExtent (ex, ey);

		ex *= SCALE;
		ey *= SCALE;

		if (m_locator[obj].Find (ex, ey, x, y, quality) >= 0)
		{
			double mean = 0, stddev = 0;
			const double THR = 2.0;
			m_locator[obj].GetExpectancy (mean, stddev);

			bool thr_cond = (fabs(quality - mean) < stddev * THR) ? true : false;

#ifdef _DEBUG
			printf ("object: %d location: %lf %lf q: %lf\n", obj, x, y, quality);
#endif
			if (quality > max_quality && thr_cond)
			{
				max_quality = quality;
				max_obj = obj;
				max_x = x;
				max_y = y;
			}
		}
	}

	m_last_known_object = max_obj;

	if (max_obj == -1)
	{
		printf ("I (COG) don't know about this object, if there's one at all!\n");
		out.PeerCopy(scan);
		m_orientation = 0;
		m_displacement = 0;
		return -1;
	}

	// if max_quality is not good enough then skip the following search.
	// if pointiness is not good enough then skip the following search.
#ifdef _DEBUG
	printf ("object is %d, improving position\n", max_obj);
#endif

	double ex, ey;
	m_locator[max_obj].GetExtent (ex, ey);
	ex *= SCALE;
	ey *= SCALE;

	// try to improve the position estimation.
	double betterx = 0, bettery = 0;
	m_locator[max_obj].ImprovedSearch (scan, ex, ey, max_x, max_y, betterx, bettery);

#ifdef _DEBUG
	printf ("object is %d, looking for orientation\n", max_obj);
#endif

	double angle = -1;
	double angle2 = -1;
	double ave = 0;
	double std = 0;
	m_locator[max_obj].GetNumberOfPoints (ave, std);

	// need to add a threshold on pointiness!
	YARPSearchRotation sr (50, 0.0);
	sr.Search (int(m_canonical_stats->m_neuron_numbers(max_obj+1)), *m_canonical, scan, betterx, bettery, ave, std, angle, angle2);
	
	// store for future use.
	m_orientation = angle;
	m_displacement = 0;

#ifdef _DEBUG
	printf ("orientation: %lf\n", angle * radToDeg);
#endif
	int x1 = betterx + cos(angle) * 40;
	int y1 = bettery + sin(angle) * 40;
	int x2 = betterx - cos(angle) * 40;
	int y2 = bettery - sin(angle) * 40;
	AddSegment (m_backp[max_obj], red, x1, y1, x2, y2);
	AddRectangle (m_backp[max_obj], red, int(betterx+.5), int(bettery+.5), int (ex/2+.5), int (ey/2+.5));

	AddCircleOutline (m_backp[max_obj], red, int(max_x+.5), int(max_y+.5), 10);
	AddCircle (m_backp[max_obj], red, int(betterx+.5), int(bettery+.5), 5);

	// return processed image.
	out.PeerCopy (m_backp[max_obj]);

	return 0;
}

//
// only find whether there's an object.
//
int YARPObjectContainer::FindObject (YARPImageOf<YarpPixelBGR>& scan)
{
	if (!m_active)
	{
		printf ("YARPObjectContainer: need to update stats first\n");
		m_last_known_object = -1;
		m_orientation = 0;
		m_displacement = 0;
		return -1;
	}

	double x, y, quality;
	double max_quality = 0; 
	int max_obj = -1;
	double max_x = 0, max_y = 0;

	const int NOBJ = m_canonical_stats->m_goodneurons;

	for (int obj = 0; obj < NOBJ; obj++)
	{
		m_locator[obj].BackProject (scan, m_backp[obj]);

		double ex, ey;
		m_locator[obj].GetExtent (ex, ey);

		ex *= SCALE;
		ey *= SCALE;

		if (m_locator[obj].Find (ex, ey, x, y, quality) >= 0)
		{
			double mean = 0, stddev = 0;
			const double THR = 2.0;
			m_locator[obj].GetExpectancy (mean, stddev);

			bool thr_cond = (fabs(quality - mean) < stddev * THR) ? true : false;

#ifdef _DEBUG
			printf ("object: %d location: %lf %lf q: %lf\n", obj, x, y, quality);
#endif
			if (quality > max_quality && thr_cond)
			{
				max_quality = quality;
				max_obj = obj;
				max_x = x;
				max_y = y;
			}
		}
	}

	m_last_known_object = max_obj;

	if (max_obj == -1)
	{
		printf ("I (COG) don't know about this object, if there's one at all!\n");
		m_orientation = 0;
		m_displacement = 0;
		return -1;
	}

#ifdef _DEBUG
	printf ("object is %d\n", max_obj);
#endif

	// non valid orientation and displacement!
	m_orientation = 0;
	m_displacement = 0;
	return max_obj;
}


int YARPObjectContainer::GetLastKnownObjectPose (double& orientation, double& displacement) const 
{ 
	orientation = m_orientation; 
	displacement = m_displacement;
	return 0;
}

int YARPObjectContainer::ResetObject (void)
{
	m_orientation = m_displacement = 0;
	m_last_known_object = -1;
	return 0;
}

int YARPObjectContainer::SetLastKnownObjectPose (double orientation, double displacement)
{ 
	m_orientation = orientation; 
	m_displacement = displacement;
	return 0;
}


//
// only find object_no.
//
int YARPObjectContainer::Segment (int object_no, YARPImageOf<YarpPixelBGR>& scan, YARPImageOf<YarpPixelBGR>& out, int& xx, int& yy)
{
	if (!m_active)
	{
		printf ("YARPObjectContainer: need to update stats first\n");
		out.PeerCopy(scan);
		xx = yy = 0;
		return -1;
	}

	double x, y, quality;
	m_locator[object_no].BackProject (scan, m_backp[object_no]);

	double ex, ey;
	m_locator[object_no].GetExtent (ex, ey);

	ex *= SCALE;
	ey *= SCALE;

	bool valid = false;

	if (m_locator[object_no].Find (ex, ey, x, y, quality) >= 0)
	{
		double mean = 0, stddev = 0;
		const double THR = 4.0;		// it was 2.0
		m_locator[object_no].GetExpectancy (mean, stddev);

		valid = (fabs(quality - mean) < stddev * THR) ? true : false;

#ifdef _DEBUG
		printf ("object: %d location: %lf %lf q: %lf\n", object_no, x, y, quality);
#endif
	}

	if (valid)
	{
		YarpPixelBGR red;
		red.r = 255;
		red.g = red.b = 0;

		double betterx = x;
		double bettery = y;
		AddRectangle (m_backp[object_no], red, int(betterx+.5), int(bettery+.5), int (ex/2+.5), int (ey/2+.5));

		//AddCircleOutline (m_backp[object_no], red, int(max_x+.5), int(max_y+.5), 10);
		AddCircle (m_backp[object_no], red, int(betterx+.5), int(bettery+.5), 5);

		// return processed image.
		out.PeerCopy (m_backp[object_no]);

		xx = int (betterx + .5);
		yy = int (bettery + .5);
	}
	else
	{
		xx = yy = 0;
	}

	return (valid) ? 0 : -1;
}

#undef SCALE


//
//
// trash.
//
//


#if 0
	int index = int (fmod (angle+pi, pi) / (pi / STATNBINS)) + 1;

	int index_p1 = index + 1;
	if (index_p1 > STATNBINS)	
		index_p1 = 1;
	int index_m1 = index - 1;
	if (index_m1 < 1)
		index_m1 = STATNBINS;

	// get the best action...
	double amax = 0;
	
	// return bestaction.
	bestaction = -1;

	for (int i = 0; i < MAXNUMBEROFACTIONS; i++)
	{
		double score = 
			m_canonical_stats->m_actions[max_obj](i+1, index) +
			m_canonical_stats->m_actions[max_obj](i+1, index_p1) +
			m_canonical_stats->m_actions[max_obj](i+1, index_m1);
		score /= 3; // just to make sense of the values, doesn't need to /3.

		if (score > amax)
		{
			bestaction = i;
			amax = score;
		}
	}

#endif

#if 0
	double max_x = 0, max_y = 0;

	// if max_quality is not good enough then skip the following search.
	// if pointiness is not good enough then skip the following search.
	printf ("object is %d, improving position\n", object_no);

	double ex, ey;
	m_locator[object_no].GetExtent (ex, ey);
	ex *= SCALE;
	ey *= SCALE;

	// try to improve the position estimation.
	double betterx = 0, bettery = 0;
	m_locator[object_no].ImprovedSearch (scan, ex, ey, max_x, max_y, betterx, bettery);

	printf ("object is %d, looking for orientation\n", object_no);

	double angle = -1;
	double angle2 = -1;
	double ave = 0;
	double std = 0;
	m_locator[object_no].GetNumberOfPoints (ave, std);

	// need to add a threshold on pointiness!
	YARPSearchRotation sr (50, 0.0);
	sr.Search (int(m_canonical_stats->m_neuron_numbers(object_no+1)), *m_canonical, scan, betterx, bettery, ave, std, angle, angle2);
	
	// store for future use.
	m_orientation = angle;
	m_displacement = 0;

	printf ("orientation: %lf\n", angle * radToDeg);
	int x1 = betterx + cos(angle) * 40;
	int y1 = bettery + sin(angle) * 40;
	int x2 = betterx - cos(angle) * 40;
	int y2 = bettery - sin(angle) * 40;
	AddSegment (m_backp[max_obj], red, x1, y1, x2, y2);

#endif

#endif
