#include <stdio.h>

#include "YARPSyncComm.h"
#include "YARPNativeSyncComm.h"
#include "YARPSocketSyncComm.h"
#include "YARPNameID_defs.h"
#include "YARPNameService.h"
#include "YARPNativeNameService.h"

static void Complete(YARPNameID& dest)
{
  if (dest.GetMode()==YARP_NAME_MODE_NULL)
    {
      dest = YARPNameService::GetRegistration();
    }
  if (dest.GetMode()==YARP_NAME_MODE_NULL)
    {
      if (YARPNativeNameService::IsNonTrivial())
	{
	  dest = YARPNameID(YARP_NAME_MODE_NATIVE,0);
	}
      else
	{
	  dest = YARPNameID(YARP_NAME_MODE_SOCKET,0);
	}
    }
}

int YARPSyncComm::Send(YARPNameID dest, char *buffer, int buffer_length,
		       char *return_buffer, int return_buffer_length)
{
  Complete(dest);
  if (dest.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::Send(dest,buffer,buffer_length,
				      return_buffer, return_buffer_length);
    }
  return YARPSocketSyncComm::Send(dest,buffer,buffer_length,
				  return_buffer, return_buffer_length);
}


YARPNameID YARPSyncComm::BlockingReceive(YARPNameID src, char *buffer, 
				  int buffer_length)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::BlockingReceive(src,buffer,buffer_length);

    }
  return YARPSocketSyncComm::BlockingReceive(src,buffer,buffer_length);
}


YARPNameID YARPSyncComm::PollingReceive(YARPNameID src, char *buffer, 
				 int buffer_length)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::PollingReceive(src,buffer,buffer_length);
    }
  return YARPSocketSyncComm::PollingReceive(src,buffer,buffer_length);
}


int YARPSyncComm::ContinuedReceive(YARPNameID src, char *buffer, 
				   int buffer_length)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::ContinuedReceive(src,buffer,buffer_length);
    }
  return YARPSocketSyncComm::ContinuedReceive(src,buffer,buffer_length);
}


int YARPSyncComm::Reply(YARPNameID src, char *buffer, int buffer_length)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::Reply(src,buffer,buffer_length);
    }
  return YARPSocketSyncComm::Reply(src,buffer,buffer_length);
}


int YARPSyncComm::Send(YARPNameID dest, YARPMultipartMessage& msg,
		YARPMultipartMessage& return_msg)
{
  Complete(dest);
  if (dest.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::Send(dest,msg,return_msg);
    }
  return YARPSocketSyncComm::Send(dest,msg,return_msg);
}


YARPNameID YARPSyncComm::BlockingReceive(YARPNameID src, 
					 YARPMultipartMessage& msg)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::BlockingReceive(src,msg);
    }
  return YARPSocketSyncComm::BlockingReceive(src,msg);
}


YARPNameID YARPSyncComm::PollingReceive(YARPNameID src, 
					YARPMultipartMessage& msg)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::PollingReceive(src,msg);
    }
  return YARPSocketSyncComm::PollingReceive(src,msg);
}


int YARPSyncComm::Reply(YARPNameID src, YARPMultipartMessage& msg)
{
  Complete(src);
  if (src.GetMode()==YARP_NAME_MODE_NATIVE)
    {
      return YARPNativeSyncComm::Reply(src,msg);

    }
  return YARPSocketSyncComm::Reply(src,msg);
}


