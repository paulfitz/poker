#include "YARPVisualSearch.h"
#include "YARPColorSpaceTool.h"
#include "YARPImageDraw.h"
#include "YARPRandomNumber.h"
#include "IntegralImageTool.h"
#include "YARPShapeBoundary.h"
#include "YARPImageFile.h"

//#define CYL_BUCKETS 9 /* TWEAKED */
//#define CYL2_BUCKETS (CYL_BUCKETS)
//#define LUM_BUCKETS 5 /* TWEAKED */
//#define VAR_BUCKETS 1 /* TWEAKED */
#define COLOR_TYPE 0 /* TWEAKED */
#define COMPARISON_MODE 0 /* TWEAKED */
#define NORMALIZE_LOWER 1

// color_type can be 0 or 1

#define CYL_BUCKETS 6
#define CYL2_BUCKETS 6
#define LUM_BUCKETS 3
#define VAR_BUCKETS 1

#define TOP_VARIANCE 50 /* TWEAKED */
#define VAR_WINDOW 2 /* TWEAKED */

#define RGBtoCYL my_color

void my_color(float r, float g, float b, float& c1, float& c2, float& c3)
{
  r /= 256;
  g /= 256;
  b /= 256;
  c3 = (r+g+b);
#if COLOR_TYPE == 0
  c1 = r-g;
  c2 = r-b;
#else
  c1 = (r-g)/(c3+0.0001);
  c2 = (r-b)/(c3+0.0001);
  /*
  if (c1>1 || c1<-1 || c2>1 || c2<-1) 
    {
      printf(">>>>>>>>> %g %g %g // %g %g\n", r, g, b, c1, c2);
    }
  */
#endif
  c3 /= 3.0;
//  printf("%g %g %g\n", c1, c2, c3);
}

template <class T>
T max(T x, T y)
{
  return (x>y)?x:y;
}

template <class T>
T min(T x, T y)
{
  return (x<y)?x:y;
}

template <class T>
T square(T x)
{
  return x*x;
}

void RotateTemplate(YARPImageOf<YarpPixelFloat>& src, 
		    YARPImageOf<YarpPixelFloat>& dest,
		    float angle,
		    int x, int y, int scale)
{
  float cosa = cos(angle);
  float sina = sin(angle);
  float SQUEEZE = 4.0/3.0;
  float threshold = scale*scale;
  for (int xx=x-scale; xx<=x+scale; xx++)
    {
      for (int yy=y-scale; yy<=y+scale; yy++)
	{
	  float delta = (xx-x)*(xx-x)+(yy-y)*(yy-y);
	  if (delta<=threshold)
	    {
	      int xx2 = (int)(0.5+x+((SQUEEZE*(xx-x)*cosa-(yy-y)*sina)/SQUEEZE));
	      int yy2 = (int)(0.5+(y+SQUEEZE*(xx-x)*sina+(yy-y)*cosa));
	      dest.SafePixel(xx,yy) = src.SafePixel(xx2,yy2);
	    }
	}
    }
}

static float AlignBlobHelper(YARPImageOf<YarpPixelFloat>& src,
			     YARPImageOf<YarpPixelFloat>& ref,
			     YARPImageOf<YarpPixelFloat>& dest_aux,
			     int& xx, int& yy, int rad, int step,
			     const char *msg)
{
  float best = 0.0; //999999999999999999.0;
  int bx = xx, by = yy;
  IntegralImageTool ii_tool;
  for (int dx=xx-rad; dx<=xx+rad; dx+=step)
    {
      for (int dy=yy-rad; dy<=yy+rad; dy+=step)
	{
	  dest_aux.Zero();
	  ii_tool.Offset(src,dest_aux,dx,dy);
	  float total = 0;
	  IMGFOR(dest_aux,x,y)
	    {
	      float q = fabs(dest_aux(x,y));
	      total += q*(ref(x,y)>128);
	    }
	  //printf("%g %d %d\n", total, dx, dy);
	  if (total>best)
	    {
	      best = total;
	      bx = dx;
	      by = dy;
	    }
	}
    }
  //printf("*** %s scale: %d %d  (rad %d step %d)\n", msg, bx, by, rad, step);
  xx = bx;
  yy = by;
  return best;
}


static float AlignBlobHelper2(YARPImageOf<YarpPixelFloat>& src,
			      YARPShapeBoundary& sb,
			      YARPImageOf<YarpPixelFloat>& dest_aux,
			      float theta, float del, float step)
{
  float btheta = theta;
  float best = 0.0;
  YARPImageOf<YarpPixelMono> work;
  for (float th = -del; th<= +del; th += step)
    {
      YARPShapeBoundary sb2 = sb;
      sb2.RotateAboutCenter(th);
      dest_aux.Zero();
      SatisfySize(src,work);
      sb2.GetMask(work);
      float total = 0;
      IMGFOR(dest_aux,x,y)
	{
	  float q = fabs(src(x,y));
	  total += q*(work(x,y)>128);
	}
      //printf("%g %d %d\n", total, dx, dy);
      if (total>best)
	{
	  best = total;
	  btheta = th;
	}
   }

  printf("*** angle %g (%g)\n", btheta, best);

  return btheta;
}



class YARPLocalTarget
{
public:
  float c1, c2;
  float x, y, dx, dy;

  int hist[CYL_BUCKETS][CYL2_BUCKETS][LUM_BUCKETS][VAR_BUCKETS];
  float hist_count;

  void ResetHist()
    {
      for (int i=0; i<CYL_BUCKETS; i++)
	{
	  for (int j=0; j<CYL2_BUCKETS; j++)
	    {
	      for (int k=0; k<LUM_BUCKETS; k++)
		{
		  for (int l=0; l<VAR_BUCKETS; l++)
		    {
		      hist[i][j][k][l] = 0;
		    }
		}
	    }
	}
      hist_count = 0;
    }

  void Show()
    {
      for (int i=0; i<CYL_BUCKETS; i++)
	{
	  for (int j=0; j<CYL2_BUCKETS; j++)
	    {
	      for (int k=0; k<LUM_BUCKETS; k++)
		{
		  for (int l=0; l<VAR_BUCKETS; l++)
		    {
		      printf("%d ", hist[i][j][k][l]);
		    }
		}
	    }
	}
      printf("\n");
    }

  void AddHist(float c1, float c2, float c3, float var)
    {
      int idx1 = (int)(((c1+1)/2.0)*CYL_BUCKETS);
      int idx2 = (int)(((c2+1)/2.0)*CYL2_BUCKETS);
      int idx3 = (int)(c3*LUM_BUCKETS);
      int idx4 = (int)((var/TOP_VARIANCE)*VAR_BUCKETS);
      idx4 = max(idx4,0);
      /*
      if (!(idx1>=0 && idx2>=0 && idx3>=0 && idx4>=0))
	{
	  printf(">>>>>>>>>> %g %g %g %g\n", c1, c2, c3, var);
	  fflush(stdout);
	}
      */
      assert(idx1>=0 && idx2>=0 && idx3>=0 && idx4>=0);
      if (idx1>=CYL_BUCKETS) idx1 = CYL_BUCKETS-1;
      if (idx2>=CYL2_BUCKETS) idx2 = CYL2_BUCKETS-1;
      if (idx3>=LUM_BUCKETS) idx3 = LUM_BUCKETS-1;
      if (idx4>=VAR_BUCKETS) idx4 = VAR_BUCKETS-1;
      //printf("%d %d\n", idx1, idx2);
      hist[idx1][idx2][idx3][idx4]++;
      hist_count++;
    }

  float GetHist(float c1, float c2, float c3, float var)
    {
      int idx1 = (int)(((c1+1)/2.0)*CYL_BUCKETS);
      int idx2 = (int)(((c2+1)/2.0)*CYL2_BUCKETS);
      int idx3 = (int)(c3*LUM_BUCKETS);
      int idx4 = (int)((var/TOP_VARIANCE)*VAR_BUCKETS);
      assert(idx1>=0 && idx2>=0 && idx3>=0 && idx4>=0);
      if (idx1>=CYL_BUCKETS) idx1 = CYL_BUCKETS-1;
      if (idx2>=CYL2_BUCKETS) idx2 = CYL2_BUCKETS-1;
      if (idx3>=LUM_BUCKETS) idx3 = LUM_BUCKETS-1;
      if (idx4>=VAR_BUCKETS) idx4 = VAR_BUCKETS-1;
      //printf("%d %d\n", idx1, idx2);
      return hist[idx1][idx2][idx3][idx4];
    }

  float GetHistTotal()
  {
    return hist_count;
  }


  void Sample(YARPImageOf<YarpPixelBGR>& src,
	      float s_x, float s_y, float s_dx, float s_dy);

  float Compare(YARPLocalTarget& target);
};

void YARPLocalTarget::Sample(YARPImageOf<YarpPixelBGR>& src,
			     float s_x, float s_y, float s_dx, float s_dy)
{
//  int not_supported = 1;
//  assert(not_supported==0);
  ResetHist();
  x = s_x;
  y = s_y;
  dx = s_dx;
  dy = s_dy;
  c1 = c2 = 0;
//  float rad = sqrt(s_area/M_PI);
  for (int x=max((int)(s_x-dx),0); 
       x<=min((int)(s_x+dx),src.GetWidth()-1); x++)
    {
      for (int y=max((int)(s_y-dy),0); 
	   y<=min((int)(s_y+dy),src.GetHeight()-1); y++)
	{
//	  if (square(x-s_x)+square(y-s_y)<=rad*rad)
	    {
	      YarpPixelBGR& pix = src(x,y);
	      float nc1, nc2, nc3;
	      RGBtoCYL(pix.r,pix.g,pix.b,nc1,nc2,nc3);
	      AddHist(nc1,nc2,nc3,0);
	    }
	}
    }
}






#include <math.h>

void nrerror(char error_text[])
{
  fprintf(stderr,error_text);
  exit(1);
}

float gammln(float xx)
{
  double x,y,tmp,ser;
  static double cof[6]={76.18009172947146,-86.50532032941677,
			24.01409824083091,-1.231739572450155,
			0.1208650973866179e-2,-0.5395239384953e-5};
  int j;
  y=x=xx;
  tmp=x+5.5;
  tmp -= (x+0.5)*log(tmp);
  ser=1.000000000190015;
  for (j=0;j<=5;j++) ser += cof[j]/++y;
  return -tmp+log(2.5066282746310005*ser/x);
}


#define ITMAX 100
#define EPS 3.0e-7
void gser(float *gamser, float a, float x, float *gln)
{
  int n;
  float sum,del,ap;
  *gln=gammln(a);
  if (x <= 0.0) {
    if (x < 0.0) nrerror("x less than 0 in routine gser");
    *gamser=0.0;
    return;
  } else {
    ap=a;
    del=sum=1.0/a;
    for (n=1;n<=ITMAX;n++) {
      ++ap;
      del *= x/ap;
      sum += del;
      if (fabs(del) < fabs(sum)*EPS) {
	*gamser=sum*exp(-x+a*log(x)-(*gln));
	return;
      }
    }
    nrerror("a too large, ITMAX too small in routine gser");
    return;
  }
}

#define FPMIN 1.0e-30
void gcf(float *gammcf, float a, float x, float *gln)
{
  int i;
  float an,b,c,d,del,h;
  *gln=gammln(a);
  b=x+1.0-a;
  c=1.0/FPMIN;
  d=1.0/b;
  h=d;
  for (i=1;i<=ITMAX;i++) {
    an = -i*(i-a);
    b += 2.0;
    d=an*d+b;
    if (fabs(d) < FPMIN) d=FPMIN;
    c=b+an/c;
    if (fabs(c) < FPMIN) c=FPMIN;
    d=1.0/d;
    del=d*c;
    h *= del;
    if (fabs(del-1.0) < EPS) break;
  }
  if (i > ITMAX) nrerror("a too large, ITMAX too small in gcf");
  *gammcf=exp(-x+a*log(x)-(*gln))*h;
}

float gammq(float a, float x)
{
  float gamser,gammcf,gln;
  if (x < 0.0 || a <= 0.0) return 0.0; //nrerror("Invalid arguments in routine gammq");
  if (x < (a+1.0)) { //Use the series representation
    gser(&gamser,a,x,&gln);
    return 1.0-gamser; //and take its complement.
  } else { //Use the continued fraction representation.
    gcf(&gammcf,a,x,&gln);
    return gammcf;
  }
}
/*
Given the arrays bins1[1..nbins] and bins2[1..nbins],containin two sets of binned
data,and given the number of constraints knstrn (normally 1 or 0 ,this routine returns the
number of degrees of freedom df the chi-square chsq and the si ni ?cance prob A small value
of prob indicates a signi ?cant di ?erence between the distributions bins1 and bins2 Notethat
bins1 and bins2 are both float arrays,although they will normally contain integer values.
*/

float no_info = 0.1;

void chstwo(float bins1[], float bins2[], int nbins, int knstrn, float *df,
	    float *chsq, float *prob)
{
  int j;
  float temp;
  *df=nbins-knstrn;
  *chsq=0.0;
#define NO_INFO (no_info)
  for (j=1;j<=nbins;j++)
    if (bins1[j] <= NO_INFO && bins2[j] <= NO_INFO)
      --(*df);   //No data means one less degree of freedom.
    else {
      temp=bins1[j]-bins2[j];
      *chsq += temp*temp/(bins1[j]+bins2[j]);
    }
  *prob=gammq(0.5*(*df),0.5*(*chsq));  //Chi-square probability function.See § 6.2.
}



// target is REFERENCE image
float YARPLocalTarget::Compare(YARPLocalTarget& target)
{
  float result = 0;
  float ref = 0;
  float hc = max(1,(int)(hist_count+0.5));
  float target_hc = max(1,(int)(target.hist_count+0.5));
#if COMPARISON_MODE == 2
#define NBINS (CYL_BUCKETS*CYL2_BUCKETS*LUM_BUCKETS*VAR_BUCKETS)
  float data1[NBINS], data2[NBINS];
  int *hist1 = (&hist[0][0][0][0]);
  int *hist2 = (&target.hist[0][0][0][0]);
  float max1 = 0, max2 = 0;
  for (int i=0; i<NBINS; i++)
    {
      data1[i] = hist1[i]/hc;
      max1 = max(max1,data1[i]);
      data2[i] = hist2[i]/target_hc;
      max2 = max(max2,data2[i]);
    }
  no_info = (min(max1,max2)/10);
  float df;
  float chsq;
  float prob;
  chstwo(data1,data2,NBINS,1,&df,&chsq,&prob);
  result = prob;
#define PROB_FACTOR 100 /* TWEAKED */
  result = (1-prob)*PROB_FACTOR;
  if (result>1) result = 1;
  result = 1-result;
#else
  for (int i=0; i<CYL_BUCKETS; i++)
    {
      for (int j=0; j<CYL2_BUCKETS; j++)
	{
	  for (int k=0; k<LUM_BUCKETS; k++)
	    {
	      for (int l=0; l<VAR_BUCKETS; l++)
		{
		  float v0 = hist[i][j][k][l]/hc;
		  float v1 = target.hist[i][j][k][l]/target_hc;
#if COMPARISON_MODE == 0
		  //float w = min(v0,v1); //max(v0,v1)-min(v0,v1);
		  float w = min(v0,v1); //max(v0,v1)-min(v0,v1);
#elif COMPARISON_MODE == 1
		  assert(v0>=0 && v1>=0);
#define PARAM_C1_A 5 /* TWEAKED */
#define PARAM_C1_B 1 /* TWEAKED */
		  float w = log(PARAM_C1_A*min(v0,v1)+PARAM_C1_B);
		  ref += log(PARAM_C1_A*max(v0,v1)+PARAM_C1_B);
#endif
		  result += w;
		}
	    }
	}
    }
#endif
  //  result /= (CYL_BUCKETS*CYL_BUCKETS);
  //result /= max(target_hc,hc);
  //printf("Result is %g %g %g\n", result, hc, target_hc);
  //result /= max(1,(int)(target.hist_count+0.5));

#if COMPARISON_MODE == 1
  result /= (ref+0.0001);
#endif

  //result /= 2;
  return result;
}

class YARPVisualSearchHelper
{
public:
  int target_set;
  int background_set;
  YARPLocalTarget target;
  YARPLocalTarget background;
  YARPLocalTarget aux_target;
  YARPImageOf<YarpPixelFloat> chosen;
  YARPImageOf<YarpPixelBGR> original;
  float saturation_measure;

  YARPVisualSearchHelper() { Reset(); }


  void Show()
    {
      target.Show();
    }

  void Reset() { background_set = target_set = 0; saturation_measure = 0; }

  int IsActive()
  { return target_set; }

  // add a target; by default will be searched for
  int AddPartial(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelFloat>& mask, YARPImageOf<YarpPixelFloat>& var,int fore=1,int dx=0, int dy=0, float factor=1);

  int Add(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelFloat>& mask,
	  int dx, int dy, int fore_only = 0, float factor = 1);

  int Add(YARPImageOf<YarpPixelBGR>& src, float x, float y, float dx,
	  float dy);
  
  // search for previously set target
  int Apply(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest);
  
  float Apply(YARPImageOf<YarpPixelBGR>& src, YARPImageOf<YarpPixelBGR>& dest,
	      float x, float y, float dx=-1, float dy=-1);

  void BackProject(YARPImageOf<YarpPixelBGR>& src, 
		   YARPImageOf<YarpPixelFloat>& dest);

  void Align(YARPImageOf<YarpPixelFloat>& src,
	     YARPImageOf<YarpPixelFloat>& dest);

  float Compare(YARPVisualSearchHelper& alt);

  void Opportunistic(YARPImageOf<YarpPixelBGR>& src, 
		     YARPImageOf<YarpPixelFloat>& mask, 
		     YARPImageOf<YarpPixelBGR>& dest);

  float x_local, y_local, v_local;

  float Localize(YARPImageOf<YarpPixelBGR>& src,
		 YARPImageOf<YarpPixelBGR>& dest,
		 float& x,float& y,float radius);

  float Orient(YARPImageOf<YarpPixelBGR>& src,
	       YARPImageOf<YarpPixelBGR>& dest,
	       float& x, float& y, float& theta);

  float AlignBlob(YARPImageOf<YarpPixelBGR>& real_src,
		  YARPImageOf<YarpPixelFloat>& src,
		  YARPImageOf<YarpPixelFloat>& ref,
		  YARPImageOf<YarpPixelFloat>& dest_aux,
		  YARPImageOf<YarpPixelBGR>& dest,
		  float xx, float yy, float radius,
		  float xref, float yref,
		  int *ext_dx = NULL,
		  int *ext_dy = NULL,
		  int active = 1,
		  double theta = 0);

};


float YARPVisualSearchHelper::AlignBlob(
		      YARPImageOf<YarpPixelBGR>& real_src,
		       YARPImageOf<YarpPixelFloat>& src,
		      YARPImageOf<YarpPixelFloat>& ref,
		      YARPImageOf<YarpPixelFloat>& dest_aux,
		      YARPImageOf<YarpPixelBGR>& dest,
		      float xx, float yy, float radius,
		      float xref, float yref,
		      int *ext_dx,
		      int *ext_dy,
		      int active,
		      double theta)
{
  int value_set = 0;
  YARPImageOf<YarpPixelMono> mono;
  YARPImageOf<YarpPixelFloat> ref2;
  mono.CastCopy(ref);

  IntegralImageTool ii_tool;
  YARPImageOf<YarpPixelFloat> work;
  SatisfySize(src,work);
  SatisfySize(src,dest_aux);
  float best = 0.0; //999999999999999999.0;
  int bx = 0, by = 0;
  if (active)
    {
      float dth;
      //printf("Scanning image for object...\n");
      if (radius>src.GetWidth())
	{
	  best = AlignBlobHelper(src,ref,dest_aux,bx,by,60,6,"coarse");
	  best = AlignBlobHelper(src,ref,dest_aux,bx,by,10,3,"medium");
	  best = AlignBlobHelper(src,ref,dest_aux,bx,by,2,1,"fine");
	}
      else
	{
	  bx = (int)xref-src.GetWidth()/2;
	  by = (int)yref-src.GetHeight()/2;

	  int rad = (int)radius;
	  int step = (int)(radius/4);
	  while (step>=1)
	    {
	      best = AlignBlobHelper(src,ref,dest_aux,bx,by,rad,step,"adaptive");
	      rad = step+1;
	      step = (int)(0.5+rad/4);
	    }
	  //best = AlignBlobHelper(src,ref,dest_aux,bx,by,3,1,"fine");
	}
      x_local = bx + src.GetWidth()/2;
      y_local = by + src.GetHeight()/2;

#if 0
      YARPVisualSearchHelper seeker2;
      seeker2.Add(real_src,ref,bx,by);
      v_local = Compare(seeker2);
      //printf("value here is %g\n", v_local);
      value_set = 1;
#endif
      //      printf("Omitting search in orientation\n");

#if 0
      YARPShapeBoundary sb;
      sb.SetFromMask(ref);
      sb.RotateAboutCenter(theta);
      dth = AlignBlobHelper2(src,sb,dest_aux,theta,1.0,0.025);
      sb.RotateAboutCenter(dth);
      sb.GetMask(mono);  ref2.CastCopy(mono);
      AlignBlobHelper(src,ref2,dest_aux,bx,by,2,1);
      dth = AlignBlobHelper2(src,sb,dest_aux,theta+dth,0.05,0.005);
      sb.RotateAboutCenter(dth);
      sb.GetMask(mono);  ref2.CastCopy(mono);
#else
      ref2.CastCopy(ref);
#endif
    }
  else
    {
      assert(1==0);
      if (ext_dx!=NULL) bx = (*ext_dx);
      if (ext_dy!=NULL) by = (*ext_dy);
    }
  //dest.CastCopy(src);
  //work.Zero();

  dest_aux.Zero();
  ii_tool.Offset(ref2,dest_aux,-bx,-by);
  bx = -bx;

  IMGFOR(dest,x,y)
    {
      
      if (dest_aux(x,y)<128)
	{
	  //	  if (((x+y)%7)==0)
	    {
	      dest(x,y).r /= 4;
	      dest(x,y).g /= 4;
	      dest(x,y).b /= 4;
	    }
	}
      else
	{
	  dest(x,y) = original.SafePixel(x-bx,y-by);
	}
      //if (src(x,y)>128)
      //{
      //dest(x,y).r = 255;
      //dest(x,y).b = 255;
      //}
    }

  //printf(">>> %d %d // %g %g\n", bx, by, xref, yref);
  int tx = (int)(xx+bx);
  int ty = (int)(yy+by);
  YarpPixelBGR pixr(255,0,0);
  YarpPixelBGR pixg(0,255,0);
  AddCircleOutline(dest,pixr,tx,ty,5);
  AddCircleOutline(dest,pixg,tx,ty,6);
  AddCircleOutline(dest,pixr,tx,ty,7);
  AddCrossHair(dest,pixg,tx,ty,7);

  if (ext_dx!=NULL) (*ext_dx) = (int)(bx+0.5);
  if (ext_dy!=NULL) (*ext_dy) = (int)(by+0.5);

//  dest_aux.CastCopy(dest);

  if (!value_set)
    {
      float area = 0;
      IMGFOR(ref,x3,y3)
	{
	  area += (ref(x3,y3));
	}
      
      v_local = (best/(area+0.01));
    }

  static int ct = 0;
  char buf[256];
  sprintf(buf,"BP%04d.ppm",ct);
  YARPImageFile::Write(buf,dest);
  ct++;

  
  return v_local;
}



void YARPVisualSearchHelper::Opportunistic(YARPImageOf<YarpPixelBGR>& src, 
		   YARPImageOf<YarpPixelFloat>& mask, 
		   YARPImageOf<YarpPixelBGR>& dest)
{
  printf("::: opportunistic analysis\n");
  // one good tool is containment.  Search for a well-contained blob
}

void YARPVisualSearchHelper::BackProject(YARPImageOf<YarpPixelBGR>& src, 
					 YARPImageOf<YarpPixelFloat>& dest)
{
  SatisfySize(src,dest);
  //dest.CastCopy(src);
  float c1, c2, c3;
  
  float fct = target.GetHistTotal();
  float bct = background.GetHistTotal();

  YARPImageOf<YarpPixelFloat> var, work2;
  work2.CastCopy(src);
  SatisfySize(work2,var);
  IntegralImageTool ii_tool;
  ii_tool.GetVariance(work2,var,VAR_WINDOW);

  IMGFOR(dest,x,y)
    {
      YarpPixelBGR& in = src(x,y);
      YarpPixelFloat& out = dest(x,y);
      RGBtoCYL(in.r,in.g,in.b,c1,c2,c3);
      float fore = target.GetHist(c1,c2,c3,var(x,y));
      float back = background.GetHist(c1,c2,c3,var(x,y));
      
      //out = 255.0*3*fore/(target.GetHistTotal()+1.0);

      //out = 255.0*(fore/(1+fct)-back/(1+bct));
      //---> IDEALLY USE THIS!  need back
      out = 255.0*(fore-back)/(fore+back+1);

      if (out>255) out = 255;
      if (out<0) out = 0;
    }

  static int ct = 0;
  char buf[256];
  sprintf(buf,"bp%04d.ppm",ct);
  YARPImageOf<YarpPixelMono> mono;
  mono.CastCopy(dest);
  YARPImageFile::Write(buf,mono);
  ct++;
}


float YARPVisualSearchHelper::Localize(YARPImageOf<YarpPixelBGR>& src,
				       YARPImageOf<YarpPixelBGR>& dest,
				       float& x,float& y,float radius)
{
  YARPImageOf<YarpPixelFloat> back_proj;
  x_local = src.GetWidth()/2;
  y_local = src.GetHeight()/2;
  v_local = 1;
  BackProject(src,back_proj);
  YARPImageOf<YarpPixelFloat> work;
  work.PeerCopy(back_proj);
  AlignBlob(src,work,chosen,back_proj,dest,target.x,target.y,radius,x,y);
  x = x_local;
  y = y_local;
  return v_local;
}


float YARPVisualSearchHelper::Orient(YARPImageOf<YarpPixelBGR>& src,
				     YARPImageOf<YarpPixelBGR>& dest,
				     float& x, float& y, float& theta)
{
  YARPImageOf<YarpPixelFloat> back_proj;
  x_local = src.GetWidth()/2;
  y_local = src.GetHeight()/2;
  v_local = 1;
  BackProject(src,back_proj);
  YARPImageOf<YarpPixelFloat> work, ref2;
  YARPImageOf<YarpPixelMono> mono;
  work.PeerCopy(back_proj);
  
  //AlignBlob(src,work,chosen,back_proj,dest,target.x,target.y,radius,x,y);
  //x = x_local;
  //y = y_local;
  //return v_local;

  char buf[256];

  float dth = 0;
  int bx = -(int)(x-target.x), by = (int)(y-target.y);
  int rx = 0, ry = 0;
  int ox = bx, oy = by;
  theta = 0;

  YARPShapeBoundary sb;
  mono.CastCopy(chosen);
  sb.SetFromMask(mono);
  sb.RotateAboutCenter(theta);
  sb.Translate(-x+target.x,y-target.y);
  for (int i=0; i<10; i++)
    {
      static int ct = 0;
      sprintf(buf,"sample%04d.pgm",ct);
      YARPImageFile::Write(buf,mono);
      ct++;

      dth = AlignBlobHelper2(work,sb,back_proj,theta,1.6,0.001);
      sb.RotateAboutCenter(dth);
      theta += dth;

      sb.GetMask(mono);  ref2.CastCopy(mono);

      int dx = 0, dy = 0;
      AlignBlobHelper(work,ref2,back_proj,dx,dy,8,1,"orient");
      dx = -dx;
      sb.Translate(dx,dy);
      printf("nudge %d %d\n", dx, dy);
    }
  //sb.GetMask(mono);  
  //ref2.CastCopy(mono);
  //sb.Show(dest);
  dest.CastCopy(back_proj);
  sb.Show(dest,0);
  return 0;
}




void YARPVisualSearchHelper::Align(YARPImageOf<YarpPixelFloat>& src,
				   YARPImageOf<YarpPixelFloat>& dest)
{
  //AlignBlob(src,dest);
}


int YARPVisualSearchHelper::Add(YARPImageOf<YarpPixelBGR>& src, 
				YARPImageOf<YarpPixelFloat>& mask,
				int dx, int dy, int fore_only, float factor)
{
  chosen.PeerCopy(mask);
  YARPImageOf<YarpPixelFloat> var, work;
  original.PeerCopy(src);
  work.CastCopy(src);
  SatisfySize(work,var);
  IntegralImageTool ii_tool;
  var.Zero();
  ii_tool.GetVariance(work,var,VAR_WINDOW);

  /*
  int ct1=0, ct2=0;
  IMGFOR(var,x,y)
    {
      if (var(x,y)<0)
	{
	  ct1++;
	}
      ct2++;
    }
  printf(">>>> VARs %d (should be 0) versus %d\n", ct1, ct2);
  */


  if (!fore_only)
    {
      AddPartial(src,mask,var,0,dx,dy,1);
    }

  return AddPartial(src,mask,var,1,dx,dy,factor);
}

int YARPVisualSearchHelper::AddPartial(YARPImageOf<YarpPixelBGR>& src, 
				       YARPImageOf<YarpPixelFloat>& mask,
				       YARPImageOf<YarpPixelFloat>& var,
				       int fore, int dx, int dy, float factor)

{
  float total_c1 = 0;
  float total_c2 = 0;
  float total_y = 0;
  float c1,c2,c3;
  float total_x0 =0, total_y0 = 0;
  float total_area = 0;
  float sat_ct = 0;
  float sat_ref = 0;
  YARPLocalTarget& actor = *(fore?(&target):(&background));
  actor.ResetHist();
  mask.NullPixel() = 0;
  int use_factor = 0;
  if (factor<0.9999 || factor>1.0001)
    {
      use_factor = 1;
    }
  float rr, gg, bb;
  IMGFOR(src,x,y)
    {
      float m = mask.SafePixel(x-dx,y-dy);
      if (!fore)
	{
	  m = 255 - m;
	}
      YarpPixelBGR& pix = src(x,y);
      if (!use_factor)
	{
	  
	  RGBtoCYL(pix.r,pix.g,pix.b,c1,c2,c3);
	}
      else
	{
	  rr = min(pix.r*factor,255.0f);
	  gg = min(pix.g*factor,255.0f);
	  bb = min(pix.b*factor,255.0f);
	  RGBtoCYL(rr,gg,bb,c1,c2,c3);
	}
      total_c1 += c1*m;
      total_c2 += c2*m;
      total_x0 += x*m;
      total_y0 += y*m;
      total_y += m;
      total_area += (m>128)?1:0;
      //if (m>0.001) printf("%4d %4d %g:%g/%g\n", x, y, m, c1,c2);
      if (m>128)
	{
	  /*
	  if (var(x,y)<0)
	    {
	      printf(">>>> var warning %d %d %g\n", x, y, var(x,y));
	    }
	  */
	  actor.AddHist(c1,c2,c3,var(x,y));
	  /*
	  if (pix.r>250 || pix.g>250 || pix.b>250)
	    {
	      int top = max(pix.r,max(pix.g,pix.b));
	      unsigned char next = 0;
	      if (pix.r!=top) next = max(next,pix.r);
	      if (pix.g!=top) next = max(next,pix.g);
	      if (pix.b!=top) next = max(next,pix.b);
	      if (next==0) next = top;
	      if (next>200)
		{
		  sat_ct++;
		}
	    }
	  */
	  sat_ct += min(pix.r,min(pix.g,pix.b));
	  sat_ref++;
	}
    }
  if (sat_ref<1) sat_ref = 1;
  sat_ct /= sat_ref;
  if (fore)
    {
      //printf("SATURATION normalization factor %g\n", sat_ct);
      saturation_measure = sat_ct;
    }
  if (total_y<0.001) total_y = 0.001;
  total_c1 /= total_y;
  total_c2 /= total_y;
  total_x0 /= total_y;
  total_y0 /= total_y;
  actor.c1 = total_c1;
  actor.c2 = total_c2;
  actor.x = total_x0;
  actor.y = total_y0;
  actor.dx = sqrt(total_area);
  actor.dy = sqrt(total_area);
  //printf("Set color to %g/%g at %g,%g area %g\n", total_c1, total_c2,
  //total_x0, total_y0, total_area);
  if (fore)
    {
      target_set = 1;
      //aux_target.Sample(src,total_x0,total_y0,sqrt(total_area)/2,
      //sqrt(total_area)/2);
    }
  else
    {
      background_set = 1;
    }
  //target.Sample(src,total_x0,total_y0,total_area/2);
  return 0;
}

int YARPVisualSearchHelper::Add(YARPImageOf<YarpPixelBGR>& src, 
				float x, float y, float dx, float dy)
{
  target_set = 1;
  target.Sample(src,x,y,dx,dy);
  aux_target.Sample(src,x,y,dx/4,dy/4);
  return 0;
}
  

int YARPVisualSearchHelper::Apply(YARPImageOf<YarpPixelBGR>& src, 
				  YARPImageOf<YarpPixelBGR>& dest)
{
  dest.PeerCopy(src);
  if (target_set)
    {
      float c1, c2, c3;
#if 0
      IMGFOR(dest,x,y)
	{
	  if (y==0) printf(">> %d\n",x);
	  YarpPixelBGR& pix = src(x,y);
	  YarpPixelBGR& pix_dest = dest(x,y);
	  //RGBtoCYL(pix.r,pix.g,pix.b,c1,c2,c3);
	  //float diff = fabs(c1-target.c1);
	  //diff *= 1000.0;
	  //if (diff>255) diff = 255;
	  YARPLocalTarget pt;
	  pt.Sample(src,x,y,target.area);
	  float diff = 255.0*pt.Compare(target);
	  //	  diff *= 3;
	  if (diff>255) diff = 255;
	  pix_dest.r = (unsigned char)diff;
	  pix_dest.g = 0;
	  pix_dest.b = 0;
	}
#else
      dest.PeerCopy(src);
      float xx = 64, yy = 64, ss = 64;
      float best_x = xx;
      float best_y = yy;
      float best_diff = 0.0;
      int MAXI = 10;
      for (int i=0; i<=MAXI; i++)
	{
	  printf("i %d\n", i);
	  for (int j=0; j<100; j++)
	    {
	      float x = xx+(1-2*YARPRandom::Uniform())*ss;
	      float y = yy+(1-2*YARPRandom::Uniform())*ss;
	      YarpPixelBGR& pix_dest = dest.SafePixel((int)x,(int)y);
	      YARPLocalTarget pt, pt2;
	      pt.Sample(src,x,y,target.dx,target.dy);
	      pt2.Sample(src,x,y,aux_target.dx,aux_target.dy);
	      float diff = pt.Compare(target);
	      //float diff2 = pt2.Compare(aux_target);
	      //diff = diff+diff2/5;
	      if (diff>best_diff)
		{
		  best_diff = diff;
		  best_x = x;
		  best_y = y;
		}
	      pix_dest.r = 0;
	      pix_dest.g = (255*(i!=MAXI));
	      pix_dest.b = 0;
	    }
	  xx = best_x;
	  yy = best_y;
	  ss *= 0.75;
	}
#endif
    }
  return 0;
}


float YARPVisualSearchHelper::Apply(YARPImageOf<YarpPixelBGR>& src, 
				    YARPImageOf<YarpPixelBGR>& dest,
				    float x, float y, float dx, float dy)
{
  YARPLocalTarget pt;
  pt.Sample(src,x,y,(dx<0)?target.dx:dx,(dy<0)?target.dy:dy);
  //pt2.Sample(src,x,y,aux_target.area);
  float diff = pt.Compare(target);
  return diff;
}


float YARPVisualSearchHelper::Compare(YARPVisualSearchHelper& alt)
{
  float diff = 0;
#if NORMALIZE_LOWER == 1
  //printf("Normalizing lower...\n");
  float sat1 = saturation_measure;
  float sat2 = alt.saturation_measure;
  int swap = (sat2>sat1);
  YARPVisualSearchHelper& src = *(swap?(&alt):this);
  YARPVisualSearchHelper& dest = *(swap?this:(&alt));
  if (swap)
    {
      float tsat = sat1;
      sat1 = sat2;
      sat2 = tsat;
    }
  // the saturation_measure of src is > that of dest
  YARPVisualSearchHelper tmp;
  tmp.Add(dest.original,dest.chosen,0,0,1,(sat1+0.0001)/(sat2+0.0001));
  diff = src.target.Compare(tmp.target);
#else
  diff = alt.target.Compare(target);
#endif
  return diff;
}



////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define RES (*((YARPVisualSearchHelper *)(system_resource)))
#define GETRES(x) (*((YARPVisualSearchHelper *)((x) . system_resource)))

YARPVisualSearch::YARPVisualSearch()
{
  system_resource = NULL;
  system_resource = new YARPVisualSearchHelper;
  assert(system_resource!=NULL);
}


YARPVisualSearch::~YARPVisualSearch()
{
  delete (&RES);
}


void YARPVisualSearch::Reset()
{
  RES.Reset();
}


void YARPVisualSearch::Apply(YARPImageOf<YarpPixelBGR>& src, 
			     YARPImageOf<YarpPixelBGR>& dest)
{
  RES.Apply(src,dest);
}


void YARPVisualSearch::Add(YARPImageOf<YarpPixelBGR>& src,
			   YARPImageOf<YarpPixelFloat>& mask,
			   int dx, int dy)
{
  RES.Add(src,mask,dx,dy);
}

void YARPVisualSearch::Add(YARPImageOf<YarpPixelBGR>& src, 
			   YARPImageOf<YarpPixelMono>& mask,
			   int dx, int dy)
{
  YARPImageOf<YarpPixelFloat> mask2;
  mask2.CastCopy(mask);
  RES.Add(src,mask2,dx,dy);
}

void YARPVisualSearch::Add(YARPImageOf<YarpPixelBGR>& src, 
			   float x, float y, float dx, float dy)
{
  RES.Add(src,x,y,dx,dy);
}

float YARPVisualSearch::Apply(YARPImageOf<YarpPixelBGR>& src, 
			      YARPImageOf<YarpPixelBGR>& dest,
			      float x, float y,float dx,float dy)
{
  return RES.Apply(src,dest,x,y,dx,dy);
}

void YARPVisualSearch::BackProject(YARPImageOf<YarpPixelBGR>& src, 
				   YARPImageOf<YarpPixelFloat>& dest)
{
  //YARPImageOf<YarpPixelBGR> dest2;
  //  SatisfySize(src,dest2);
  RES.BackProject(src,dest);
}

/*
void YARPVisualSearch::BackProject(YARPImageOf<YarpPixelBGR>& src, 
				   YARPImageOf<YarpPixelFloat>& dest_aux,
				   YARPImageOf<YarpPixelBGR>& dest)
{
  assert(1==0);
  //RES.BackProject(src,dest,dest_aux);
}
*/



float YARPVisualSearch::Compare(YARPVisualSearch& alt)
{
  return RES.Compare(GETRES(alt));
}



void YARPVisualSearch::Opportunistic(YARPImageOf<YarpPixelBGR>& src, 
				     YARPImageOf<YarpPixelFloat>& mask, 
				     YARPImageOf<YarpPixelBGR>& dest)
{
  RES.Opportunistic(src,mask,dest);
}


float YARPVisualSearch::Localize(YARPImageOf<YarpPixelBGR>& src,
				 YARPImageOf<YarpPixelBGR>& dest,
				 float& x, float& y,float radius)
{
  return RES.Localize(src,dest,x,y,radius);
}


float YARPVisualSearch::Orient(YARPImageOf<YarpPixelBGR>& src,
			       YARPImageOf<YarpPixelBGR>& dest,
			       float& x, float& y, float& theta)
{
  return RES.Orient(src,dest,x,y,theta);
}


int YARPVisualSearch::IsActive()
{
  return RES.IsActive();
}

void YARPVisualSearch::Show()
{
  RES.Show();
}

