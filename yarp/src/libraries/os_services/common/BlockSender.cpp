#include <iostream>
#ifndef __QNX4__
using namespace std;
#endif
//#include <sys/kernel.h>
//#include <sys/sendmx.h>
#include "BlockSender.h"

#include "debug.h"

#include <errno.h>

#include "YARPSyncComm.h"

//#undef DEBUG_LEVEL
//#define DEBUG_LEVEL 100

int BlockSender::Fire()
{
  int result = 0;
  if (pieces>0)
    {
      DBG(95) printf("Sending %d pieces\n",pieces);
      errno = 0;
      BlockUnit *bu = entries.begin();
      YARPMultipartMessage msg(bu,pieces);
      YARPMultipartMessage reply_msg(1);
      char buf[100];
      reply_msg.Set(0,buf,sizeof(buf));
      result = YARPSyncComm::Send(pid,msg,reply_msg);
      pieces = 0;
      DBG(95) printf("Sent %d pieces\n",pieces);
    }
  cursor = entries.begin();
  if (result==-1)
    {
      failed = 1;
      DBG(45) cout << "*** BlockSender::Fire() failed, err#" << errno << endl;
    }
  return (result!=-1);
}

int BlockSender::AddPiece(char *buffer, int len)
{
  DBG(95) printf("Adding piece, length %d (avail %d)\n", len, available);
  if (cursor == entries.end())
    {
      if (__debug_level>=50)
	{
	  cout << "*** NEW stl " << __FILE__ << ":" << __LINE__ << endl;
	}
      cursor = entries.insert(cursor,BlockUnit(buffer,len));
    }
  else
    {
      cursor->Set(buffer,len);
    }
  pieces++;
  cursor++;
  return 1;
}

int BlockSender::Add(char *buffer, int len)
{
  while (len>0)
    {
      if (len>available)
	{
	  AddPiece(buffer,available);
	  len -= available;
	  buffer += available;
	  Fire();
	  available = max_packet;
	}
      else
	{
	  AddPiece(buffer,len);
	  available -= len;
	  if (available == 0)
	    {
	      Fire();
	      available = max_packet;
	    }
	  len = 0;
	}
    }
  return 1;
}


