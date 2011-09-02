#include <stdio.h>
#include <stdarg.h>

#define YARP_USE_OLD_PRINTF
#include "YARPAll.h"

#include "YARPSemaphore.h"

static YARPSemaphore services_sema(1);

// thread-safe version of printf
void YARP_safe_printf(char *format,...)
{
  va_list arglist;
  services_sema.Wait();
  va_start(arglist,format);
  vprintf(format,arglist);
  va_end(arglist);
  services_sema.Post();
}

void YARP_unsafe_printf(char *format,...)
{
  va_list arglist;
  va_start(arglist,format);
  vprintf(format,arglist);
  va_end(arglist);
}

void YARP_output_wait()
{
  services_sema.Wait();
}

void YARP_output_post()
{
  services_sema.Post();
}
