#include "YARPImageDraw.h"
#include "YARPShapeBoundary.h"

#ifdef __QNX__
#define for if (1) for
#endif


/*
 * Generalized Polygon Fill
 *
 * Implementation by Curt McDowell,
 * Carnegie Mellon University,
 * for Computer Graphics (15-462),
 * November 13, 1988
 *
 * Takes a raster and a polygon.
 * The polygon is filled within the raster
 * The polygonal area is clipped to the raster bounds.
 * A fill pattern must be specified (by another raster).
 * Current drawing mode is honored (three possibilities).
 */

struct edge {
    struct edge *next;
    long yTop, yBot;
    long xNowWhole, xNowNum, xNowDen, xNowDir;
    long xNowNumStep;
};

#define MAXVERTICAL     1024
#define VOFFSET (MAXVERTICAL/2)
#define RAS YARPImageOf<YarpPixelBGR>

#define POLYGON YARPShapeBoundary
#define POINT YARPShapeElement

template <class T>
T ABS(T x)
{
  return (x>0)?x:-x;
}

template <class T>
T SGN(T x)
{
  return (x>0)?1:((x<0)?-1:0);
}

static int polygon_GetN(POLYGON *p)
{ 
  return p->edge.size(); 
}

static POINT *polygon_RefNth(POLYGON *p, int i)
{
  return &(p->edge[i]);
}

static int point_GetX(POINT *pt)
{
  return (int)(pt->x+0.5) + VOFFSET;
}
  
static int point_GetY(POINT *pt)
{
  return (int)(pt->y+0.5) + VOFFSET;
}
  

static void FillEdges(POLYGON *p, struct edge *edgeTable[])
{
    int i, j, n = polygon_GetN(p);

    for (i = 0; i < MAXVERTICAL; i++)
        edgeTable[i] = NULL;

    for (i = 0; i < n; i++) {
        POINT *p1, *p2, *p3;
        struct edge *e;
        p1 = polygon_RefNth(p, i);
        p2 = polygon_RefNth(p, (i + 1) % n);
        if (point_GetY(p1) == point_GetY(p2))
            continue;   /* Skip horiz. edges */
        /* Find next vertex not level with p2 */
        for (j = (i + 2) % n; ; j = (j + 1) % n) {
            p3 = polygon_RefNth(p, j);
            if (point_GetY(p2) != point_GetY(p3))
                break;
        }
        e = new edge;
        e->xNowNumStep = ABS(point_GetX(p1) - point_GetX(p2));
        if (point_GetY(p2) > point_GetY(p1)) {
            e->yTop = point_GetY(p1);
            e->yBot = point_GetY(p2);
            e->xNowWhole = point_GetX(p1);
            e->xNowDir = SGN(point_GetX(p2) - point_GetX(p1));
            e->xNowDen = e->yBot - e->yTop;
            e->xNowNum = (e->xNowDen >> 1);
            if (point_GetY(p3) > point_GetY(p2))
                e->yBot--;
        } else {
            e->yTop = point_GetY(p2);
            e->yBot = point_GetY(p1);
            e->xNowWhole = point_GetX(p2);
            e->xNowDir = SGN(point_GetX(p1) - point_GetX(p2));
            e->xNowDen = e->yBot - e->yTop;
            e->xNowNum = (e->xNowDen >> 1);
            if (point_GetY(p3) < point_GetY(p2)) {
                e->yTop++;
                e->xNowNum += e->xNowNumStep;
                while (e->xNowNum >= e->xNowDen) {
                    e->xNowWhole += e->xNowDir;
                    e->xNowNum -= e->xNowDen;
                }
            }
        }
        e->next = edgeTable[e->yTop];
        edgeTable[e->yTop] = e;
    }
}

/*
 * UpdateActive first removes any edges which curY has entirely
 * passed by.  The removed edges are freed.
 * It then removes any edges from the edge table at curY and
 * places them on the active list.
 */

static struct edge *UpdateActive(struct edge *active, 
				 struct edge *edgeTable[], long int curY)
{
    struct edge *e, **ep;
    for (ep = &active, e = *ep; e != NULL; e = *ep)
        if (e->yBot < curY) {
            *ep = e->next;
            delete e;
        } else
            ep = &e->next;
    *ep = edgeTable[curY];
    return active;
}

/*
 * DrawRuns first uses an insertion sort to order the X
 * coordinates of each active edge.  It updates the X coordinates
 * for each edge as it does this.
 * Then it draws a run between each pair of coordinates,
 * using the specified fill pattern.
 *
 * This routine is very slow and it would not be that
 * difficult to speed it way up.
 */

template <class T>
static void DrawRuns(YARPImageOf<T> *r, struct edge *active, 
		     long int curY, YARPImageOf<T> *pat,
		     const T& pixel)
{
    struct edge *e;
    static long xCoords[100];
    long numCoords = 0;
    int i, x;
    for (e = active; e != NULL; e = e->next) {
        for (i = numCoords; i > 0 &&
          xCoords[i - 1] > e->xNowWhole; i--)
            xCoords[i] = xCoords[i - 1];
        xCoords[i] = e->xNowWhole;
        numCoords++;
        e->xNowNum += e->xNowNumStep;
        while (e->xNowNum >= e->xNowDen) {
            e->xNowWhole += e->xNowDir;
            e->xNowNum -= e->xNowDen;
        }
    }
    if (numCoords % 2)  /* Protect from degenerate polygons */
        xCoords[numCoords] = xCoords[numCoords - 1],
        numCoords++;
    for (i = 0; i < numCoords; i += 2) {
        /* Here's the graphics-dependent part. */
        /* All we need is to draw a horizontal line */
        /* from (xCoords[i], curY) to (xCoords[i + 1], curY). */
        /* Example: I want to fill the polygon with a pattern. */
        /* (This example is very slow because it's done point by */
        /* point, thus not taking advantage of the potential for */
        /* speed afforded by the scan conversion...) */

        for (x = xCoords[i]; x <= xCoords[i + 1]; x++)
	  {
            //if (ras_TestPointModulo(pat, x, curY))
	    //ras_Point(r, x, curY);
	    (*r).SafePixel(x-VOFFSET,curY-VOFFSET) = pixel;
	  }
    }
}

/*
 * This version of the fill takes a fill pattern argument.  It may
 * be removed throughout for straight single-color fills.
 */

template <class T>
void ras_FillPolygon(YARPImageOf<T> *r, POLYGON *p, YARPImageOf<T> *pat,
		     const T& pixel)
{
    static struct edge *edgeTable[MAXVERTICAL];
    struct edge *active;
    long curY;

    FillEdges(p, edgeTable);

    for (curY = 0; edgeTable[curY] == NULL; curY++)
        if (curY == MAXVERTICAL - 1)
            return;     /* No edges in polygon */

    for (active = NULL; (active = UpdateActive(active,
      edgeTable, curY)) != NULL; curY++)
        DrawRuns(r, active, curY, pat, pixel);
}



void YARPShapeBoundary::Show(YARPImageOf<YarpPixelBGR>& img, int fill)
{
  int ct = 0;
  YarpPixelBGR hi(255,0,0);
  if (fill)
    {
      ras_FillPolygon(&img,this,&img,hi);
    }
  for (vector<YARPShapeElement>::iterator it = edge.begin();
       it != edge.end(); it++)
    {
      float xx = (*it).x;
      float yy = (*it).y;
      YarpPixelBGR pix(0,255,0);
      AddCircleOutline(img,pix,(int)xx,(int)yy,4);
      //img.SafePixel(xx,yy) = pix;
      ct++;
    }
}

/*
void YARPShapeBoundary::GetMask(YARPImageOf<YarpPixelFloat>& img)
{
  YarpPixelFloat hi = 255;
  img.Zero();
  ras_FillPolygon(&img,this,&img,hi);
}
*/

void YARPShapeBoundary::GetMask(YARPImageOf<YarpPixelMono>& img)
{
  YarpPixelMono hi = 255;
  img.Zero();
  ras_FillPolygon(&img,this,&img,hi);
}


void YARPShapeBoundary::Transform( double sx,	/* x-scale changes */
				   double sy,	/* y-scale changes */
				   double rt,	/* rotation changes */
				   double tx,	/* x-translation changes */
				   double ty,	/* y-translation changes */
				   double idx,	/* x-dilation changes */
				   double idy )	/* y-dilation changes */
{
  double sinrt = sin(rt);
  double cosrt = cos(rt);
  float row, col;

  for (vector<YARPShapeElement>::iterator it = edge.begin();
       it != edge.end(); it++)
    {
      /* rotation and dilation */
      col = (cosrt + sinrt*idy)   * (*it).x +
	(cosrt*idx + sinrt)   * (*it).y;
      row = ( -sinrt+ cosrt*idy ) * (*it).x +
	( -sinrt*idx + cosrt) * (*it).y;
      
      /* scaling */
      col *= sx;
      row *= sy;
      
      /* translation */
      col += tx;
      row += ty;
      
      (*it).x = (float)col;
      (*it).y = (float)row;
    }
}


void YARPShapeBoundary::Translate(double dx, double dy)
{
  for (vector<YARPShapeElement>::iterator it = edge.begin();
       it != edge.end(); it++)
    {
      (*it).x += dx;
      (*it).y += dy;
    }
}


void YARPShapeBoundary::RotateAbout(double angle, double x, double y)
{
  Translate(-x,-y);
  Rotate(angle);
  Translate(x,y);
}


static int con_dx[] = 
{
  0, 1, 1, 1, 0, -1, -1, -1
};

static int con_dy[] = 
{
  -1, -1, 0, 1, 1, 1, 0, -1
};

static int getdx(int idx)
{
  return con_dx[(idx+256)%8];
}

static int getdy(int idx)
{
  return con_dy[(idx+256)%8];
}

static int getfreq(int dir0, int dir1)
{
  int dx0 = getdx(dir0);
  int dy0 = getdy(dir0);
  int dx1 = getdx(dir1);
  int dy1 = getdy(dir1);
  int ct = 1;
  if (ABS(dx0-dx1)>=2)
    {
      ct += 4;
    }
  if (ABS(dy0-dy1)>=2)
    {
      ct += 4;
    }
  return ct;
}

/*
  701
  6 2
  543
 */

void YARPShapeBoundary::Add(float x, float y)
{ 
  edge.push_back(YARPShapeElement(x,y)); 
}


void YARPShapeBoundary::SetFromMask(YARPImageOf<YarpPixelMono>& img, int freq)
{
  YARPImageOf<YarpPixelMono> work;
  SatisfySize(img,work);
  work.PeerCopy(img);
  IMGFOR(work,x,y)
    {
      if (work(x,y)) work(x,y) = 1;
    }
  int ww = work.GetWidth();
  int hh = work.GetHeight();
  for (int x=0; x<ww; x++)
    {
      work(x,0) = work(x,hh-1) = 0;
    }
  for (int y=0; y<hh; y++)
    {
      work(0,y) = work(ww-1,y) = 0;
    }
  Reset();
  int dir = 2;
  int active = 0;
  int xx = 0, yy = 0;
  for (int y=0; y<work.GetHeight() && !active; y++)
    {
      for (int x=0; x<work.GetWidth() && !active; x++)
	{
	  if (work(x,y))
	    {
	      xx = x;
	      yy = y;
	      active = 1;
	    }
	}
    }
  int ct = 50;
  while (active)
    {
      //printf("Okay at %d\n", __LINE__); fflush(stdout);
      work(xx,yy) = 2;
      //printf("Okay at %d\n", __LINE__); fflush(stdout);
      if (ct>=freq)
	{
	  //printf("Okay at %d\n", __LINE__); fflush(stdout);
	  //printf("Adding %d %d\n", xx, yy); fflush(stdout);
	  Add(xx,yy);
	  //printf("Okay at %d\n", __LINE__); fflush(stdout);
	  ct = 0;
	}
      //printf("Okay at %d\n", __LINE__); fflush(stdout);
      //printf("Added %d %d\n", xx, yy);
      int dir_back = dir+4+1;
      int saw_space = 0;
      active = 0;
      for (int d=dir_back; d<dir_back+16; d++)
	{
	  int v = work(xx+getdx(d),yy+getdy(d));
	  //printf("scanning %d %d %d\n", xx+getdx(d),yy+getdy(d), v);
	  if (v==0)
	    {
	      saw_space = 1;
	    }
	  if (v==1 && saw_space)
	    {
	      xx += getdx(d);
	      yy += getdy(d);
	      active = 1;
	      ct += getfreq(dir,d);
	      dir = d;
	      break;
	    }
	}
    }
  //printf("Got through okay\n");
}


YARPShapeElement YARPShapeBoundary::GetCenter()
{
  float tx = 0, ty = 0;
  int ct = 0;
  for (vector<YARPShapeElement>::iterator it = edge.begin();
       it != edge.end(); it++)
    {
      tx += (*it).x;
      ty += (*it).y;
      ct++;
    }
  if (ct<1) ct = 1;
  return YARPShapeElement(tx/ct,ty/ct);
}

void YARPShapeBoundary::RotateAboutCenter(double theta)
{
  YARPShapeElement el = GetCenter();
  RotateAbout(theta,el.GetX(),el.GetY());
}

void YARPShapeBoundary::ScaleAbout(double scale,double x, double y)
{
  Translate(-x,-y);
  Scale(scale);
  Translate(x,y);
}

void YARPShapeBoundary::ScaleAboutCenter(double scale)
{
  YARPShapeElement el = GetCenter();
  ScaleAbout(scale,el.GetX(),el.GetY());
}

