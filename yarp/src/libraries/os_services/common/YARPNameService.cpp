#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fstream>
#ifndef __QNX4__
using namespace std;
#endif
#include "YARPNameService.h"
#include "YARPSemaphore.h"
#include "YARPSocket.h"
#include "YARPSocketNameService.h"
#include "YARPNativeNameService.h"
#include "YARPNameID.h"

#include "wide_nameloc.h"
#include "YARPNameID_defs.h"

#define CONFIG_FILE "/etc/yarp.conf"

static YARPSemaphore mutex(1);
static int is_connected = 0, tried_to_connect = 0;
static int registration_mode = YARP_NAME_MODE_NULL;
static YARPOutputSocket namer;

#define SCATTERSHOT

int YARPNameService::ConnectNameServer(const char *name)
{
  int con = 0;
  mutex.Wait();
  con = is_connected || (name==NULL&&tried_to_connect);
  mutex.Post();

#ifdef SCATTERSHOT
  if (tried_to_connect)
    {
      mutex.Wait();
      namer.Close();
      mutex.Post();
    }
  con = 0;
#endif
  
  if (!con)
    {
      char aux_name[256];
      mutex.Wait();
      if (name==NULL)
	{
	  ifstream fin(CONFIG_FILE);
	  is_connected = 0;
	  while (!fin.eof() && !is_connected)
	    {
	      fin.getline(aux_name,sizeof(aux_name));
	      if (strchr(aux_name,'|')!=NULL)
		{
//		  fprintf(stderr, 
//			  "Trying name server \"%s\"...\n", 
//			 aux_name);
		  if (namer.Connect(aux_name) != -1)
		    {
		      name = aux_name;
		      is_connected = 1;
		    }
		}
	    }
	}
      if (name==NULL)
	{
	  fprintf(stderr,"Could not connect name server, check %s\n", CONFIG_FILE);
	}
      tried_to_connect = 1;
      mutex.Post();
    }
  return 0;
}


int ConnectNameServer(YARPOutputSocket& namer)
{
  int is_connected = 0;
  if (1)
    {
      char aux_name[256];
	{
	  ifstream fin(CONFIG_FILE);
	  is_connected = 0;
	  while (!fin.eof() && !is_connected)
	    {
	      fin.getline(aux_name,sizeof(aux_name));
	      if (strchr(aux_name,'|')!=NULL)
		{
       //	  fprintf(stderr, 
//			  "Trying name server \"%s\"...\n", 
//			 aux_name);
		  if (namer.Connect(aux_name) != -1)
		    {
		      is_connected = 1;
		    }
		}
	    }
	}
      if (!is_connected)
	{
	  fprintf(stderr,"Could not connect name server, check %s\n", CONFIG_FILE);
	}
    }
  return is_connected;
}




YARPNameID YARPNameService::GetRegistration()
{
  mutex.Wait();
  int mode = registration_mode;
  mutex.Post();
  return YARPNameID(registration_mode,0);
}

int YARPNameService::RegisterName(const char *name, int native)
{
  if (strchr(name,'|')==NULL)
    {
      if (!(native && YARPNativeNameService::IsNonTrivial()))
	{
	  int all_result = YARPSocketNameService::RegisterName(name);
	  int in_port = YARPSocketNameService::GetAssignedPort();
	  
	  //ConnectNameServer(NULL);
	  YARPOutputSocket namer;
	  ::ConnectNameServer(namer);
	  
	  NameServiceHeader hdr;
	  hdr.port = in_port;
	  hdr.machine_length = 0;
	  hdr.key_length = strlen(name)+1;
	  hdr.spare2 = hdr.spare1 = -1;
	  char hostname[256] = "localhost";
	  YARPNetworkObject::GetHostName(hostname,sizeof(hostname));
	  hdr.machine_length = strlen(hostname)+1;
	  hdr.request_type = NAME_REGISTER;
	  
	  mutex.Wait();
	  namer.SendBegin((char*)(&hdr),sizeof(hdr));
	  namer.SendContinue(hostname,hdr.machine_length);
	  namer.SendContinue((char*)name,hdr.key_length);
	  char reply[256];
	  int result = namer.SendEnd(reply,sizeof(reply));
	  registration_mode = YARP_NAME_MODE_SOCKET;
	  mutex.Post();
	  if (result>=0)
	    {
	      fprintf(stderr,"Registered <%s> as [%s]\n", name, reply);
	    }
	  else
	    {
	      fprintf(stderr,"Couldn't contact name server to register %s\n", name);
	    }
	  fprintf(stderr,"Remote name registration of <%s>\n", name);
	  return all_result;
	}
      else
	{
	  // Otherwise, native registration
	  fprintf(stderr,"Native name registration of <%s>\n", name);
	  int all_result = YARPNativeNameService::RegisterName(name);
	  mutex.Wait();
	  registration_mode = YARP_NAME_MODE_NATIVE;
	  mutex.Post();
	  return all_result;
	}
    }

  int all_result = YARPSocketNameService::RegisterName(name);
  int in_port = YARPSocketNameService::GetAssignedPort();
  fprintf(stderr,"Socket name registration of <%s>\n", name);
  mutex.Wait();
  registration_mode = YARP_NAME_MODE_SOCKET;
  mutex.Post();
  return all_result;
}

YARPNameID YARPNameService::LocateName(const char *name, int native)
{
  YARPNameID all_result;

  if (strchr(name,'|')==NULL)
    {
      if (native && YARPNativeNameService::IsNonTrivial())
	{
	  all_result = YARPNativeNameService::LocateName(name);
	  fprintf(stderr,"Native name lookup of <%s>\n", name);
	}
      
      if (!all_result.IsValid())
	{
	  fprintf(stderr,"Remote name lookup of <%s>\n", name);
	  //ConnectNameServer(NULL);
	  YARPOutputSocket namer;
	  ::ConnectNameServer(namer);
	  
	  NameServiceHeader hdr;
	  hdr.port = 0;
	  hdr.machine_length = 0;
	  hdr.key_length = strlen(name)+1;
	  hdr.spare2 = hdr.spare1 = -1;
	  hdr.request_type = NAME_LOOKUP;
	  
	  mutex.Wait();
	  namer.SendBegin((char*)(&hdr),sizeof(hdr));
	  namer.SendContinue((char*)name,hdr.key_length);
	  char reply[256];
	  int result = namer.SendEnd(reply,sizeof(reply));
	  mutex.Post();
	  if (result>=0 && reply[0]!='?')
	    {
	      fprintf(stderr,"Name <%s> is being directed to [%s]\n", name, reply);
	      all_result = YARPSocketNameService::LocateName(reply);
	    }
	  else
	    {
	      if (result>=0)
		{
		  fprintf(stderr,"Couldn't find <%s>\n", name);
		}
	      else
		{
		  fprintf(stderr,"Couldn't connect to name service to lookup <%s>\n", name);
		}
	    }
	}
    }
  else
    {
      fprintf(stderr,"Socket name lookup of <%s>\n", name);
      all_result = YARPSocketNameService::LocateName(name);
    }

  return all_result;
}


/*
int YARPNameService::ReadOffset(int delta, int absolute)
{
  return YARPSocketNameService::ReadOffset(delta,absolute);
}
*/
