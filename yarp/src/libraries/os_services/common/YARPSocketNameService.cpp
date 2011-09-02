#ifdef __WIN_MSVC__
#include <windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "YARPSocket.h"
#include "YARPTime.h"
#include "YARPSocketNameService.h"
#include "ThreadInput.h"
#include "YARPNameID_defs.h"

#define DBG(x) if ((x)>=40) 
//#define DBG(x) if (0) 


#ifdef __LINUX__
#include <pthread.h>

class ISHolder
{
public:
  YARPInputSocket sock;
  int offset;

  ISHolder() { offset = 0; }
};  

pthread_key_t registry_key;
pthread_once_t once_control = PTHREAD_ONCE_INIT;

static void destroy_from_key(void *ptr)
{
  if (ptr!=NULL)
    {
      delete ((ISHolder*)ptr);
    }
  DBG(5) printf("^^^^^^^^ Destroyed an input socket handler\n");
}
   
static void init_registry_key(void)
{
  int status = pthread_key_create(&registry_key, destroy_from_key);
  DBG(5) printf("^^^^^^^^ Initialized a key\n");
}

ISHolder *GetThreadData()
{
  void *key = pthread_getspecific(registry_key);
  return (ISHolder *) key;
}

YARPInputSocket *GetThreadSocket()
{
  ISHolder *data = GetThreadData();
  if (data!=NULL)
    {
      return &(data->sock);
    }
  return NULL;
}

int YARPSocketNameService::GetAssignedPort()
{
  int result = -1;
  YARPInputSocket *is = GetThreadSocket();
  if (is!=NULL)
    {
      result = is->GetAssignedPort();
    }
  return result;
}

/*  
int YARPNameService::ReadOffset(int delta, int absolute)
{
  // only one receiver possible per thread a-la qnx, so okay to do:
  int offset = 0;
  ISHolder *holder = GetThreadData();
  if (holder!=NULL)
    {
      int offset = holder->offset;
      if (absolute) { offset = deta; }
      else { offset += delta; }
      holder->offset = offset;
    }
  return delta;
}
*/

int YARPSocketNameService::RegisterName(const char *name)
{

  int name_status = -1;
  int result = -1;
  pthread_once(&once_control, &init_registry_key);

  void *key = pthread_getspecific(registry_key);
  assert(key==NULL);
  if (key==NULL)
    {
      ISHolder *holder = new ISHolder;
      assert(holder!=NULL);
      holder->sock.Register(name);
      result = holder->sock.GetAssignedPort();
      pthread_setspecific(registry_key, (void *)holder);
      DBG(5) printf("^^^^^^^^ Made an input socket handler\n");
    } 

  return result;
}

YARPNameID YARPSocketNameService::LocateName(const char *name)
{
  YARPOutputSocket os;
  os.Connect(name);
  os.InhibitDisconnect();

  return YARPNameID(YARP_NAME_MODE_SOCKET,os.GetIdentifier());
}


#else

/**************************************************************************
 qnx and cygwin version follows
 **************************************************************************/

#include <sys/types.h>
#ifndef __WIN__
#include <unistd.h>
#endif
#include <map>
#ifdef __WIN_MSVC__
#include <process.h>
#include <functional>
using namespace std;
#endif
#include "YARPSemaphore.h"

#define times ignore

#ifndef __WIN__
#include <sys/psinfo.h>
#endif

YARPSemaphore mutex(1);

class ISHolder
{
public:
  YARPInputSocket sock;
  
  int operator==(const ISHolder& other)
    {
      return sock.GetIdentifier() == other.sock.GetIdentifier();
    }
  int operator!=(const ISHolder& other)
    {
      return sock.GetIdentifier() != other.sock.GetIdentifier();
    }
  int operator<(const ISHolder& other)
    {
      return sock.GetIdentifier() < other.sock.GetIdentifier();
    }
};

typedef YARPInputSocket *PYARPInputSocket;
typedef map<int, PYARPInputSocket, less<int> > ISMap;

ISMap is_map;


#ifndef __WIN__
static int my_getpid()
{
  struct _psinfo2 info;
  qnx_getids(0,&info);
  return info.pid;
}
#else
static int my_getpid()
{
#ifdef __WIN_MSVC__
  return GetCurrentThreadId();
#else
  return getpid();
#endif
}
#endif

YARPInputSocket *test_global = NULL;

YARPInputSocket *GetThreadSocket()
{
  int pid = my_getpid();
  YARPInputSocket *result = NULL;
  mutex.Wait();
  if (is_map.find(pid)!=is_map.end())
    {
      result = is_map[pid];
    }
  mutex.Post();
  return result;
}

int YARPSocketNameService::GetAssignedPort()
{
  int result = -1;
  YARPInputSocket *is = GetThreadSocket();
  if (is!=NULL)
    {
      result = is->GetAssignedPort();
    }
  return result;
}

int YARPSocketNameService::RegisterName(const char *name)
{

  int name_status = -1;
  int result = -1;

  //static YARPInputSocket sock;
  //sock.Register(name);
  //test_global = &sock;

  DBG(5) printf("^^^^^^^^ checking for previous definitions\n");
  if (GetThreadSocket()==NULL)
    {
      DBG(5) printf("^^^^^^^^ checks out okay\n");
      int pid = my_getpid();
      mutex.Wait();
      DBG(5) printf("^^^^^^^^ creating\n");
      YARPInputSocket *is = new YARPInputSocket;
      is_map[pid] = is;
      DBG(5) printf("^^^^^^^^ preparing to register\n");
      is->Register(name);
      result = is->GetAssignedPort();
      mutex.Post();
      DBG(5) printf("^^^^^^^^ Made an input socket handler\n");
      fflush(stdout);
    }

  return result;
}

YARPNameID YARPSocketNameService::LocateName(const char *name)
{
  YARPOutputSocket os;
  os.Connect(name);
  os.InhibitDisconnect();

  return YARPNameID(YARP_NAME_MODE_SOCKET,os.GetIdentifier());
}

#endif

