
#include <stdio.h>
#include <stdlib.h>

#ifndef __QNX__
#include <string>
using namespace std;
#ifndef __WIN_MSVC__
#include <unistd.h>  // just for gethostname
#else
#include <winsock2.h>
#endif
#else
#include "strng.h"
#include <unix.h>  // just for gethostname
#endif

#include <assert.h>


#include "TinySocket.h"

#include "YARPSocket.h"


extern int TINY_VERBOSE;
#define DBG if (TINY_VERBOSE)
//#define DBG if (0)


class ISData
{
public:
  SocketOwner owner;
};

ISData& ISDATA(void *x)
{
  assert(x!=NULL);
  return *((ISData*)x);
}

int YARPNetworkObject::GetHostName(char *buffer, int buffer_length)
{
  int result = gethostname(buffer,buffer_length);
#ifndef __QNX4__
#ifndef __WIN__
  // QNX4 just doesn't have getdomainname or any obvious equivalent
  // cygwin version doesn't have getdomainname or any obvious equivalent
  if (result==0)
    {
      if (strchr(buffer,'.')==NULL)
	{
	  int delta = strlen(buffer);
	  buffer += delta;
	  buffer_length -= delta;
	  if (buffer_length>=1)
	    {
	      *buffer = '.';
	      buffer++;
	      buffer_length--;
	    }
	  result = getdomainname(buffer,buffer_length);
	}
    }
#endif
#endif
  return result;
}

YARPInputSocket::YARPInputSocket()
{ 
  system_resources = NULL; 
  identifier = -1; 
  assigned_port = -1;
  system_resources = new ISData;
  assert(system_resources!=NULL);
}

YARPInputSocket::YARPInputSocket(const YARPInputSocket& other)
{
  system_resources = NULL; 
  identifier = -1; 
  assigned_port = -1;
  system_resources = new ISData(ISDATA(other.system_resources));
  assert(system_resources!=NULL);
}


YARPInputSocket::~YARPInputSocket()
{
  if (system_resources!=NULL)
    {
      //Close();
      delete ((ISData*)system_resources);
      system_resources = NULL;
    }
}

int YARPInputSocket::Close()
{
  int result = ISDATA(system_resources).owner.Close();
  return result;
}

int YARPInputSocket::Close(int reply_id)
{
  int result = ISDATA(system_resources).owner.Close(reply_id);
  return result;
}

int YARPInputSocket::Register(const char *name)
{
  DBG printf ("Registering port with name %s\n", name);
  int port = GetPort(name);
  DBG printf ("Registering port %d\n", port);
  int result = ISDATA(system_resources).owner.Connect(port);
  assigned_port = ISDATA(system_resources).owner.GetAssignedPort();
  DBG printf ("Assigned port is %d\n", assigned_port);
  return result;
}


int YARPInputSocket::PollingReceiveBegin(char *buffer, int buffer_length, 
					 int *reply_id)
{
  return ISDATA(system_resources).owner.PollingRead(buffer,buffer_length,
						    reply_id);
}


int YARPInputSocket::ReceiveBegin(char *buffer, int buffer_length, 
				  int *reply_id)
{
  return ISDATA(system_resources).owner.Read(buffer,buffer_length,reply_id);
}


int YARPInputSocket::ReceiveContinue(int reply_id, char *buffer, 
				     int buffer_length)
{
  return ISDATA(system_resources).owner.ReceiveMore(reply_id,buffer,buffer_length);
}

int YARPInputSocket::ReceiveReplying(int reply_id, char *reply_buffer,
				     int reply_buffer_length)
{
  return ISDATA(system_resources).owner.BeginReply(reply_id,reply_buffer,
						   reply_buffer_length);
}


int YARPInputSocket::ReceiveEnd(int reply_id, char *reply_buffer, 
				int reply_buffer_length)
{
  return ISDATA(system_resources).owner.Reply(reply_id,reply_buffer,
					      reply_buffer_length);
}


class OSData
{
public:
  TinySocket sock;
};

OSData& OSDATA(void *x)
{
  assert(x!=NULL);
  return *((OSData*)x);
}

YARPOutputSocket::YARPOutputSocket()
{ 
  system_resources = NULL;
  identifier = -1;
  system_resources = new OSData;
  assert(system_resources!=NULL);
}

YARPOutputSocket::~YARPOutputSocket()
{
  if (system_resources!=NULL)
    {
      //Close();
      delete ((OSData*)system_resources);
      system_resources = NULL;
    }
}


int YARPOutputSocket::Close()
{
  int result = OSDATA(system_resources).sock.Close();
  return result;
}



int YARPOutputSocket::Connect(const char *name)
{
  string machine = GetBase(name);
  int port = GetPort(name);
  const char *str;
#ifndef __QNX__
  str = machine.c_str();
#else
  str = machine.AsChars();
#endif
  DBG printf ("Connecting to port %d on %s\n", port, str);
  OSDATA(system_resources).sock.Connect(str,port);
  identifier = OSDATA(system_resources).sock.GetSocketPID();
  return identifier;
}


int YARPOutputSocket::SendBegin(char *buffer, int buffer_length)
{
  return SendBlock(OSDATA(system_resources).sock,buffer,buffer_length);
}


int YARPOutputSocket::SendContinue(char *buffer, int buffer_length)
{
  return SendBlock(OSDATA(system_resources).sock,buffer,buffer_length,0);
}

int YARPOutputSocket::SendReceivingReply(char *reply_buffer, 
					 int reply_buffer_length)
{
  return ::SendBeginEnd(OSDATA(system_resources).sock,reply_buffer,
			reply_buffer_length);
}


int YARPOutputSocket::SendEnd(char *reply_buffer, int reply_buffer_length)
{
  return ::SendEnd(OSDATA(system_resources).sock,reply_buffer,
		   reply_buffer_length);
}



void YARPOutputSocket::InhibitDisconnect()
{
  OSDATA(system_resources).sock.InhibitDisconnect();
}

int YARPOutputSocket::GetIdentifier()
{
  return OSDATA(system_resources).sock.GetSocketPID();
}

void YARPOutputSocket::SetIdentifier(int n_identifier)
{
  OSDATA(system_resources).sock.ForcePID(n_identifier);
  identifier = n_identifier;
}


#include "YARPTime.h"

void server(const char *name)
{
  YARPInputSocket s;
  s.Register(name);
  while (1)
    {
      char buf[1000] = "not set";
      int reply_id = -1;
      int len = s.ReceiveBegin(buf,sizeof(buf),&reply_id);
      printf("Get data (%d): %s\n", len, buf);
      char reply[1000];
      sprintf(reply,"I acknowledge --> %s", buf);
      s.ReceiveEnd(reply_id,reply,strlen(reply)+1);
    }
}

void client(const char *name)
{
  YARPOutputSocket s;
  s.Connect(name);

  char buf[1000];
  char buf2[1000];

  for (int i=1; i<=20; i++)
    {
      sprintf(buf, "hello there time #%d", i);
      int len = strlen(buf)+1;
      //int len2 = Send(c,buf,len,buf2,sizeof(buf2));
      s.SendBegin(buf,len);
      int len2 = s.SendEnd(buf2,sizeof(buf2));
      if (len2>0)
	{
	  printf("Got response (%d): %s\n", len2, buf2);
	}
      else
	{
	  printf("Got response (%d)\n", len2);
	}
      YARPTime::DelayInSeconds(2.0);
    }
}

/*
int main(int argc, char *argv[])
{
  //    TinySocket::Verbose();

    if (argc == 2)
      {
	if (argv[1][0] == '+')
	  {
	    server(argv[1]+1);
	  }
	else
	  {
	    client(argv[1]);
	  }
      }
    return 0;
}

*/

