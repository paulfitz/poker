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

#ifndef BRESENHM_H
#define BRESENHM_H

// line segment has end points (x0,y0) and (x1,y1)
void Line2D (int x0, int y0, int x1, int y1, void (*callback)(int,int));

// line segment has end points (x0,y0,z0) and (x1,y1,z1)
void Line3D (int x0, int y0, int z0, int x1, int y1, int z1,
             void (*callback)(int,int,int));

// line segment has end points (x0,y0,z0,p0) and (x1,y1,z1,p1)
void Line4D (int x0, int y0, int z0, int p0, int x1, int y1, int z1, int p1,
             void (*callback)(int,int,int,int));

// circle is (x-xc)^2 + (y-yc)^2 = r^2
void Circle2D (int xc, int yc, int r, void (*callback)(int,int));

// ellipse is (x-xc)^2/a^2 + (y-yc)^2/b^2 = 1
void Ellipse2D (int xc, int yc, int a, int b, void (*callback)(int,int));

// Ellipse is (x-xc)^2/a^2 + (y-yc)^2/b^2 = 1.  Arc has end points
// (fx0,fy0) and (fx1,fy1) where (fxi-xc)^2/a^2 + (fyi-yc)/b^2 = 1 for
// i = 0,1.  The arc is traversed in clockwise order.
void EllipseArc2D (int xc, int yc, int a, int b, float fx0, float fy0,
    float fx1, float fy1, void (*callback)(int,int));

// General ellipse whose shape and orientation is determined by the oriented
// bounding box with center (xc,yc) and axes (xa,ya) and (xb,yb) where
// Dot((xa,ya),(xb,yb)) = 0.  On an integer lattice, the choice of axes
// satisfies n*(xb,yb) = m*(-ya,xa) for some positive integers n and m.
//
// Choose xa > 0, ya >= 0, xb <= 0, and yb > 0.
void GeneralEllipse2D (int xc, int yc, int xa, int ya, int xb, int yb,
    void (*callback)(int,int));

#endif
