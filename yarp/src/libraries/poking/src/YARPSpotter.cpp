#include <assert.h>
#include <stdio.h>
#include "YARPSpotter.h"

#include <fstream.h>
#include <strstream.h>
#include <assert.h>
#include <math.h>

#ifdef __LINUX__
#define TESTING
#define SAVE_SECOND_FILES
#define SHOW_SMALL_CIRCLE
#define SHOW_IMPLICATED
//#define SHOW_SHORTS
#else
#define USE_CUTOFF
#define SHOW_SHORTS
#endif

//#define RIGHT_ALIGN

#ifdef __LINUX_MORE
//#define FORCE_MATCH
//#define ENRICH_REGIONS
#define SHOW_SMALL_CIRCLE
//#define USE_CONTINUUM
#define SUPER_SENSITIVE
//#define ORIENTATION_SPECIFIC
//#define SHARP_TARGET
#define SCALE_SPECIFIC
#define FINE_DETAIL
//#define USE_CUTOFF
#endif

#define DETAIL_THETA (3)
#define CONTINUUM_FACTOR1 (1)
#define CONTINUUM_FACTOR2 (0.01)

#define LOW_THETA (1e5)

//#define SHOW_WIDE_CIRCLE
#ifdef __QNX__
#define MINIMAL_ANTICS
#endif

#define LIGHT_MARKINGS

#define CONSISTENT_SCALE

//#define STATEFUL

#define MIDDLE_POSITION

#define MIN_MATCH (-2.0)

#include <hash_map>
#include <algorithm>

#include "YARPRandomNumber.h"
#include "YARPImage.h"
#include "YARPImageFile.h"
#include "YARPImageDraw.h"
#include "YARPBinaryAnalysis.h"
#include "IntegralImageTool.h"
#include "YARPShapeBoundary.h"
#include "YARPImageLabel.h"
#include "YARPFineOrientation.h"
#include "YARPNetworkTypes.h"
#include "YARPSemaphore.h"
#include "YARPVisualSearch.h"

#include "YARPSpotter.h"

#include "fit_ellipse.h"


#include "YARPImage.h"
#include "YARPImageDraw.h"

#include "graph.h"
#include "bresenhm.h"
//#define ORIGINAL

//#ifndef TESTING
//#endif

#define CAN_EXPLAIN

static int g_use_graph = 1;
static int g_found = 0;
static int g_found_x = 0;
static int g_found_y = 0;
static int g_found_id = 0;
static int g_found_r = 0;
static int g_target = -1;
static int g_training = 0;
static YARPImageOf<YarpPixelBGR> g_precedent;
static YARPImageOf<YarpPixelMono> g_precedent_mask;

#define RGBtoCYL my_color2

static YARPSemaphore mutex(1);

static void my_color2(float r, float g, float b, 
		      float& c1, float& c2, float& c3)
{
  r /= 256;
  g /= 256;
  b /= 256;
  c3 = (r+g+b);
  c1 = r-g;
  c2 = r-b;
  c3 /= 3.0;
//  printf("%g %g %g\n", c1, c2, c3);
}


void MeanOrient(double dx1, double dy1, double dx2, double dy2,
		double& dx3, double& dy3)
{
  double p = dx1*dx2+dy1*dy2;
  if (p<0)
    {
      dx2 *= -1;
      dy2 *= -1;
    }
  dx1 += dx2;
  dy1 += dy2;
  if (fabs(dx1)<0.001 && fabs(dy1)<0.001)
    {
      dx1 = 1;
      dy1 = 0;
    }
  double th = atan2(dy1,dx1);
  dx3 = cos(th);
  dy3 = sin(th);
}

//void Line2D (int x0, int y0, int x1, int y1, void (*callback)(int,int));

static int dln_x0, dln_y0, dln_x1, dln_y1;
static YARPImageOf<YarpPixelBGR> *dln_dest = NULL;
static YarpPixelBGR dln_pix;

void DrawLineHelper(int x, int y)
{
  dln_dest->SafePixel(x,y) = dln_pix;
}


void DrawLine(YARPImageOf<YarpPixelBGR>& dest,
	      YarpPixelBGR pix, int x0, int y0, int x1, int y1)
{
  dln_x0 = x0;
  dln_y0 = y0;
  dln_x1 = x1;
  dln_y1 = y1;
  dln_dest = &dest;
  dln_pix = pix;
  Line2D(x0,y0,x1,y1,DrawLineHelper);
  dln_dest = NULL;
}


#define DEL 1
#define DIAG 1.4142136
#define HALF_DIAG 2.236068
#define LINE_STRENGTH 10.0
#define PRE_MOTION 5
#define DIM 128

inline float Compare3(YARPImageOf<YarpPixelFloat>& src_motion, 
		      YARPImageOf<YarpPixelFloat>& src_variance, 
		      int i0, int j0, int i1, int j1)
{
  float v = 0;
  float s0 = src_motion(i0,j0);
  float s1 = src_motion(i1,j1);
  float v0 = src_variance(i0,j0);
  float v1 = src_variance(i1,j1);
#ifdef ORIGINAL
  if ((s0<10)&&(s1<10))
    {
      v = 1;
    }
  else
    {
      v = 2;
    }

  if (v0>10 || v1>10)
    {
      v = 3;
    }
#else
  v = 1;
  if (s0>10 || s1>10)
    {
      v = 2;
    }
  if (v0>10 || v1>10)
    {
      v = 2;
    }
  //v = 1;
#endif


  return v;
}


// red pixels are definitely foreground
// green pixels are definitely background
// black pixels have no allegiance

static void GroupOptim(YARPImageOf<YarpPixelBGR>& src, 
		       YARPImageOf<YarpPixelBGR>& dest, float p1, float p2)
{
  SatisfySize(src,dest);

  static Graph::node_id nodes[DIM][DIM];
  Graph *g = new Graph();
  
  for (int i=0; i<DIM; i++)
    {
      for (int j=0; j<DIM; j++)
	{
	  nodes[i][j] = g->add_node();
	}
    }

  YARPImageOf<YarpPixelFloat> src_motion, src_variance;
  SatisfySize(src,src_motion);
  SatisfySize(src,src_variance);
  src_motion.Zero();
  src_variance.Zero();
  IMGFOR(src,x,y)
    {
      YarpPixelBGR& pix = src(x,y);
      if (pix.r>128)
	{
	  src_motion(x,y) = 255;
	}
      if (pix.g>128)
	{
	  src_variance(x,y) = 255;
	}
    }

  float force = 1e12;
  for (int i=0; i<DIM; i++)
    {
      for (int j=0; j<DIM; j++)
	{
	  float mot = src_motion(i,j);
	  float var = src_variance(i,j);
	  if (i<=0 || j<=0 || i>=DIM-1 || j>=DIM-1)
	    {
	      g->set_tweights(nodes[i][j], LINE_STRENGTH*1e10,
			      0.0);
	    }
	  else
	    {
	      if (mot>128)
		{
		  g->set_tweights(nodes[i][j], 0.0, LINE_STRENGTH*force);
		}
	      else if (var>128)
		{
		  g->set_tweights(nodes[i][j], LINE_STRENGTH*force, 0.0);
		}
	      else
		{
		  //g->set_tweights(nodes[i][j], LINE_STRENGTH*0.0,
		  //LINE_STRENGTH*0.00);
		  g->set_tweights(nodes[i][j], LINE_STRENGTH*p1,
				  LINE_STRENGTH*0.00);
		}
	    }
	}
    }

  //YARPImageOf<YarpPixelFloat>& src_variance = src_motion;
  for (int i=0; i<DIM; i++)
    {
      for (int j=0; j<DIM; j++)
	{

	  if (i>0)
	    {
	      float v = Compare3(src_motion,src_variance,i,j,i-1,j);
	      g->add_edge(nodes[i][j],nodes[i-1][j],v,v);
	    }
	  if (j>0)
	    {
	      float v = Compare3(src_motion,src_variance,i,j,i,j-1);
	      g->add_edge(nodes[i][j],nodes[i][j-1],v,v);
	    }
	  if (i<DIM-1)
	    {
	      float v = Compare3(src_motion,src_variance,i,j,i+1,j);
	      g->add_edge(nodes[i][j],nodes[i+1][j],v,v);
	    }
	  if (j<DIM-1)
	    {
	      float v = Compare3(src_motion,src_variance,i,j,i,j+1);
	      g->add_edge(nodes[i][j],nodes[i][j+1],v,v);
	    }
	  if (i>0)
	    {
	      if (j>0)
		{
		  float v = DIAG*Compare3(src_motion,src_variance,i,j,i-1,j-1);
		  g->add_edge(nodes[i][j],nodes[i-1][j-1],v,v);
		}
	      if (j<DIM-1)
		{
		  float v = DIAG*Compare3(src_motion,src_variance,i,j,i-1,j+1);
		  g->add_edge(nodes[i][j],nodes[i-1][j+1],v,v);
		}
	    }
	  if (i<DIM-1)
	    {
	      if (j>0)
		{
		  float v = DIAG*Compare3(src_motion,src_variance,i,j,i+1,j-1);
		  g->add_edge(nodes[i][j],nodes[i+1][j-1],v,v);
		}
	      if (j<DIM-1)
		{
		  float v = DIAG*Compare3(src_motion,src_variance,i,j,i+1,j+1);
		  g->add_edge(nodes[i][j],nodes[i+1][j+1],v,v);
		}
	    }
	  
	  // HALF_DIAG
	  
	  if (1)
	    {
	      if (i>1)
		{
		  if (j>0)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i-2,j-1);
		      g->add_edge(nodes[i][j],nodes[i-2][j-1],v,v);
		    }
		  if (j<DIM-1)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i-2,j+1);
		      g->add_edge(nodes[i][j],nodes[i-2][j+1],v,v);
		    }
		}
	      if (i<DIM-2)
		{
		  if (j>0)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i+2,j-1);
		      g->add_edge(nodes[i][j],nodes[i+2][j-1],v,v);
		    }
		  if (j<DIM-1)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i+2,j+1);
		      g->add_edge(nodes[i][j],nodes[i+2][j+1],v,v);
		    }
		}
	      if (j>1)
		{
		  if (i>0)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i-1,j-2);
		      g->add_edge(nodes[i][j],nodes[i-1][j-2],v,v);
		    }
		  if (i<DIM-1)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i+1,j-2);
		      g->add_edge(nodes[i][j],nodes[i+1][j-2],v,v);
		    }
		}
	      if (j<DIM-2)
		{
		  if (i>0)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i-1,j+2);
		      g->add_edge(nodes[i][j],nodes[i-1][j+2],v,v);
		    }
		  if (i<DIM-1)
		    {
		      float v = HALF_DIAG*Compare3(src_motion,src_variance,
						   i,j,i+1,j+2);
		      g->add_edge(nodes[i][j],nodes[i+1][j+2],v,v);
		    }
		}
	    }
	}
    }

  Graph::flowtype flow = g -> maxflow();
  
  printf("group motion flow = %g\n", (double)flow);

  dest.PeerCopy(src);
  for (int i=0; i<DIM; i++)
    {
      for (int j=0; j<DIM; j++)
	{
	  int active = 1;
	  if (g->what_segment(nodes[i][j]) == Graph::SOURCE)
	    {
	      active = 0;
	    }
	  if (!active)
	    {
	    }
	  else
	    {
	      dest(i,j).b = 255;
	    }
	}
    }

  delete g;
}



#ifdef TESTING
#define OUTPUT_SELECT
#define OUTPUT_ORIENT
#define OUTPUT_CHOICES
#endif


#define BASE "/tmp/food/"
//#define BASE "/tmp/food/"


#define MAX_POINT 50

class EllipseDrawer
{
public:
  void Apply(YARPImageOf<YarpPixelMono>& dest,
	     double xc, double yc,
	     double a, double b,
	     double xa, double ya)
    {
      double xb, yb;

      xb = -ya;
      yb = +xa;

      if (a<0.0001) a = 0.0001;
      if (b<0.0001) b = 0.0001;
      
      IMGFOR(dest,x,y)
	{
	  double dota = (x-xc)*xa + (y-yc)*ya;
	  double dotb = (x-xc)*xb + (y-yc)*yb;
	  dota = fabs(dota);
	  dotb = fabs(dotb);
	  double v = (dota/a)*(dota/a) + (dotb/b)*(dotb/b);
	  if (v<=1)
	    {
	      dest.SafePixel(x,y) = 255;
	    }
	}
    } 
};




//extern void GroupOptim(YARPImageOf<YarpPixelBGR>& src, 
//	       YARPImageOf<YarpPixelBGR>& dest,
//	       float p1, float p2);



//#define PRUNE_OVERLAP

#define RESEGMENT
#define SHADOW_OUTLINE

//#define BRAND_IMAGE

//#ifndef __QNX__
//#endif

//#define SCALE_SPECIFIC

//#define COHERENT_SPECIFIC

#define COHERENT_USER

#define COLOR_SPECIFIC

//#define AREA_SPECIFIC

#ifndef __QNX__
//#define SAVE_FILES
#endif
//#define SAVE_INPUT

#define STORE_ID

//#include "orient.h"

#undef printf
#define printf fflush(stdout); printf

#define MULTI_ORACLE

//#define FORCE_FIRST
#define MIN_CT 2

//#define CALIBRATE_THETA

#ifdef CALIBRATE_THETA
#ifndef FORCE_FIRST
#define FORCE_FIRST
#endif
#endif

template <class T>
T sq(T x)
{
  return x*x;
}

template <class T>
double dist(T dx1, T dy1, T dx2, T dy2)
{
  return sqrt(sq((double)dx1-dx2) + sq((double)dy1-dy2));
}

template <class T>
T mymax(T x, T y)
{
  return (x>y)?x:y;
}

template <class T>
T mymin(T x, T y)
{
  return (x<y)?x:y;
}


double nm_x = 0;
double nm_x2 = 0;
int nm_ct = 0;

YARPFineOrientation fine;

void NoteMatch(double strength)
{
  nm_x += strength;
  nm_x2 += strength*strength;
  nm_ct++;
}

double GetNoteMatchMean()
{
  return nm_x/mymax(nm_ct,1);
}

double GetNoteMatchStd()
{
  return sqrt(fabs(nm_x2/mymax(nm_ct,1) - sq(nm_x/mymax(nm_ct,1))));
}


class GeometryTool
{
public:
  double Intersect(double x0, double y0, double dx0, double dy0,
		   double x1, double y1, double dx1, double dy1,
		   double& xint, double& yint)
    {
      double ix1, iy1;
      ix1 = x1 - x0;
      iy1 = y1 - y0;
      double bnum = ix1*dy0-iy1*dx0;
      double bden = dx0*dy1-dx1*dy0;
      double eps = 0.000001;
      if (fabs(bden)<eps)
	{
	  if (bden<0)
	    {
	      bden = -eps;
	    }
	  else
	    {
	      bden = eps;
	    }
	}
      double b = bnum/bden;
      xint = x1+b*dx1;
      yint = y1+b*dy1;
      return fabs(bden);
    }
};

char name_label[256] = "";


template <class T>
void AddDeltaCircleOutline(YARPImageOf<T>& dest,
		      int i, int j, int r, int w, int step)
{
  float d, r2 = (r+w/2)*(r+w/2), r2l = (r-w/2)*(r-w/2);
  for (int ii=i-r-w/2; ii<=i+r+w/2; ii++)
    {
      for (int jj=j-r-w/2; jj<=j+r+w/2; jj++)
	{
	  d = (ii-i)*(ii-i)+(jj-j)*(jj-j);
	  if (d<=r2 && d>=r2l)
	    {
	      T& pix2 = dest.SafePixel(ii,jj);
	      //int next = pix2.r+step;
	      //if (next>255) next = 255;
	      pix2 += step;
	      assert(step<200);
	      // was pix2 += r
	    }
	}
    }
}


template <class T>
void AddDeltaCircle(YARPImageOf<T>& dest,
		      int i, int j, int r, float step)
{
  int w = 0;
  float d, r2 = (r+w/2)*(r+w/2), r2l = (r-w/2)*(r-w/2);
  for (int ii=i-r-w/2; ii<=i+r+w/2; ii++)
    {
      for (int jj=j-r-w/2; jj<=j+r+w/2; jj++)
	{
	  d = (ii-i)*(ii-i)+(jj-j)*(jj-j);
	  if (d<=r2)
	    {
	      T& pix2 = dest.SafePixel(ii,jj);
	      //int next = pix2.r+step;
	      //if (next>255) next = 255;
	      pix2 += step*(1/(d+r+1));
	      // was pix2 += r
	    }
	}
    }
}


template <class T>
void AddDeltaCircleStraight(YARPImageOf<T>& dest,
			    int i, int j, int r, float step)
{
  int w = 0;
  float d, r2 = (r+w/2)*(r+w/2), r2l = (r-w/2)*(r-w/2);
  for (int ii=i-r-w/2; ii<=i+r+w/2; ii++)
    {
      for (int jj=j-r-w/2; jj<=j+r+w/2; jj++)
	{
	  d = (ii-i)*(ii-i)+(jj-j)*(jj-j);
	  if (d<=r2)
	    {
	      T& pix2 = dest.SafePixel(ii,jj);
	      pix2 += step;
	    }
	}
    }
}


template <class T>
void AddDeltaCrossHair(YARPImageOf<T>& dest, int i, int j, int r, float step)
{
  for (int ii=i-r; ii<=i+r; ii++)
    {
      for (int jj=j-r; jj<=j+r; jj++)
	{
	  if (ii==i||jj==j)
	    {
	      dest.SafePixel(ii,jj) += step;
	    }
	}
    }
}





template <class T>
void RotateTemplate(YARPImageOf<T>& src, 
		    YARPImageOf<T>& dest,
		    float angle,
		    int x, int y, int scale)
{
  float cosa = cos(angle);
  float sina = sin(angle);
  float SQUEEZE = 1.0;
  float threshold = scale*scale;
  for (int xx=x-scale; xx<=x+scale; xx++)
    {
      for (int yy=y-scale; yy<=y+scale; yy++)
	{
	  //float delta = (xx-x)*(xx-x)+(yy-y)*(yy-y);
	  //if (delta<=threshold)
	    {
	      int xx2 = (int)(0.5+x+((SQUEEZE*(xx-x)*cosa-(yy-y)*sina)/SQUEEZE));
	      int yy2 = (int)(0.5+(y+SQUEEZE*(xx-x)*sina+(yy-y)*cosa));
	      dest.SafePixel(xx,yy) = src.SafePixel(xx2,yy2);
	    }
	}
    }
}

double Compare(YARPImageOf<YarpPixelBGR>& src,
	       YARPImageOf<YarpPixelBGR>& ref,
		 int x0, int y0,
		 int x1, int y1)
{
  double total = 0;
  IMGFOR(ref,x,y)
    {
      YarpPixelBGR& pix = ref(x,y);
      if (pix.r+pix.g+pix.b>0)
	{
	  YarpPixelBGR& pixs = src.SafePixel(x-x1+x0,y-y1+y0);
	  double v = sq(fabs(pix.r-pix.g-pixs.r+pixs.g))
	    +sq(fabs(pix.g-pix.b-pixs.g+pixs.b));
	  //double v = fabs(pix.r-pixs.r)+fabs(pix.g-pixs.g)+fabs(pix.b-pixs.b);
	  total += v;
	}
    }
  return total;
}

double FindAngle(YARPImageOf<YarpPixelBGR>& src,
		 YARPImageOf<YarpPixelBGR>& ref,
		 YARPImageOf<YarpPixelMono>& ref_mask,
		 YARPImageOf<YarpPixelBGR>& dest,
		 int x0, int y0,
		 int x1, int y1)
{
  YARPImageOf<YarpPixelBGR> tmp;
  SatisfySize(src,dest);
  SatisfySize(ref,tmp);
  double best = 1e10;
  double best_index = 0;
  for (float theta=0.0; theta<M_PI/2; theta+=0.05)
    {
      RotateTemplate(ref,tmp,theta,x1,y1,100);
      double v = Compare(src,tmp,x0,y0,x1,y1);
      if (v<best)
	{
	  best_index = theta;
	  best = v;
	}
    }
  for (float theta=best_index-0.05; theta<best_index+0.05; theta+=0.005)
    {
      RotateTemplate(ref,tmp,theta,x1,y1,100);
      double v = Compare(src,tmp,x0,y0,x1,y1);
      if (v<best)
	{
	  best_index = theta;
	  best = v;
	}
    }
  /*
  RotateTemplate(ref,tmp,best_index,x1,y1,100);
  IMGFOR(tmp,x,y)
    {
      YarpPixelBGR& pix = tmp(x,y);
      if (pix.r+pix.g+pix.b>0)
	{
	  dest.SafePixel(x-x1+x0,y-y1+y0) = pix;
	}
    }
  */
  YARPShapeBoundary sb;
  sb.SetFromMask(ref_mask);
  sb.RotateAbout(best_index,x1,y1);
  sb.Translate(-x1+x0,-y1+y0);
  sb.Show(dest,0);

#if 0
  YARPImageOf<YarpPixelMono> back;
  SatisfySize(dest,back);
  sb.GetMask(back);
  char buf[256];
  char buf2[256];
  sprintf(buf,"%s.pgm",name_label);
  for (int i=0; i<strlen(buf); i++)
    {
      if (buf[i]=='/')
	{
	  buf[i] = 'Z';
	}
    }
  sprintf(buf2,"/mnt/state/images/%s",buf);
  YARPImageFile::Write(buf2,back);
#endif
  
  return best_index;
}



class OrientedPatch
{
public:
  double x, y, dx, dy;
  double area;
  int lo_x, hi_x, lo_y, hi_y;
  //int x_lo, x_hi, y_lo, y_hi;

  int operator == (const OrientedPatch& op)
    { return 0; }

  double GetLen()
    {
      return dist(lo_x,lo_y,hi_x,hi_y);
    }

  void Write(ostream& os)
    {
      char buf[256];
      sprintf(buf,"%g %g %g %g %g %d %d %d %d ", x, y, dx, dy, area,
	      lo_x, lo_y, hi_x, hi_y);
      os << buf;
    }

  void Read(istream& is)
    {
      is >> x >> y >> dx >> dy >> area >> lo_x >> lo_y >> hi_x >> hi_y;
    }
};


/*
class Chew
{
public:
  int x;

  bool operator == (const Chew& op)
    { return false; }

  int operator == (long int op)
    { return 0; }

  bool operator < (long int op)
    { return false; }

  int operator != (const Chew& op)
    { return 0; }

  int operator < (const Chew& op)
    { return 0; }

  int operator > (const Chew& op)
    { return 0; }

  const Chew& operator = (const Chew& op)
    { return *this; }

};

typedef hash_map<long int,Chew,hash<long int>,equal_to<long int> > hash_it;


hash_it hello;
*/


typedef NetInt32 lint;
typedef NetInt32 longint;

typedef hash_map<lint,lint,hash<lint>,equal_to<lint> > hash_ii;


typedef hash_map<lint,OrientedPatch,hash<lint>,equal_to<lint> > hash_ip;




double AngularCost(double x0, double x1)
{
  double result = x0-x1;
  while (result>M_PI) 
    {
      //printf("11 %g\n", result);
      result-=2*M_PI;
    }
  while (result<-M_PI) 
    {
      //printf("22 %g\n", result);
      result+=2*M_PI;
    }
  return fabs(result);
}

double AngularCost(double x0, double x1, double x2)
{
  double result = 0;
  result += mymin(AngularCost(x0,x1),AngularCost(x0,x1+M_PI));
  result += mymin(AngularCost(x0,x2),AngularCost(x0,x2+M_PI));
  return result;
}

class Geometry
{
public:
  OrientedPatch p1, p2;
  int x0, y0;
  double xacc, yacc;
  int acc;
  int r, g, b;
  int r1, g1, b1;
  int r2, g2, b2;
  double t1, t2, t3, ss;
  double ref_area;
  int need_hash;
  lint hashnum;

  Geometry()
    {
      need_hash = 1;
    }

  int operator == (const Geometry& geo)
    { return 0; }


  void Add(Geometry& geo)
    {
      double xe, ye, re, power,escale;
      Map(geo,xe,ye,re,power,escale);
      //geo.Map(*this,xe,ye,re,power);
      xacc += xe;
      yacc += ye;
      acc++;
      x0 = (int)(xacc/acc);
      y0 = (int)(yacc/acc);
    }

  void Set(OrientedPatch& np1, OrientedPatch& np2,
	   int x, int y, double area = 0)
    {
      p1 = np1;
      p2 = np2;
      x0 = x;
      y0 = y;
      //Reorder();
      need_hash = 1;
      xacc = x;
      yacc = y;
      acc = 1;
      ref_area = area;
    }
  
  void Explain()
    {
      need_hash = 0;
      BaseGetHash();
    }


  int IsCoherent()
    {
      double d1 = dist(p1.lo_x,p1.lo_y,p2.lo_x,p2.lo_y);
      double d2 = dist(p1.lo_x,p1.lo_y,p2.hi_x,p2.hi_y);
      double d3 = dist(p1.hi_x,p1.hi_y,p2.lo_x,p2.lo_y);
      double d4 = dist(p1.hi_x,p1.hi_y,p2.hi_x,p2.hi_y);
      double dmin = mymin(d1,mymin(d2,mymin(d3,d4)));
      double dmax = mymax(d1,mymax(d2,mymax(d3,d4)));
      //if (dmin>=10)
      //{
      //  printf("D'oh! out by %g (%g)\n", dmin, dmax);
      //}
      //return (dmin<10);
	//return 1;
      return (dmin<dmax*0.2);
    }

  double WideScale()
    {
      double d1 = dist(p1.lo_x,p1.lo_y,p2.lo_x,p2.lo_y);
      double d2 = dist(p1.lo_x,p1.lo_y,p2.hi_x,p2.hi_y);
      double d3 = dist(p1.hi_x,p1.hi_y,p2.lo_x,p2.lo_y);
      double d4 = dist(p1.hi_x,p1.hi_y,p2.hi_x,p2.hi_y);
      double dmin = mymin(d1,mymin(d2,mymin(d3,d4)));
      double dmax = mymax(d1,mymax(d2,mymax(d3,d4)));

      double da = dist(p1.lo_x,p1.lo_y,p1.hi_x,p1.hi_y);
      double db = dist(p1.lo_x,p1.lo_y,p1.hi_x,p1.hi_y);


      if (dmin<1) dmin = 1;
      return ((da+db)/2)/dmax;
      //return ((da+db)/2)/dmax;
    }

  double Coherence(int explain=0)
    {
      double d1 = dist(p1.lo_x,p1.lo_y,p2.lo_x,p2.lo_y);
      double d2 = dist(p1.lo_x,p1.lo_y,p2.hi_x,p2.hi_y);
      double d3 = dist(p1.hi_x,p1.hi_y,p2.lo_x,p2.lo_y);
      double d4 = dist(p1.hi_x,p1.hi_y,p2.hi_x,p2.hi_y);
      double dmin = mymin(d1,mymin(d2,mymin(d3,d4)));
      double dmax = mymax(d1,mymax(d2,mymax(d3,d4)));
      //if (dmin>=10)
      //{
      //  printf("D'oh! out by %g (%g)\n", dmin, dmax);
      //}
      //return (dmin<10);
	//return 1;
      
#ifdef CAN_EXPLAIN
      if (explain)
	{
	  printf("%g %g %g %g %g %g\n", d1, d2, d3, d4, dmin, dmax);
	}
#endif

      if (dmin<1) dmin = 1;
      return dmax/dmin;
    }

  void Reorder()
    {
      /*
	Find the point of intersection
       */


      // put the patches in a canonical order that is
      // invariant to scale and rotation

      // p1 and p3 are as different as possible
      /*

      OrientedPatch tmp;
      double mtmp;

      double m12 = fabs(p1.dx*p2.dx+p1.dy*p2.dy);
      double m13 = fabs(p1.dx*p3.dx+p1.dy*p3.dy);
      double m23 = fabs(p2.dx*p3.dx+p2.dy*p3.dy);

      if (m12<m13)
	{
	  // swap role of 2 with 3
	  tmp = p3;
	  p3 = p2;
	  p2 = tmp;
	  mtmp = m13;
	  m13 = m12;
	  m12 = mtmp;
	}
      // now, m13<m12

      if (m23<m13)
	{
	  // swap role of 1 with 2
	  tmp = p1;
	  p1 = p2;
	  p2 = tmp;
	  mtmp = m23;
	  m23 = m13;
	  m13 = mtmp;
	}
      // now, m13<m12 and m13<m23

      // so p1 and p3 are far, p2 is in between
      double a1 = atan2(p1.y,p1.x);
      double a2 = atan2(p2.y,p2.x);
      double del = a1-a2;
      if (del>M_PI/2)  del = -M_PI+del;
      if (del<-M_PI/2) del = M_PI+del;
      if (del<0)
	{
	  // swap role of 1 with 3
	  tmp = p1;
	  p1 = p3;
	  p3 = tmp;
	  mtmp = m23;
	  m23 = m13;
	  m13 = mtmp;
	}

      t1 = atan2(p1.dy,p1.dx);
      t2 = atan2(p2.dy,p2.dx);
      t3 = atan2(p3.dy,p3.dx);
      double x1 = (p1.x+p2.x+p3.x)/3;
      double y1 = (p1.y+p2.y+p3.y)/3;
      ss = (dist(x1,y1,p1.x,p1.y)+dist(x1,y1,p2.x,p2.y)+
	    dist(x1,y1,p3.x,p3.y))/3;
      */
    }

  void Show(YARPImageOf<YarpPixelFloat>& dest, float val)
    {
      YarpPixelBGR mark(255,255,255);
      AddDeltaCrossHair(dest,(int)(p1.x+0.5),(int)(p1.y+0.5),0,val);
      AddDeltaCrossHair(dest,(int)(p2.x+0.5),(int)(p2.y+0.5),0,val);
      //AddDeltaCrossHair(dest,(int)p3.x,(int)p3.y,1,val);
    }

  void SetAppearance(YARPImageOf<YarpPixelBGR>& src)
    {
      double x1 = (p1.x+p2.x)/2;
      double y1 = (p1.y+p2.y)/2;
      YarpPixelBGR& pix1 = src.SafePixel((int)x1,(int)y1);
      YarpPixelBGR& pix2 = src.SafePixel((int)x1+1,(int)y1);
      //YarpPixelBGR& pix3 = src.SafePixel((int)x1,(int)y1+1);
      r = (int)((pix1.r+pix2.r)/2.0);
      g = (int)((pix1.g+pix2.g)/2.0);
      b = (int)((pix1.b+pix2.b)/2.0);
      {
	YarpPixelBGR& pix1 = src.SafePixel((int)((3*p1.x+x1)/4),(int)((3*p1.y+y1)/4));
	YarpPixelBGR& pix2 = src.SafePixel((int)((3*p2.x+x1)/4),(int)((3*p2.y+y1)/4));
	//YarpPixelBGR& pix1 = src.SafePixel((int)((p1.x+x1)/2),(int)((p1.y+y1)/2));
	//YarpPixelBGR& pix2 = src.SafePixel((int)((p2.x+x1)/2),(int)((p2.y+y1)/2));
	r1 = pix1.r;
	g1 = pix1.g;
	b1 = pix1.b;
	r2 = pix2.r;
	g2 = pix2.g;
	b2 = pix2.b;
      }
    }

  lint GetHash()
    {
      if (need_hash)
	{
	  hashnum = BaseGetHash();
	  need_hash = 0;
	}
      return hashnum;
    }

  double xint, yint, qint;
  double dint1, dint2, mint, mint2;
  double xb0, yb0, xb1, yb1;
  double xb2, yb2;
  
  lint BaseGetHash()
    {
      GeometryTool gt;
      qint = gt.Intersect(p1.x,p1.y,p1.dx,p1.dy,
			  p2.x,p2.y,p2.dx,p2.dy,
			  xint, yint);
      dint1 = dist(p1.x,p1.y,xint,yint);
      dint2 = dist(p2.x,p2.y,xint,yint);
      mint = (p1.x-xint)*(p2.x-xint)+
	(p1.y-yint)*(p2.y-yint);
      mint2 = mint/mymax(dint1*dint2,0.000001);
      // mint2 is cos(theta) between p1, p2 and intersecting point
      if (mint2>0.95)
	{
	  xint = (p1.x+p2.x)/2;
	  yint = (p1.y+p2.y)/2;
	  //  xint = p1.lo_x;
	  // yint = p1.lo_y;
	  dint1 = dist(p1.x,p1.y,xint,yint);
	  dint2 = dist(p2.x,p2.y,xint,yint);
	  mint = (p1.x-xint)*(p2.x-xint)+
	    (p1.y-yint)*(p2.y-yint);
	  mint2 = mint/mymax(dint1*dint2,0.000001);
	}

      double vx0 = p1.x-xint;
      double vy0 = p1.y-yint;
      double vd0 = mymax(dist(vx0,vy0,0.0,0.0),0.000001);
      vx0 /= vd0;
      vy0 /= vd0;

      double vx1 = p2.x-xint;
      double vy1 = p2.y-yint;
      double vd1 = mymax(dist(vx1,vy1,0.0,0.0),0.000001);
      vx1 /= vd1;
      vy1 /= vd1;

      //if (vx0*vx1+vy0*vy1<0)
      //{
      //  vx1 = -1;
      //  vy1 = -1;
      //}
      
      double vx2 = vx0+vx1;
      double vy2 = vy0+vy1;
      double vd2 = mymax(dist(vx2,vy2,0.0,0.0),0.000001);
      vx2 /= vd2;
      vy2 /= vd2;

      xb0 = vx2;
      yb0 = vy2;
      
      xb1 = -yb0;
      yb1 = xb0;

      xb2 = (x0-xint)*xb0+(y0-yint)*yb0;
      yb2 = (x0-xint)*xb1+(y0-yint)*yb1;

      lint h = 0;
      //int k = 7;
      //int k2 = 5;
      //int k3 = 4;
      int k = 6;
      int k2 = 4;
      int k3 = 4; 
      int k4 = 8;
      int k5 = 5;
      int k6 = 5;
      int k7 = 5;

      lint h1 = (int)((mint2+1)*k/2+0.5);
      if (h1>=k) h1 = k-1;
      if (h1<0)  h1 = 0;
      h = h*k + h1;

      lint h2 = (int)(k2*fabs(dint1-dint2)/mymax(dint1,mymax(0.000001,dint2)));
      h = h*k2 + h2;

      lint h3 = 0, h4 = 0;
#ifdef COHERENT_USER
      double coh = Coherence();
      h3 = (int)(mymin(coh,(double)(k3-1.0)));
      h = h*k3 + h3;

      double ws = WideScale();
      h4 = (int)(mymin(ws*k4*2,(double)(k4-1.0)));
      h = h*k4 + h4;
#endif

#ifdef CAN_EXPLAIN
      if (need_hash==0)
	{
	  printf("%ld %ld %ld %ld\n", h, h1, h2, h3);
	  printf("Coh %g\n", coh);
	  Coherence(1);
	}
#endif

      lint h5 = 0, h6 = 0;
#ifdef ORIENTATION_SPECIFIC
      h5 = (int)(((1+fabs(p1.dx)*((p1.dx*p1.dy<0)?-1:1))/2)*k5);
      h6 = (int)(((1+fabs(p2.dx)*((p2.dx*p2.dy<0)?-1:1))/2)*k6);
      h = h*k5 + h5;
      h = h*k6 + h6;
#endif      

#ifdef SCALE_SPECIFIC
      {
      double d1 = dist(p1.lo_x,p1.lo_y,p2.lo_x,p2.lo_y);
      double d2 = dist(p1.lo_x,p1.lo_y,p2.hi_x,p2.hi_y);
      double d3 = dist(p1.hi_x,p1.hi_y,p2.lo_x,p2.lo_y);
      double d4 = dist(p1.hi_x,p1.hi_y,p2.hi_x,p2.hi_y);
      double dmin = mymin(d1,mymin(d2,mymin(d3,d4)));
      double dmax = mymax(d1,mymax(d2,mymax(d3,d4)));
      int h7 = (int)(dmax/4);
      if (h7>=k7) h7 = k7-1;
      h = h*k7+h7;
      }
#endif

      //printf("Angular info %ld %ld\n", h1, h2);

#ifdef COLOR_SPECIFIC
      double lum = (r+g+b+5)/3.0;
      //printf("%ld %ld\n", __LINE__, idx);
      //idx = idx*100 + (lint)(10*r/lum);
      //idx = idx*100 + (lint)(10*g/lum);
      //idx = idx*100 + (lint)(10*b/lum);

      h = h*2 + (lint)(r>(g));
      h = h*2 + (lint)(r>(b));
      h = h*2 + (lint)(g>(b));
      h = h*2 + (lint)(r>g+50);
      h = h*2 + (lint)(r>b+50);
      h = h*2 + (lint)(g>b+50);

      h = h*2 + (lint)(r1>(g));
      h = h*2 + (lint)(r1>(b));
      h = h*2 + (lint)(g1>(b));

      h = h*2 + (lint)(r2>(g));
      h = h*2 + (lint)(r2>(b));
      h = h*2 + (lint)(g2>(b));

      h = h*2 + (lint)(r2>(g1));
      h = h*2 + (lint)(r2>(b1));
      h = h*2 + (lint)(g2>(b1));

      h = h*2 + (lint)(r1>(g1));
      h = h*2 + (lint)(r1>(b1));
      h = h*2 + (lint)(g1>(b1));

      h = h*2 + (lint)(r2>(g2));
      h = h*2 + (lint)(r2>(b2));
      h = h*2 + (lint)(g2>(b2));

#ifdef SUPER_SENSITIVE

      h = h*2 + (lint)(r1>(g+50));
      h = h*2 + (lint)(r1>(b+50));
      h = h*2 + (lint)(g1>(b+50));

      h = h*2 + (lint)(r2>(g+50));
      h = h*2 + (lint)(r2>(b+50));
      h = h*2 + (lint)(g2>(b+50));

      h = h*2 + (lint)(r2>(g1+50));
      h = h*2 + (lint)(r2>(b1+50));
      h = h*2 + (lint)(g2>(b1+50));

      h = h*2 + (lint)(r1>(g1+50));
      h = h*2 + (lint)(r1>(b1+50));
      h = h*2 + (lint)(g1>(b1+50));

      h = h*2 + (lint)(r2>(g2+50));
      h = h*2 + (lint)(r2>(b2+50));
      h = h*2 + (lint)(g2>(b2+50));
#endif

      //h = h*100 + (lint)(10*r/lum);
      //h = h*100 + (lint)(10*g/lum);
      //h = h*100 + (lint)(10*b/lum);

      //idx = idx*100 + (lint)(10*g2/lum);
      //printf("%ld %ld\n", __LINE__, idx);
      //idx = idx*100 + (lint)(lum/20);
#endif

      //printf("ID is %lu // %lu %lu %lu %lu\n", h, h1, h2, h3, h4);

      return h;
      

      /*
      double m12 = fabs(p1.dx*p2.dx+p1.dy*p2.dy);
      double m13 = fabs(p1.dx*p3.dx+p1.dy*p3.dy);
      double m23 = fabs(p2.dx*p3.dx+p2.dy*p3.dy);
      double d12 = dist(p1.x,p1.y,p2.x,p2.y);
      double d13 = dist(p1.x,p1.y,p3.x,p3.y);
      double d23 = dist(p2.x,p2.y,p3.x,p3.y);
      double dmax = mymax(d12,mymax(d13,d23));
      int k = 10;
      //printf("%ld: %g %g %g %g %g %g %g\n", __LINE__, m12, m13, m23, d12, d13, d23, dmax);
      lint idx = (lint)(m12*k)*k*k+(lint)(m13*k)*k+(lint)(m23*k);
      //printf("%ld %ld\n", __LINE__, idx);
      lint idx2 = 
	(lint)(d12/dmax*k)*k*k+(lint)(d13/dmax*k)*k+(lint)(d23/dmax*k);
      //printf("%ld %ld %ld\n", __LINE__, idx, idx2);
      idx = idx+idx2*k*k*k;
      //printf("%ld %ld\n", __LINE__, idx);
#ifdef SCALE_SPECIFIC
      idx = idx*100 + (lint)(dmax/10);
#endif
      //printf("%ld %ld\n", __LINE__, idx);

#ifdef AREA_SPECIFIC
      //printf("areas %g %g %g\n", p1.area, p2.area, p3.area);
      double a1 = mymax(p1.area,0.01);
      double a2 = mymax(p2.area,0.01);
      double a3 = mymax(p3.area,0.01);
      {
	int nx = (int)(10*a1/a3);
	nx = mymin(99,nx);
	idx = idx*100 + nx;
      }
      {
	int nx = (int)(10*a2/a3);
	nx = mymin(99,nx);
	idx = idx*100 + nx;
      }
#endif

#ifdef COLOR_SPECIFIC
      double lum = (r+g+b+5)/3.0;
      //printf("%ld %ld\n", __LINE__, idx);
      //idx = idx*100 + (lint)(10*r/lum);
      //idx = idx*100 + (lint)(10*g/lum);
      //idx = idx*100 + (lint)(10*b/lum);
      idx = idx*2 + (lint)(r>(g+10));
      idx = idx*2 + (lint)(r>(b+10));
      idx = idx*2 + (lint)(g>(b+10));
      idx = idx*2 + (lint)(r>(g+50));
      idx = idx*2 + (lint)(r>(b+50));
      idx = idx*2 + (lint)(g>(b+50));

      //idx = idx*100 + (lint)(10*g2/lum);
      //printf("%ld %ld\n", __LINE__, idx);
      //idx = idx*100 + (lint)(lum/20);
#endif
      //printf("%ld %ld\n", __LINE__, idx);
      //exit(0);
      return idx;
      */
    }

  void Map(Geometry& ref,double& xe, double& ye, double& re, 
	   double& power, double& escale)
    {
      GetHash();
      ref.GetHash();

      double pow = 25;
      //double pow = 10;
      escale = 1;
      if (0) //fabs(mint2)>0.95)
	{
	  // Basically parallel
	  xe = (p1.x+p2.x)/2.0;
	  ye = (p1.y+p2.y)/2.0;
	  re = pow;
	  power = mymax(p1.area,p2.area); //PFHIT
	  
	  //xe = 0; ye = 0; re = 0; power = 0;
	}
      else
	{
	  // Have a workable corner
	  double dme = (dint1+dint2)/2;
	  double dref = (ref.dint1+ref.dint2)/2;
	  double ss = dme/mymax(dref,0.00001);

	  xe = xint+(ref.xb2*xb0+ref.yb2*xb1)*ss;
	  ye = yint+(ref.xb2*yb0+ref.yb2*yb1)*ss;
	  re = pow;
	  //power = 1;
	  //power = mymin(p1.area,p2.area)/ss;
	  power = mymin(p1.GetLen(),p2.GetLen())/ss;
	  //power = mymin(100.0,power);
	  escale = ss;
#ifdef ORIENTATION_SPECIFIC
#if 0
	  double r1 = fabs(p1.dx*ref.p1.dx+p1.dy*ref.p1.dy);
	  double r2 = fabs(p2.dx*ref.p2.dx+p2.dy*ref.p2.dy);
	  if (r2<0.9 || r1<0.9)
	    {
	      power = 0;
	    }
#endif
#endif
	}


      /*
      double x1 = (p1.x+p2.x+p3.x)/3;
      double y1 = (p1.y+p2.y+p3.y)/3;
      double x2 = (ref.p1.x+ref.p2.x+ref.p3.x)/3;
      double y2 = (ref.p1.y+ref.p2.y+ref.p3.y)/3;
      double m13 = fabs(p1.dx*p3.dx+p1.dy*p3.dy);
      // Map (x1,y1) -> (x2,y2) in position

      double s1 = ss;

      double s2 = ref.ss;

      if (fabs(s2)<0.001) { s2 = 1; }

#if 0
      //double m12 = fabs(p1.dx*p2.dx+p1.dy*p2.dy);
      //double m13 = fabs(p1.dx*p3.dx+p1.dy*p3.dy);
      //double m23 = fabs(p2.dx*p3.dx+p2.dy*p3.dy);

      //double r12 = fabs(ref.p1.dx*p2.dx+ref.p1.dy*p2.dy);
      //double r13 = fabs(ref.p1.dx*p3.dx+ref.p1.dy*p3.dy);
      //double r23 = fabs(ref.p2.dx*p3.dx+ref.p2.dy*p3.dy);

      double t1 = atan2(p1.y,p1.x);
      double t2 = atan2(p2.y,p2.x);
      double t3 = atan2(p3.y,p3.x);
      double tx1 = cos(t1*2)+cos(t2*2)+cos(t3*2);
      double ty1 = sin(t1*2)+sin(t2*2)+sin(t3*2);
      //double ang1 = atan2(ty1,tx1)/2;

      double rt1 = atan2(ref.p1.y,ref.p1.x);
      double rt2 = atan2(ref.p2.y,ref.p2.x);
      double rt3 = atan2(ref.p3.y,ref.p3.x);
      double rtx1 = cos(rt1*2)+cos(rt2*2)+cos(rt3*2);
      double rty1 = sin(rt1*2)+sin(rt2*2)+sin(rt3*2);
      //double ang2 = atan2(rty1,rtx1)/2;

      double cosang = tx1*rtx1+ty1*rty1;

      double dang = 0;

      double xx = ref.x0;
      double yy = ref.y0;
      xx -= x2;
      yy -= y2;
      double nx = xx*cos(dang)-yy*sin(dang);
      double ny = xx*sin(dang)+yy*cos(dang);
      nx += x1;
      ny += y1;

      xe = nx;
      ye = ny;
#endif

      double dt1 = t1-ref.t1;
      double dt2 = t2-ref.t2;
      double dt3 = t3-ref.t3;
      double dt = dt1;

      double g0 = AngularCost(dt,dt2,dt3);
      double g1 = AngularCost(dt+M_PI,dt2,dt3);
      if (g1<g0)
	{
	  dt = dt+M_PI;
	  g0 = g1;
	}

      xe = x1;
      ye = y1;
      re = (int)(dist(x2,y2,(double)ref.x0,(double)ref.y0)*s1/s2);

      //s1 = s2 = 1;
      //dt = 0;

      //dt = dt;
      double dxe = (ref.x0-x2)*s1/s2;
      double dye = (ref.y0-y2)*s1/s2;
      double dxe2 = dxe*cos(dt)-dye*sin(dt);
      double dye2 = dxe*sin(dt)+dye*cos(dt);

      xe = x1+dxe2;
      ye = y1+dye2;

      //xe = ref.x0;
      //ye = ref.y0;

      //re = 20;
      re = 15;
      //power = 1/(g1+1);
      //power = 1/(m13+0.5);
      power = 1;

      //xe = x1;
      //ye = y1;
      */
    }
};



typedef hash_map<lint,Geometry,hash<lint>,equal_to<lint> > hash_ig;


class Unit
{
public:
  int x1, y1;
  int x2, y2;
  double scale;

  void Show(YARPImageOf<YarpPixelFloat>& dest, float val)
    {
      YarpPixelBGR mark(255,255,255);
      AddDeltaCrossHair(dest,(int)x1,(int)y1,0,val);
      AddDeltaCrossHair(dest,(int)x2,(int)y2,0,val);
      //AddDeltaCrossHair(dest,(int)p3.x,(int)p3.y,1,val);
    }
};

typedef hash_map<lint,Unit,hash<lint>,equal_to<lint> > hash_iu;

class Appearance
{
public:
  double mu, dev;
  double match_accum, match_accum2;
  longint match_ct;
  hash_ii hist;
  longint total;

  Appearance()
    {
      total = 0;
      mu = dev = 0;
      match_accum = 0;
      match_accum2 = 0;
      match_ct = 0;
    }

  void Reset()
    {
      hash_ii blank;
      hist =  blank;
      total = 0;
      mu = dev = 0;
      match_accum = 0;
      match_accum2 = 0;
      match_ct = 0;
    }

  longint GetID(const YarpPixelBGR& pix, const YarpPixelFloat& pix_m)
    {
      float c1, c2, c3;
      RGBtoCYL(pix.r,pix.g,pix.b,c1,c2,c3);
      int k1 = 6, k2 = 6, k3 = 3, k4 = 3;
      int idx1 = (int)(((c1+1)/2.0)*k1);
      int idx2 = (int)(((c2+1)/2.0)*k2);
      int idx3 = (int)(c3*k3);
      int idx4 = 0; //(int)(pix_m*k4);
      longint idx = ((idx1*k2+idx2)*k3+idx3)*k4+idx4;
      return idx;
    }

  void Add(const YarpPixelBGR& pix, const YarpPixelFloat& pix_m)
    {
      longint id = GetID(pix,pix_m);
      hash_ii::iterator it = hist.find(id);
      if (it==hist.end())
	{
	  hist[id] = 1;
	}
      else
	{
	  (*it).second++;
	}
      total++;
    }

  void Add(YARPImageOf<YarpPixelBGR>& src, 
	   YARPImageOf<YarpPixelFloat>& src_m,
	   int x0, int y0, double rad)
    {
      IMGFOR(src,x,y)
	{
	  double d = dist(x,y,x0,y0);
	  if (d<=rad)
	    {
	      Add(src(x,y),src_m(x,y));
	    }
	}
    }

  double Match(YARPImageOf<YarpPixelBGR>& src, 
	       YARPImageOf<YarpPixelFloat>& src_m,
	       int x0, int y0, double rad)
    {
      double accum = 0;
      longint ct = 0;
      IMGFOR(src,x,y)
	{
	  double d = dist(x,y,x0,y0);
	  if (d<=rad)
	    {
	      accum += Get(src(x,y),src_m(x,y));
	      ct++;
	    }
	}
      if (total>0)
	{
	  accum /= total;
	}
      if (ct>0)
	{
	  accum /= ct;
	}
      //printf("canon match %ld %ld %g\n", total, ct, accum);
      return accum;
    }

  double EvaluateMatch(double v)
    {
      v = v - mu;
      if (fabs(dev)>0.0001)
	{
	  v /= dev;
	}
      return v;
    }

  void NoteMatch(double v)
    {
      match_accum += v;
      match_accum2 += v*v;
      match_ct++;
      mu = match_accum/match_ct;
      dev = sqrt(fabs(match_accum2/match_ct-mu*mu));
    }

  longint Get()
    {
      return total;
    }

  longint Get(const YarpPixelBGR& pix, const YarpPixelFloat& pix_m)
    {
      longint result = 0;
      longint id = GetID(pix,pix_m);
      hash_ii::iterator it = hist.find(id);
      if (it!=hist.end())
	{
	  result = hist[id];
	}
      return result;
    }

  void Write(ostream& os)
    {
      os << total << endl;
      os << mu << " " << dev << endl;
      for (hash_ii::iterator it=hist.begin(); it!=hist.end(); it++)
	{
	  os << (*it).first << " " << (*it).second << endl;
	}
    }

  void Read(istream& is)
    {
      Reset();
      is >> total;
      is >> mu >> dev;
      while (!(is.eof()||is.bad()))
	{
	  longint id = -1, val = -1;
	  is >> id >> val;
	  if (!is.eof())
	    {
	      if (id>=0 && val>=0)
		{
		  hist[id] = val;
		  //printf("canon %ld %ld\n", id, val);
		}
	    }
	}
    }
};

#define MAX_SNAPS 10

class GeometryOracle
{
public:
  hash_ii counts;
  hash_ig ig;
  int ox, oy, or;
  lint oid;
  double op;
  double oscale;
  double oarea;

  YARPImageOf<YarpPixelBGR> images[MAX_SNAPS];
  YARPImageOf<YarpPixelMono> masks[MAX_SNAPS];
  int snap_ct;

  Appearance appear;


  GeometryOracle()
    {
      snap_ct = 0;
    }

  void AddSnap(YARPImageOf<YarpPixelBGR>& image,
	       YARPImageOf<YarpPixelMono>& mask)
    {
      if (snap_ct<MAX_SNAPS)
	{
	  images[snap_ct].PeerCopy(image);
	  masks[snap_ct].PeerCopy(mask);
	  snap_ct++;
	}
    }

  double CompareSnap(YARPImageOf<YarpPixelBGR>& image,
		     YARPImageOf<YarpPixelMono>& mask)
    {
      // Online addition of data.
      YARPVisualSearch seek1;
      seek1.Add(image,mask);
      double best_cmp = -1;
      int best_idx = -1;
      for (int i=0; i<snap_ct; i++)
	{
	  YARPVisualSearch seek2;
	  seek2.Add(images[i],masks[i]);
	  double cmp = seek1.Compare(seek2);
	  if (cmp>best_cmp)
	    {
	      best_cmp = cmp;
	      best_idx = i;
	    }
	}
      return best_cmp;
    }


  void Add(Geometry& geo)
    {
      lint id = geo.GetHash();
      if (counts.find(id)==counts.end())
	{
	  counts[id] = 0;
	}
     counts[id]++;
     ig[id] = geo;
     /*
     if (ig.find(id)==ig.end())
       {
	 ig[id] = geo;
       }
     else
       {
	 ig[id].Add(geo);
       }
     */
    }
  
  int Match(Geometry& geo)
    {
      lint id = geo.GetHash();
      oid = id;
      int r = (counts.find(id)!=counts.end());
      //printf("Check for %ld: %d  (ref: %ld)\n", id, r, (*(counts.begin())).first);
      if (r)
	{
	  r = counts[id];
	  Geometry& ref = ig[id];
	  double x, y, rad, power,escale;
	  geo.Map(ref,x,y,rad,power,escale);
	  ox = (int)x;
	  oy = (int)y;
	  or = (int)rad;
	  op = power; // r for counts PFHIT
	  oscale = escale;
	  oarea = ref.ref_area;
	}
      return r;
    }

  void WriteAppearance(ostream& os)
    {
      appear.Write(os);
    }

  void Write(ostream& os)
    {
      for (hash_ig::iterator it = ig.begin(); it!=ig.end(); it++)
	{
	  lint id = (*it).first;
	  Geometry& geo = (*it).second;
	  //OrientedPatch p1, p2, p3;
	  //int x0, y0;
	  //float r, g, b;
	  int ct = counts[id];
	  if (ct>=MIN_CT)
	    {
	      char buf[1000];
	      sprintf(buf,"%d %d %d %g %d %d %d %d %d %d %d %d %d ", ct, 
		      geo.x0, geo.y0, geo.ref_area, geo.r, geo.g, geo.b,
		      geo.r1, geo.g1, geo.b1,
		      geo.r2, geo.g2, geo.b2);
#ifdef SAVE_ID
	      os << geo.GetHash() << " ";
#endif
	      os << buf;
	      geo.p1.Write(os);
	      geo.p2.Write(os);
	      //geo.p3.Write(os);
	      os << endl;
	    }
	}
    }

  void Read(istream& is)
    {
      counts.clear();
      ig.clear();
      lint entries = 0, distinct_entries = 0;
      int countdown = 5;
      longint ct = 1;
      while (!(is.bad()||is.eof()||ct==0))
	{
	  lint id0;
	  int x0, y0;
	  double r, g, b;
	  ct = -1;
#ifdef SAVE_ID
	  is >> id0;
#endif
	  Geometry geo;
	  is >> ct;
	  if (ct>=1)
	    {
	      is >> x0 >> y0 >> geo.ref_area >> r >> g >> b;
	      is >> geo.r1 >> geo.g1 >> geo.b1;
	      is >> geo.r2 >> geo.g2 >> geo.b2;
	      //is >> geo.r3 >> geo.g3 >> geo.b3;
	      entries++;
	      geo.r = (int)r;
	      geo.g = (int)g;
	      geo.b = (int)b;
	      geo.x0 = x0;
	      geo.y0 = y0;
	      geo.p1.Read(is);
	      geo.p2.Read(is);
	      //geo.p3.Read(is);
	      geo.Reorder();
	      lint id = geo.GetHash();
#if 0
	      if (id == 258865147 || 
		  (geo.r==150 && geo.g==152 && geo.b==78))
		{
		  printf("GOT IT!\n");	
		  fflush(stdout);
		  cout << id << endl;
		  cout << geo.r << " " << geo.g << " " << geo.b << endl;
		  geo.p1.Write(cout);
		  cout << " // ";
		  geo.p2.Write(cout);
		  cout << endl;
		  geo.Explain();
		  fflush(stdout);
		  exit(1);
		}
#endif

#ifdef SAVE_ID
	      if (id!=id0)
		{
		  printf("Hash %ld versus %ld\n", id, id0);
		}
	      assert(id==id0);
#endif
	      if (ig.find(id)==ig.end())
		{
		  distinct_entries++;
		}
	      ig[id] = geo;
	      counts[id] = ct;
#if 0
	      if (countdown>=0)
		{
		  printf(">>> %d %ld (%g,%g,%g%g) (%g,%g,%g,%g) (%g,%g,%g,%g) (%d,%d) (%g,%g,%g)\n", countdown, id,
			 geo.p1.x, geo.p1.y, geo.p1.dx, geo.p1.dy,
			 geo.p2.x, geo.p2.y, geo.p2.dx, geo.p2.dy,
			 geo.p3.x, geo.p3.y, geo.p3.dx, geo.p3.dy,
			 x0, y0,
			 r, g, b);
		  countdown--;
		}
#endif
	    }
	}
      printf("Got %ld entries, stored as %ld entries\n",
	     entries, distinct_entries);
      ox = oy = or = 0;
    }

  void Compare(GeometryOracle& alt)
    {
      lint overlap = 0;
      lint personal = 0;
      lint total = 0;
      int ok = 0;
      hash_ig new_ig;
      hash_ii new_counts;
      for (hash_ig::iterator it = ig.begin(); it!=ig.end(); it++)
	{
	  ok = 0;
	  total++;
	  lint id = (*it).first;
	  int over = 0;
	  if (alt.counts.find(id)!=alt.counts.end())
	    {
	      if (counts[id]<alt.counts[id]*2)
		{
		  overlap++;
		  over = 1;
		}
	      //printf("Overlap %d (%d,%d)\n", id,
	      //counts[id], alt.counts[id]);
	    }
	  if (!over)
	    {
	      if (counts[id]>=1)
		{
		  //printf("Personal %d : %d\n", id,
		  // counts[id]);
		  personal++;
		  ok = 1;
		}
	    }
	  if (ok)
	    {
	      new_counts[id] = counts[id];
	      new_ig[id] = ig[id];
	    }
	}
      ig = new_ig;
      counts = new_counts;
      printf("%ld total, %ld overlaps, %ld personal\n", total, overlap, personal);
    }

  long int GetCount()
    {
      long int result = 0;
      for (hash_ii::iterator it = counts.begin(); it!=counts.end(); it++)
	{
	  result += (*it).second;
	}
      return result;
    }


  void Spread(lint id, int ct, YARPImageOf<YarpPixelFloat>& strength)
    {
      Geometry& geo = ig[id];
      // DO SOMETHING HERE
    }

};


#define MAX_ORACLE 20

class ProtoImage
{
public:
  YARPImageOf<YarpPixelBGR> image;
  YARPImageOf<YarpPixelMono> mask;
  int x0, y0;

  void Set(YARPImageOf<YarpPixelBGR>& src)
    {
      SatisfySize(src,mask);
      mask.Zero();
      IMGFOR(src,x,y)
	{
	  if (src(x,y).r+src(x,y).g+src(x,y).b>0)
	    {
	      mask(x,y) = 255;
	    }
	}

      YARPBinaryAnalysis ana;
      ana.Apply(mask);
      x0 = (int)ana.GetX();
      y0 = (int)ana.GetY();

      //IntegralImageTool ii;
      //ii.Offset(src,image,+src.GetWidth()/2-(int)(x0),
      //-img.GetHeight()/2+(int)(y0),1);
      image = src;
    }

  void Set(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelMono>& nmask)
    {
      image = src;
      mask = nmask;
      YARPBinaryAnalysis ana;
      ana.Apply(mask);
      x0 = (int)ana.GetX();
      y0 = (int)ana.GetY();

    }
  
  void Set(const char *fname)
    {
      char buf[256];
      YARPImageOf<YarpPixelBGR> src;
      YARPImageOf<YarpPixelMono> msk;
      YARPImageFile::Read(fname,src);
      strncpy(buf,fname,sizeof(buf));
      int index = strlen(buf)-2;
      if (index>=0)
	{
	  buf[index] = 'g';
	}
      YARPImageFile::Read(fname,msk);
      Set(src,msk);
    }

  void Mark(YARPImageOf<YarpPixelBGR>& src,
	    YARPImageOf<YarpPixelBGR>& dest,
	    int x, int y)
    {
#ifdef SHOW_SHORTS
      YARPImageOf<YarpPixelBGR> small;
      if (image.GetWidth()>0)
	{
	  small.ScaledCopy(image,16,16);
	  IMGFOR(small,xm,ym)
	    {
	      if (small(xm,ym).r+small(xm,ym).g+small(xm,ym).b>0)
		{
		  dest.SafePixel(xm+x-8,ym+y-8) = small(xm,ym);
		}
	    }
	}
#endif
    }

  void Brand(YARPImageOf<YarpPixelBGR>& src,
	     YARPImageOf<YarpPixelBGR>& dest,
	     int x, int y,int big=0)
    {
      YARPImageOf<YarpPixelBGR> small;
      if (image.GetWidth()>0)
	{
#ifdef SHOW_SHORTS
	  int k = 32;
	  //int k = 16;
	  if (big) k = 32;
	  small.ScaledCopy(image,k,k);
	  IMGFOR(small,xm,ym)
	    {
	      dest.SafePixel(xm,ym) = small(xm,ym);
	    }
#endif
#if 0
	  double theta = FindAngle(src,image,mask,dest,
				   x, y, x0, y0);
	  printf("*** theta is %g\n", theta);
#endif
	}
      //YarpPixelBGR pix(255,128,255);
      //AddCrossHair(dest,pix,(int)(x+10*cos(theta)),(y+10*sin(theta)),5);
      //AddCrossHair(dest,pix,(int)(x-10*cos(theta)),(y-10*sin(theta)),5);
    }
};

ProtoImage oracle_proto[MAX_ORACLE];
GeometryOracle oracle_bank[MAX_ORACLE];
GeometryOracle oracle_bank2[MAX_ORACLE];
double oracle_scale[MAX_ORACLE];
double oracle_mean[MAX_ORACLE];
double oracle_std[MAX_ORACLE];
int oracle_count = 0;
GeometryOracle *p_oracle = (&(oracle_bank[0]));
#define oracle (*p_oracle)

void RedirectTrainer(GeometryOracle& ref)
{
  p_oracle = &ref;
}

void AddOracle(int x, const char *fname, const char *pname, const char *muname)
{
  assert(oracle_count<MAX_ORACLE);
  printf("Loading from %s, %s\n", fname, pname);
  ifstream is(fname);
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  oracle_bank[oracle_count].Read(is);
  oracle_bank[oracle_count].appear.Read(is);
  printf("Final size is %ld\n", oracle_bank[oracle_count].counts.size());
  //printf("First element is %ld\n", (*(oracle_bank[oracle_count].counts.begin())).first);
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  oracle_proto[oracle_count].Set(pname);
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  YARPImageOf<YarpPixelFloat> mean;
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  //YARPImageFile::Read(muname,mean,YARPImageFile::FORMAT_NUMERIC);
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  //oracle_mean[oracle_count] = mean(0,0);
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  //oracle_std[oracle_count] = mean(1,0);
  //printf("__LINE__ %d!\n", __LINE__); fflush(stdout);
  oracle_count++;
}


//void UpdateOracle(int index, YARPImageOf<YarpPixelBGR>& image,
//		  YARPImageOf<YarpPixelFloat>& mask)
//{
//  oracle_bank[index].AddSnap(image,mask);
//  RedirectTrainer(oracle_bank[index]);
//}

double GetRandom()
{
  return YARPRandom::ranf();
}


class Extender
{
public:

  Extender()
    {
      p_label = NULL;
    }

  float total_len;

  void Reset()
    {
      total_len = 0;
    }

  int  Move(YARPImageOf<YarpPixelBGR>& src,
	    YARPImageOf<YarpPixelBGR>& dest,
	    YARPImageOf<YarpPixelBGR>& code,
	    YARPImageOf<YarpPixelFloat>& src_x,
	    YARPImageOf<YarpPixelFloat>& src_y,
	    YARPImageOf<YarpPixelFloat>& src_m,
	    int ox, int oy, int& nx, int& ny, float dx, float dy, int del)
    {
      nx = ox;
      ny = oy;
      // scan around from ox, oy to find somewhere else going in the
      // same direction
      
      int done = 0;
      int sx = (int)(ox+dx*3.5+0.5);
      int sy = (int)(oy+dy*3.5+0.5);
      if (src_m.IsSafePixel(sx,sy))
	{
	  if (src_m(sx,sy)>0.01)
	    {
	      if (fabs(src_x(sx,sy)*dx+src_y(sx,sy)*dy)>0.85)
		{
		  nx = sx;
		  ny = sy;
		  done = 1;
		}
	    }
	}
      if (!done)
	{
	  for (int ii=sx-2; ii<=sx+2 && !done; ii++)
	    {
	      for (int jj=sy-2; jj<=sy+2 && !done; jj++)
		{
		  if (src_m.IsSafePixel(ii,jj))
		    {
		      if (src_m(ii,jj)>0.01)
			{
			  if (fabs(src_x(ii,jj)*dx+src_y(ii,jj)*dy)>0.85)
			    {
			      nx = ii;
			      ny = jj;
			      done = 1;
			    }
			}
		    }
		}
	    }
	}
      return done;
    }

  void Extend(YARPImageOf<YarpPixelBGR>& src,
	      YARPImageOf<YarpPixelBGR>& dest,
	      YARPImageOf<YarpPixelBGR>& code,
	      YARPImageOf<YarpPixelFloat>& src_x,
	      YARPImageOf<YarpPixelFloat>& src_y,
	      YARPImageOf<YarpPixelFloat>& src_m,
	      YARPImageOf<YarpPixelFloat>& dest_x,
	      YARPImageOf<YarpPixelFloat>& dest_y,
	      YARPImageOf<YarpPixelFloat>& dest_m,
	      int ox, int oy, int sense, int show)
    {
      float m = src_m(ox,oy);
      int nx = ox, ny = oy;
      if (m>0.01)
	{
	  float dx = src_x(ox,oy)*sense;
	  float dy = src_y(ox,oy)*sense;
	  for (int i=0; i<100; i++)
	    {
	      int cx = nx, cy = ny;
	      int ok = 
		Move(src,dest,code,src_x,src_y,src_m,nx,ny,cx,cy,dx,dy,3);
	      if (!ok) break;
	      nx = cx;
	      ny = cy;
	      if (show)
		{
		  dest(nx,ny) = code(ox,oy);
		}
	    }
	  float len = dist(ox,oy,nx,ny);
	  total_len += len;
	}
    }
  double GetLength() { return total_len; }


  class MyLabel : public YARPImageLabel
  {
  public:
    hash_ii counts;
    hash_ip patches;
    int at_id;

    YARPImageOf<YarpPixelFloat>& src_x;
    YARPImageOf<YarpPixelFloat>& src_y;
    YARPImageOf<YarpPixelFloat>& src_m;

    int interest;

    MyLabel(YARPImageOf<YarpPixelFloat>& nsrc_x,
	    YARPImageOf<YarpPixelFloat>& nsrc_y,
	    YARPImageOf<YarpPixelFloat>& nsrc_m) :
      src_x(nsrc_x), src_y(nsrc_y), src_m(nsrc_m)
      {
	interest = 0;
	fixed = 0;
	at_id = -1;
      }

    /*
    void Clone(MyLabel& alt)
      {
	counts = alt.counts;
	patches = alt.patches;
	at_id = alt.at_id;
      }
    */

    double tot_x, tot_y, tot_x2, tot_y2, tot_xy;
    double tot_dx, tot_dy;
    int lo_x, lo_y;
    int hi_x, hi_y;
    float lo, hi;
    int xlike;

    int count2;
    int fixed;

    void Add(OrientedPatch& p1, int ct = 100)
      {
	at_id++;
	counts[at_id] = ct;
	patches[at_id] = p1;
      }

    void Reset()
      {
	at_id = 0;
      }

    void Note(int x, int y, float dx, float dy)
      {
	tot_x += x;
	tot_y += y;
	tot_x2 += x*x;
	tot_y2 += y*y;
	tot_xy += x*y;
	float v = tot_dx*dx+tot_dy*dy;
	if (v>0)
	  {
	    tot_dx += dx;
	    tot_dy += dy;
	  }
	else
	  {
	    tot_dx -= dx;
	    tot_dy -= dy;
	  }
	count2++;
      }

    float fx, fy, fdx, fdy;

    virtual void Notify(int x, int y)
      {
	float dx = src_x(x,y);
	float dy = src_y(x,y);
	Note(x,y,dx,dy);
	float d = fdx*(x-fx)+fdy*(y-fy);
	if (d>0)
	  {
	    if (d>hi)
	      {
		hi_x = x;
		hi_y = y;
		hi = d;
	      }
	  }
	else
	  {
	    d = -d;
	    if (d>lo)
	      {
		lo_x = x;
		lo_y = y;
		lo = d;
	      }
	  }

	/*
	if (xlike)
	  {
	    if (x<lo_x)
	      {
		lo_x = x;
		lo_y = y;
	      }
	    if (x>hi_x)
	      {
		hi_x = x;
		hi_y = y;
	      }
	  }
	else
	  {
	    if (y<lo_y)
	      {
		lo_x = x;
		lo_y = y;
	      }
	    if (y>hi_y)
	      {
		hi_x = x;
		hi_y = y;
	      }
	  }
	*/
      }

    virtual void Notify(int id, int count, int finished)
      {
	if (finished)
	  {
	    //if (count>20)
#ifndef FINE_DETAIL
	    if (count>10)
#else
	    if (count>(DETAIL_THETA))
#endif
	      {
		if (id>at_id)
		  {
		    at_id = id;
		  }
		interest++;
		counts[id] = count;
		double mean_x = tot_x/count;
		double mean_y = tot_y/count;
		double mean_dx = tot_dx/count;
		double mean_dy = tot_dy/count;
		// for now this is all we care about - many other
		// measures available though
		OrientedPatch patch;
		patch.x = mean_x;
		patch.y = mean_y;
		patch.dx = mean_dx;
		patch.dy = mean_dy;
		patch.area = count;

		//PFHIT -- refine endpoints
		double len = (dist(lo_x,lo_y,hi_x,hi_y)/2);
		lo_x = (int)(mean_x+len*mean_dx);
		lo_y = (int)(mean_y+len*mean_dy);
		hi_x = (int)(mean_x-len*mean_dx);
		hi_y = (int)(mean_y-len*mean_dy);

		patch.lo_x = lo_x;
		patch.lo_y = lo_y;
		patch.hi_x = hi_x;
		patch.hi_y = hi_y;
		patches[id] = patch;
		//printf("Got (%g,%g) vector (%g,%g) count (%d,%d)\n",
		//mean_x, mean_y, mean_dx, mean_dy, count, count2);
	      }
	  }
	else
	  {
	    tot_x = tot_y = tot_x2 = tot_y2 = tot_xy = 0;
	    tot_dx = tot_dy = 0;
	    count2 = 0;
	    fixed = 0;
	    lo_x = lo_y = 10000;
	    hi_x = hi_y = -1;
	    lo = -1;
	    hi = -1;
	    xlike = 0;
	  }
      }

    virtual int IsCompatible(int x1, int y1, int x2, int y2)
      {
	int result = 0;
	float dx1 = src_x(x1,y1);
	float dy1 = src_y(x1,y1);
	if (!fixed)
	  {
	    fx = x1;
	    fy = y1;
	    lo_x = x1;  lo_y = y1;
	    hi_x = x1;  hi_y = y1;
	    fdx = dx1;
	    fdy = dy1;
	    if (fabs(fdx)>fabs(fdy))
	      {
		xlike = 1;
	      }
	    else
	      {
		xlike = 0;
	      }
	    fixed = 1;
	  }
	if (x1!=x2 || y1!=y2)
	  {
	    float xx = fabs((x2-fx)*fdx+(y2-fy)*fdy);
	    float yy = fabs(-(x2-fx)*fdy+(y2-fy)*fdx);
	    int doomed = 0;
	    if (xx<0.001) xx = 0.001;
	    doomed = (yy>(0.1*xx+3));
	    doomed = 0;

	    if (!doomed)
	      {
		float dx2 = src_x(x2,y2);
		float dy2 = src_y(x2,y2);
		//if (src_m(x1,y1)>=4 && src_m(x2,y2)>=4)
		//if (src_m(x1,y1)>=5 && src_m(x2,y2)>=5)
		//if (src_m(x1,y1)>=6 && src_m(x2,y2)>=6)
		if (src_m(x1,y1)>=0.5 && src_m(x2,y2)>=0.5)
		  {
		    //if (src_m(x1,y1)<2 || src_m(x2,y2)<2)
		    //{
		    //  result = 0;
		    //}
		    if (fabs(dx1*dx2+dy1*dy2)>0.95)
		      {
			if (fabs(dx1)+fabs(dy1)>0.1)
			  {
			    if (fabs(dx1)+fabs(dy1)>0.1)
			      {
				result = 1;
			      }
			  }
		      }
		  }
	      }
	    if (x2<3 || y2<3 || x2>=src_x.GetWidth()-3 || y2>=src_y.GetHeight()-3)
	      {
		result = 0;
	      }
	  }
	else
	  {
	    result = 1;
	  }
	/*
	if (count2>100)
	  {
	    result = 0;
	  }
	*/
	return result;
      }
  };

  MyLabel *p_label;
  //YARPImageOf<YarpPixelInt> id;


  virtual ~Extender()
   {
     if (p_label!=NULL)
       {
	 delete p_label;
	 p_label = NULL;
       }
   }


  YARPImageOf<YarpPixelInt> id;
  
  double mct[MAX_ORACLE];
  
  YARPImageOf<YarpPixelFloat> destf[MAX_ORACLE], destb[MAX_ORACLE],
    prevf[MAX_ORACLE];


  void Enrich(YARPImageOf<YarpPixelBGR>& dest, int viewer=0)
    {
      assert(p_label!=NULL);
      MyLabel& label = (*p_label);
      //MyLabel label2;
      hash_ip patches = label.patches;
      //label2.Clone(label);
      for (hash_ip::iterator it=patches.begin(); 
	   it!=patches.end();
	   it++)
	{
	  OrientedPatch& p1 = (*it).second;
	  for (hash_ip::iterator it2=it; 
	       //for (hash_ip::iterator it2=label.patches.begin(); 
	       it2!=patches.end();
	       it2++)
	    {
	      OrientedPatch& p2 = (*it2).second;
	      if (it!=it2)
		{
		  double rel = fabs(p1.dx*p2.dx+p1.dy*p2.dy);
		  if (rel>0.98)
		    {
		      double qx = p2.x-p1.x;
		      double qy = p2.y-p1.y;
		      double mag1 = dist(0.0,0.0,qx,qy);
		      double mag2 = fabs(qx*p1.dx+qy*p1.dy);
		      double mag3 = fabs(qx*p2.dx+qy*p2.dy);

		      double d1 = dist(p1.lo_x,p1.lo_y,p2.lo_x,p2.lo_y);
		      double d2 = dist(p1.lo_x,p1.lo_y,p2.hi_x,p2.hi_y);
		      double d3 = dist(p1.hi_x,p1.hi_y,p2.lo_x,p2.lo_y);
		      double d4 = dist(p1.hi_x,p1.hi_y,p2.hi_x,p2.hi_y);
		      double dmin = mymin(d1,mymin(d2,mymin(d3,d4)));
		      double dmax = mymax(d1,mymax(d2,mymax(d3,d4)));
		      
		      double da = dist(p1.lo_x,p1.lo_y,p1.hi_x,p1.hi_y);
		      double db = dist(p1.lo_x,p1.lo_y,p1.hi_x,p1.hi_y);

		      if (mag2*1.05>mag1 && mag1*1.05>mag2 &&
			  mag3*1.05>mag1 && mag1*1.05>mag3)
			  if (da>dmin*0.25 && db>dmin*0.25)
 // && da>6 && db>6)
			{
			  OrientedPatch p3;
			  //double x, y, dx, dy;
			  //double area;
			  //int lo_x, hi_x, lo_y, hi_y;
			  p3.x = (p1.x+p2.x)/2;
			  p3.y = (p1.y+p2.y)/2;
			  MeanOrient(p1.dx,p1.dy,p2.dx,p2.dy,
				     p3.dx, p3.dy);
			  //p3.dx = p1.dx;
			  //p3.dy = p1.dy;
			  p3.area = p1.area+p2.area;
			  if (d1<d2)
			    {
			      p3.hi_x = p2.hi_x;
			      p3.hi_y = p2.hi_y;
			    }
			  else
			    {
			      p3.hi_x = p2.lo_x;
			      p3.hi_y = p2.lo_y;
			    }
			  if (d1>d3)
			    {
			      p3.lo_x = p1.lo_x;
			      p3.lo_y = p1.lo_y;
			    }
			  else
			    {
			      p3.lo_x = p1.hi_x;
			      p3.lo_y = p1.hi_y;
			    }
			  
			  if (viewer)
			    {
			      //printf("Candidate enrichables at (%g,%g) and (%g,%g)\n",
				//     p1.x, p1.y,
				  //   p2.x, p2.y);
			      YarpPixelBGR pix(0,255,255);
			      //DrawLine(dest,pix,int(p1.x),int(p1.y),
				//       int(p2.x),int(p2.y));
			      DrawLine(dest,pix,int(p3.lo_x),int(p3.lo_y),
				       int(p3.hi_x),int(p3.hi_y));
			    }
			  else
			    {
			      label.Add(p3);
			      for (int dx=-1; dx<=1; dx++)
				{
				  for (int dy=-1; dy<=1; dy++)
				    {
				      id.SafePixel((int)(p3.x+0.5+dx),
						   (int)(p3.y+0.5+dy)) =
					(label.at_id);
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }

  double scale_est;
  double area_est;

  void Scan(YARPImageOf<YarpPixelBGR>& src, int x0, int y0,
	    int match, int implicate, int oo,
	    YARPImageOf<YarpPixelFloat>& destf,
	    YARPImageOf<YarpPixelFloat>& destb,
	    YARPImageOf<YarpPixelFloat>& vtarget,
	    //YARPImageOf<YarpPixelFloat>& scaler,
	    //YARPImageOf<YarpPixelFloat>& scaler_ct,
	    int xtarget, int ytarget, double ref_area = 0)
   {
      assert(p_label!=NULL);
      MyLabel& label = (*p_label);

      SatisfySize(src,destf);
      SatisfySize(src,destb);
      SatisfySize(src,vtarget);

      double scale = 0;
      double scale_area = 0;
      double scale_ct = 0;


      hash_iu implicated_units;
      int iu_ct = 0;

      //#ifdef CONSISTENT_SCALE
      // if (implicate)
      //{
	  //  SatisfySize(src,scaler_ct);
      //  SatisfySize(src,scaler);
      //  scaler.Zero();
      //  scaler_ct.Zero();
      //}
      //#endif

      int match_ct = 0;
      for (hash_ip::iterator it=label.patches.begin(); 
	   it!=label.patches.end();
	   it++)
	{
	  OrientedPatch& p1 = (*it).second;
	  for (hash_ip::iterator it2=label.patches.begin(); 
	       it2!=label.patches.end();
	       it2++)
	    {
	      OrientedPatch& p2 = (*it2).second;
	      if (it!=it2)
		{
		  Geometry g;
		  g.Set(p1,p2,x0,y0,ref_area);
		  g.SetAppearance(src);
		  if (!match)
		    {
#ifdef COHERENT_SPECIFIC		   
		      if (g.IsCoherent())
#endif
			{
			  oracle.Add(g);
			}
		    }
		  else 
#ifdef COHERENT_SPECIFIC		   
		    if (g.IsCoherent())
#endif
		    {
		      int top = oracle_count;
		      //for (int oo=0; oo<top; oo++)
			{
			  int r;
			  int rx, ry, rad, rid;
			  double rop;
			  double rscale, rarea;
			  r = oracle_bank[oo].Match(g);
			  if (r) mct[oo] += r;
#if 0
			  if (r)
			    {
			      printf("%d Match id is %ld\n", r, g.GetHash());	
			      g.p1.Write(cout);
			      printf(" // ");
			      g.p2.Write(cout);
			      printf("\n");
			    }
#endif
			  rx = oracle_bank[oo].ox;
			  ry = oracle_bank[oo].oy;
			  rad = oracle_bank[oo].or;
			  rid = oracle_bank[oo].oid;
			  rop = oracle_bank[oo].op;
			  rscale = oracle_bank[oo].oscale;
			  rarea = oracle_bank[oo].oarea;
			  //if (!r) printf("FAILED MATCH!!\n");
			  if (r)
			    {
			      //printf("MATCH!!\n");
			      YarpPixelBGR pix(255,255,255);
			      destf.NullPixel() = 0;
			      float ref = 
				destf.SafePixel(xtarget,ytarget);
#ifndef USE_CONTINUUM_NONONO
			      AddDeltaCircle(destf,
					     (int)rx,(int)ry,
					     (int)rad,(float)(r*rop)); //PFHIT was r*rop
#else
			      AddDeltaCircle(destf,
					     (int)rx,(int)ry,
					     (int)rad,(float)(r*rop*CONTINUUM_FACTOR1));
#endif
			      destf.NullPixel() = 0;
			      float ref2 = 
				destf.SafePixel(xtarget,ytarget);
			      if (implicate)
				{
				  vtarget.NullPixel() = 0;
#ifdef SHARP_TARGET
				  if (vtarget.SafePixel((int)rx,(int)ry)>0.9)
#else
				  if (vtarget.SafePixel((int)rx,(int)ry)>0.1)
#endif
				    {
				      float del = fabs(ref-ref2);
				      // g is implicated
				      
				      //g.Show(destb,1);

				      if (del>0.001)
					{
					  Unit uu;
					  uu.x1 = (int)p1.x;
					  uu.y1 = (int)p1.y;
					  uu.x2 = (int)p2.x;
					  uu.y2 = (int)p2.y;
					  uu.scale = rscale;

					  implicated_units[iu_ct] =
					    uu;
					  iu_ct++;
					  
					  scale += rscale;
					  scale_area += rscale*rscale*rarea;
					  scale_ct++;
					}
				      //AddDeltaCircleStraight(scaler,
				      //     (int)rx,(int)ry,
				      //     (int)rad,(float)rscale);
				      //AddDeltaCircleStraight(scaler_ct,
				      //     (int)rx,(int)ry,
				      //     (int)rad,1.0f);

				    }
				}
			      match_ct += r;
			    }
			}
		    }
		}
	    }
	}
      if (scale_ct>0.001) 
	{
	  scale/=scale_ct;
	  scale_area/=scale_ct;
	}
      if (match)
	{
	  printf("%d matches\n", match_ct);
	}
      if (implicate)
	{
	  printf("scale is %g (area %g, radius %g)\n", scale, scale_area,
		 sqrt(scale_area));
	  for (hash_iu::iterator it=implicated_units.begin();
	       it!=implicated_units.end(); it++)
	    {
	      Unit& uu = (*it).second;
	      if (uu.scale<scale*1.5)
		{
		  uu.Show(destb,1);
		}
	    }
	}
      scale_est = scale;
      area_est = scale_area;
   }


  void BaseGroup(YARPImageOf<YarpPixelFloat>& src_x,
	     YARPImageOf<YarpPixelFloat>& src_y,
	     YARPImageOf<YarpPixelFloat>& src_m,
	     YARPImageOf<YarpPixelBGR>& src, 
	     YARPImageOf<YarpPixelBGR>& code, 
	     YARPImageOf<YarpPixelBGR>& dest, 
	     int x0, int y0,
	     int match=0)
    {
      if (p_label!=NULL)
	{
	  delete p_label;
	  p_label = NULL;
	}
      p_label = new MyLabel(src_x,src_y,src_m);
      assert(p_label!=NULL);
      MyLabel& label = (*p_label);

      SatisfySize(src_x,dest);
      dest.Zero();
      SatisfySize(src_x,id);
      id.Zero();
      label.Reset();
      int r = label.Apply(src_x.GetWidth(),src_x.GetHeight(),id);

#ifdef ENRICH_REGIONS
      Enrich(dest,0);
#endif

#ifndef MINIMAL_ANTICS
      YarpPixelBGR pixmark(255,255,255);
      if (1)
	{
	  IMGFOR(dest,x,y)
	    {
	      dest(x,y) = pixmark;
	    }
	  pixmark = YarpPixelBGR(0,0,0);
	}
      if (0) IMGFOR(id,x,y)
	{
	  //printf(":: %d %d (%d)\n", x,y, id(x,y));
	  if (label.counts.find(id(x,y))!=label.counts.end())
	    {
	      dest(x,y).r = (id(x,y)*341)%200+50;
	      dest(x,y).g = (id(x,y)*9707)%256;
	      dest(x,y).b = (id(x,y)*914)%256;
	    }
	}
      for (hash_ip::iterator it = label.patches.begin();
	   it!=label.patches.end(); it++)
	{
	  int q = (*it).first;
	  OrientedPatch& p = (*it).second;
	  AddCrossHair(dest,YarpPixelBGR(255,128,0),p.x,p.y,3);
	  //AddCrossHair(dest,YarpPixelBGR(255,128,0),p.lo_x,p.lo_y,2);
	  //AddCrossHair(dest,YarpPixelBGR(255,128,0),p.hi_x,p.hi_y,2);
	  //DrawLine(dest,YarpPixelBGR(0,128,255),p.lo_x,p.lo_y,
	  //p.hi_x,p.hi_y);
	  
	  //AddCircle(dest,YarpPixelBGR(255,128,255),
	  //(int)(p.x),(int)(p.y),3);

	  int len = (int)(dist(p.lo_x,p.lo_y,p.hi_x,p.hi_y)/2);
	  DrawLine(dest,pixmark,
		   (int)(p.x+len*p.dx+0.5),(int)(p.y+len*p.dy+0.5),
		   (int)(p.x-len*p.dx+0.5),(int)(p.y-len*p.dy+0.5));
	}
#endif
      
#ifdef OUTPUT_ORIENT
      static int tck=0;
      //if (match)
	{
	  char buf[1000];
	  sprintf(buf,"%s%s_ofood%06d_%d.ppm",BASE,name_label,tck,9);
	  YARPImageFile::Write(buf,dest);
	  tck++;
	}
#endif

      /*
      for (hash_ip::iterator it=label.patches.begin(); it!=label.patches.end();
	   it++)
	{
	  double x = (*it).second.x;
	  double y = (*it).second.y;
	  double dx = (*it).second.dx;
	  double dy = (*it).second.dy;
	  double r = sqrt(fabs(dx*dx+dy*dy));
	  if (r>0.0001)
	    {
	      dx /= r;
	      dy /= r;
	    }
	  for (int i=0; i<10; i++)
	    {
	      int px = (int)(0.5+x+dx*i);
	      int py = (int)(0.5+y+dy*i);
	      int mx = (int)(0.5+x-dx*i);
	      int my = (int)(0.5+y-dy*i);
	      YarpPixelBGR white(255,255,255);
	      dest.SafePixel(px,py) = white;
	      dest.SafePixel(mx,my) = white;
	    }
	}
      */
      printf("Max size ID is %d\n", r);
      printf("reasonably sized blobs: %d\n", label.interest);
      int cutoff = 40;
      //int use_triplets = (label.interest<cutoff);
      //use_triplets = 0;
#ifndef USE_CUTOFF
      cutoff = 10000;
#endif
      if (label.interest>cutoff)
	{
	  printf("Chopping some smaller sized regions\n");
	  hash_ip np = label.patches;
	  label.patches = hash_ip();
#define MAX_CTS (40)
	  int cts[MAX_CTS];
	  for (int i=0; i<MAX_CTS; i++)
	    {
	      cts[i] = 0;
	    }
	  for (hash_ip::iterator it = np.begin();
	       it!=np.end(); it++)
	    {
	      int q = (*it).first;
	      OrientedPatch& p = (*it).second;
	      int idx = label.counts[q];
	      if (idx>=MAX_CTS) idx = MAX_CTS-1;
	      cts[idx]++;
	    }
	  int ct = 0;
	  int top = MAX_CTS-1;
	  for (int i=MAX_CTS-1; i>=0; i--)
	    {
	      ct += cts[i];
	      if (ct>cutoff)
		{
		  break;
		}
	      top = i;
	    }
	  ct = 0;
	  for (hash_ip::iterator it = np.begin();
	       it!=np.end(); it++)
	    {
	      int q = (*it).first;
	      OrientedPatch& p = (*it).second;
	      if (label.counts[q]>=top)
		{
		  label.patches[q] = p;
		  ct++;
		}
	    }
	  printf("left with reasonably sized blobs: %d\n", ct);
	}
    }

    int xtarget, ytarget, itarget;
    YARPImageOf<YarpPixelFloat> vtarget;








    void Group(YARPImageOf<YarpPixelFloat>& src_x,
	       YARPImageOf<YarpPixelFloat>& src_y,
	       YARPImageOf<YarpPixelFloat>& src_m,
	       YARPImageOf<YarpPixelBGR>& src, 
	       YARPImageOf<YarpPixelBGR>& code, 
	       YARPImageOf<YarpPixelBGR>& dest, 
	       int x0, int y0,
	       int match=0, double ref_area = 0)
    {
      xtarget = 64;
      ytarget = 64;
      static int round_one = 1;
      
      BaseGroup(src_x,src_y,src_m,src,code,dest,x0,y0,match);
      
      assert(p_label!=NULL);
      MyLabel& label = (*p_label);

      oracle_count = mymax(oracle_count,1);

      int scan_lo = 0;
      int scan_hi = oracle_count-1;
      if (g_target>=0)
	{
	  scan_lo = g_target;
	  scan_hi = g_target;
	}

      for (int i=0; i<MAX_ORACLE; i++)
	{
	  SatisfySize(dest,destf[i]);
	  SatisfySize(dest,destb[i]);
	  SatisfySize(dest,prevf[i]);
	  if (round_one)
	    {
	      prevf[i].Zero();
	    }
	}
      round_one = 0;
      
      if (match)
	{
	  for (int i=0; i<oracle_count; i++)
	    {
	      mct[i] = 0;
	    }
	  for (int i=0; i<oracle_count; i++)
	    {
	      SatisfySize(dest,destf[i]);
	      SatisfySize(dest,destb[i]);
	      SatisfySize(dest,prevf[i]);
	      destf[i].Zero();
#ifdef USE_CONTINUUM_NONONO
	      //destf[i].PeerCopy(prevf[i]);
	      int oo = i;
	      IMGFOR(prevf[oo],x,y)
		{
		    {
		      destf[oo](x,y) = prevf[oo](x,y);
		    }
		}

	      IMGFOR(destf[i],x,y)
	      {
		//	        destf[i](x,y) *= CONTINUUM_FACTOR2;
	      }
#endif
	      destb[i].Zero();
	    }
	}

      for (int oo=scan_lo; oo<=scan_hi; oo++)
	{
	  Scan(src,x0,y0,(match>0),0,oo,destf[oo],destb[oo],
	       vtarget,xtarget,ytarget,ref_area);
#ifdef USE_CONTINUUM_NONONO
	  prevf[oo].PeerCopy(destf[oo]);
#endif
	}

      if (match)
	{
	  double topper[MAX_ORACLE];
	  double xtopper[MAX_ORACLE];
	  double ytopper[MAX_ORACLE];
	  double xmtopper[MAX_ORACLE];
	  double ymtopper[MAX_ORACLE];
	  double btopper[MAX_ORACLE];
	  double vtopper[MAX_ORACLE];
	  double vetopper[MAX_ORACLE];
	  double worth_stable[MAX_ORACLE];
	  double worth_response[MAX_ORACLE];

	  static double pxtopper[MAX_ORACLE];
	  static double pytopper[MAX_ORACLE];
	  static double pxydist[MAX_ORACLE];
	  static int first = 1;
	  
	  if (first)
	    {
	      for (int i=0; i<oracle_count; i++)
		{
		  pxtopper[i] = pytopper[i] = pxydist[i] = 0;
		}
	      first = 0;
	    }

	  printf(">>> %s : ", name_label);
	  int besti = 0;
	  double best = -1;
	  int bestflati = 0;
	  double bestflat = -1;
	  int beststablei = 0;
	  double beststable = 1e20;
	  for (int i=scan_lo; i<=scan_hi; i++)
	    {
	      double top = 0.0001;
	      double xtop = 64, ytop = 64;
	      double accum = 0;
	      IMGFOR(dest,x,y)
		{
		  float v = destf[i](x,y);
		  if (v>top)
		    {
		      top = v;
		      xtop = x;
		      ytop = y;
		    }
		  accum += sq(v);
		}
	      //double val = 10000.0*mct[i]/oracle_scale[i];
	      topper[i] = top;
	      xtopper[i] = xtop;
	      ytopper[i] = ytop;
	      xmtopper[i] = xtop;
	      ymtopper[i] = ytop;
	      double d = dist((double)xtop,(double)ytop,
			      pxtopper[i],pytopper[i]);
	      pxtopper[i] = xtop;
	      pytopper[i] = ytop;
	      if (accum<100)
		{
		  d = 64;
		}
	      double d2 = pxydist[i] = 0.75*pxydist[i] + 0.25*d;

	      worth_response[i] = accum;
	      worth_stable[i] = d2;

	      printf("%g (%g) ", accum, d2);

	      if (accum>bestflat)
		{
		  bestflat = accum;
		  bestflati = i;
		}
	      if (d2<beststable && bestflat>100.0)
		{
		  beststablei = i;
		  beststable = d2;
		}
	    }
	  float bestworth = 2; //bestflat/10000.0;
	  besti = bestflati;
	  if (beststable<10)
	    {
	      if (worth_stable[besti]>25)
		{
		  besti = beststablei;
		}
	    }
	  printf(": %d (%d)", besti, bestflati);
	  printf("\n");

	  YarpPixelBGR pix(255,128,0);
	  YarpPixelBGR pix2(0,128,255);
	  YarpPixelBGR pix3(0,255,0);
	  YarpPixelBGR pix4(255,0,0);
	  YarpPixelBGR pixwhite(255,255,255);

	  //if (bestworth>1)
#ifdef MINIMAL_ANTICS
	  for (int i=besti; i<=besti; i++)
#else
	  for (int i=scan_lo; i<=scan_hi; i++)
#endif
	    {
	      float top = topper[i];

	      float total = 0;
	      float tx = 0, ty = 0;
	      IMGFOR(dest,x,y)
		{
		  destf[i](x,y) /= top;
		  float v = destf[i](x,y);
		  if (v>0.25)
		    {
		      if (dist(xtopper[i],ytopper[i],(double)x,(double)y)<15)
			{
			  tx += v*x;
			  ty += v*y;
			  total += v;
			}
		    }
		}
	      if (total>0.01)
		{
		  tx /= total;
		  ty /= total;
		  xtopper[i] = tx;
		  ytopper[i] = ty;
		}

	      Scan(src,x0,y0,match,1,i,vtarget,destb[i],
		   destf[i],xtopper[i], ytopper[i]);

#ifdef USE_CONTINUUM
	      float nmax = 0.0001;
	      IMGFOR(src,x,y)
		{
		  prevf[i](x,y) += topper[i]*destf[i](x,y);
		  prevf[i](x,y) *= 0.75;
		  if (prevf[i](x,y)>20000)
		    {
		      prevf[i](x,y) = 20000;
		    }
		  if (prevf[i](x,y)>nmax)
		    {
		      nmax = prevf[i](x,y);
		    }
		}
	      IMGFOR(src,x,y)
		{
		  destf[i](x,y) = prevf[i](x,y)/nmax;
		}
#endif

	      double scale = scale_est;
	      double area = area_est;
	      double radius = sqrt(area_est);

	      double val = 0, val2 = 0;
	      if (g_training==1)
		{
		  oracle.appear.Add(src,src_m,xtopper[i],ytopper[i],radius);
		  val = oracle.appear.Match(src,src_m,
					    xtopper[i],
					    ytopper[i],radius);
		  val2 = 0;
		}
	      else if (g_training==2)
		{
		  val = oracle.appear.Match(src,src_m,
					    xtopper[i],
					    ytopper[i],radius);
		  oracle.appear.NoteMatch(val);
		  val2 = 0;
		}
	      else
		{
		  val = oracle_bank[i].appear.Match(src,src_m,
						    xtopper[i],
						    ytopper[i],radius);
		  //printf("canon match is %g\n", val);
		  val2 = oracle_bank[i].appear.EvaluateMatch(val);
		}
	      vtopper[i] = val;
	      vetopper[i] = val2;

	      float topb = 0.00001;
	      int topx = 64;
	      int topy = 64;
	      IMGFOR(dest,x,y)
		{
		  if (destb[i](x,y)>topb)
		    {
		      topb = destb[i](x,y);
		    }
		}
	      btopper[i] = topb;
	      IMGFOR(dest,x,y)
		{
		  destb[i](x,y) /= topb;
		}

#ifndef MINIMAL_ANTICS
	      {
		static int tck=0;
		char buf[1000];
		sprintf(buf,"%s%s_rfood%06d_%d.ppm",BASE,name_label,tck,i);
		YARPImageOf<YarpPixelBGR> dest2;
		dest2 = src;
		IMGFOR(dest,x,y)
		  {
		    dest2(x,y).r = 255-(int)(255*destf[i](x,y));
		    dest2(x,y).g = 255-(int)(255*destf[i](x,y));
		    dest2(x,y).b = 255-(int)(255*destf[i](x,y));
		  }
		int topx = xtarget = (int)xtopper[i];
		int topy = ytarget = (int)ytopper[i];
#if 0
#ifndef LIGHT_MARKINGS
		AddCircle(dest2,pixwhite,topx,topy,5);
#endif
		AddCircleOutline(dest2,pix,topx,topy,5);
		AddCircleOutline(dest2,pix,topx,topy,7);
		AddCircleOutline(dest2,pix2,topx,topy,6);
		int big = (vetopper[i]>MIN_MATCH);
		  {
		    oracle_proto[i].Brand(src,dest2,8,8,big);
		  }
#endif
#ifdef TESTING
		YARPImageFile::Write(buf,dest2);
#endif
		tck++;
	      }
#endif
	    }

	  dest = src;

	  int topx = xtarget = (int)xtopper[besti];
	  int topy = ytarget = (int)ytopper[besti];
	  double topv = vtopper[besti];
	  double topve = vetopper[besti];
	  itarget = besti;
	  vtarget = destf[besti];
	  NoteMatch(bestflat);
	  printf("++++ %s %d %d %g %g\n", name_label, xtarget, ytarget, 
		 topv, topve);

	  if (!g_found)
	    {
	      g_found_x = xtarget;
	      g_found_y = ytarget;
	    }

	  YARPImageOf<YarpPixelBGR> dest_base;
	  dest_base = dest;

	  float dbt = 0.2; //1.5;  // was 0.2
#ifdef MINIMAL_ANTICS
	  for (int i=besti; i<=besti; i++)
#else
	  for (int i=scan_lo; i<=scan_hi; i++)
#endif
	    {
	      YARPImageOf<YarpPixelFloat> bounder;
	      SatisfySize(src,bounder);
	      bounder.Zero();
	      float xe[MAX_POINT], ye[MAX_POINT];
	      int pct = 0;
	      IMGFOR(dest,x,y)
		{
		  //if (destb[i](x,y)>0.20)
		  if (destb[i](x,y)>dbt)
		    {
		      if (pct<MAX_POINT)
			{
			  xe[pct] = x;
			  ye[pct] = y;
			  pct++;
			}
		      //dest(x,y) = pix2;
		      if (g_use_graph)
			{
			  int mid = id(x,y);
			  IMGFOR(id,xx,yy)
			    {
			      if (id(xx,yy)==mid)
				{
				  //dest(xx,yy) = pix4;
				  bounder(xx,yy) = 255;
				}
			    }
			}
		    }
		}

	      if (topper[i]>0.0)
		{
		  //oracle_proto[i].Mark(src,dest,(int)(xtopper[i]),
		  //(int)(ytopper[i]));
		}

	      if (pct<6 && pct>=3)
		{
		  int bpct = pct;
		  for (int i=0; i<bpct; i++)
		    {
		      if (pct<MAX_POINT)
			{
			  xe[pct] = xe[i]+1;
			  ye[pct] = ye[i]+1;
			  pct++;
			  //printf("INFLATE!!!!!!! %g %g\n",xe[pct],ye[pct]);
			}
		    }
		    }
	      
	      
	      YARPImageOf<YarpPixelBGR> dest2;
	      dest2 = dest_base;


#if 0
		  if (pct>=6)
		    {
		      double xc, yc;
		      double xa, ya;
		      double la, lb;
		      fit_ellipse(xe,ye,pct,xc,yc,xa,ya,la,lb);
		      printf("(%g,%g) %gx%g (%g,%g)\n", xc, yc, la, lb, xa, ya);
		      YARPImageOf<YarpPixelMono> foo;
		      SatisfySize(src,foo);
		      EllipseDrawer ed;
		      foo.Zero();
		      ed.Apply(foo, xc, yc, la, lb, xa, ya);
		      IMGFOR(foo,x,y)
			{
			  if (foo(x,y))
			    {
			      dest2(x,y).r = 255;
			      dest2(x,y).g = 0;
			      dest2(x,y).b = 255;
			    }
			}
		    }
#endif


	      if (g_use_graph)
		{
		  YARPImageOf<YarpPixelBGR> work1, work2;
		  YARPImageOf<YarpPixelInt> work3, work4;
		  YARPImageOf<YarpPixelMono> work5;
		  SatisfySize(src,work1);
		  SatisfySize(src,work2);
		  SatisfySize(src,work3);
		  SatisfySize(src,work4);
		  SatisfySize(src,work5);
		  work1.Zero();
		  work2.Zero();
		  work3.Zero();
		  work4.Zero();
		  work5.Zero();
		  

		  if (g_use_graph)
		    {
		      IMGFOR(work1,x,y)
			{
			  work1(x,y).r = (int)bounder(x,y);
			}
		      
		      GroupOptim(work1,work2,1,0);
		      IMGFOR(work2,x,y)
			{
			  if (work2(x,y).b>128)
			    {
			      //dest(x,y).b = 255;
			      work3(x,y) = 255;
			    }
			}
		      YARPImageLabel lab;
		      int id = lab.Apply(work3,work4);
		      work1.Zero();
		      IMGFOR(work1,x,y)
			{
			  if (work4(x,y)==id)
			    {
			      work1(x,y).r = 255;
			    }
			}
		      GroupOptim(work1,work2,0,0);
		      YARPImageOf<YarpPixelMono> bnd_mask;
		      SatisfySize(work2,bnd_mask);
		      bnd_mask.Zero();
		      IMGFOR(work2,x,y)
			{
			  if (work2(x,y).b>128)
			    {
			      bnd_mask(x,y) = 255;
			    }
			}
		      YARPShapeBoundary bnd;
		      bnd.SetFromMask(bnd_mask,2);
		      YarpPixelBGR pixred(255,0,0);
		      YarpPixelBGR pixblue(0,0,255);
		      {
			for (int i=0; i<bnd.GetPointCount(); i++)
			  {
			    YARPShapeElement el = bnd.GetPoint(i);
			    AddCircle(dest2,pixred,(int)(el.x+0.5),(int)(el.y+0.5),3);
			  }
			for (int i=0; i<bnd.GetPointCount(); i++)
			  {
			    YARPShapeElement el = bnd.GetPoint(i);
			    AddCircle(dest2,pixblue,(int)(el.x+0.5),(int)(el.y+0.5),2);
			  }
		      }
		    }
		}
	      
	      IMGFOR(dest,x,y)
		{
		  if (destb[i](x,y)>dbt)
		    {
#ifdef LIGHT_MARKINGS
		      hash_ip::iterator it = label.patches.find(id(x,y));
		      if (it==label.patches.end())
			{
			  // illusory contour
			  printf("Illusory match\n");
			}
		      if (it!=label.patches.end())
			{
			  int q = (*it).first;
			  OrientedPatch& p = (*it).second;
			  //AddCrossHair(dest,YarpPixelBGR(255,128,0),p.lo_x,p.lo_y,2);
			  //AddCrossHair(dest,YarpPixelBGR(255,128,0),p.hi_x,p.hi_y,2);
			  //DrawLine(dest,YarpPixelBGR(0,128,255),p.lo_x,p.lo_y,
			  //p.hi_x,p.hi_y);
	  
			  //AddCircle(dest2,YarpPixelBGR(255,0,0),
				//    (int)(p.x),(int)(p.y),3);
			  int dd = 7;
			  int len = (int)(dist(p.lo_x,p.lo_y,p.hi_x,p.hi_y)/2);
			  if (len>dd)
			    {
			      dd = len;
			    }
			  //dd = 6;
#ifdef SHOW_IMPLICATED
			  DrawLine(dest2,YarpPixelBGR(255,0,0),
				   (int)(p.x+int(dd*p.dx+0.5)),
				   (int)(p.y+int(dd*p.dy+0.5)),
				   (int)(p.x-int(dd*p.dx-0.5)),
				   (int)(p.y-int(dd*p.dy-0.5)));
#endif
			  //DrawLine(dest2,YarpPixelBGR(255,0,0),
				//   (int)(p.x+1*p.dy),(int)(p.y-1*p.dx),
			  // (int)(p.x-1*p.dy),(int)(p.y+1*p.dx));
			}
		      /*
		      int mid = id(x,y);
		      IMGFOR(dest2,xx,yy)
			{
			  if (id(xx,yy)==mid)
			    {
			      dest2(xx,yy).r = 128;
			      dest2(xx,yy).g = 128;
			      dest2(xx,yy).b = 128;
			    }
			}
		      */
		      //AddCrossHair(dest2,YarpPixelBGR(255,0,0),x,y,4);
		      
#else
		      AddCrossHair(dest2,pix3,x,y,5);
#endif
		    }
		}

	      {
		static int tck=0;
		char buf[1000];
		sprintf(buf,"%s%s_%d_sfood%06d.ppm",BASE,name_label,i,tck);
		//YARPImageOf<YarpPixelBGR> dest2;
		//dest2 = dest;
		//IMGFOR(dest,x,y)
		// {
		//  dest2(x,y).g = (int)(255*destf[i](x,y));
		//  dest2(x,y).b = (int)(255*destf[i](x,y));
		//}
		int topx = xtarget = (int)xtopper[i];
		int topy = ytarget = (int)ytopper[i];
#ifdef LIGHT_MARKINGS
		//AddCircle(dest2,pixwhite,topx,topy,5);
#ifdef SHOW_SMALL_CIRCLE
		AddCircleOutline(dest2,pix,topx,topy,4);
		AddCircleOutline(dest2,pix,topx,topy,6);
		AddCircleOutline(dest2,pix2,topx,topy,5);
#endif

#else
		AddCircle(dest2,pixwhite,topx,topy,5);
		AddCircleOutline(dest2,pix,topx,topy,5);
		AddCircleOutline(dest2,pix,topx,topy,7);
		AddCircleOutline(dest2,pix2,topx,topy,6);
#endif
		int big = (vetopper[i]>MIN_MATCH);
		  {
		    oracle_proto[i].Brand(src,dest2,8,8,big);
		  }
#ifdef TESTING
		YARPImageFile::Write(buf,dest2);
#endif
		tck++;
	      }
		
		if (i==besti)
		  {
		    //if (bestflat>1000)
		      {
			dest = dest2;
		      }
		  }
		
		
	    }

	  /*
	  AddCircle(dest,pixwhite,topx,topy,5);
	  AddCircleOutline(dest,pix,topx,topy,5);
	  AddCircleOutline(dest,pix,topx,topy,7);
	  AddCircleOutline(dest,pix2,topx,topy,6);
	  */

	  static int ptopx = -64, ptopy = -64, pbesti = -1;
	  static int stable = 0;
	  //if (dist(ptopx,ptopy,topx,topy)<25 && pbesti==besti && bestflat>1000)

	  //if (dist(ptopx,ptopy,topx,topy)<25 && pbesti==besti && bestflat>1000) //PFHIT
	  //if (dist(ptopx,ptopy,topx,topy)<25 && pbesti==besti) // && bestflat>5e9) //PFHIT
	  if (dist(ptopx,ptopy,topx,topy)<25 && pbesti==besti && bestflat>=LOW_THETA) // && bestflat>5e9) //PFHIT
	    //if (dist(ptopx,ptopy,topx,topy)<25 && pbesti==besti && bestflat>1000) //PFHIT
	    {
	      stable++;
	      if (stable>=4)
		{
#ifdef SHOW_WIDE_CIRCLE
		  //if (stable>=3)
		  {
		    AddCircleOutline(dest,pix,topx,topy,20);
		    AddCircleOutline(dest,pix,topx,topy,22);
		    g_found = 1;
		    g_found_x = topx;
		    g_found_y = topy;
		    g_found_r = 1;
		    g_found_id = besti;
		  }
#endif
		}
	    }
	  else
	    {
	      stable -= 2;
	      if (stable<0) stable = 0;
	      dest = src; //PFHIT
	    }
	  pbesti = besti;
	  ptopx = topx;
	  ptopy = topy;

	  // NOW lets actually look for a prototypical view of the object
#ifdef BRAND_IMAGE
	  //	  oracle_proto[besti].Brand(src,dest,xtarget,ytarget);
#endif
	  g_precedent = oracle_proto[besti].image;
	  g_precedent_mask = oracle_proto[besti].mask;

	}
      else
	{
	  YarpPixelBGR pix(0,128,255);
	  AddCrossHair(dest,pix,x0,y0,10);
	}

      //YARPImageOf<YarpPixelFloat> tmp;
      //tmp.CastCopy(id);
      //YARPImageFile::Write("tmp.txt",tmp);
#ifdef ENRICH_REGIONS
      //Enrich(dest,1);
#endif


    }
};

















  
static void Process(YARPImageOf<YarpPixelBGR>& src,
		    YARPImageOf<YarpPixelMono>& mask,
		    YARPImageOf<YarpPixelBGR>& dest, int match = 0,
		    int px = -1, int py = -1)
{

  YARPImageOf<YarpPixelFloat> xdata, ydata, mdata;
  YARPImageOf<YarpPixelFloat> xdata2, ydata2, mdata2;
  YARPImageOf<YarpPixelBGR> code;

  YARPBinaryAnalysis ana;
  ana.Apply(mask);
  double x0 = ana.GetX();
  double y0 = ana.GetY();
  double area = ana.GetArea();
  if (px>=0 && py>=0)
    {
      x0 = px;
      y0 = py;
    }

  //printf("+++ %s %g %g\n", name_label, x0, y0);
  //return;

#ifdef RIGHT_ALIGN
  printf("WARNING RIGHT_ALIGN\n");
  if (!match)
    {
      x0 = -1;
      y0 = -1;
      float ct = 0;
      float ct_all = 0;
      IMGFOR(mask,x,y)
	{
	  ct_all++;
	  if (mask(x,y)>128)
	    {
	      ct++;
	      if (x>x0)
		{
		  x0 = x;
		  y0 = y;
		}
	    }
	}
      if (ct/ct_all>0.1 || ct/ct_all<0.01)
	{
	  return;
	}
    }
#endif

  //printf("*** step1\n");  fflush(stdout);

  SatisfySize(src,code);
  SatisfySize(src,xdata);
  SatisfySize(src,ydata);
  SatisfySize(src,mdata);
  SatisfySize(src,xdata2);
  SatisfySize(src,ydata2);
  SatisfySize(src,mdata2);
  //xdata2.Zero();
  //ydata2.Zero();
  //mdata2.Zero();

  //printf("*** step1b\n");  fflush(stdout);

  YARPImageOf<YarpPixelBGR> src2;
  fine.SetDemocracy(0);
  fine.SetLuminanceFilter(11);
  fine.SetQuotient(2);
  fine.Apply(src,code,xdata,ydata,mdata);
#ifdef OUTPUT_ORIENT
  {
    static int tck=0;
    //if (match)
    {
      char buf[1000];
      IMGFOR(code,x2,y2)
	{
	  if (x2<3 || y2<3 || 
	      x2>=code.GetWidth()-3 || y2>=code.GetHeight()-3)
	    {
	      code(x2,y2) = YarpPixelBGR(0,0,0);
	    }
#if 0
	  YarpPixelBGR& pix = code(x2,y2);
	  if (pix.r==0&&pix.g==0&&pix.b==0)
	    {
	      pix = YarpPixelBGR(255,255,255);
	    }
#endif
	}
      sprintf(buf,"%s%s_ufood%06d_%d.ppm",BASE,name_label,tck,9);
      YARPImageFile::Write(buf,code);
      tck++;
    }
  }
#endif

  if (!match)
    {
      IMGFOR(mdata,x,y)
	{
	  if (mask(x,y)<128)
	    {
	      mdata(x,y) = 0;
	    }
	}
    }

  //printf("*** step2\n");  fflush(stdout);

  dest.PeerCopy(src);
  dest.Zero();

  //printf("*** step3\n");  fflush(stdout);
  
  Extender ext;

  //printf("*** step4\n");  fflush(stdout);

  int ct1 = 0;
  xdata2 = xdata;
  ydata2 = ydata;
  /*
  IMGFOR(dest,ox,oy)
    {
      mdata2(ox,oy) = 0;
      ext.Reset();
      ext.Extend(src,dest,code,xdata,ydata,mdata,
		 xdata2,ydata2,mdata2,ox,oy,1,0);
      ext.Extend(src,dest,code,xdata,ydata,mdata,xdata2,ydata2,mdata2,
		 ox,oy,-1,0);
      mdata2(ox,oy) = ext.GetLength();
      if (ext.GetLength()>10)
	{
	  ct1++;
	  ext.Reset();
	  ext.Extend(src,dest,code,xdata,ydata,mdata,xdata2,ydata2,mdata2,
		     ox,oy,1,1);
	  ext.Extend(src,dest,code,xdata,ydata,mdata,xdata2,ydata2,mdata2,
		     ox,oy,-1,1);
	}
      //dest(ox,oy) = src(ox,oy);
    }
  printf("Oriented count is %d (%d)\n", ct1, ct1*ct1);
  */

  ext.Group(xdata,ydata,mdata,src,code,dest,(int)x0,(int)y0,match,area);
  //ext.Group(xdata2,ydata2,mdata2,src,code,dest,(int)x0,(int)y0,match);
  //ext.Group(xdata2,ydata2,mdata2,src,code,dest,(int)x0,(int)y0,match?1:0);
#ifdef SHADOW_OUTLINE
  //if (match==2)
  //{
  //  ext.Group(xdata2,ydata2,mdata2,src,code,dest,(int)x0,(int)y0,2);
  //}
#endif

  //YARPImageFile::Write("xdata.txt",xdata,YARPImageFile::FORMAT_NUMERIC);
  //YARPImageFile::Write("ydata.txt",ydata,YARPImageFile::FORMAT_NUMERIC);
  //YARPImageFile::Write("mdata.txt",mdata,YARPImageFile::FORMAT_NUMERIC);
  
  //YARPImageFile::Write("orient.ppm",code);
  
#ifdef SAVE_SECOND_FILES
  static int tck=0;
  if (match)
    {
      char buf[1000];
      sprintf(buf,"%s%s_food%06d.ppm",BASE,name_label,tck);
      YARPImageFile::Write(buf,dest);
      tck++;
    }
#endif
}


#define MYN 20
static void ellipse_main()
{
  float x[MYN], y[MYN];
  double p[6];
  float theta;
  for (int i=0; i<MYN; i++)
    {
      theta = i*2*M_PI/MYN;
      x[i] = 64 + 32*cos(theta);
      y[i] = 64 + 64*sin(theta);
      //x[i] = 20 + 40*cos(theta);
      //y[i] = 80*sin(theta);
    }
  double xc, yc;
  double xa, ya;
  double la, lb;
  fit_ellipse(x,y,MYN,xc,yc,xa,ya,la,lb);
  printf("(%g,%g) %gx%g (%g,%g)\n", xc, yc, la, lb, xa, ya);
  YARPImageOf<YarpPixelMono> foo;
  foo.Resize(128,128);
  EllipseDrawer ed;
  foo.Zero();
  ed.Apply(foo, xc, yc, la, lb, xa, ya);
  YARPImageFile::Write("foo.pgm", foo);
  exit(0);
}


class Spotter
{
public:
  int x, y;

  Spotter()
    {
      fine.Init();
      //fine.Init("/mnt/state/DayOne/orient/current/orient.txt");
      fine.SetDemocracy(0);
    }
};


YARPSpotter::YARPSpotter()
{
  system_resource = (void*)(new Spotter);
  assert(system_resource!=NULL);
  //ellipse_main();
  InitDefaults();
}

YARPSpotter::~YARPSpotter()
{
  delete ((Spotter*)system_resource);
  system_resource = NULL;
}

void YARPSpotter::AddItem(int id, const char *main_data_file, 
			  const char *prototype_file, 
			  const char *means_file)
{
  AddOracle(id,main_data_file,prototype_file,means_file);
}

void YARPSpotter::Train(YARPImageOf<YarpPixelBGR>& src,
			YARPImageOf<YarpPixelMono>& mask,
			YARPImageOf<YarpPixelBGR>& dest,
			int px, int py)
{
  mutex.Wait();
  RedirectTrainer(oracle_bank[0]);
  Process(src,mask,dest,0,px,py);
  /*
  oracle_bank[0] = oracle;  // super inefficient!
  oracle_scale[0] = 1;
  oracle_mean[0] = 1;
  oracle_std[0] = 1;
  oracle_count = 1;
  */
  if (oracle_count<1) oracle_count = 1;
  oracle_proto[0].Set(src,mask);
  mutex.Post();
}

void YARPSpotter::WriteTrain(ostream& os, int all)
{
  if (all==0)
    {
      oracle.Write(os);
      os << "0" << endl;
      oracle.WriteAppearance(os);
      oracle_bank[0] = oracle;
      oracle_scale[0] = 1;
      oracle_mean[0] = 1;
      oracle_std[0] = 1;
      oracle_count = 1;
    }
  else
    {
      for (int i=0; i<oracle_count; i++)
	{
	  char buf[256], buf2[256], buf3[256];
	  sprintf(buf,"autohist_%04d.txt",i);
	  ofstream fout(buf);
	  oracle_bank[i].Write(fout);
	  fout << "0" << endl;
	  oracle_bank[i].WriteAppearance(fout);
	  sprintf(buf2,"autoproto_%04d.ppm",i);
	  YARPImageFile::Write(buf2,oracle_proto[i].image);
	  sprintf(buf3,"autoproto_%04d.pgm",i);
	  YARPImageFile::Write(buf3,oracle_proto[i].mask);
	  os << i << " " << buf << " " << buf2 << " blank.txt" << endl;
	}
    }
}


void YARPSpotter::PruneOverlap()
{
  for (int i=0; i<oracle_count; i++)
    {
      //printf("Assign begins\n"); fflush(stdout);
      oracle_bank2[i] = oracle_bank[i];
      //printf("Assign ends\n"); fflush(stdout);
      for (int j=0; j<oracle_count; j++)
	{
	  if (i!=j)
	    {
	      printf("Compare %d and %d\n", i, j);
	      oracle_bank2[i].Compare(oracle_bank[j]);
	    }
	}
    }
  for (int i=0; i<oracle_count; i++)
    {
      oracle_bank[i] = oracle_bank2[i];
    }
}


void YARPSpotter::CalibrateItems()
{
 for (int i=0; i<oracle_count; i++)
    {
      //      oracle_scale[i] = oracle_bank[i].GetCount();
#ifndef CALIBRATE_THETA
      //oracle_scale[i] *= oracle_mean[i];
#endif
    }
}



void YARPSpotter::Test(YARPImageOf<YarpPixelBGR>& src,
		       YARPImageOf<YarpPixelBGR>& dest,
		       int training)
{
  mutex.Wait();
  g_use_graph = use_graph;
  g_found = 0;
  g_training = training;
  YARPImageOf<YarpPixelMono> mask;
#ifdef RESEGMENT
  Process(src,mask,dest,2);
#else
  Process(src,mask,dest,1);
#endif
  mutex.Post();
}


void YARPSpotter::SetLabel(const char *str)
{
  char buf[256];
  strncpy(buf,str,sizeof(buf));
  char *last_slash = buf;
  char *scan = buf;
  while (*scan!='\0')
    {
      if (*scan=='/')
	{
	  last_slash = scan;
	}
      if (*scan=='.')
	{
	  *scan = '\0';
	}
      else
	{
	  scan++;
	}
    }
  if (*last_slash=='/')
    {
      last_slash++;
    }
  strncpy(name_label,last_slash,sizeof(name_label));
}










#define MAXARG 10
#define MAXLEN 256




class FileProcessor
{
public:
  virtual void Apply(int argc, char *argv[])
    {
    }

  void Process(const char *fname)
    {
      ifstream fin(fname);
      char buf[MAXLEN];
      int savable = 0;
      while ((!fin.eof()) && (!fin.bad()))
	{
	  fin.getline(buf,sizeof(buf));
	  if (!fin.eof())
	    {
	      savable = 1;
	      //printf("{%s}\n", buf);
	      char bufs[MAXARG][MAXLEN];
	      char *bufs2[MAXARG];
	      bufs2[0] = "";
	      istrstream sin(buf);
	      int index = 1;
	      while ((!sin.eof()) && (!sin.bad()) && index<MAXARG)
		{
		  bufs[index][0] = '\0';
		  sin >> (&bufs[index][0]);
		  bufs2[index] = (&bufs[index][0]);
		  if (!sin.bad())
		    {
		      if (bufs[index][0]!='\0')
			{
			  index++;
			}
		    }
		}
#if 1
	      printf("Call: ");
	      for (int i=1; i<index; i++)
		{
		  printf("[%s] ", bufs[i]);
		}
	      printf("\n");
#endif
	      Apply(index, bufs2);
	    }
	}
    }
};


class FPOracle : public FileProcessor
{
public:
  YARPSpotter& spot;

  FPOracle(YARPSpotter& n_spot) : spot(n_spot) {}

  virtual void Apply(int argc, char *argv[])
    {
      assert(argc==5);
      argc--;
      argv++;
      spot.AddItem(atoi(argv[0]), argv[1], argv[2], argv[3]);
      //AddOracle(0,"train0.txt","train0.ppm","mean0.txt");
    }
};


void YARPSpotter::AddItems(const char *file_name)
{
  FPOracle fp_oracle(*this);
  fp_oracle.Process(file_name);
}


int YARPSpotter::Found()
{
  return g_found;
}

int YARPSpotter::GetX()
{
  return g_found_x;
}

int YARPSpotter::GetY()
{
  return g_found_y;
}

int YARPSpotter::GetR()
{
  return g_found_r;
}

int YARPSpotter::GetID()
{
  return g_found_id;
}

void YARPSpotter::SetTarget(int target)
{
  g_target = target;
}


void YARPSpotter::Add(YARPImageOf<YarpPixelBGR>& src,
		      YARPImageOf<YarpPixelMono>& mask)
{
  double best_cmp = -1;
  int best_idx = -1;
  for (int i=0; i<oracle_count; i++)
    {
      double cmp = oracle_bank[i].CompareSnap(src,mask);
      if (cmp>best_cmp)
	{
	  best_cmp = cmp;
	  best_idx = i;
	}
    }
  if (best_cmp>0.7)
    {
      // fine
    }
  else
    {
      // find a new oracle
      if (oracle_count<MAX_ORACLE)
	{
	  best_idx = oracle_count;
	  oracle_count++;
	}
      else
	{
	  // leave best_idx as it is
	}
    }
  if (best_idx>=0)
    {
      printf("*** adding to index %d\n", best_idx);
      oracle_bank[best_idx].AddSnap(src,mask);
      RedirectTrainer(oracle_bank[best_idx]);
      //UpdateOracle(best_idx,src,mask);
      YARPImageOf<YarpPixelBGR> dest;
      Train(src,mask,dest);
      Test(src,dest,1);
      Test(src,dest,2);
      if (best_cmp>0.9 || oracle_proto[best_idx].image.GetWidth()==0)
	{
	  oracle_proto[best_idx].Set(src,mask);
	}
    }
}



YARPImageOf<YarpPixelBGR>& YARPSpotter::GetPrecedent()
{
  return g_precedent;
}

YARPImageOf<YarpPixelMono>& YARPSpotter::GetPrecedentMask()
{
  return g_precedent_mask;
}


