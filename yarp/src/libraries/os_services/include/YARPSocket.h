#ifndef YARPSOCKET_INC
#define YARPSOCKET_INC

#include "YARPAll.h"

#include <stdlib.h>

#include "YARPNetworkTypes.h" // not strictly necessary here

class YARPNetworkObject
{
public:
  static int GetHostName(char *buffer, int buffer_length);
};

class YARPInputSocket : public YARPNetworkObject
{
protected:
  void *system_resources;
  int identifier;
  int assigned_port;

public:

  YARPInputSocket();

  YARPInputSocket(const YARPInputSocket& other);

  virtual ~YARPInputSocket();

  int Close(int reply_id);
  int Close();

  int Register(const char *name);

  int PollingReceiveBegin(char *buffer, int buffer_length, 
			  int *reply_id = NULL);
  int ReceiveBegin(char *buffer, int buffer_length, int *reply_id = NULL);
  int ReceiveContinue(int reply_id, char *buffer, int buffer_length);
  int ReceiveReplying(int reply_id, char *reply_buffer,
		     int reply_buffer_length);
  int ReceiveEnd(int reply_id, char *reply_buffer, int reply_buffer_length);

  int GetIdentifier() const { return identifier; }
  int GetAssignedPort() const { return assigned_port; }
};

class YARPOutputSocket : public YARPNetworkObject
{
protected:
  void *system_resources;
  int identifier;

public:

  YARPOutputSocket();

  virtual ~YARPOutputSocket();

  int Close();

  int Connect(const char *name);

  int SendBegin(char *buffer, int buffer_length);
  int SendContinue(char *buffer, int buffer_length);
  int SendReceivingReply(char *reply_buffer, int reply_buffer_length);
  int SendEnd(char *reply_buffer, int reply_buffer_length);

  void InhibitDisconnect();
  int GetIdentifier();
  void SetIdentifier(int n_identifier);
};

#endif
