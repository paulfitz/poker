// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000, All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

#include <math.h>
#include "bresenhm.h"

#ifndef LINUX_BUILD
typedef float __int64;
template <class T>
T abs(T x) { return (x<0)?(-x):x; }
#else
typedef long long int __int64;
#endif


//---------------------------------------------------------------------------
void Line2D (int x0, int y0, int x1, int y1, void (*callback)(int,int))
{
    // starting point of line
    int x = x0, y = y0;

    // direction of line
    int dx = x1-x0, dy = y1-y0;

    // increment or decrement depending on direction of line
    int sx = (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
    int sy = (dy > 0 ? 1 : (dy < 0 ? -1 : 0));

    // decision parameters for voxel selection
    if ( dx < 0 ) dx = -dx;
    if ( dy < 0 ) dy = -dy;
    int ax = 2*dx, ay = 2*dy;
    int decx, decy;

    // determine largest direction component, single-step related variable
    int max = dx, var = 0;
    if ( dy > max ) { var = 1; }

    // traverse Bresenham line
    switch ( var )
    {
    case 0:  // single-step in x-direction
        for (decy=ay-dx; /**/; x += sx, decy += ay)
        {
            // process pixel
            callback(x,y);

            // take Bresenham step
            if ( x == x1 ) break;
            if ( decy >= 0 ) { decy -= ax; y += sy; }
        }
        break;
    case 1:  // single-step in y-direction
        for (decx=ax-dy; /**/; y += sy, decx += ax)
        {
            // process pixel
            callback(x,y);

            // take Bresenham step
            if ( y == y1 ) break;
            if ( decx >= 0 ) { decx -= ay; x += sx; }
        }
        break;
    }
}
//---------------------------------------------------------------------------
void Line3D (int x0, int y0, int z0, int x1, int y1, int z1,
    void (*callback)(int,int,int))
{
    // starting point of line
    int x = x0, y = y0, z = z0;

    // direction of line
    int dx = x1-x0, dy = y1-y0, dz = z1-z0;

    // increment or decrement depending on direction of line
    int sx = (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
    int sy = (dy > 0 ? 1 : (dy < 0 ? -1 : 0));
    int sz = (dz > 0 ? 1 : (dz < 0 ? -1 : 0));

    // decision parameters for voxel selection
    if ( dx < 0 ) dx = -dx;
    if ( dy < 0 ) dy = -dy;
    if ( dz < 0 ) dz = -dz;
    int ax = 2*dx, ay = 2*dy, az = 2*dz;
    int decx, decy, decz;

    // determine largest direction component, single-step related variable
    int max = dx, var = 0;
    if ( dy > max ) { max = dy; var = 1; }
    if ( dz > max ) { var = 2; }

    // traverse Bresenham line
    switch ( var )
    {
    case 0:  // single-step in x-direction
        for (decy=ay-dx, decz=az-dx; /**/; x += sx, decy += ay, decz += az)
        {
            // process voxel
            callback(x,y,z);

            // take Bresenham step
            if ( x == x1 ) break;
            if ( decy >= 0 ) { decy -= ax; y += sy; }
            if ( decz >= 0 ) { decz -= ax; z += sz; }
        }
        break;
    case 1:  // single-step in y-direction
        for (decx=ax-dy, decz=az-dy; /**/; y += sy, decx += ax, decz += az)
        {
            // process voxel
            callback(x,y,z);

            // take Bresenham step
            if ( y == y1 ) break;
            if ( decx >= 0 ) { decx -= ay; x += sx; }
            if ( decz >= 0 ) { decz -= ay; z += sz; }
        }
        break;
    case 2:  // single-step in z-direction
        for (decx=ax-dz, decy=ay-dz; /**/; z += sz, decx += ax, decy += ay)
        {
            // process voxel
            callback(x,y,z);

            // take Bresenham step
            if ( z == z1 ) break;
            if ( decx >= 0 ) { decx -= az; x += sx; }
            if ( decy >= 0 ) { decy -= az; y += sy; }
        }
        break;
    }
}
//---------------------------------------------------------------------------
void Line4D (int x0, int y0, int z0, int p0, int x1, int y1, int z1, int p1,
    void (*callback)(int,int,int,int))
{
    // starting point of line
    int x = x0, y = y0, z = z0, p = p0;

    // direction of line
    int dx = x1-x0, dy = y1-y0, dz = z1-z0, dp = p1-p0;

    // increment or decrement depending on direction of line
    int sx = (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
    int sy = (dy > 0 ? 1 : (dy < 0 ? -1 : 0));
    int sz = (dz > 0 ? 1 : (dz < 0 ? -1 : 0));
    int sp = (dp > 0 ? 1 : (dp < 0 ? -1 : 0));

    // decision parameters for voxel selection
    if ( dx < 0 ) dx = -dx;
    if ( dy < 0 ) dy = -dy;
    if ( dz < 0 ) dz = -dz;
    if ( dp < 0 ) dp = -dp;
    int ax = 2*dx, ay = 2*dy, az = 2*dz, ap = 2*dp;
    int decx, decy, decz, decp;

    // determine largest direction component, single-step related variable
    int max = dx, var = 0;
    if ( dy > max ) { max = dy; var = 1; }
    if ( dz > max ) { max = dz; var = 2; }
    if ( dp > max ) { var = 3; }

    // traverse Bresenham line
    switch ( var )
    {
    case 0:  // single-step in x-direction
        for (decy = ay-dx, decz = az-dx, decp = ap-dx;
             /**/;
             x += sx, decy += ay, decz += az, decp += ap)
        {
            // process hypervoxel
            callback(x,y,z,p);

            // take Bresenham step
            if ( x == x1 ) break;
            if ( decy >= 0 ) { decy -= ax; y += sy; }
            if ( decz >= 0 ) { decz -= ax; z += sz; }
            if ( decp >= 0 ) { decp -= ax; p += sp; }
        }
        break;
    case 1:  // single-step in y-direction
        for (decx = ax-dy, decz = az-dy, decp = ap-dy;
             /**/;
             y += sy, decx += ax, decz += az, decp += ap)
        {
            // process hypervoxel
            callback(x,y,z,p);

            // take Bresenham step
            if ( y == y1 ) break;
            if ( decx >= 0 ) { decx -= ay; x += sx; }
            if ( decz >= 0 ) { decz -= ay; z += sz; }
            if ( decp >= 0 ) { decp -= ay; p += sp; }
        }
        break;
    case 2:  // single-step in z-direction
        for (decx = ax-dz, decy = ay-dz, decp = ap-dz;
             /**/;
             z += sz, decx += ax, decy += ay, decp += ap)
        {
            // process hypervoxel
            callback(x,y,z,p);

            // take Bresenham step
            if ( z == z1 ) break;
            if ( decx >= 0 ) { decx -= az; x += sx; }
            if ( decy >= 0 ) { decy -= az; y += sy; }
            if ( decp >= 0 ) { decp -= az; p += sp; }
        }
        break;
    case 3:  // single-step in p-direction
        for (decx = ax-dp, decy = ay-dp, decz = az-dp;
             /**/;
             p += sp, decx += ax, decy += ay, decz += az)
        {
            // process voxel
            callback(x,y,z,p);

            // take Bresenham step
            if ( p == p1 ) break;
            if ( decx >= 0 ) { decx -= ap; x += sx; }
            if ( decy >= 0 ) { decy -= ap; y += sy; }
            if ( decz >= 0 ) { decz -= ap; z += sz; }
        }
        break;
    }
}
//---------------------------------------------------------------------------
void Circle2D (int xc, int yc, int r, void (*callback)(int,int))
{
    for (int x = 0, y = r, dec = 3-2*r; x <= y; x++)
    {
        callback(xc+x,yc+y);
        callback(xc+x,yc-y);
        callback(xc-x,yc+y);
        callback(xc-x,yc-y);
        callback(xc+y,yc+x);
        callback(xc+y,yc-x);
        callback(xc-y,yc+x);
        callback(xc-y,yc-x);

        if ( dec >= 0 )
            dec += -4*(y--)+4;
        dec += 4*x+6;
    }
}
//---------------------------------------------------------------------------
void Ellipse2D (int xc, int yc, int a, int b, void (*callback)(int,int))
{
    int a2 = a*a;
    int b2 = b*b;

    int x, y, dec;
    for (x = 0, y = b, dec = 2*b2+a2*(1-2*b); b2*x <= a2*y; x++)
    {
        callback(xc+x,yc+y);
        callback(xc-x,yc+y);
        callback(xc+x,yc-y);
        callback(xc-x,yc-y);

        if ( dec >= 0 )
            dec += 4*a2*(1-(y--));
        dec += b2*(4*x+6);
    }

    for (x = a, y = 0, dec = 2*a2+b2*(1-2*a); a2*y <= b2*x; y++)
    {
        callback(xc+x,yc+y);
        callback(xc-x,yc+y);
        callback(xc+x,yc-y);
        callback(xc-x,yc-y);

        if ( dec >= 0 )
            dec += 4*b2*(1-(x--));
        dec += a2*(4*y+6);
    }
}
//---------------------------------------------------------------------------
static void SelectEllipsePoint (int a2, int b2, float x, float y, int& ix,
    int& iy)
{
    int xfloor = int(floor(x));
    int yfloor = int(floor(y));
    int xincr = b2*(2*xfloor+1);
    int yincr = a2*(2*yfloor+1);
    int base = b2*xfloor*xfloor+a2*yfloor*yfloor-a2*b2;
    int a00 = abs(base);
    int a10 = abs(base+xincr);
    int a01 = abs(base+yincr);
    int a11 = abs(base+xincr+yincr);

    int min = a00;
    ix = xfloor;
    iy = yfloor;
    if ( a10 < min )
    {
        min = a10;
        ix = xfloor+1;
        iy = yfloor;
    }
    if ( a01 < min )
    {
        min = a01;
        ix = xfloor;
        iy = yfloor+1;
    }
    if ( a11 < min )
    {
        min = a11;
        ix = xfloor+1;
        iy = yfloor+1;
    }
}
//---------------------------------------------------------------------------
static int WhichArc (int a2, int b2, int x, int y)
{
    if ( x > 0 )
    {
        if ( y > 0 )
            return ( b2*x <  a2*y ? 0 : 1 );
        else if ( y < 0 )
            return ( b2*x > -a2*y ? 2 : 3 );
        else
            return 2;
    }
    else if ( x < 0 )
    {
        if ( y < 0 )
            return ( a2*y <  b2*x ? 4 : 5 );
        else if ( y > 0 )
            return ( a2*y < -b2*x ? 6 : 7 );
        else
            return 6;
    }
    else
    {
        return ( y > 0 ? 0 : 4 );
    }
}
//---------------------------------------------------------------------------
void EllipseArc2D (int xc, int yc, int a, int b, float fx0, float fy0,
    float fx1, float fy1, void (*callback)(int,int))
{
    // Assert (within floating point roundoff errors):
    //   (fx0-xc)^2/a^2 + (fy0-yc)^2/b^2 = 1
    //   (fx1-xc)^2/a^2 + (fy1-yc)^2/b^2 = 1
    // Assume if (fx0,fy0) == (fx1,fy1), then entire ellipse should be drawn.
    //
    // Integer points on arc are guaranteed to be traversed clockwise.

    const int a2 = a*a, b2 = b*b;

    // get integer end points for arc
    int x0, y0, x1, y1;
    SelectEllipsePoint(a2,b2,fx0-xc,fy0-yc,x0,y0);
    SelectEllipsePoint(a2,b2,fx1-xc,fy1-yc,x1,y1);

    int dx = x0 - x1;
    int dy = y0 - y1;
    int sqrlen = dx*dx+dy*dy;
    if ( sqrlen == 1 || ( sqrlen == 2 && abs(dx) == 1 ) )
    {
        callback(xc+x0,yc+y0);
        callback(xc+x1,yc+y1);
        return;
    }

    // determine initial case for arc drawing
    int arc = WhichArc(a2,b2,x0,y0);
    while ( 1 )
    {
        // process the pixel
        callback(xc+x0,yc+y0);

        // Determine next pixel to process.  Notation <(x,y),dy/dx>
        // indicates point on ellipse and slope at that point.
        int sigma;
        switch ( arc )
        {
        case 0:  // <(0,b),0> to <(u0,v0),-1>
            x0++;
            dx++;
            sigma = b2*x0*x0+a2*(y0-1)*(y0-1)-a2*b2;
            if ( sigma >= 0 )
            {
                y0--;
                dy--;
            }
            if ( b2*x0 >= a2*y0 )
                arc = 1;
            break;
        case 1:  // <(u0,v0),-1> to <(a,0),infinity>
            y0--;
            dy--;
            sigma = b2*x0*x0+a2*y0*y0-a2*b2;
            if ( sigma < 0 )
            {
                x0++;
                dx++;
            }
            if ( y0 == 0 )
                arc = 2;
            break;
        case 2:  // <(a,0),infinity> to <(u1,v1),+1>
            y0--;
            dy--;
            sigma = b2*(x0-1)*(x0-1)+a2*y0*y0-a2*b2;
            if ( sigma >= 0 )
            {
                x0--;
                dx--;
            }
            if ( b2*x0 <= -a2*y0 )
                arc = 3;
            break;
        case 3:  // <(u1,v1),+1> to <(0,-b),0>
            x0--;
            dx--;
            sigma = b2*x0*x0+a2*y0*y0-a2*b2;
            if ( sigma < 0 )
            {
                y0--;
                dy--;
            }
            if ( x0 == 0 )
                arc = 4;
            break;
        case 4:  // <(0,-b),0> to <(u2,v2,-1)>
            x0--;
            dx--;
            sigma = b2*x0*x0+a2*(y0+1)*(y0+1)-a2*b2;
            if ( sigma >= 0 )
            {
                y0++;
                dy++;
            }
            if ( a2*y0 >= b2*x0 )
                arc = 5;
            break;
        case 5:  // <(u2,v2,-1)> to <(-a,0),infinity>
            y0++;
            dy++;
            sigma = b2*x0*x0+a2*y0*y0-a2*b2;
            if ( sigma < 0 )
            {
                x0--;
                dx--;
            }
            if ( y0 == 0 )
                arc = 6;
            break;
        case 6:  // <(-a,0),infinity> to <(u3,v3),+1>
            y0++;
            dy++;
            sigma = b2*(x0+1)*(x0+1)+a2*y0*y0-a2*b2;
            if ( sigma >= 0 )
            {
                x0++;
                dx++;
            }
            if ( a2*y0 >= -b2*x0 )
                arc = 7;
            break;
        case 7:  // <(u3,v3),+1> to <(0,b),0>
            x0++;
            dx++;
            sigma = b2*x0*x0+a2*y0*y0-a2*b2;
            if ( sigma < 0 )
            {
                y0++;
                dy++;
            }
            if ( x0 == 0 )
                arc = 0;
            break;
        }

        sqrlen = dx*dx+dy*dy;
        if ( sqrlen <= 1 )
            break;
    }
}
//---------------------------------------------------------------------------
void GeneralEllipse2D (int xc, int yc, int xa, int ya, int xb, int yb,
    void (*callback)(int,int))
{
    // assert:  xa > 0, ya >= 0, xb <= 0, yb > 0

    // Ellipse is a*(x-xc)^2+2*b*(x-xc)*(y-yc)+c*(y-yc)^2 = d where
    //
    //   a = xa^2*Lb^4 + xb^2*La^4
    //   b = xa*ya*Lb^4 + xb*yb*La^4
    //   c = ya^2*Lb^4 + yb^2*La^4
    //   d = La^4*Lb^4
    //   La^2 = xa^2+ya^2
    //   Lb^2 = xb^2+yb^2
    //
    // Pixel determination is performed relative to origin (0,0).  The
    // ellipse at origin is a*x^2+b*x*y+c*y^2=d.  Slope of curve is
    // dy/dx = -(a*x+b*y)/(b*x+c*y).  Slope at (xb,yb) is
    // dy/dx = -xb/yb >= 0 and slope at (xa,ya) is dy/dx = -xa/ya < 0.

    __int64 xa2 = xa*xa, ya2 = ya*ya, xb2 = xb*xb, yb2 = yb*yb;
    __int64 xaya = xa*ya, xbyb = xb*yb;
    __int64 La2 = xa2+ya2, La4 = La2*La2, Lb2 = xb2+yb2, Lb4 = Lb2*Lb2;
    __int64 a = xa2*Lb4+xb2*La4;
    __int64 b = xaya*Lb4+xbyb*La4;
    __int64 c = ya2*Lb4+yb2*La4;
    __int64 d = La4*Lb4;

    __int64 dx, dy, sigma;
    int x, y, xp1, ym1, yp1;

    if ( ya <= xa )
    {
        // start at (-xa,-ya)
        x = -xa;
        y = -ya;
        dx = -(b*xa+c*ya);
        dy = a*xa+b*ya;

        // arc from (-xa,-ya) to point (x0,y0) where dx/dy = 0
        while ( dx <= 0 )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            y++;
            sigma = a*x*x+2*b*x*y+c*y*y-d;
            if ( sigma < 0 )
            {
                dx -= b;
                dy += a;
                x--;
            }
            dx += c;
            dy -= b;
        }

        // arc from (x0,y0) to point (x1,y1) where dy/dx = 1
        while ( dx <= dy )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            y++;
            xp1 = x+1;
            sigma = a*xp1*xp1+2*b*xp1*y+c*y*y-d;
            if ( sigma >= 0 )
            {
                dx += b;
                dy -= a;
                x = xp1;
            }
            dx += c;
            dy -= b;
        }

        // arc from (x1,y1) to point (x2,y2) where dy/dx = 0
        while ( dy >= 0 )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            x++;
            sigma = a*x*x+2*b*x*y+c*y*y-d;
            if ( sigma < 0 )
            {
                dx += c;
                dy -= b;
                y++;
            }
            dx += b;
            dy -= a;
        }

        // arc from (x2,y2) to point (x3,y3) where dy/dx = -1
        while ( dy >= -dx )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            x++;
            ym1 = y-1;
            sigma = a*x*x+2*b*x*ym1+c*ym1*ym1-d;
            if ( sigma >= 0 )
            {
                dx -= c;
                dy += b;
                y = ym1;
            }
            dx += b;
            dy -= a;
        }

        // arc from (x3,y3) to (xa,ya)
        while ( y >= ya )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            y--;
            sigma = a*x*x+2*b*x*y+c*y*y-d;
            if ( sigma < 0 )
            {
                dx += b;
                dy -= a;
                x++;
            }
            dx -= c;
            dy += b;
        }
    }
    else
    {
        // start at (-xa,-ya)
        x = -xa;
        y = -ya;
        dx = -(b*xa+c*ya);
        dy = a*xa+b*ya;

        // arc from (-xa,-ya) to point (x0,y0) where dy/dx = -1
        while ( -dx >= dy )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            x--;
            yp1 = y+1;
            sigma = a*x*x+2*b*x*yp1+c*yp1*yp1-d;
            if ( sigma >= 0 )
            {
                dx += c;
                dy -= b;
                y = yp1;
            }
            dx -= b;
            dy += a;
        }

        // arc from (x0,y0) to point (x1,y1) where dx/dy = 0
        while ( dx <= 0 )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            y++;
            sigma = a*x*x+2*b*x*y+c*y*y-d;
            if ( sigma < 0 )
            {
                dx -= b;
                dy += a;
                x--;
            }
            dx += c;
            dy -= b;
        }

        // arc from (x1,y1) to point (x2,y2) where dy/dx = 1
        while ( dx <= dy )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            y++;
            xp1 = x+1;
            sigma = a*xp1*xp1+2*b*xp1*y+c*y*y-d;
            if ( sigma >= 0 )
            {
                dx += b;
                dy -= a;
                x = xp1;
            }
            dx += c;
            dy -= b;
        }

        // arc from (x2,y2) to point (x3,y3) where dy/dx = 0
        while ( dy >= 0 )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            x++;
            sigma = a*x*x+2*b*x*y+c*y*y-d;
            if ( sigma < 0 )
            {
                dx += c;
                dy -= b;
                y++;
            }
            dx += b;
            dy -= a;
        }

        // arc from (x3,y3) to (xa,ya)
        while ( x <= xa )
        {
            callback(xc+x,yc+y);
            callback(xc-x,yc-y);
            x++;
            ym1 = y-1;
            sigma = a*x*x+2*b*x*ym1+c*ym1*ym1-d;
            if ( sigma >= 0 )
            {
                dx -= c;
                dy += b;
                y = ym1;
            }
            dx += b;
            dy -= a;
        }
    }
}
//---------------------------------------------------------------------------

#ifdef BRESENHM_TEST

// To test arcs of axis-aligned ellipse, make sure this is defined
//#define TEST_ARC

// To test drawing arbitrary orientation ellipse, make sure this is defined
#define TEST_GEN

#include <windows.h>
#include <stdlib.h>

const int WSIZE = 512;
HDC g_hDC = 0;

void RandomAxes (int& xa, int& ya, int& xb, int& yb)
{
    // generate axis (xa,ya) in first quadrant with length in [4,16]
    double theta = 2.0*atan(1.0)*(rand()/double(RAND_MAX));
    double cs = cos(theta);
    double sn = sin(theta);
    int length = 4 + rand() % 13;
    xa = int(length*cs);
    ya = int(length*sn);

    // generate axis (xb,yb) as a 90-degree rotation counterclockwise
    // of (xa,ya)
    xb = -ya;
    yb = +xa;

    // generate random integer scalings in [1,8]
    int na = 1 + rand() % 8;
    int nb = 1 + rand() % 8;

    // adjust axes
    xa *= na;
    ya *= na;
    xb *= nb;
    yb *= nb;
}

void DrawLinePixel (int x, int y)
{
    SetPixel(g_hDC,x,WSIZE-1-y,RGB(128,128,128));
}

void DrawEllipsePixel (int x, int y)
{
    SetPixel(g_hDC,x,WSIZE-1-y,RGB(0,0,255));
}

long FAR PASCAL 
WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int xc = WSIZE/2;
    static int yc = WSIZE/2;
    static int xa, ya, xb, yb;
    static int a = WSIZE/4, b = WSIZE/8;

    switch ( message ) 
    {
    case WM_CREATE:
    {
#ifdef TEST_GEN
        RandomAxes(xa,ya,xb,yb);
#endif
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        g_hDC = BeginPaint(hWnd,&ps);

#ifdef TEST_ARC
        // Test elliptical arc drawing.  Keep pressing 'n' to get new
        // arcs to draw.

        float dx0 = -a + 2.0f*a*(rand()/float(RAND_MAX));
        float sign0 = 2.0f*( rand() % 2 ) - 1.0f;
        float fx0 = xc + dx0;
        float fy0 = yc + sign0*b*float(sqrt(fabs(1.0f-dx0*dx0/(a*a))));

        float dx1 = -a + 2.0f*a*(rand()/float(RAND_MAX));
        float sign1 = 2.0f*( rand() % 2 ) - 1.0f;
        float fx1 = xc + dx1;
        float fy1 = yc + sign1*b*float(sqrt(fabs(1.0f-dx1*dx1/(a*a))));
        EllipseArc2D(xc,yc,a,b,fx0,fy0,fx1,fy1,DrawEllipsePixel);
#endif

#ifdef TEST_GEN

        // draw bounding rectangle
        Line2D(xc-xa-xb,yc-ya-yb,xc+xa-xb,yc+ya-yb,DrawLinePixel);
        Line2D(xc+xa-xb,yc+ya-yb,xc+xa+xb,yc+ya+yb,DrawLinePixel);
        Line2D(xc+xa+xb,yc+ya+yb,xc-xa+xb,yc-ya+yb,DrawLinePixel);
        Line2D(xc-xa+xb,yc-ya+yb,xc-xa-xb,yc-ya-yb,DrawLinePixel);

        // draw oriented ellipse
        GeneralEllipse2D(xc,yc,xa,ya,xb,yb,DrawEllipsePixel);
#endif   
        EndPaint(hWnd,&ps);
        return 0;
    }
    case WM_CHAR:
    {
        switch ( wParam )
        {
        case 'n':
        case 'N':
#ifdef TEST_GEN
            RandomAxes(xa,ya,xb,yb);
#endif
            InvalidateRect(hWnd,NULL,TRUE);
            break;
        case 'q':
        case 'Q':
        case VK_ESCAPE:
            PostMessage(hWnd,WM_DESTROY,0,0);
        }
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hWnd,message,wParam,lParam);
}

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine, int nCmdShow)
{
    static char szAppName[] = "Bresenham Test";

    WNDCLASS wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAppName;
    RegisterClass(&wc);

    RECT rect = { 0, 0, WSIZE-1, WSIZE-1 };
    AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);

    HWND hWnd = CreateWindow (
        szAppName,
        "Bresenham Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right-rect.left+1,
        rect.bottom-rect.top+1,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    ShowWindow(hWnd,nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while ( TRUE )
    {
        if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT )
                break;

            HACCEL hAccel = NULL;
            if ( !TranslateAccelerator( hWnd, hAccel, &msg ) )
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    return msg.wParam;
}
#endif
