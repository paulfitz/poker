/*
paulfitz Mon May 21 13:50:02 EDT 2001
*/
#ifndef YARPTime_INC
#define YARPTime_INC

#include "YARPAll.h"

// defer to YARP2
#include <yarp/os/Time.h>

class YARPTime
{
public:
  static double GetTimeAsSeconds() {
    return yarp::os::Time::now();
  }

  // Assertion fails if insufficient resources
  static void DelayInSeconds(double delay_in_seconds) {
    yarp::os::Time::delay(delay_in_seconds);
  }
};

#endif
