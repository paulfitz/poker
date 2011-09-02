/*
paulfitz Tue May 22 15:34:43 EDT 2001
 */

#ifndef YARPNameService_INC
#define YARPNameService_INC

#include "YARPAll.h"

#include <stdlib.h>

#include "YARPNameID.h"

class YARPNameService
{
public:
  static int ConnectNameServer(const char *name = NULL);

  /* zero if successful */
  // if native flag set, register with native name service only
  static int RegisterName(const char *name, int native=1);

  static YARPNameID GetRegistration();

  // if native flag set, search native name service first, then global
  static YARPNameID LocateName(const char *name, int native=1);

};

#endif

