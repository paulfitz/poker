/*
paulfitz Mon May 21 13:35:42 EDT 2001
 */

#ifndef YARPSemaphore_INC
#define YARPSemaphore_INC

#include "YARPAll.h"

// defer to YARP2
#include <yarp/os/Semaphore.h>

class YARPSemaphore : public yarp::os::Semaphore
{
private:
public:
  // Assertion fails if insufficient resources at initialization
 YARPSemaphore(int initial_count=1) : yarp::os::Semaphore(initial_count) {}

  //YARPSemaphore(const YARPSemaphore& yt);

  //virtual ~YARPSemaphore();
  

  void BlockingWait() { wait(); }
  int PollingWait() { return check()?1:0; }

  int Wait(int blocking = 1)
    {
      if (blocking) { BlockingWait(); return 1; }
      else          { return PollingWait(); } 
    }

  void Post() { post(); }
};

#endif
