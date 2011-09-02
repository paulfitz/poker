//
// YARPFlowTracker.cpp
//
//

#ifndef __LINUX__

#include "YARPFlowTracker.h"
#include "YARPTime.h"
#include "SegPort.h"

#define LOGGING_SEGMENTATIONS
#define SENDING_SEGMENTATIONS
#define LOGGING_DIR "/mnt/state/DayOne/pokes"

//
// Class YARPImageSequence.
//

#ifdef SENDING_SEGMENTATIONS

OutputSegPort out_seg;

class AutoOutSeg
{
public:
  AutoOutSeg()
    {
      out_seg.Register("/shlc/o:seg");
    }
};
static AutoOutSeg auto_out_seg;

#endif

YARPImageSequence::YARPImageSequence (int size, int w, int h) 
{
	m_size = size;
	m_array = new YARPImageOf<YarpPixelBGR>[m_size];
	assert (m_array != NULL);
	
	m_head_pos = new CVisDVector[m_size];
	assert (m_head_pos != NULL);
	m_arm_pos = new CVisDVector[m_size];
	assert (m_arm_pos != NULL);
	m_arm_error = new CVisDVector[m_size];
	assert (m_arm_error != NULL);

	for (int i = 0; i < m_size; i++)
	{
		m_array[i].Resize (w, h);
		m_array[i].Zero();

		m_head_pos[i].Resize (NumHeadJoints);
		m_arm_pos[i].Resize (NumArmJoints);
		m_arm_error[i].Resize (NumArmJoints);
	}

	m_counter = 0;
	m_start_pushing_frame = 0;
	m_stop_pushing_frame = 0;

	m_action_number = -1;
	m_reinforcer = 0;
}

YARPImageSequence::~YARPImageSequence () 
{
	if (m_head_pos != NULL) delete[] m_head_pos;
	if (m_arm_pos != NULL) delete[] m_arm_pos;
	if (m_arm_error != NULL) delete[] m_arm_error;

	if (m_array != NULL) delete[] m_array;
}

void YARPImageSequence::AddFrame (YARPImageOf<YarpPixelBGR>& i, CVisDVector& hj, CVisDVector& aj, CVisDVector& ae)
{
	if (m_counter < m_size)
	{
		m_array[m_counter] = i;
		m_head_pos[m_counter] = hj;
		m_arm_pos[m_counter] = aj;
		m_arm_error[m_counter] = ae;

		m_counter ++;
	}
	else
		printf ("YARPImageSequence: the buffer is full\n");
}

int YARPImageSequence::Save(char *dname, char *base) 
{
	FILE *fp;
	char buf[512];

	// save images.
	if (m_counter <= 0)
	{
		printf ("YARPImageSequence: data is empty, can't save\n");
		return -1;
	}

	sprintf (buf, "%s/description.txt\0", dname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		printf ("YARPImageSequence: can't save file description.txt\n");
		return -1;
	}

	fprintf (fp, "directory : %s\n", dname);
	fprintf (fp, "image filename : %s\n", base);
	fprintf (fp, "number of frames : %d\n", m_counter);

	fprintf (fp, "start pushing frame : %d\n", m_start_pushing_frame);
	fprintf (fp, "stop pushing frame : %d\n", m_stop_pushing_frame);
	fprintf (fp, "action id : %d\n", m_action_number);
	fprintf (fp, "reinforcer : %lf\n", m_reinforcer);

	fclose (fp);
	
	for (int i = 0; i < m_counter; i++)
	{
		sprintf (buf, "%s/%s%04d.ppm\0", dname, base, i);
		int ret = YARPImageFile::Write (buf, m_array[i]);

		if (ret < 0)
		{
			printf ("YARPImageSequence: can't save frame %s\n", buf);
			return -1;
		}
	}

	// save other data.
	sprintf (buf, "%s/arm_position.txt\0", dname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		printf ("YARPImageSequence: can't save file arm_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumArmJoints; j++)
		{
			fprintf (fp, "%lf ", m_arm_pos[i](j));
		}
		fprintf (fp, "\n");
	}
	fclose (fp);

	sprintf (buf, "%s/head_position.txt\0", dname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		printf ("YARPImageSequence: can't save file head_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumHeadJoints; j++)
		{
			fprintf (fp, "%lf ", m_head_pos[i](j));
		}
		fprintf (fp, "\n");
	}
	fclose (fp);

	return 0;
}

int YARPImageSequence::Load(char *dname, char *base)
{
	FILE *fp;
	char buf[512];
	
	sprintf (buf, "%s/description.txt\0", dname);
	fp = fopen (buf, "r");
	if (fp == NULL)
	{
		printf ("YARPImageSequence: can't load file description.txt\n");
		return -1;
	}

	fscanf (fp, "directory : %s\n", buf);
	fscanf (fp, "image filename : %s\n", buf);
	fscanf (fp, "number of frames : %d\n", &m_counter);
	printf ("number of frames to load %d\n", m_counter);

	fscanf (fp, "start pushing frame : %d\n", &m_start_pushing_frame);
	fscanf (fp, "stop pushing frame : %d\n", &m_stop_pushing_frame);
	fscanf (fp, "action id : %d\n", &m_action_number);
	fscanf (fp, "reinforcer : %lf\n", &m_reinforcer);

	fclose (fp);
	
	for (int i = 0; i < m_counter; i++)
	{
		sprintf (buf, "%s/%s%04d.ppm\0", dname, base, i);
		int ret = YARPImageFile::Read (buf, m_array[i]);

		if (ret < 0)
		{
			printf ("YARPImageSequence: can't load frame %s\n", buf);
			return -1;
		}
	}

	// load other data.
	sprintf (buf, "%s/arm_position.txt\0", dname);
	fp = fopen (buf, "r");
	if (fp == NULL)
	{
		printf ("YARPImageSequence: can't load file arm_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumArmJoints; j++)
		{
			double tmp;
			fscanf (fp, "%lf ", &tmp);
			m_arm_pos[i](j) = tmp;
		}
		fscanf (fp, "\n");
	}
	fclose (fp);

	sprintf (buf, "%s/head_position.txt\0", dname);
	fp = fopen (buf, "r");
	if (fp == NULL)
	{
		printf ("YARPImageSequence: can't read file head_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumHeadJoints; j++)
		{
			double tmp;
			fscanf (fp, "%lf ", &tmp); 
			m_head_pos[i](j) = tmp;
		}
		fscanf (fp, "\n");
	}
	fclose (fp);

	return 0;
}

YARPImageOf<YarpPixelBGR>& YARPImageSequence::GetImageRef(int i) 
{ 
	if (i < 0 || i >= m_counter)
	{
		printf ("YARPImageSequence: %d is not in memory, returning frame 0\n", i);
		return m_array[0];
	}

	return m_array[i];
}

//
//
// class YARPOutputImageSequence
int YARPOutputImageSequence::Save(char *dname, char *base) 
{
	FILE *fp;
	char buf[512];

	// save images.
	if (m_counter <= 0)
	{
		printf ("Sequence: data is empty, can't save\n");
		return -1;
	}

	sprintf (buf, "%s/description_o.txt\0", dname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		printf ("YARPOutputImageSequence: can't save file description.txt\n");
		return -1;
	}

	fprintf (fp, "directory : %s\n", dname);
	fprintf (fp, "image filename : %s\n", base);
	fprintf (fp, "number of frames : %d\n", m_counter);

	fprintf (fp, "start pushing frame : %d\n", m_start_pushing_frame);
	fprintf (fp, "stop pushing frame : %d\n", m_stop_pushing_frame);
	fprintf (fp, "action id : %d\n", m_action_number);
	fprintf (fp, "reinforcer : %lf\n", m_reinforcer);

	fclose (fp);
	
	for (int i = 0; i < m_counter; i++)
	{
		sprintf (buf, "%s/%s_o%04d.ppm\0", dname, base, i);
		int ret = YARPImageFile::Write (buf, m_array[i]);

		if (ret < 0)
		{
			printf ("YARPOutputImageSequence: can't save frame %s\n", buf);
			return -1;
		}
	}

	// save other data.
	sprintf (buf, "%s/arm_position_o.txt\0", dname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		printf ("YARPOutputImageSequence: can't save file arm_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumArmJoints; j++)
		{
			fprintf (fp, "%lf ", m_arm_pos[i](j));
		}
		fprintf (fp, "\n");
	}
	fclose (fp);

	sprintf (buf, "%s/head_position_o.txt\0", dname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		printf ("YARPOutputImageSequence: can't save file head_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumHeadJoints; j++)
		{
			fprintf (fp, "%lf ", m_head_pos[i](j));
		}
		fprintf (fp, "\n");
	}
	fclose (fp);

	return 0;
}

int YARPOutputImageSequence::Load(char *dname, char *base)
{
	FILE *fp;
	char buf[512];
	
	sprintf (buf, "%s/description_o.txt\0", dname);
	fp = fopen (buf, "r");
	if (fp == NULL)
	{
		printf ("YARPOutputImageSequence: can't load file description.txt\n");
		return -1;
	}

	fscanf (fp, "directory : %s\n", buf);
	fscanf (fp, "image filename : %s\n", buf);
	fscanf (fp, "number of frames : %d\n", &m_counter);
	printf ("number of frames to load %d\n", m_counter);

	fscanf (fp, "start pushing frame : %d\n", &m_start_pushing_frame);
	fscanf (fp, "stop pushing frame : %d\n", &m_stop_pushing_frame);
	fscanf (fp, "action id : %d\n", &m_action_number);
	fscanf (fp, "reinforcer : %lf\n", &m_reinforcer);

	fclose (fp);
	
	for (int i = 0; i < m_counter; i++)
	{
		sprintf (buf, "%s/%s_o%04d.ppm\0", dname, base, i);
		int ret = YARPImageFile::Read (buf, m_array[i]);

		if (ret < 0)
		{
			printf ("YARPOutputImageSequence: can't load frame %s\n", buf);
			return -1;
		}
	}

	// load other data.
	sprintf (buf, "%s/arm_position_o.txt\0", dname);
	fp = fopen (buf, "r");
	if (fp == NULL)
	{
		printf ("YARPOutputImageSequence: can't load file arm_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumArmJoints; j++)
		{
			double tmp;
			fscanf (fp, "%lf ", &tmp);
			m_arm_pos[i](j) = tmp;
		}
		fscanf (fp, "\n");
	}
	fclose (fp);

	sprintf (buf, "%s/head_position_o.txt\0", dname);
	fp = fopen (buf, "r");
	if (fp == NULL)
	{
		printf ("YARPOutputImageSequence: can't read file head_position.txt\n");
		return -1;
	} 
	for (int i = 0; i < m_counter; i++)
	{
		for (int j = 1; j <= NumHeadJoints; j++)
		{
			double tmp;
			fscanf (fp, "%lf ", &tmp); 
			m_head_pos[i](j) = tmp;
		}
		fscanf (fp, "\n");
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
//
//
//
//
YARPFlowTracker::YARPFlowTracker (int size, int width, int height) :
		of (width, height, BLOCKSIZE, MAXFLOW, 1, BLOCKINC),
		tracker (width, height, TEMPSIZE, TEMPSIZE, RANGE, RANGE, 1),
		oseq (size, width, height),
		cumulative_trsf(4),
		processor (),
		OOVERFLOW (of.L())
{
	m_maxsize = size;	
	m_width = width;
	m_height = height;

	of.GetNBlocks (ox, oy);
	of.SetSpatialSmooth();
	of.SetVarianceThr (20.0);

	tracker.SetVarianceThr (20.0);

	// ALLOC temporary storage.

	vx = new int*[m_maxsize];
	vy = new int*[m_maxsize];
	assert (vx != NULL && vy != NULL);

	for (int i = 0; i < m_maxsize; i++)
	{
		vx[i] = new int[ox*oy];
		vy[i] = new int[ox*oy];
		assert (vx[i] != NULL && vy[i] != NULL);
	}

	cumulative_trsf = 0;

	trsf = new CVisDVector[FRAMES_TRACKED+1];
	assert (trsf != NULL);
	for (int i = 0; i < FRAMES_TRACKED+1; i++)
	{
		trsf[i].Resize(4);
		trsf[i] = 0;
	}

	mono.Resize (m_width, m_height);
	outimage.Resize (m_width, m_height);
	extended_segmentation.Resize (m_width, m_height);
	segmentation_mask.Resize (m_width, m_height);
	segmentation_mask_copy.Resize (m_width, m_height);
	output_image.Resize (m_width, m_height);
	mask.Resize (m_width, m_height);

	contact = 0;
	contact_frame = 0;
	howmanycontacts = 0;
	lack_of_vectors = 0;

	dirx = diry = 0;

	m_stepping = 0;

	seq = NULL;
}

YARPFlowTracker::~YARPFlowTracker () 
{
	if (vx != NULL)
	{
		for (int i = 0; i < m_maxsize; i++)
			if (vx[i] != NULL)
				delete[] vx[i];
		delete[] vx;
	}

	if (vy != NULL)
	{
		for (int i = 0; i < m_maxsize; i++)
			if (vy[i] != NULL)
				delete[] vy[i];
		delete[] vy;
	}

	if (trsf != NULL)
		delete[] trsf;
}


//
// helper.
int 
YARPFlowTracker::GrowMask (const YARPImageOf<YarpPixelMono>& src, YARPImageOf<YarpPixelMono>& dest)
{
	// block filter 7x7
	const int w = segmentation_mask.GetWidth();
	const int h = segmentation_mask.GetHeight();
	unsigned char *ss = (unsigned char *)src.GetRawBuffer();

	dest.Zero();

	// save in +2,+2...
	unsigned char *s[7];
	for (int ll = 0; ll < 7; ll++)
		s[ll] = ss+ll*w;

	int accum = 0;

	for (int i = 0; i < h-7; i++)
	{
		for (int j = 0; j < w-7; j++)
		{
			accum = 0;
			for (int k = 0; k < 7; k++)
			{
				accum += s[0][k];
				accum += s[1][k];
				accum += s[2][k];
				accum += s[3][k];
				accum += s[4][k];
				accum += s[5][k];
				accum += s[6][k];
			}

			accum /= 49;
			accum = (accum >= 64) ? 255 : 0;
		
			dest (j+3, i+3) = accum;

			s[0] ++;
			s[1] ++;
			s[2] ++;
			s[3] ++;
			s[4] ++;
			s[5] ++;
			s[6] ++;
		}

		ss += w;
		s[0] = ss;
		s[1] = s[0]+w;
		s[2] = s[1]+w;
		s[3] = s[2]+w;
		s[4] = s[3]+w;
		s[5] = s[4]+w;
		s[6] = s[5]+w;
	}

	return 0;
}


//
// Generate the segmented image and send to a given port.
//	- it gets image from processed sequence.
int 
YARPFlowTracker::GenerateAndSend(YARPOutputPortOf<YARPGenericImage>& port)
{
	// segmentation_mask_copy
	// frame from seq
	// port from reader

	// YARPImageSequence& seq = reader->GetSequenceRef();
	// YARPOutputPortOf<YARPGenericImage>& port = reader->GetSegmentedImagePort();

	YARPImageOf<YarpPixelBGR>& frame = seq->GetImageRef (contact_frame);
	YARPImageOf<YarpPixelMono>& mask = segmentation_mask_copy;

#ifdef LOGGING_SEGMENTATIONS
	// awful to just hack this in here... bound to cause trouble...
	// but I'm going to do it anyway.
#ifdef SENDING_SEGMENTATIONS
	{
	  // send frame and mask
	  out_seg.Content().c1.PeerCopy(frame);
	  out_seg.Content().c2.PeerCopy(frame);
	  out_seg.Content().c2.CastCopy(mask);
	  out_seg.Write();
	}
#endif	  
	{
	  char buf[256];
	  long int tick = (long int) YARPTime::GetTimeAsSeconds();
	  sprintf(buf,"%s/%ld.ppm", LOGGING_DIR, tick);
	  printf("Saving frame %s\n", buf);
	  YARPImageFile::Write(buf,frame);
	  sprintf(buf,"%s/%ld.pgm", LOGGING_DIR, tick);
	  printf("Saving mask %s\n", buf);
	  YARPImageFile::Write(buf,mask);
	}
#endif

	YarpPixelBGR black;
	black.r = black.g = black.b = 0;
	YarpPixelBGR black1;
	black1.r = black1.g = black1.b = 1;

	for (int i = 0; i < mask.GetHeight(); i++)
		for (int j = 0; j < mask.GetWidth(); j++)
		{
			if (frame(j,i).r != 0 ||
				frame(j,i).g != 0 ||
				frame(j,i).b != 0)
				output_image(j,i) = (mask(j,i) != 0) ? frame(j,i) : black;
			else
				output_image(j,i) = (mask(j,i) != 0) ? frame(j,i) : black1;
		}

	// send.
	port.Content().Refer (output_image); 
	port.Write ();

	return 0;
}


//
// LATER: remove printfs...
//
// main tracking function. Processes a given sequence of type YARPImageSequence.
int 
YARPFlowTracker::Apply (YARPOutputPortOf<YARPGenericImage>& port)
{
	printf ("ox %d oy %d\n", ox, oy);

	// PROCESSING SEQUENCE.
	int start = seq->GetStartPushing();
	int stop = seq->GetStopPushing();
	printf ("Processing from frame %d to %d\n", start, stop); 

	printf ("Sequence has %d frames\n", seq->GetNumElements());
	assert (stop-start+1 <= m_maxsize);	

	// PREPARE SEQUENCE FOR PROCESSING.
	processor.Reset ();

	oseq.Reset ();

	contact = 0;
	contact_frame = 0;
	howmanycontacts = 0;

	lack_of_vectors = 0;

	int i, frame;
	for (frame = start; frame <= stop; frame++)
	{
		oseq.IncrementCounter ();
		contact = processor.Apply (seq->GetImageRef (frame), 
								   oseq.GetImageRef (frame-start));

		oseq.GetImageRef (frame-start) = seq->GetImageRef (frame);

		if (contact)
		{
			processor.GetPokeDirection (dirx, diry);
			processor.GetSegmentedImage (segmentation_mask);
			segmentation_mask_copy = segmentation_mask;
		
			processor.GetFlipper(flipper_segmentation_mask);
			flipper_segmentation_mask_copy = flipper_segmentation_mask;
						
			ba.Apply(segmentation_mask_copy);
			orientation = ba.GetAngle();
			orientation_quality = ba.GetPointiness();

			// enlarge it twice.
			GrowMask (segmentation_mask, extended_segmentation);
			segmentation_mask = extended_segmentation;
			GrowMask (segmentation_mask, extended_segmentation);

			CenterOfMass (extended_segmentation, com_x, com_y);

			contact_frame = frame;
			howmanycontacts++;

			GenerateAndSend (port);
		}
	}

	if (howmanycontacts == 0)
	{
		printf ("No poking detected... \n");

		// nothing much to do.
		return -1;
	}

	// OPTICFLOW.	
	int startopticflow = contact_frame - 5; //10;
	if (startopticflow < start) startopticflow = start;

	int endopticflow = contact_frame + FRAMES_TRACKED;
	if (endopticflow > stop) endopticflow = stop;

	// TRACK.
	int endtracking = contact_frame+FRAMES_TRACKED;
	if (endtracking > stop) endtracking = stop;

	// optic flow mask, initialize optic flow and additional tracker.
	mask.Zero();

	YARPColorConverter::RGB2Grayscale (seq->GetImageRef(startopticflow), mono);
	of.Initialize (mono);

	bool trackvalid = false;
	int sx = com_x, sy = com_y;
	int sx2 = com_x, sy2 = com_y;
	tracker.Initialize (seq->GetImageRef(startopticflow), com_x, com_y);
	
	YarpPixelBGR green;
	green.r = green.b = 0;
	green.g = 0;

	i = 0;
	for (frame = startopticflow+1; frame <= endopticflow; frame++)
	{
		AddCircleOutline (oseq.GetImageRef(frame-start+1), green, sx, sy, 10);

		if (tracker.IsTracking())
		{
			tracker.Apply (seq->GetImageRef(frame), true, sx2, sy2, trackvalid);
			printf ("frame: %d, valid: %d, sx, sy: %d %d\n", frame, trackvalid, sx2, sy2);
		}
 
		YARPColorConverter::RGB2Grayscale (seq->GetImageRef(frame), mono);

		if (frame < contact_frame)
			of.Apply (mono, mask, sx2-sx, sy2-sy, outimage, vx[frame-start], vy[frame-start]);
		else
			of.Apply (mono, extended_segmentation, sx2-sx, sy2-sy, outimage, vx[frame-start], vy[frame-start]);

		sx = sx2;
		sy = sy2;
 
		of.DrawFlow (oseq.GetImageRef (frame-start));

		if (frame == contact_frame)
			WriteMask (extended_segmentation, oseq.GetImageRef (frame-start));

		if (frame >= contact_frame+1 && frame <= endtracking)
		{
			if (ComputeRotation (extended_segmentation, 
								 vx[frame-start], 
								 vy[frame-start], 
								 ox, 
								 oy, 
								 trsf[i], 
								 10) == -2)
				lack_of_vectors++;
			i++;

			WriteMask (extended_segmentation, oseq.GetImageRef (frame-start));
		}
	}

	CenterOfMass (extended_segmentation, final_x, final_y);
	printf ("starting point: %d %d\n", com_x, com_y);
	printf ("center of mass: %d %d\n", final_x, final_y);

	dispframe = contact_frame - start - 10;	// it was -5.
	m_stepping = 2;

	if (lack_of_vectors > 6)
	{
		printf ("optic flow is poor, skipping frames\n");

		oseq.Reset ();

		// adjust start frame parity.
		int newstart = start;
		if (((contact_frame % 2) == 0 && (newstart % 2) == 1) ||
			((contact_frame % 2) == 1 && (newstart % 2) == 0)
			)
			newstart ++;

		lack_of_vectors = 0;

		printf ("re-processing from frame %d to %d\n", start, stop); 
		
		// RECOMPUTING INDEX ETC.
		// 
		segmentation_mask = segmentation_mask_copy; 
		flipper_segmentation_mask = flipper_segmentation_mask_copy; 

		// enlarge it twice.
		GrowMask (segmentation_mask, extended_segmentation);
		segmentation_mask = extended_segmentation;
		GrowMask (segmentation_mask, extended_segmentation);

		// contact frame is ok.
		// poke dir is ok.
		// center of mass is ok.
				
		// RECOMPUTE OPTIC FLOW.
		int startopticflow = contact_frame - 10; //20;
		if (startopticflow < newstart) startopticflow = newstart;
		if (((contact_frame % 2) == 0 && (startopticflow % 2) == 1) ||
			((contact_frame % 2) == 1 && (startopticflow % 2) == 0)
			)
			startopticflow ++;

		int endopticflow = contact_frame + FRAMES_TRACKED*2;
		if (endopticflow > stop) endopticflow = stop;


		// TRACK.
		int endtracking = contact_frame+FRAMES_TRACKED*2;
		if (endtracking > stop) endtracking = stop;


		YARPColorConverter::RGB2Grayscale (seq->GetImageRef(startopticflow), mono);
		of.Initialize (mono);

		bool trackvalid = false;
		int sx = com_x, sy = com_y;
		int sx2 = com_x, sy2 = com_y;
		tracker.Initialize (seq->GetImageRef(startopticflow), com_x, com_y);
		
		YarpPixelBGR green;
		green.r = green.b = 0;
		green.g = 0;

		for (frame = start; frame <= stop; frame++)
		{
			oseq.IncrementCounter ();
			oseq.GetImageRef (frame-start) = seq->GetImageRef (frame);		
		}

		i = 0;
		for (frame = startopticflow+2; frame <= endopticflow; frame+=2)
		{
			AddCircleOutline (oseq.GetImageRef(frame-start+2), green, sx, sy, 10);

			if (tracker.IsTracking())
			{
				tracker.Apply (seq->GetImageRef(frame), true, sx2, sy2, trackvalid);
				printf ("frame: %d, valid: %d, sx, sy: %d %d\n", frame, trackvalid, sx2, sy2);
			}

			YARPColorConverter::RGB2Grayscale (seq->GetImageRef(frame), mono);
			if (frame < contact_frame)
				of.Apply (mono, mask, sx2-sx, sy2-sy, outimage, vx[frame-start], vy[frame-start]);
			else
				of.Apply (mono, extended_segmentation, sx2-sx, sy2-sy, outimage, vx[frame-start], vy[frame-start]);

			sx = sx2;
			sy = sy2;

			of.DrawFlow (oseq.GetImageRef (frame-start));

			if (frame == contact_frame)
				WriteMask (extended_segmentation, oseq.GetImageRef (frame-start));

			if (frame >= contact_frame+2 && frame <= endtracking)
			{
				if (ComputeRotation (extended_segmentation, 
									 vx[frame-start], 
									 vy[frame-start], 
									 ox, 
									 oy, 
									 trsf[i], 
									 10) == -2)
					lack_of_vectors++;
				i++;

				WriteMask (extended_segmentation, oseq.GetImageRef (frame-start));
			}
		}

		CenterOfMass (extended_segmentation, final_x, final_y);
		printf ("starting point: %d %d\n", com_x, com_y);
		printf ("center of mass: %d %d\n", final_x, final_y);

		printf ("improved? %d\n", lack_of_vectors);

		//
		//
		//
		m_stepping = 4;
		dispframe = contact_frame - start - 10;
		if (dispframe < 0) dispframe = 0;

		if (lack_of_vectors > 6)
		{
			// bad sequence.
			printf ("Still bad flow after post-processing\n");
			return -2;
		}
	}

	return 0;
}


// returns the mask and frame of contact.
int
YARPFlowTracker::GetObjectProperties (YARPImageOf<YarpPixelMono>& mask, YARPImageOf<YarpPixelBGR>& frame)
{
	// contact_frame
	mask.CastCopy (segmentation_mask_copy);
	frame.CastCopy (seq->GetImageRef(contact_frame));

	return 0;
}

// returns the displacement vector of the object. 
int
YARPFlowTracker::GetDisplacement (CVisDVector& v1, CVisDVector& v2, CVisDVector& i1, CVisDVector& i2)
{
	CogGaze gaze;
	CVisDVector& head = seq->GetHeadPositionPtr()[contact_frame];
	double torso = seq->GetArmPositionPtr()[contact_frame](3);		// 3 is the torso YAW.
	
	JointPos hj;
	ArmJoints aj;

	for (int i = 1; i <= NumHeadJoints; i++) 
		hj(i) = head(i);

	memset (aj.j, 0, sizeof(double) * NumArmJoints);
	aj.j[2] = torso;

	gaze.Apply (hj, aj);

	// com_x, com_y, final_x, final_y need to be mapped to the wide camera.
	double cx = com_x, cy = com_y;
	i1(1) = cx;
	i1(2) = cy;
	YARPCogMapCameras::Foveal2WideangleSquashed (cx, cy, cx, cy);
	printf ("remapped starting point : %lf %lf\n", cx, cy);

	float t1, t2, t3;
	gaze.DeIntersect (float(cx), float(cy), t1, t2, t3);
	v1(1) = t1;
	v1(2) = t2;
	v1(3) = t3;

	cx = final_x;
	cy = final_y;
	i2(1) = cx;
	i2(2) = cy;
	YARPCogMapCameras::Foveal2WideangleSquashed (cx, cy, cx, cy);
	printf ("remapped ending point : %lf %lf\n", cx, cy);

	gaze.DeIntersect (float(cx), float(cy), t1, t2, t3);
	v2(1) = t1;
	v2(2) = t2;
	v2(3) = t3;

	return 0;	
}

int
YARPFlowTracker::GetPushingDirection (CVisDVector& p)
{
	p(1) = dirx;
	p(2) = diry;
	return 0;
}

int 
YARPFlowTracker::GetMotorData (int& start, int& stop, CVisDVector **head, CVisDVector **arm, int& contact)
{
	// store all movement.
	start = seq->GetStartPushing();
	stop = seq->GetStopPushing();

	*head = seq->GetHeadPositionPtr();
	*arm = seq->GetArmPositionPtr();

	contact = contact_frame;

	return 0;
}

int 
YARPFlowTracker::GetActionID (int& id)
{
	id = seq->GetActionID();
	return 0;
}

int 
YARPFlowTracker::GetReinforcer (double& r)
{
	r = 0.0;
	return 0;
}

int
YARPFlowTracker::GetOrientation (double& o, double& quality)
{
	o = orientation;
	quality = orientation_quality;
	return 0;
}

int 
YARPFlowTracker::CenterOfMass (YARPImageOf<YarpPixelMono>& in, int& x, int& y)
{
	double xx = 0, yy = 0;
	int count = 0;

	for (int i = 0; i < in.GetHeight(); i++)
		for (int j = 0; j < in.GetWidth(); j++)
		{
			if (in(j, i) != 0)
			{
				xx += j;
				yy += i;
				count ++;
			}
		}

	if (count != 0)
	{
		x = int(xx / count + .5);
		y = int(yy / count + .5);
	}
	else
	{
		x = in.GetWidth()/2;
		y = in.GetHeight()/2;
	}

	return 0;
}

int 
YARPFlowTracker::ComputeCumulativeTrsf (void)
{
	CVisDVector tmp(4);
	cumulative_trsf = trsf[0];

	for (int i = 1; i < FRAMES_TRACKED; i++)
	{
		tmp = cumulative_trsf;
		CVisDVector& t = trsf[i];

		cumulative_trsf(1) = t(1) * tmp(1) - t(2) * tmp(2);
		cumulative_trsf(2) = t(1) * tmp(2) + t(2) * tmp(1);
		cumulative_trsf(3) = t(1) * tmp(3) + t(2) * tmp(4) + t(3);
		cumulative_trsf(4) = -t(2) * tmp(3) + t(1) * tmp(4) + t(4);
	}

	return 0;
}

int
YARPFlowTracker::ComputeRotation (
	YARPImageOf<YarpPixelMono>& mask, 
	int *vx, 
	int *vy, 
	int ox, 
	int oy, 
	CVisDVector& trsf, 
	int thr)
{
	const int border = 1;

	trsf = 0;
	trsf(1) = 1;

	YARPImageOf<YarpPixelMono> tmp;
	tmp.Resize (mask.GetWidth(), mask.GetHeight());
	tmp.Zero();

	double avex = 0;
	double avey = 0;
	double average = 0;
	int count = 0;

	// compute average displacement.
	for (int i = border; i < oy-border; i++)
		for (int j = border; j < ox-border; j++)
		{
			if (vx[i*ox+j] <= OOVERFLOW &&
				vy[i*ox+j] <= OOVERFLOW &&
				mask (j*BLOCKINC+BLOCKSIZE/2, i*BLOCKINC+BLOCKSIZE/2) != 0 
				&& 
				(fabs(vx[i*ox+j]) > 0 || fabs(vy[i*ox+j]) > 0) 
				)
			{
				avex += vx[i*ox+j];
				avey += vy[i*ox+j];
				average += sqrt(vx[i*ox+j]*vx[i*ox+j]+vy[i*ox+j]*vy[i*ox+j]);
				count++;
			}
		}	

	if (count > 0)
	{
		avex /= count;
		avey /= count;
		average /= count;
	}

	//
	if (count >= thr)
	{
		CVisDMatrix A (count * 2, 4);
		CVisDMatrix At (4, count * 2);
		CVisDMatrix sqA (4, 4);
		CVisDVector sqB (4);

		CVisDVector b (count * 2);

		CVisDVector solution(4);

		count = 1;
		for (int i = border; i < oy-border; i++)
			for (int j = border; j < ox-border; j++)
			{
			if (vx[i*ox+j] <= OOVERFLOW &&
				vy[i*ox+j] <= OOVERFLOW &&
					mask (j*BLOCKINC+BLOCKSIZE/2, i*BLOCKINC+BLOCKSIZE/2) != 0 
					&& 
					(fabs(vx[i*ox+j]) > 0 || fabs(vy[i*ox+j]) > 0) 
				)
				{
					A(count,1) = j*BLOCKINC+BLOCKSIZE/2;
					A(count,2) = i*BLOCKINC+BLOCKSIZE/2;
					A(count,3) = 1;
					A(count,4) = 0;
					b(count) = j*BLOCKINC+BLOCKSIZE/2+vx[i*ox+j];
					count++;
					A(count,1) = i*BLOCKINC+BLOCKSIZE/2;
					A(count,2) = -(j*BLOCKINC+BLOCKSIZE/2);
					A(count,3) = 0;
					A(count,4) = 1;
					b(count) = i*BLOCKINC+BLOCKSIZE/2+vy[i*ox+j];
					count++;
				}
			}	
		
		// solve by LU.
		At = A.Transposed ();
		sqA = At * A;
		sqB = At * b;
		VisDMatrixLU (sqA, sqB, solution);

		trsf = solution;

		// apply tranformation to mask.
		double& aa = solution(1);
		double& bb = solution(2);
		double& t1 = solution(3);
		double& t2 = solution(4);

	  for (int i = 0; i < mask.GetHeight(); i++)
			for (int j = 0; j < mask.GetWidth(); j++)
			{
				if (mask (j, i) != 0)
				{
					int dx = int(aa * j + bb * i + t1 +.5);
					int dy = int(-bb * j + aa * i + t2 + .5);

					tmp.SafePixel (dx, dy) = 255;
				}
			}

		mask = tmp;

		return 0;
	}
	else
		return -2;

	return -1;
}

int
YARPFlowTracker::WriteMask (YARPImageOf<YarpPixelMono>& mask, YARPImageOf<YarpPixelBGR>& out, YarpPixelBGR *color)
{
	YarpPixelBGR c;
	if (color == NULL)
	{
		c.r = 255;
		c.g = 0;
		c.b = 255;
	}
	else
		c = *color;

	for (int j = 0; j < mask.GetHeight(); j++)
		for (int i = 0; i < mask.GetWidth(); i++) 
			if (mask(i,j) != 0)
			{
				//out(i,j) = c;
				//out(i,j).r = c.r;
				out(i,j).b = c.b;
			}

	return 0;
}



#endif
