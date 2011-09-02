/*********************************************************************
 * base.h
 *********************************************************************/

#ifndef _KLT_BASE_H_
#define _KLT_BASE_H_

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef schar
#define schar signed char
#endif

#ifndef uint
#define uint unsigned int
#endif

#ifndef ushort
#define ushort unsigned short
#endif

#ifndef ulong
#define ulong unsigned long
#endif

#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))
#define max3(a,b,c)	((a) > (b) ? max((a),(c)) : max((b),(c)))
#define min3(a,b,c)	((a) < (b) ? min((a),(c)) : min((b),(c)))

#define expf(x) (float)exp((double)(x))
#define logf(x) (float)log((double)(x))
#define fsqrt(x) (float)sqrt((double)(x))
#define fabsf(x) (float)fabs((double)(x))

#endif

