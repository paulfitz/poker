#include "YARPImage.h"
#include "YARPImageFile.h"
#include "YARPImageDraw.h"
#include "YARPRandomNumber.h"
#include "graph.h"

#include <yarp/os/all.h>

#define ORIGINAL

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

void test_optim(YARPImageOf<YarpPixelBGR>& src, 
		YARPImageOf<YarpPixelBGR>& dest)
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
		  g->set_tweights(nodes[i][j], LINE_STRENGTH*0.0,
		  LINE_STRENGTH*0.00);
		  //g->set_tweights(nodes[i][j], LINE_STRENGTH*0.1,
		  //LINE_STRENGTH*0.00);
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
}




double rnd() {
  return YARPRandom::ranf();
}

void generate_test_case(int mode) {
  YARPImageOf<YarpPixelBGR> src, dest;
  src.Resize(128,128);
  src.Zero();
  if (mode & 1)
    {
      // some background noise
      IMGFOR(src,x,y)
	{
	  if (rnd()<0.01)
	    {
	      if (rnd()<0.5)
		{
		  src(x,y).r = 255;
		}
	      else
		{
		  src(x,y).g = 255;
		}
	    }
	}
    }
  float cert = 0.5;
  if (mode & 2)
    {
      int off1 = +0;
      for (int i=20; i<50; i++)
	{
	  for (int j=35; j<37; j++)
	    {
	      if (rnd()<cert) src(i,j+off1).r = 255;
	    }
	  for (int j=55; j<57; j++)
	    {
	      if (rnd()<cert) src(i,j+off1).r = 255;
	    }

	  if (mode & 4)
	    {
	      // lower block
	      int off = +0;
	      for (int j=85; j<87; j++)
		{
		  if (rnd()<cert) src(i,j+off).r = 255;
		}
	      for (int j=95; j<97; j++)
		{
		  if (rnd()<cert) src(i,j+off).r = 255;
		}
	    }
	}
    }

  if (mode & 8)
    {
      // negation
      int offn = -1;
      int str = 3;
      for (int i=30; i<40+str; i++)
	{
	  for (int j=65; j<67; j++)
	    {
	      if (rnd()<0.5) src(i,j+offn).g = 255;
	    }
	}
    }

  if (mode & 16)
    {
      for (int i=15; i<17; i++)
	{
	  for (int j=43; j<46; j++)
	    {
	      src(i,j).r = 255;
	    }
	}
    }


  if (mode & 32)
    {
      // side bar
      for (int j=100; j<120; j++)
	{
	  for (int i=90; i<92; i++)
	    {
	      if (rnd()<0.3) src(i,j).r = 255;
	    }
	  for (int i=100; i<102; i++)
	    {
	      if (rnd()<0.3) src(i,j).r = 255;
	    }
	}
    }
  test_optim(src,dest);
  int grey = 255;
  IMGFOR(src,x,y)
    {
      int t = src(x,y).r;
      src(x,y).r = src(x,y).g;
      src(x,y).g = t;
      if ((src(x,y).r||src(x,y).g||src(x,y).b))
	{
	  if (src(x,y).b == 0)
	    {
	      src(x,y).r /= 1.5;
	      src(x,y).g /= 1.5;
	    }
	}
      t = dest(x,y).r;
      dest(x,y).r = dest(x,y).g;
      dest(x,y).g = t;
      if ((dest(x,y).r||dest(x,y).g||dest(x,y).b))
	{
	  if (src(x,y).b == 0)
	    {
	      dest(x,y).r /= 1.5;
	      dest(x,y).g /= 1.5;
	    }
	}
    }
  IMGFOR(src,x,y)
    {
      if (!(src(x,y).r||src(x,y).g||src(x,y).b))
	{
	  src(x,y).r = src(x,y).g = src(x,y).b = grey;
	}
      if (!(dest(x,y).r||dest(x,y).g||dest(x,y).b))
	{
	  dest(x,y).r = dest(x,y).g = dest(x,y).b = grey;
	}
    }
  char buf[256];
  sprintf(buf,"case_%03d_in.ppm",mode);
  YARPImageFile::Write(buf,src);
  printf("Wrote %s\n", buf);
  sprintf(buf,"case_%03d_out.ppm",mode);
  YARPImageFile::Write(buf,dest);
  printf("Wrote %s\n", buf);
}

int main() {
  int N = 1;
  int T = 2;
  int L = 4;
  int B = 8;
  int S = 16;
  int F = 32;

  int cases[] = {
    T,
    T+S,
    T+L+S+F,
    T+L+S+B+F,
    T+L+S+B+N+F
  };

  for (int i=0; i<sizeof(cases)/sizeof(int); i++) {
    generate_test_case(cases[i]);
  }

  return 0;
}

