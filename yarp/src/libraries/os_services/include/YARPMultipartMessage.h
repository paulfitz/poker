/*
paulfitz Tue May 22 15:34:43 EDT 2001
 */

#ifndef YARPMultipartMessage_INC
#define YARPMultipartMessage_INC

#include "YARPAll.h"

#include <stdlib.h>


class YARPMultipartMessage
{
protected:
  void *system_resource;
  int top_index, length, owned;
public:
  YARPMultipartMessage()
    { system_resource = NULL; top_index = -1; length = -1; owned = 1; }

  YARPMultipartMessage(int n_length)
    { system_resource = NULL; top_index = -1; length = -1; 
      Resize(n_length);  owned = 1; }

  YARPMultipartMessage(void *buffer, int entries)
    { owned = 0;  system_resource = buffer;  top_index = entries-1;
      length = entries; }

  virtual ~YARPMultipartMessage();

  void Resize(int n_length);

  void Set(int index, char *buffer, int buffer_length);
  char *GetBuffer(int index);
  int GetBufferLength(int index);

  void Reset();

  int GetParts()       { return top_index+1; }
  void *GetRawBuffer() { return system_resource; }

};


#endif
