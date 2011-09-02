//
// YARPRandomNumber.h
//

#ifndef __YARPRandomNumberh__
#define __YARPRandomNumberh__

#include <stdlib.h>

//
// normal distribution random number generator.
// - it might be slow, the standard rand() is used as source.
// - beware of initializing the seed of rand.

// original code copyright reported below.

/* boxmuller.c           Implements the Polar form of the Box-Muller
                         Transformation

                      (c) Copyright 1994, Everett F. Carter Jr.
                          Permission is granted by the author to use
			  this software for any application provided this
			  copyright notice is preserved.

*/

#include <math.h>

//extern float ranf();         /* ranf() is uniform in 0..1 */

//
// YARP random number generator services could go here.
class YARPRandom
{
public:
	static double ranf ()
	{
		return double (rand ()) / double (RAND_MAX);
	}

	static double Uniform ()
	  { return ranf(); }

	// normal random number gen. m = mean, s = stddev.
	static double box_muller(double m, double s)	
	{				        
		float x1, x2, w, y1;
		static double y2;
		static int use_last = 0;

		if (use_last)		        /* use value from previous call */
		{
			y1 = y2;
			use_last = 0;
		}
		else
		{
			do {
				x1 = 2.0 * ranf() - 1.0;
				x2 = 2.0 * ranf() - 1.0;
				w = x1 * x1 + x2 * x2;
			} while ( w >= 1.0 );

			w = sqrt( (-2.0 * log( w ) ) / w );
			y1 = x1 * w;
			y2 = x2 * w;
			use_last = 1;
		}

		return( m + y1 * s );
	}

public:
	static inline void Seed (int seed) { srand (seed); }
	static inline double RandN () { return box_muller (0.0, 1.0); }
	static inline double RandOne () { return double (rand()) / double (RAND_MAX); }
	static inline int Rand () { return rand (); }
	static inline int Rand (int min, int max);
};

inline int YARPRandom::Rand (int min, int max)
{
	int ret = int ((double (rand()) / double (RAND_MAX)) * (max - min + 1) + min);

	// there's a small chance the value is = max+1
	if (ret <= max)
		return ret;
	else
		return max;
}

#endif	// __YARPRandomNumberh__
