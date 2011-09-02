/*
paulfitz Tue May 22 15:34:43 EDT 2001
 */

#ifndef YARPNameID_INC
#define YARPNameID_INC

#include "YARPAll.h"

class YARPNameID
{
public:
  int mode;
  int raw_id;
  
  YARPNameID() { raw_id = 0;  mode = -1; }

  YARPNameID(int n_mode, int n_raw_id)
    { mode = n_mode;  raw_id = n_raw_id; }

  int operator == (const YARPNameID& other)
    { return (raw_id==other.raw_id)&&(mode==other.mode); }

  int operator != (const YARPNameID& other)
    { return !(operator==(other)); }

  int GetMode() { return mode; }
  int GetRawIdentifier() { return raw_id; }
  int IsValid()   { return raw_id>0; }
  int IsGeneric() { return raw_id = 0; }
  int IsError()   { return raw_id<0; }

  int IsConsistent(int n_mode) 
    { return (mode==n_mode) || (mode==-1); }
  
  void Invalidate()
    { raw_id = -1;  mode = -1; }
};

#define YARP_NAMEID_NULL YARPNameID()

#endif
