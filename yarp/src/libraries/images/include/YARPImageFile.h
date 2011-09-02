/*
paulfitz Wed May 30 18:35:14 EDT 2001
*/

/*
pasa: June 2002, don't exit on open failure (gracefully returns -1, 0 if OK).
*/

#ifndef YARPImageFile_INC
#define YARPImageFile_INC

#include "YARPImage.h"

class YARPImageFile
{
public:
  enum
  {
    FORMAT_NULL,
    FORMAT_ANY,
    FORMAT_PGM,
    FORMAT_PPM,
    FORMAT_NUMERIC,
  };
  static int Read(const char *src, YARPGenericImage& dest, 
		  int format=FORMAT_ANY);
  static int Write(const char *dest, YARPGenericImage& src,
		   int format=FORMAT_ANY);
};


#endif
