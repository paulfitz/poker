
//#include <sys/kernel.h>
#include "BlockReceiver.h"

#include "debug.h"

#include "YARPSyncComm.h"

int BlockReceiver::End()
{
  if (reply_pending) // && !failed)
    {
      if (pid.IsValid()) 
	{
	  char buf[] = "ok";
	  int r = YARPSyncComm::Reply(pid,buf,sizeof(buf));
	  if (r==-1)
	    {
	      DBG(50) printf("BlockReceiver failed 1\n");
	      failed = 1;
	    }
	}
      reply_pending = 0;
    }
  return !failed;
}

int BlockReceiver::Get()
{
  if (!has_msg && !failed)
    {
      DBG(95) printf("Receiving...\n");
      char buf[100];
      pid = YARPSyncComm::BlockingReceive(pid,buf,0);
      if (pid.IsValid())
	{
	  has_msg = 1;
	  offset = 0;
	  reply_pending = 1;
	}
      else
	{
	  DBG(50) printf("BlockReceiver failed 2\n");
	  failed = 1;
	  pid.Invalidate();
	}
    }
  return has_msg;
}

int BlockReceiver::Get(char *buffer, int len)
{
  int bytes;
  int target = len;
  int terminated = 0;
  while (len>0 && pid.IsValid() && !terminated)
    {
      Get();
      if (has_msg)
	{
	  DBG(95) printf("Reading %d, %d remaining...\n",target,len);
	  //bytes = Readmsg(pid,offset,buffer,len);
	  bytes = YARPSyncComm::ContinuedReceive(pid,buffer,len);
	  if (bytes==0)
	    {
	      End();
	      has_msg = 0;
	      Get();
	      if (has_msg)
		{
		  bytes = YARPSyncComm::ContinuedReceive(pid,buffer,len);
		}
	    }
	  DBG(95) printf("Got %d of %d...\n",bytes,target);
	  if (bytes<0)
	    {
	      failed = 1;
	      pid.Invalidate();
	    }
	  else
	    {
	      if (bytes>len)
		{
		  failed = 1;
		  terminated = 1;
		}
	      if (bytes>0)
		{
		  buffer += bytes;
		  len -= bytes;
		  offset += bytes;
		}
	    }
	}
      else
	{
	  terminated = 1;
	}
    }
  DBG(95) printf("Stopping %d failed=%d terminated=%d...\n",target,
		 failed,terminated);
  return !failed;
}

