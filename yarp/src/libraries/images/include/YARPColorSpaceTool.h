#ifndef YARPColorSpaceTool_INC
#define YARPColorSpaceTool_INC

#include "YARPImage.h"

#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932
#endif

#define MIN_OF_3(x,y,z) ((x<y)?((x<z)?x:z):((y<z)?y:z))
#define MAX_OF_3(x,y,z) ((x>y)?((x>z)?x:z):((y>z)?y:z))

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//              if s == 0, then h = 0 (really should be undefined)

inline void RGBtoHSVRaw( float r, float g, float b, float& h, float& s, float& v )
{
        float min, max, delta;

        min = MIN_OF_3( r, g, b );
        max = MAX_OF_3( r, g, b );
        v = max;                               // v

        delta = max - min;

        if( max > 0.001 )
                s = delta / max;               // s
        else {
                // r = g = b = 0                // s = 0, v is undefined
                s = 0;
                h = 0;
                return;
        }
  
  if (delta<0.001)
    {
      delta = 1;
    }

        if( r == max )
                h = ( g - b ) / delta;         // between yellow & magenta
        else if( g == max )
                h = 2 + ( b - r ) / delta;     // between cyan & yellow
        else
                h = 4 + ( r - g ) / delta;     // between magenta & cyan

        h *= 60;                               // degrees
        if( h < 0 )
                h += 360;

}

inline void HSVtoRGBRaw(float h, float s, float v, 
			float& r, float& g, float& b)
{
        int i;
        float f, p, q, t;

        if( s == 0 ) {
                // achromatic (grey)
                r = g = b = v;
                return;
        }

        h /= 60;                        // sector 0 to 5
        i = (int)floor( h );
        f = h - i;                      // factorial part of h
        p = v * ( 1 - s );
        q = v * ( 1 - s * f );
        t = v * ( 1 - s * ( 1 - f ) );

        switch( i ) {
                case 0:
                        r = v;
                        g = t;
                        b = p;
                        break;
                case 1:
                        r = q;
                        g = v;
                        b = p;
                        break;
	        case 2:
                        r = p;
                        g = v;
                        b = t;
                        break;
                case 3:
                        r = p;
                        g = q;
                        b = v;
                        break;
                case 4:
                        r = t;
                        g = p;
                        b = v;
                        break;
                default:                // case 5:
                        r = v;
                        g = p;
                        b = q;
                        break;
        }

}

inline void RGBtoHSV( float r, float g, float b, float& h, float& s, float& v )
{
  RGBtoHSVRaw(r/256.0,g/256.0,b/256.0,h,s,v);
  h *= M_PI/180;
}

inline void RGBtoCYL( float r, float g, float b, float& c1, float& c2, float& c3)
{
  float h, s;
  RGBtoHSV(r,g,b, h, s, c3);
  c1 = s*cos(h);
  c2 = s*sin(h);
}


#endif
