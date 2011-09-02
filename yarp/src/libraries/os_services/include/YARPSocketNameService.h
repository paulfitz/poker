/*
paulfitz Tue May 22 15:34:43 EDT 2001
 */

#ifndef YARPSocketNameService_INC
#define YARPSocketNameService_INC

#include "YARPAll.h"

#include "YARPNameID.h"

class YARPSocketNameService
{
public:
  /* zero if successful */
  static int RegisterName(const char *name);

  static int GetAssignedPort();

  static YARPNameID LocateName(const char *name);
};

#endif


