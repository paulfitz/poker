#ifndef YARPAll_INC
#define YARPAll_INC

#ifdef __QNX4__
// sorry about this, but necessary for QNX compatible user code using the STL
// need to silently ignore using namespace std in Watcom compiler
#define using static
#define namespace int
#endif

#include <stdio.h>

// thread-safe version of printf
void YARP_safe_printf(char *format,...);
void YARP_unsafe_printf(char *format,...);
void YARP_output_wait();
void YARP_output_post();

// actually WIN32 is thread safe!
#ifndef __WIN__
#ifndef YARP_USE_OLD_PRINTF
#define printf YARP_safe_printf
#endif
#endif

#endif
