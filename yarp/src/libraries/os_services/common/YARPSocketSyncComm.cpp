#include <assert.h>
#include <stdio.h>

#include "YARPSocketSyncComm.h"

#include "YARPSocket.h"

#include "ThreadInput.h"
#include "YARPNameID_defs.h"
#include "BlockPrefix.h"

#include "debug.h"


/*
  Code sends along a preamble to permit transparent bridging.  Preamble
  is otherwise ignored completely. since the QNX spec doesn't have it.
 */


int YARPSocketSyncComm::Send(YARPNameID dest, char *buffer, 
				 int buffer_length,
				 char *return_buffer, int return_buffer_length)
{
  YARPOutputSocket os;
  assert(dest.IsConsistent(YARP_NAME_MODE_SOCKET));
  os.SetIdentifier(dest.GetRawIdentifier());
  os.InhibitDisconnect();
  //os.SendBegin(buffer,buffer_length);
  //return os.SendEnd(return_buffer,return_buffer_length);

  BlockPrefix prefix;
  prefix.total_blocks = 1;
  prefix.reply_blocks = 1;
  prefix.size = buffer_length;
  prefix.reply_size = return_buffer_length;
  os.SendBegin((char*)(&prefix),sizeof(prefix));
  os.SendContinue(buffer,buffer_length);
  char ch = -1;
  os.SendReceivingReply(&ch,1);
  if (ch==0)
    {
      return os.SendEnd(return_buffer,return_buffer_length);
    }
  return -1;
}


YARPNameID YARPSocketSyncComm::BlockingReceive(YARPNameID src, char *buffer, 
				    int buffer_length)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  assert(ts!=NULL);
  //int id = -1;
  //int ct = ts->ReceiveBegin(buffer,buffer_length, &id);
  //return YARPNameID(YARP_NAME_MODE_SOCKET,id);

  BlockPrefix prefix;
  int id = -1;
  int ct = ts->ReceiveBegin((char*)(&prefix),sizeof(prefix),&id);
  if (ct>=0)
    {
      if (prefix.size<0)
	{
	  for (int i=0;i<prefix.total_blocks+prefix.reply_blocks && ct>=0; i++)
	    {
	      NetInt32 x;
	      ct = ts->ReceiveContinue(id,(char*)(&x),sizeof(x));
	    }
	}
      ct = ts->ReceiveContinue(id,buffer,buffer_length);
    }
  if (ct<0) 
    {
      //printf("Failed in YARPSocketSyncComm::BlockingReceive\n");
      id = -1;
    }
  return YARPNameID(YARP_NAME_MODE_SOCKET,id);
}


YARPNameID YARPSocketSyncComm::PollingReceive(YARPNameID src, char *buffer, 
				   int buffer_length)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  assert(ts!=NULL);
  //int id = -1;
  //int ct = ts->PollingReceiveBegin(buffer,buffer_length, &id);
  //return YARPNameID(YARP_NAME_MODE_SOCKET,id);

  BlockPrefix prefix;
  int id = -1;
  int ct = ts->PollingReceiveBegin((char*)(&prefix),sizeof(prefix),&id);
  if (ct>=0)
    {
      if (prefix.size<0)
	{
	  for (int i=0;i<prefix.total_blocks+prefix.reply_blocks && ct>=0; i++)
	    {
	      NetInt32 x;
	      ct = ts->ReceiveContinue(id,(char*)(&x),sizeof(x));
	    }
	}
      ct = ts->ReceiveContinue(id,buffer,buffer_length);
    }
  if (ct<0) id = -1;
  return YARPNameID(YARP_NAME_MODE_SOCKET,id);
}


int YARPSocketSyncComm::ContinuedReceive(YARPNameID src, char *buffer, 
					 int buffer_length)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  assert(ts!=NULL);

  int ct = -1;
  if (src.IsValid())
    {
      ct = ts->ReceiveContinue(src.GetRawIdentifier(),buffer,buffer_length);
    }
  return ct;
}


int YARPSocketSyncComm::Reply(YARPNameID src, char *buffer, 
				  int buffer_length)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  assert(ts!=NULL);
  char ch = 0;
  ts->ReceiveReplying(src.GetRawIdentifier(),&ch,1);
  return ts->ReceiveEnd(src.GetRawIdentifier(),buffer,buffer_length);
}


int YARPSocketSyncComm::InvalidReply(YARPNameID src)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  assert(ts!=NULL);
  char ch = 0;
  return ts->ReceiveEnd(src.GetRawIdentifier(),&ch,1);
}


int YARPSocketSyncComm::Send(YARPNameID dest, YARPMultipartMessage& msg,
				 YARPMultipartMessage& return_msg)
{
  YARPOutputSocket os;
  assert(dest.IsConsistent(YARP_NAME_MODE_SOCKET));
  os.SetIdentifier(dest.GetRawIdentifier());
  os.InhibitDisconnect();
  int send_parts = msg.GetParts();
  int return_parts = return_msg.GetParts();
  assert(send_parts>=1);
  assert(return_parts>=1);
  int i;

  DBG(50) printf("Get %d send_parts, %d return_parts\n", send_parts, 
		 return_parts);

  /* preamble code begins */
  BlockPrefix prefix;
  prefix.total_blocks = send_parts;
  prefix.reply_blocks = return_parts;
  prefix.size = -1;
  prefix.reply_size = -1;
  os.SendBegin((char*)(&prefix),sizeof(prefix));
  for (i=0;i<send_parts; i++)
    {
      NetInt32 x = msg.GetBufferLength(i);
      os.SendContinue((char*)(&x),sizeof(x));
    }
  for (i=0;i<return_parts; i++)
    {
      NetInt32 x = return_msg.GetBufferLength(i);
      os.SendContinue((char*)(&x),sizeof(x));
    }
  os.SendContinue(msg.GetBuffer(0),msg.GetBufferLength(0));
  /* preamble code ends */

  //os.SendBegin(msg.GetBuffer(0),msg.GetBufferLength(0));

  for (i=1;i<send_parts; i++)
    {
      os.SendContinue(msg.GetBuffer(i),msg.GetBufferLength(i));      
    }
  char ch = -1;
  os.SendReceivingReply(&ch,1);
  if (ch==0)
    {
      for (i=0;i<return_parts-1; i++)
	{
	  os.SendReceivingReply(return_msg.GetBuffer(i),
				return_msg.GetBufferLength(i));
	}
      int result = os.SendEnd(return_msg.GetBuffer(return_parts-1),
			  return_msg.GetBufferLength(return_parts-1));
      return result;
    }
  return -1;
}


YARPNameID YARPSocketSyncComm::BlockingReceive(YARPNameID src, 
					 YARPMultipartMessage& msg)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(ts!=NULL);
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  int id = -1;
  int receive_parts = msg.GetParts();
  assert(receive_parts>=1);

  /* preamble code begins */
  BlockPrefix prefix;
  ts->ReceiveBegin((char*)(&prefix),sizeof(prefix), &id);
  if (id!=-1)
    {
      if (prefix.size<0)
	{
	  int i;
	  NetInt32 x;
	  for (i=0;i<prefix.total_blocks; i++)
	    {
	      ts->ReceiveContinue(id,(char*)(&x),sizeof(x));
	    }
	  for (i=0;i<prefix.reply_blocks; i++)
	    {
	      ts->ReceiveContinue(id,(char*)(&x),sizeof(x));
	    }
	}
      int ct = ts->ReceiveContinue(id,msg.GetBuffer(0),msg.GetBufferLength(0));
    }
  /* preamble code ends */

  //  int ct = ts->ReceiveBegin(msg.GetBuffer(0),msg.GetBufferLength(0), &id);
  if (id!=-1)
    {
      for (int i=1; i<receive_parts; i++)
	{
	  int ct2 = ts->ReceiveContinue(id,msg.GetBuffer(i),msg.GetBufferLength(i));
	  DBG(5) printf("^^^ additional receive of %d bytes\n", ct2);
	}
    }
  return YARPNameID(YARP_NAME_MODE_SOCKET,id);
}


YARPNameID YARPSocketSyncComm::PollingReceive(YARPNameID src, 
					YARPMultipartMessage& msg)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  assert(ts!=NULL);
  int id = -1;
  int receive_parts = msg.GetParts();
  assert(receive_parts>=1);

  /* preamble code begins */
  BlockPrefix prefix;
  ts->PollingReceiveBegin((char*)(&prefix),sizeof(prefix), &id);
  if (id!=-1)
    {
      if (prefix.size<0)
	{
	  int i;
	  NetInt32 x;
	  for (i=0;i<prefix.total_blocks; i++)
	    {
	      ts->ReceiveContinue(id,(char*)(&x),sizeof(x));
	    }
	  for (i=0;i<prefix.reply_blocks; i++)
	    {
	      ts->ReceiveContinue(id,(char*)(&x),sizeof(x));
	    }
	}
      int ct = ts->ReceiveContinue(id,msg.GetBuffer(0),msg.GetBufferLength(0));
    }
  /* preamble code ends */

  //int ct = ts->PollingReceiveBegin(msg.GetBuffer(0),msg.GetBufferLength(0), 
  //			   &id);
  if (id!=-1)
    {
      for (int i=1; i<receive_parts; i++)
	{
	  ts->ReceiveContinue(id,msg.GetBuffer(i),msg.GetBufferLength(i));
	}
    }
  return YARPNameID(YARP_NAME_MODE_SOCKET,id);
}



int YARPSocketSyncComm::Reply(YARPNameID src, YARPMultipartMessage& msg)
{
  YARPInputSocket *ts = GetThreadSocket();
  assert(ts!=NULL);
  int reply_parts = msg.GetParts();
  assert(reply_parts>=1);

  assert(src.IsConsistent(YARP_NAME_MODE_SOCKET));
  char ch = 0;
  ts->ReceiveReplying(src.GetRawIdentifier(),&ch,1);
  for (int i=0; i<reply_parts-1; i++)
    {
      ts->ReceiveReplying(src.GetRawIdentifier(),msg.GetBuffer(i),
			  msg.GetBufferLength(i));
    }

  return ts->ReceiveEnd(src.GetRawIdentifier(),msg.GetBuffer(reply_parts-1),
			msg.GetBufferLength(reply_parts-1));
}




