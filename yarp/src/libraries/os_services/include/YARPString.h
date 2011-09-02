#ifndef YARPString_INC
#define YARPString_INC

#include "YARPAll.h"

#ifndef __QNX__
#include <string>
using namespace std;
#else
#include "strng.h"
#define string String
#define c_str AsChars
#endif

typedef string YARPString;

#endif
