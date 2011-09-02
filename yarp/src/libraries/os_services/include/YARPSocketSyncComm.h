/*
paulfitz Tue May 22 15:34:43 EDT 2001
 */

#ifndef YARPSocketSyncComm_INC
#define YARPSocketSyncComm_INC

#include "YARPAll.h"

#include "YARPSocketNameService.h"
#include "YARPMultipartMessage.h"


class YARPSocketSyncComm
{
public:
  static int Send(YARPNameID dest, char *buffer, int buffer_length,
		  char *return_buffer, int return_buffer_length);
  static YARPNameID BlockingReceive(YARPNameID src, char *buffer, 
				    int buffer_length);
  static YARPNameID PollingReceive(YARPNameID src, char *buffer, 
				   int buffer_length);
  static int ContinuedReceive(YARPNameID src, char *buffer, 
			      int buffer_length);
  static int Reply(YARPNameID src, char *buffer, int buffer_length);
  static int InvalidReply(YARPNameID src);

  static int Send(YARPNameID dest, YARPMultipartMessage& msg,
		  YARPMultipartMessage& return_msg);
  static YARPNameID BlockingReceive(YARPNameID src, YARPMultipartMessage& msg);
  static YARPNameID PollingReceive(YARPNameID src, YARPMultipartMessage& msg);
  static int Reply(YARPNameID src, YARPMultipartMessage& msg);

};


#endif

