/*
paulfitz Mon May 21 13:35:42 EDT 2001
 */

#ifndef YARPBool_INC
#define YARPBool_INC

#include "YARPAll.h"

// may need some work to make this reliably portable

#ifndef __WIN__
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif
#else
// this is for windows cygwin....
#ifndef __WIN_MSVC__
#include <stl_config.h>
#endif
#endif

#define YARPBool int
#define YARPtrue 1
#define YARPfalse 0

#endif

