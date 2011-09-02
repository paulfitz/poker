#include "YARPImageDraw.h"
#include "YARPBinaryAnalysis.h"

void YARPBinaryAnalysis::Apply(YARPImageOf<YarpPixelMono>& src)
{
  active_area = total_area = 0;
  double tx = 0;
  double ty = 0;
  double tx2 = 0;
  double ty2 = 0;
  double txy = 0;
  double tx3 = 0;
  double tx2y = 0;
  double txy2 = 0;
  double ty3 = 0;
  IMGFOR(src,x,y)
    {
      int v = src(x,y);
      if (v)
	{
	  active_area++;
	  tx += x;
	  ty += y;
	  tx2 += x*x;
	  ty2 += y*y;
	  txy += x*y;
	  tx3 += x*x*x;
	  tx2y += x*x*y;
	  txy2 += x*y*y;
	  ty3 += y*y*y;
	}
      total_area++;
    }
  double area = active_area;
  if (area<0.01) area = 0.01;
  x_centroid = tx/area;

  y_centroid = ty/area;
  double mu20 = tx2 - (tx*tx)/area;
  double mu02 = ty2 - (ty*ty)/area;
  double mu11 = txy - (tx*ty)/area;
  double angle = 0;
  if (fabs(mu11)>0.001 || fabs(mu20-mu02)>0.001)
    {
      angle = 0.5*atan2(2*mu11,mu20-mu02);
    }
  principal_angle = angle;
  double eta20 = mu20/(area*area);
  double eta02 = mu02/(area*area);
  double eta11 = mu11/(area*area);
  double phi1 = eta20+eta02;
  double phi2 = (eta20-eta02)*(eta20-eta02) + 4*eta11*eta11;
  moment_of_inertia = phi1;
  isotropic_measure = sqrt(phi2);

  double mu30 = tx3 - 3*x_centroid*tx2 + 2*tx*x_centroid*x_centroid;
  double mu03 = ty3 - 3*y_centroid*ty2 + 2*ty*y_centroid*y_centroid;
  double mu12 = txy2 - 2*y_centroid*txy - x_centroid*ty2 + 
    2*tx*y_centroid*y_centroid;
  double mu21 = tx2y - 2*x_centroid*txy - y_centroid*tx2 + 
    2*ty*x_centroid*x_centroid;
  double g3 = 3.0/2.0 + 1;
  double norm3 = pow(area,g3);
  double eta30 = mu30/norm3;
  double eta21 = mu21/norm3;
  double eta12 = mu12/norm3;
  double eta03 = mu03/norm3;
  double phi3 = (eta30-3*eta12)*(eta30-3*eta12) +
    (3*eta21-eta03)*(3*eta21-eta03);
  double phi4 = (eta03+eta12)*(eta03+eta12)+(eta21+eta30)*(eta21+eta30);

  moment_invariant[0] = sqrt(phi1);
  moment_invariant[1] = sqrt(sqrt(phi2));
  moment_invariant[2] = pow(phi3,0.2);
  moment_invariant[3] = pow(phi4,0.2);
}


void YARPBinaryAnalysis::Apply(YARPImageOf<YarpPixelFloat>& src)
{
  YARPImageOf<YarpPixelMono> alt;
  alt.CastCopy(src);
  Apply(alt);
}


