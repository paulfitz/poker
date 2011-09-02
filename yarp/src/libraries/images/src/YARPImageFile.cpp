#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fstream>
using namespace std;

#include "YARPImageFile.h"


// The PGM/PPM code is old code from a long forgotten source.

static void die(char *message)
{
  fprintf(stderr, "pgm/ppm: Error - %s\n", message);
  exit(1);
}

static void warn(char *message)
{
  fprintf(stderr, "pgm/ppm: Error - %s\n", message);
}

static int SavePGM(char *src, const char *filename, int h, int w)
{
  FILE *fp = fopen(filename, "wb");
  if (!fp) 
    {
      printf("cannot open file %s for writing\n", filename);
      return -1;
    }
  else
    {
      fprintf(fp, "P5\n%d %d\n%d\n", w, h, 255);
      fwrite((void *) src, 1, (size_t) (h*w), fp);
      fclose(fp);
    }

  return 0;
}

static int SavePPM(char *src, const char *filename, int h, int w)
{
  FILE *fp = fopen(filename, "wb");
  if (!fp) 
    {
      printf("cannot open file %s for writing\n", filename);
      return -1;
    }
  else
    {
      fprintf(fp, "P6\n%d %d\n%d\n", w, h, 255);
      fwrite((void *) src, 1, (size_t) (h*w*3), fp);
      fclose(fp);
    }

  return 0;
}


static int ReadHeader(FILE *fp, int *height, int *width, int *color)
{
  char ch;
  int  maxval;

  *color = 0;
  if (fscanf(fp, "P%c\n", &ch) != 1 || (ch!='6'&&ch!='5')) 
    //die("file is not in pgm/ppm raw format; cannot read");
  {
	warn("file is not in pgm/ppm raw format; cannot read");
	return -1;
  }

  if (ch=='6') *color = 1;

    // skip comments
  ch = getc(fp);
  while (ch == '#')
    {
      do {
	ch = getc(fp);
      } while (ch != '\n');   
      ch = getc(fp);    
    }
  /*
  while (ch=='\n' || ch=='\r')
    {
      ch = getc(fp);
    }
  ungetc(ch,fp);
  */

  while(!isdigit(ch))
    {
      ch = getc(fp);
    }

  if (!isdigit(ch)) //die("cannot read header information from pgm/ppm file");
  {
	  warn("cannot read header information from pgm/ppm file");
	  return -1;
  }
  ungetc(ch, fp);

  fscanf(fp, "%d%d%d", width, height, &maxval);
  getc(fp);
  if (maxval != 255)
  {
	//die("image is not true-color (24 bit); read failed");
	warn("image is not true-color (24 bit); read failed");
	return -1;
  }

  return 0;
}


static int ImageRead(YARPGenericImage& img, const char *filename)
{
  int width, height, color, num, size;

  FILE  *fp    = fopen(filename, "rb");

  if (!fp)    //die("cannot open file for reading");
  {
	warn("cannot open file for reading");
	return -1;
  }

  if (ReadHeader(fp, &height, &width, &color) < 0)
  {
	fclose (fp);
	return -1;
  }

  if (img.GetID()==YARP_PIXEL_RGB || img.GetID() == YARP_PIXEL_MONO)
  {
	  img.SetID(color?YARP_PIXEL_RGB:YARP_PIXEL_MONO);
	  img.Resize(width,height);
	  assert(img.GetPadding() == 0);
	  assert(img.GetRawBuffer()!=NULL);
	  size = img.GetHeight()*img.GetWidth()*img.GetPixelSize();

	  num = fread((void *) img.GetRawBuffer(), 1, (size_t) size, fp);
  }
  else
  {
	  img.SetID(YARP_PIXEL_BGR);
	  img.Resize(width,height);
	  assert(img.GetPadding() == 0);
	  assert(img.GetRawBuffer()!=NULL);
	  size = img.GetHeight()*img.GetWidth()*img.GetPixelSize();

  	  YARPImageOf<YarpPixelRGB> img2;
	  img2.Resize (width,height);

	  num = fread((void *) img2.GetRawBuffer(), 1, (size_t) size, fp);

	  img.CastCopy(img2);
  }

  if (num != size) 
	{
	  printf ( "%d versus %d\n", num, size );
	  //die("cannot read image data from file");
	  warn("cannot read image data from file");
	  fclose (fp);
	  return -1;
	}

  fclose(fp);

  return 0;
}

static int ImageWrite(YARPGenericImage& img, const char *filename)
{
  assert(img.GetPadding()==0);
  if (img.GetID()==YARP_PIXEL_MONO)
    {
      SavePGM((char*)img.GetRawBuffer(),filename,img.GetHeight(),
	      img.GetWidth());
    }
  else
    {
      if (img.GetID()==YARP_PIXEL_RGB)
	{
	  assert(img.GetID()==YARP_PIXEL_RGB);
	  //      Image<YarpPixelRGB> img2;
	  //      img2.ReferOrCopy(img);
	  SavePPM((char*)img.GetRawBuffer(),filename,img.GetHeight(),
		  img.GetWidth());
	}
      else
	{
	  YARPImageOf<YarpPixelRGB> img2;
	  img2.CastCopy(img);
	  SavePPM((char*)img2.GetRawBuffer(),filename,img2.GetHeight(),
		  img2.GetWidth());	  
	}
    }

  return 0;
}


/*
void Reim(GenericImage& src,GenericImage& dest)
{
  if (dest.GetWidth()!=src.GetWidth() || 
      dest.GetHeight()!=src.GetHeight())
    {
      dest.Create(src.GetHeight(),src.GetWidth());
    }
}

void Reim(GenericImage& dest, int h, int w)
{
  if (dest.GetWidth()!=h || dest.GetHeight()!=w)
    {
      dest.Create(h,w);
    }
}
*/


int YARPImageFile::Read(const char *src, YARPGenericImage& dest, 
			int format)
{
  if (format!=YARPImageFile::FORMAT_NUMERIC)
    {
      return ImageRead(dest,src);
    }
  int hh = 0, ww = 0;
  {
    ifstream fin(src);
    int blank = 1;
    int curr = 0;
    while (!fin.eof())
      {
	int ch = fin.get();
	//if (ch!='\n') printf("[%c]",ch);
	if (ch==' ' || ch == '\t' || ch == '\r' || ch == '\n' || fin.eof())
	  {
	    if (!blank)
	      {
		if (curr==0)
		  {
		    hh++;
		  }
		curr++;
		if (curr>ww)
		  {
		    ww = curr;
		    //printf("%d\n", ww);
		  }
	      }
	    blank = 1;
	    if (ch=='\n')
	      {
		curr = 0;
	      }
	  }
	else
	  {
	    blank = 0;
	  }
      }
  }
  //printf("yyy dim %d %d\n", hh, ww);
  YARPImageOf<YarpPixelFloat> flt;
  flt.Resize(ww,hh);
  hh = 0; ww = 0;
  {
    char buf[256];
    int idx = 0;
    ifstream fin(src);
    int blank = 1;
    int curr = 0;
    while (!fin.eof())
      {
	int ch = fin.get();
	if (ch==' ' || ch == '\t' || ch == '\r' || ch == '\n' || fin.eof())
	  {
	    if (!blank)
	      {
		if (curr==0)
		  {
		    hh++;
		  }
		curr++;
		if (curr>ww)
		  {
		    ww = curr;
		  }
		buf[idx] = '\0';
		flt(curr-1,hh-1) = atof(buf);
		idx = 0;
	      }
	    blank = 1;
	    if (ch=='\n')
	      {
		curr = 0;
	      }
	  }
	else
	  {
	    buf[idx] = ch;
	    idx++;
	    assert(idx<sizeof(buf));
	    blank = 0;
	  }
      }
  }
    
  dest.CastCopy(flt);

  return 0;
}


int YARPImageFile::Write(const char *dest, YARPGenericImage& src,
			 int format)
{
  if (format!=YARPImageFile::FORMAT_NUMERIC)
    {
      return ImageWrite(src,dest);
    }
  YARPImageOf<YarpPixelFloat> flt;
  ofstream fout(dest);
  flt.CastCopy(src);
  for (int i=0; i<flt.GetHeight(); i++)
    {
      for (int j=0; j<flt.GetWidth(); j++)
	{
	  char buf[255];
	  sprintf(buf,"%g ", flt(j,i));
	  fout << buf << " ";
	}
      fout << endl;
    }
  return 0;
}


