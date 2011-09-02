//
// YARPSearchRotation.cpp
//

#include "YARPSearchRotation.h"

YARPSearchRotation::YARPSearchRotation (int thr, double quality_thr)
{
	m_threshold = thr;
	m_quality_thr = quality_thr;
}

YARPSearchRotation::~YARPSearchRotation ()
{
}


// should I compare normalized cross correlation?

double YARPSearchRotation::Compare 
	(YARPImageOf<YarpPixelMono>& mask, 
	 YARPImageOf<YarpPixelBGR>& frame, 
	 int cx,
	 int cy,
	 int x, 
	 int y, 
	 YARPImageOf<YarpPixelBGR>& img)
{
	double ssd = 0;
	int npoints = 0;

	//
	// each point of the mask must be translated of x - cx, y - cy
	const int w = mask.GetWidth();
	const int h = mask.GetHeight();

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			if (mask(j,i) != 0)
			{
				int tx = j + x - cx;
				int ty = i + y - cy;

				if (tx >= 0 && tx < w && ty >= 0 && ty < h)
				{
					YarpPixelBGR& s = frame(j,i);
					YarpPixelBGR& d = img (tx,ty);
	
					ssd += ((s.r - d.r) * (s.r - d.r));
					ssd += ((s.g - d.g) * (s.g - d.g));
					ssd += ((s.b - d.b) * (s.b - d.b));
					npoints ++;
				}
			}
		}
	}


	return ssd / (npoints * 3);
}

int YARPSearchRotation::EstimateGoodness (YARPImageOf<YarpPixelMono>& mask, 
					  double aver, double stdr)
{
	unsigned char *m = (unsigned char *)mask.GetRawBuffer();
	const int size = mask.GetWidth()*mask.GetHeight();
	int counter = 0;
	for (int i = 0; i < size; i++)
	{
	  if (*m != 0)
	    {
	      counter++;
	    }
	  m++;
	}

	if (fabs(counter-aver) > stdr*(GOODNESS))
		return -1;
	else
		return 0;
}


// given that we know the object.

int YARPSearchRotation::Search 
	(int neuron, 
	 YARPCanonicalNeurons& canonical, 
	 YARPImageOf<YarpPixelBGR>& img,
	 int x, int y,
	 double aver, double stdr,
	 double& angle,
	 double& angle2
	)
{
	if (canonical.Empty())
	{
		printf ("YARPSearchRotation: nothing to search into\n");
		angle = 0;
		angle2 = 0;
		return -1;
	}

	YARPCanonicalNeuron& n = canonical[neuron];

	double best_score = 256 * 256;
	double best_angle = 0;
	double best_angle_quality = 0;

	int best_cnt = 0;

	vector<YARPCanonicalData>::iterator store = n.GetCursor();

	//
	//
	//
	if (n.size() > m_threshold)
	{
		int cnt = 0;

		n.InitCursor();
		while (!n.EndCursor ())
		{
			YARPCanonicalData& data = n.Get();

			if (EstimateGoodness (data.m_mask, aver, stdr) < 0)
			{
#ifdef _DEBUG
				printf ("skipping pattern %d\n", cnt);
#endif
			}
			else
			{
				// for each mask + image, check correlation w/ current image.
				double score = Compare (data.m_mask, data.m_frame, int(data.m_starting_position_image(1)+.5), int(data.m_starting_position_image(2)+.5), x, y, img);
				if (score < best_score)
				{
					best_score = score;
					best_angle = data.m_object_orientation;
					best_angle_quality = data.m_object_orientation_quality;
					best_cnt = cnt;
				}
			}

			n.IncrementCursor ();
			cnt ++;
		}
	}
	else
	{
		printf ("YARPSearchRotation: not enough samples\n");
		angle = 0;
		angle2 = 0;

		n.SetCursor(store);
		return -1;
	}

	angle = best_angle;
#ifdef _DEBUG
	printf ("Search rotation: best count %d\n", best_cnt);
#endif

	n.SetCursor(store);

	// check quality also.
	// the threshold should be computed automatically by the statistics on the data.
	if (best_angle_quality < m_quality_thr)
	{
		printf ("Pointiness is poor\n");
		return -1;
	}

	return 0;
}

