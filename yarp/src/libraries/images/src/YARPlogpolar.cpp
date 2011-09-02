//
// YARPlogpolar.cpp
//

#include "YARPlogpolar.h"

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

// locally defined - undef before leaving the file.
#ifndef pi
#define pi 3.14159265359
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YARPLogPolar::YARPLogPolar(void)
{
	xCoord = yCoord = NULL;
	nEcc = nAng = 0;
	RFmin = 0;
	Size = 0;
	ro0 = 0;
	a = 0;
	q = 0;
	kxi = 0;

	l2cx = l2cy = NULL;
	use_new = false;
}

YARPLogPolar::YARPLogPolar(int necc, int nang, double rfmin, int size)
{
	nEcc = necc;
	nAng = nang;
	RFmin = rfmin;
	Size = size;

	ro0 = nAng * RFmin / (2 * pi);
	a = (ro0 + RFmin) / ro0;
	q = nAng / (2 * pi);
	kxi = nEcc / (log((size / 2 - 1) / ro0) / log(a));

	xCoord = new int[nEcc * nAng];
	yCoord = new int[nEcc * nAng];
	assert (xCoord != NULL && yCoord != NULL);

	InitSampling();

	l2cx=new int[Size*Size];
	l2cy=new int[Size*Size];
	assert (l2cx!=NULL && l2cy!=NULL);

	const double x0 = Size/2; // y0 e' uguale (img quadrata).
	double cx,cy;

	for (int i=0;i<Size;i++)
		for (int j=0;j<Size;j++)
		{
			Cart2Lp(i,j,cx,cy); //mappa il pixel (x,y) dell'imm. cartesiana nel corrispondente pixel (cx,cy) nel dominio corticale (log-polare)
			const double dx = i - x0;
			const double dy = j - x0;
			const double ro = sqrt (dx * dx + dy * dy);

			if (ro < ro0 || ro > x0 -1)
				*(l2cx+i+j*Size)=-1;
			else
			{
				*(l2cx+i+j*Size)=int(cx);
				*(l2cy+i+j*Size)=int(cy);
			}
		}

	// simply comment this to use the faster old method.
	InitNewSampling ();
}

void YARPLogPolar::Resize(int necc, int nang, double rfmin, int size)
{
	if (xCoord != NULL) delete[] xCoord;
	if (yCoord != NULL) delete[] yCoord;
	if (l2cx != NULL) delete[] l2cx;
	if (l2cy != NULL) delete[] l2cy;

	nEcc = necc;
	nAng = nang;
	RFmin = rfmin;
	Size = size;

	ro0 = nAng * RFmin / (2 * pi);
	a = (ro0 + RFmin) / ro0;
	q = nAng / (2 * pi);
	kxi = nEcc / (log((size / 2 - 1) / ro0) / log(a));

	xCoord = new int[nEcc * nAng];
	yCoord = new int[nEcc * nAng];
	assert (xCoord != NULL && yCoord != NULL);

	InitSampling();

	l2cx=new int[Size*Size];
	l2cy=new int[Size*Size];
	assert (l2cx!=NULL && l2cy!=NULL);

	const double x0 = Size/2; // y0 e' uguale (img quadrata).
	double cx,cy;

	for (int i=0;i<Size;i++)
		for (int j=0;j<Size;j++)
		{
			Cart2Lp(i,j,cx,cy);
			const double dx = i - x0;
			const double dy = j - x0;
			const double ro = sqrt (dx * dx + dy * dy);

			if (ro < ro0 || ro > x0 -1)
				*(l2cx+i+j*Size)=-1;
			else
			{
				*(l2cx+i+j*Size)=int(cx);
				*(l2cy+i+j*Size)=int(cy);
			}
		}

	// simply comment this to use the faster old method.
	InitNewSampling ();
}

YARPLogPolar::~YARPLogPolar(void)
{
	if (xCoord != NULL) delete[] xCoord;
	if (yCoord != NULL) delete[] yCoord;

	if (l2cx != NULL) delete[] l2cx;
	if (l2cy != NULL) delete[] l2cy;

	if (use_new)
	{
		const int size = nEcc * nAng;
		for (int i = 0; i < size; i++)
		{
			delete[] x_ave[i];
			delete[] y_ave[i];
		}

		delete[] x_ave;
		delete[] y_ave;
	}
}


//
//
//
//
void YARPLogPolar::InitSampling(void)
{
	double ro,theta;
	int csize = Size / 2;

	for (int u=0;u<nEcc;u++)
		for (int v=0;v<nAng;v++)
		{
			ro=ro0 * pow(a, u / kxi);
			theta = v / q;

			xCoord[v*nEcc+u]=int(csize+ro*cos(theta)+.5);
			yCoord[v*nEcc+u]=int(csize+ro*sin(theta)+.5);
		}
}

void YARPLogPolar::InitNewSampling(void)
{
	const double coverage = 0.6; // gives the amount of averaging (i.e. smoothing)
	double ro,theta;
	int csize = Size / 2;
	int receptive_field;

	x_ave = new int* [nEcc * nAng];
	y_ave = new int* [nEcc * nAng];
	assert (x_ave != NULL && y_ave != NULL);

	for (int u = 0; u < nEcc; u++)
	{
		// compute the size of the receptive field (given the actual ecc)
		ro = ro0 * pow(a, u / kxi);
		if (u == 0)
			receptive_field = 0;
		else
		{
 			double new_ro = ro0 * pow(a, (u + 1) / kxi);
			receptive_field = int(coverage * (new_ro - ro) + .5);
		}

		for (int v = 0;v < nAng; v++)
		{
			theta = v / q;
			int index = v * nEcc + u;

			if (receptive_field == 0)
			{
				x_ave[index] = new int[2];
				y_ave[index] = new int[1];
				assert (x_ave[index] != NULL && y_ave[index] != NULL);
				x_ave[index][0] = 1;

				x_ave[index][1] = int(csize + ro * cos(theta) + .5);
				y_ave[index][0] = int(csize + ro * sin(theta) + .5);
			}
			else
			{
				// COMPUTE A NON NULL RF.
				// describe a square around the center and fill the array 
				// with all the pixel within the circle given by the 
				// receptive field size.
				const int central_x = int(csize + ro * cos(theta) + .5);
				const int central_y = int(csize + ro * sin(theta) + .5); 
				int xmin = central_x - receptive_field;
				if (xmin < 0) xmin = 0;
				int xmax = central_x + receptive_field;
				if (xmax >= Size) xmax = Size - 1;
				int ymin = central_y - receptive_field;
				if (ymin < 0) ymin = 0;
				int ymax = central_y + receptive_field;
				if (ymax >= Size) ymax = Size - 1;

				// waste of mem. or time?
				int count = 0;

				int radius;
				for (int uu = xmin; uu <= xmax; uu++)
					for (int vv = ymin; vv <= ymax; vv++)
					{
						 radius = (uu - central_x) * (uu - central_x) + 
							 (vv - central_y) * (vv - central_y);

						 if (radius <= receptive_field * receptive_field)
						 {
							 count ++;
						 }
					}
				assert (count != 0); // paranoid!

				x_ave[index] = new int[count + 1];
				y_ave[index] = new int[count];
				assert (x_ave[index] != NULL && y_ave[index] != NULL);

				x_ave[index][0] = 0;
				count = 0;
				for (int uu = xmin; uu <= xmax; uu++)
					for (int vv = ymin; vv <= ymax; vv++)
					{
						 radius = (uu - central_x) * (uu - central_x) + 
							 (vv - central_y) * (vv - central_y);

						 if (radius <= receptive_field * receptive_field)
						 {
							 // new good point!
							 x_ave[index][0] ++;
							 x_ave[index][count + 1] = uu;
							 y_ave[index][count] = vv;
							 count++;
						 }
					}
			}
		}
	}

	use_new = true;
}

// average across RF points.
void YARPLogPolar::Cart2LpAverage(const YARPGenericImage& is, YARPGenericImage& id) const
{
	assert(is.GetWidth() == Size && is.GetHeight() == Size);
	assert(id.GetWidth() == nEcc && id.GetHeight() == nAng);
	assert(id.GetID () == is.GetID ());

	unsigned char **dst = (unsigned char **)id.GetArray ();
	unsigned char **src = (unsigned char **)is.GetArray ();

	switch (id.GetID())
	{
	default:
	case YARP_PIXEL_INVALID:
		printf ("*** LopPolar conversion: not a valid pixel type\n");
		exit(1);
		break;

	case YARP_PIXEL_MONO:
	case YARP_PIXEL_MONO_SIGNED:
		{
			int accumulator;

			for (int u = 0; u < nEcc; u++)
			{
				for (int v = 0; v < nAng; v++)
				{
					int index = v * nEcc + u;
					int rflinsize = x_ave[index][0];
					accumulator = 0;

					for (int kk = 0; kk < rflinsize; kk++)
					{
						accumulator += src[y_ave[index][kk]][x_ave[index][kk+1]];
					}

					dst[v][u] = accumulator / rflinsize;
				}
			}
		}
		break;

	case YARP_PIXEL_HSV:
	case YARP_PIXEL_RGB:
	case YARP_PIXEL_BGR:
		{
			int accumulator0;
			int accumulator1;
			int accumulator2;

			for (int u = 0; u < nEcc; u++)
			{
				for (int v = 0; v < nAng; v++)
				{
					int index = v * nEcc + u;
					int rflinsize = x_ave[index][0];
					accumulator0 = 0;
					accumulator1 = 0;
					accumulator2 = 0;

					for (int kk = 0; kk < rflinsize; kk++)
					{
						int x = x_ave[index][kk+1];
						int y = y_ave[index][kk];
						accumulator0 += src[y][x*3];
						accumulator1 += src[y][x*3+1];
						accumulator2 += src[y][x*3+2];
					}

					dst[v][u*3] = accumulator0 / rflinsize;
					dst[v][u*3+1] = accumulator1 / rflinsize;
					dst[v][u*3+2] = accumulator2 / rflinsize;
				}
			}
		}
		break;
	}
}

// optionally swap channels.
void YARPLogPolar::Cart2LpSwap(const YARPGenericImage& is, YARPGenericImage& id) const
{
	assert(is.GetWidth() == Size && is.GetHeight() == Size);
	assert(id.GetWidth() == nEcc && id.GetHeight() == nAng);
	assert(is.GetPixelSize () == id.GetPixelSize ());

	unsigned char **dst = (unsigned char **)id.GetArray ();
	unsigned char **src = (unsigned char **)is.GetArray ();

	switch (id.GetID())
	{
	default:
	case YARP_PIXEL_INVALID:
		printf ("*** LopPolar conversion: not a valid pixel type\n");
		exit(1);
		break;

	case YARP_PIXEL_MONO:
	case YARP_PIXEL_MONO_SIGNED:
		{
			int accumulator;

			for (int u = 0; u < nEcc; u++)
			{
				for (int v = 0; v < nAng; v++)
				{
					int index = v * nEcc + u;
					int rflinsize = x_ave[index][0];
					accumulator = 0;

					for (int kk = 0; kk < rflinsize; kk++)
					{
						accumulator += src[y_ave[index][kk]][x_ave[index][kk+1]];
					}

					dst[v][u] = accumulator / rflinsize;
				}
			}
		}
		break;

	case YARP_PIXEL_HSV:
		{
			int accumulator0;
			int accumulator1;
			int accumulator2;

			for (int u = 0; u < nEcc; u++)
			{
				for (int v = 0; v < nAng; v++)
				{
					int index = v * nEcc + u;
					int rflinsize = x_ave[index][0];
					accumulator0 = 0;
					accumulator1 = 0;
					accumulator2 = 0;

					for (int kk = 0; kk < rflinsize; kk++)
					{
						int x = x_ave[index][kk+1];
						int y = y_ave[index][kk];
						accumulator0 += src[y][x*3];
						accumulator1 += src[y][x*3+1];
						accumulator2 += src[y][x*3+2];
					}

					dst[v][u*3] = accumulator0 / rflinsize;
					dst[v][u*3+1] = accumulator1 / rflinsize;
					dst[v][u*3+2] = accumulator2 / rflinsize;
				}
			}
		}
		break;

	case YARP_PIXEL_RGB:
	case YARP_PIXEL_BGR:
		if (is.GetID() == id.GetID())
		{
			int accumulator0;
			int accumulator1;
			int accumulator2;

			for (int u = 0; u < nEcc; u++)
			{
				for (int v = 0; v < nAng; v++)
				{
					int index = v * nEcc + u;
					int rflinsize = x_ave[index][0];
					accumulator0 = 0;
					accumulator1 = 0;
					accumulator2 = 0;

					for (int kk = 0; kk < rflinsize; kk++)
					{
						int x = x_ave[index][kk+1];
						int y = y_ave[index][kk];
						accumulator0 += src[y][x*3];
						accumulator1 += src[y][x*3+1];
						accumulator2 += src[y][x*3+2];
					}

					dst[v][u*3] = accumulator0 / rflinsize;
					dst[v][u*3+1] = accumulator1 / rflinsize;
					dst[v][u*3+2] = accumulator2 / rflinsize;
				}
			}
		}
		else
		{
			// RGB->BGR or VV.
			int accumulator0;
			int accumulator1;
			int accumulator2;

			for (int u = 0; u < nEcc; u++)
			{
				for (int v = 0; v < nAng; v++)
				{
					int index = v * nEcc + u;
					int rflinsize = x_ave[index][0];
					accumulator0 = 0;
					accumulator1 = 0;
					accumulator2 = 0;

					for (int kk = 0; kk < rflinsize; kk++)
					{
						int x = x_ave[index][kk+1];
						int y = y_ave[index][kk];
						accumulator0 += src[y][x*3];
						accumulator1 += src[y][x*3+1];
						accumulator2 += src[y][x*3+2];
					}

					dst[v][u*3] = accumulator2 / rflinsize;
					dst[v][u*3+1] = accumulator1 / rflinsize;
					dst[v][u*3+2] = accumulator0 / rflinsize;
				}
			}
		}
		break;
	}
}

void YARPLogPolar::Cart2Lp(const YARPGenericImage& is, YARPGenericImage& id) const
{
	assert(is.GetWidth() == Size && is.GetHeight() == Size);
	assert(id.GetWidth() == nEcc && id.GetHeight() == nAng);
	assert(is.GetID() == id.GetID());

	unsigned char **dst = (unsigned char **)id.GetArray();
	unsigned char **src = (unsigned char **)is.GetArray();
	
	int index;
	switch (is.GetID())
	{
	default:
	case YARP_PIXEL_INVALID:
		printf ("*** LopPolar conversion: not a valid pixel type\n");
		exit(1);
		break;

	case YARP_PIXEL_MONO:
	case YARP_PIXEL_MONO_SIGNED:
		{
			for (int v=0;v<nAng;v++)
				for (int u=0;u<nEcc;u++)
				{
					index=v*nEcc+u;
					dst[v][u] = src[yCoord[index]][xCoord[index]];
				}
		}
		break;

	case YARP_PIXEL_RGB:
	case YARP_PIXEL_HSV:
	case YARP_PIXEL_BGR:
		{
			for (int v=0;v<nAng;v++)
				for (int u=0;u<nEcc;u++)
				{
					index=v*nEcc+u;
					dst[v][u*3] = src[yCoord[index]][xCoord[index]*3];
					dst[v][u*3+1] = src[yCoord[index]][xCoord[index]*3+1];
					dst[v][u*3+2] = src[yCoord[index]][xCoord[index]*3+2];
				}
		}
		break;
	}
}

void YARPLogPolar::Lp2Cart(const YARPGenericImage& is, YARPGenericImage& id) const
{
	assert(id.GetWidth() == Size && id.GetHeight() == Size);
	assert(is.GetWidth() == nEcc && is.GetHeight() == nAng);
	assert(is.GetID() == id.GetID());

	unsigned char** src = (unsigned char **)is.GetArray(); 
	unsigned char** dest = (unsigned char **)id.GetArray();

	switch (is.GetID())
	{
	default:
	case YARP_PIXEL_INVALID:
		printf ("*** LopPolar conversion: not a valid pixel type\n");
		exit(1);
		break;

	case YARP_PIXEL_MONO:
	case YARP_PIXEL_MONO_SIGNED:
		{
			for (int i=0;i<Size;i++)
				for (int j=0;j<Size;j++) 
				{
					if (*(l2cx+i+j*Size)==-1) 
						dest[j][i]=0;
					else
						dest[j][i]=src[*(l2cy+i+j*Size)][*(l2cx+i+j*Size)];
				}
		}
		break;

	case YARP_PIXEL_RGB:
	case YARP_PIXEL_HSV:
	case YARP_PIXEL_BGR:
		{
			for (int i=0;i<Size;i++)
				for (int j=0;j<Size;j++) 
				{
					if (*(l2cx+i+j*Size)==-1) 
					{
						dest[j][i*3]=0;
						dest[j][i*3+1]=0;
						dest[j][i*3+2]=0;
					}
					else
					{
						dest[j][i*3]=src[*(l2cy+i+j*Size)][*(l2cx+i+j*Size) * 3];
						dest[j][i*3+1]=src[*(l2cy+i+j*Size)][*(l2cx+i+j*Size) * 3 + 1];
						dest[j][i*3+2]=src[*(l2cy+i+j*Size)][*(l2cx+i+j*Size) * 3 + 2];
					}
				}
		}
		break;
	}
}

// un-definitions
#undef pi
