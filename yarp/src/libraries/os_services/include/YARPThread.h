/*
paulfitz Mon May 21 13:42:24 EDT 2001
*/
#ifndef YARPThread_INC
#define YARPThread_INC

/*
Ideally, would use POSIX semaphores, threads etc.
 */

#include "YARPAll.h"

class YARPThread
{
private:
  void *system_resource;
  int identifier;
  int size;
public:

  YARPThread();
  
  YARPThread(const YARPThread& yt);

  virtual ~YARPThread();

  // Assertion fails if insufficient resources at initialization.
  // stack_size of zero means use default stack size
  void Begin(int stack_size=0);
  void End();

  virtual void Body() = 0; // this is the body of the thread

  int GetIdentifier() { return identifier; }

  int IsTerminated();
  // If you are in MS-Windows, you should call this
  // every now and then, and leave Body() if the result
  // is non-zero.  If you don't, you may be terminated
  // forceably with loss of memory and resources you are
  // holding.
  
  // Forcibly halt all threads (late addition, just in QNX implementation)
  static void TerminateAll();

  static void PrepareForDeath();
  static int IsDying();
};



/* abstraction for thread-specific data */
class YARPThreadSpecificBase
{
private:
  void *system_resource;  
public:
  YARPThreadSpecificBase();
  virtual ~YARPThreadSpecificBase();

  void Set(int len);
  char *Get();
};

template <class T>
class YARPThreadSpecific : public YARPThreadSpecificBase
{
public:
  YARPThreadSpecific()
    {
      Set(sizeof(T));
    }

  T& Content()
    {
      return *((T*)Get());
    }
};

#endif
