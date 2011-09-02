
#include <assert.h>
#include <stdio.h>

#include "Port.h"

#include "YARPPort.h"
#include "YARPNameService.h"

#include "Sendables.h"

#include "YARPSemaphore.h"
#include "YARPAll.h"
#include <list>
using namespace std;

//int __debug_level = 40;
int __debug_level = 40;


typedef list<YARPPort *> PortList;
static PortList port_list;
static YARPSemaphore port_list_mutex(1);

static void AddPort(YARPPort *port)
{
  port_list_mutex.Wait();
  port_list.push_back(port);
  port_list_mutex.Post();
}

static void RemovePort(YARPPort *port)
{
  port_list_mutex.Wait();
#ifndef __QNX__
  //  port_list.erase(port);
#endif
  port_list_mutex.Post();
}

class YARPSendable : public Sendable
{
public:
  YARPPortContent *ypc;
  int owned;

  YARPSendable() { ypc = NULL;  owned = 0; }

  YARPSendable(YARPPortContent *n_ypc, int n_owned = 1)
    {
      ypc = NULL;  owned = 0;
      Attach(n_ypc,n_owned);
      //printf("####### sendable %ld (%d)\n", (long int)(this), ref_count);
    }

  virtual ~YARPSendable() { 
    if (!YARPThread::IsDying())
      {
	if (owned) delete ypc; 
      }
  }

  void Attach(YARPPortContent *n_ypc, int n_owned = 1)
    {
      assert(ypc==NULL);
      ypc = n_ypc;
      owned = n_owned;
    }

  YARPPortContent *Content()
    {
      return ypc;
    }

  virtual int Write(BlockSender& sender)
    {
      assert(ypc!=NULL);
      return ypc->Write(sender);
    }

  virtual int Read(BlockReceiver& receiver)
    {
      assert(ypc!=NULL);
      return ypc->Read(receiver);
    }

  virtual int Destroy()
    {
      assert(ypc!=NULL);
      ypc->Recycle();

      return Sendable::Destroy();
    }

};

template <class T>
class YARPSendablesOf : public Sendables
{
public:
  YARPPort *port;

  //YARPSendables() { port = NULL; }
  YARPSendablesOf() { port = NULL; }
  
  void Attach(YARPPort& yp) { port = &yp; }

  void Put(T *s)
    {
      PutSendable(s);
    }

  T *Get()
    {
      //printf("***Get() > 1\n");
      T *t = (T*)GetSendable();
      /*
      if (t!=NULL)
	{
	  printf("***Get() > 2a %ld (%d)\n", (long int) t, t->ref_count);
	  printf("***Get() > 2a %ld (%d)\n", (long int) t, ((Sendable*)t)->ref_count);
	}
      */
      if (t==NULL)
	{
	  assert(port!=NULL);
	  t = new T(port->CreateContent());
	  //printf("***Get() > 3\n");
	  assert(t!=NULL);
	  t->ZeroRef();
	}
      //printf("***Get() > 3b\n");
      assert(t!=NULL);
      //printf("***Get() > 4\n");
      t->owner = this;
      return t;
    }
};

class PortData : public Port
{
public:
  //SendablesOf<YARPSendable> sendables;
  //CountedPtr<YARPSendable> p_s;
  YARPSendablesOf<YARPSendable> sendables;
  CountedPtr<YARPSendable> p_s;
  int service_type;
  YARPInputPort *in_owner;
  YARPOutputPort *out_owner;

  void Attach(YARPPort& yp)
  {
    sendables.Attach(yp);
  }

  virtual void OnRead()
    { if (in_owner!=NULL) in_owner->OnRead(); }

  virtual void OnSend()
    { if (out_owner!=NULL) out_owner->OnWrite(); }

};

PortData& CastPortData(void *system_resource)
{
  assert(system_resource!=NULL);
  return *((PortData *)system_resource);
}

#define PD CastPortData(system_resource)

YARPPort::YARPPort()
{
  system_resource = new PortData;
  assert(system_resource!=NULL);
  content = NULL;
  PD.in_owner = NULL;
  PD.out_owner = NULL;
  PD.Attach(*this);
  AddPort(this);
}


YARPPort::~YARPPort()
{
  RemovePort(this);
  if (system_resource!=NULL && !YARPThread::IsDying())
    {
      delete ((PortData*)system_resource);
    }
}


int YARPPort::Register(const char *name)
{
  PD.SetName(name);
  return 0;
}

int YARPPort::IsReceiving()
{
  return PD.CountClients();
}

int YARPPort::IsSending()
{
  return PD.IsSending();
}

void YARPPort::FinishSend()
{
  PD.FinishSend();
}

int YARPPort::Connect(const char *name)
{
  //maddog  PD.Say(name);
  //maddog  return 0;
  return PD.Say(name);
}


int YARPPort::Connect(const char *src_name, const char *dest_name)
{
  YARPNameID id = YARPNameService::LocateName(src_name);
  if (id.IsValid())
    {
      Port p;
      p.SayServer(id,dest_name);
    }
  return 0;
}


YARPPortContent& YARPPort::Content()
{
  if (content==NULL)
    {
      fprintf(stderr,"Content requested for port %s when it was not available\n", PD.name.c_str());
      fprintf(stderr,"Please make sure you understand the lifetime of the content associated with\nan input or output port\n");
      exit(1);
    }
  assert(content!=NULL);
  return *content;
}


void YARPPort::Deactivate()
{
  PD.Deactivate();
}


void YARPPort::DeactivateAll()
{
  port_list_mutex.Wait();
  for (PortList::iterator it=port_list.begin(); it!=port_list.end(); it++)
    {
      (*it)->Deactivate();
    }
  port_list_mutex.Post();
}



YARPInputPort::YARPInputPort(int n_service_type)
{
  PD.service_type = n_service_type;
  PD.in_owner = this;
}


YARPInputPort::~YARPInputPort()
{
}


int YARPInputPort::Register(const char *name)
{
  int service_type = PD.service_type;
  PD.TakeReceiverIncoming(new YARPSendable(CreateContent()));
  if (service_type == DOUBLE_BUFFERS || service_type == TRIPLE_BUFFERS)
    {
      PD.TakeReceiverLatest(new YARPSendable(CreateContent()));
    }
  if (service_type == TRIPLE_BUFFERS)
    {
      PD.TakeReceiverAccess(new YARPSendable(CreateContent()));
    }
  return YARPPort::Register(name);
}


bool YARPInputPort::Read(bool wait)
{
  PD.Relinquish();
  content = NULL;
  YARPSendable *ptr = (YARPSendable *)PD.Acquire(wait);
  if (ptr!=NULL)
    {
      content = ptr->Content();
    }
  return (content!=NULL);
}


YARPOutputPort::YARPOutputPort(int n_service_type)
{
  PD.service_type = n_service_type;
  PD.out_owner = this;
}


YARPOutputPort::~YARPOutputPort()
{
}


int YARPOutputPort::Register(const char *name)
{
  return YARPPort::Register(name);
}


YARPPortContent& YARPOutputPort::Content()
{
  //printf("*** Content() > 1\n");
  if (content == NULL)
    {
      //printf("*** Content() > 2\n");
      YARPSendable *p = PD.sendables.Get();
      //printf("*** Content() > 2b\n");
      PD.p_s.Set(p);
      //printf("*** Content() > 3\n");
      YARPSendable *sendable = PD.p_s.Ptr();
      assert(sendable!=NULL);
      //printf("*** Content() > 4\n");
      if (sendable->Content()==NULL)
	{
	  //printf("*** Content() > 5\n");
	  sendable->Attach(CreateContent());
	}
      //printf("*** Content() > 6\n");
      content = sendable->Content();
      //printf("*** Content() > 7\n");
    }
  //printf("*** Content() > 8\n");
  return YARPPort::Content();
}



void YARPOutputPort::Write(bool wait)
{
//  printf("*** Write() > 1\n");
  PD.RequireCompleteSend(wait);
//  printf("*** Write() > 2\n");
  PD.Share(PD.p_s.Ptr());
//  printf("*** Write() > 3\n");
  PD.p_s.Reset();
//  printf("*** Write() > 4\n");
  content = NULL;
}

