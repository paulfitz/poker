/*
paulfitz Tue May 22 15:34:43 EDT 2001
 */

#ifndef YARPNativeNameService_INC
#define YARPNativeNameService_INC

#include "YARPAll.h"

#include "YARPNameID.h"

class YARPNativeNameService
{
public:
  /* zero if successful */
  static int RegisterName(const char *name);

  static YARPNameID LocateName(const char *name);

  static int IsNonTrivial();
};

#endif


