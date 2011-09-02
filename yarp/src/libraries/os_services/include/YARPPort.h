/*
paulfitz Sat May 26 22:34:44 EDT 2001
*/
#ifndef YARPPort_INC
#define YARPPort_INC

#include "YARPAll.h"

#include <assert.h>

#include "YARPBool.h"
#include "YARPPortContent.h"
#include "YARPNameID.h"

class YARPPort
{
protected:
  void *system_resource;  
  YARPPortContent *content;
public:
  virtual int Register(const char *name);

  int Connect(const char *name);

  YARPPort();
  virtual ~YARPPort();

  virtual YARPPortContent *CreateContent() = 0;

  YARPPortContent& Content();

  static int Connect(const char *src_name, const char *dest_name);

  int IsReceiving();

  int IsSending();

  void FinishSend();

  void Deactivate();

  static void DeactivateAll();
};

class YARPInputPort : public YARPPort
{
public:
  enum
  {
    NO_BUFFERS,
    DOUBLE_BUFFERS,
    TRIPLE_BUFFERS,
    DEFAULT_BUFFERS = TRIPLE_BUFFERS
  };

  YARPInputPort(int n_service_type = DEFAULT_BUFFERS);
  virtual ~YARPInputPort();

  virtual int Register(const char *name);

  bool Read(bool wait=true);

  virtual void OnRead() {}

  YARPPortContent& Content() { return YARPPort::Content(); }
};


class YARPOutputPort : public YARPPort
{
public:
  enum
  {
    MANY_OUTPUTS,
    SINGLE_OUTPUT,
    DEFAULT_OUTPUTS = MANY_OUTPUTS
  };

  YARPOutputPort(int n_service_type = MANY_OUTPUTS);
  virtual ~YARPOutputPort();

  virtual int Register(const char *name);

  YARPPortContent& Content();

  void Write(bool wait=false);

  virtual void OnWrite() {}
};


template <class T>
class YARPInputPortOf : public YARPInputPort
{
public:
  YARPInputPortOf(int n_service_type = DEFAULT_BUFFERS) :
     YARPInputPort(n_service_type) {}
  
  virtual YARPPortContent *CreateContent() 
    { return new YARPPortContentOf<T>; }

  T& Content()
    {
      assert(content!=NULL);
      return ((YARPPortContentOf<T>*)content)->Content();
    }
};

template <class T>
class YARPOutputPortOf : public YARPOutputPort
{
public:
  virtual YARPPortContent *CreateContent() 
    { return new YARPPortContentOf<T>; }

  T& Content()
    {
      YARPOutputPort::Content();
      assert(content!=NULL);
      return ((YARPPortContentOf<T>*)content)->Content();
    }
};

template <class T>
class YARPBasicInputPort : public YARPInputPort
{
public:
  YARPBasicInputPort(int n_service_type = DEFAULT_BUFFERS) :
     YARPInputPort(n_service_type) {}

  virtual YARPPortContent *CreateContent() 
    { return new T; }

  T& Content()
    {
      return *((T*)(&YARPPort::Content()));
    }
};

template <class T>
class YARPBasicOutputPort : public YARPOutputPort
{
public:
  YARPBasicOutputPort(int n_service_type = MANY_OUTPUTS) :
    YARPOutputPort(n_service_type)
    {}

  virtual YARPPortContent *CreateContent() 
    { return new T; }

  T& Content()
    {
      return *((T*)(&YARPOutputPort::Content()));
    }
};

#endif
