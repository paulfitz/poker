/*
paulfitz Mon May 21 16:51:13 EDT 2001
*/
#ifndef YARPRefCounted_INC
#define YARPRefCounted_INC

#include <assert.h>

#include "YARPAll.h"

#include "YARPBool.h"

// All the operations in YARPRefCounted need to be mutexed.

#define YARPREFCOUNT_SINGLE_MUTEX

#ifndef YARPREFCOUNT_SINGLE_MUTEX
#error "Code not written for per-ref mutex"
#endif

class YARPRefCount;

typedef YARPRefCount *PYARPRefCount;

PYARPRefCount AddYarpRefCount(PYARPRefCount& ref);


class YARPRefCount
{
protected:

  int ref_count;

public:

  YARPRefCount();
  virtual ~YARPRefCount();

  int GetReferenceCount() { return ref_count; }

  void AddRef();
  void RemoveRef();
  void ZeroRef();

  virtual void Destroy() {}

  friend PYARPRefCount AddYarpRefCount(PYARPRefCount& ref);
};

class YARPRefCounted : public YARPRefCount
{

public:

  virtual void Destroy() {}

  // returns NULL if only one user of object and always_clone is false, 
  // otherwise should return a copy of the object if possible
  virtual void *Clone(bool always_clone)
    { 
      return NULL; 
    }
};


class YARPRefCountedBuffer : public YARPRefCounted
{
protected:
  char *memory;
  int len;
  int owned;

public:

  YARPRefCountedBuffer()
    {
      memory = NULL; owned = 0;
    }

  YARPRefCountedBuffer(int nlen) 
    { 
      memory = new char[nlen];
      assert(memory!=NULL);
      owned = 1;
      len = nlen; 
    }

  YARPRefCountedBuffer(char *n_memory, int n_len, int n_owned = 0)
    { memory = n_memory;  len = n_len;  owned = n_owned; }

  virtual ~YARPRefCountedBuffer()
    {
      if (memory!=NULL && owned)
	{
	  delete[] memory;
	}
      memory = NULL;
    }  

  void Set(char *n_memory, int n_len, int n_owned = 0)
    { 
      assert(memory==NULL);  
      memory = n_memory;  len = n_len;  
      owned = n_owned; 
    }

  virtual void *Clone(int always_clone)
    { int is_implemented=0; assert(is_implemented==1); return NULL; }

  char *GetRawBuffer()
    { return memory; }

  int GetLength()
    { return len; }

  void ForceLength(int n_len)
    { len = n_len; }

  int GetReferenceCount()
    { return ref_count; }

};


// Intended to be applied to RefCounted objects only
template <class T>
class YARPRefCountedPtr
{
public:
  T *ptr;

  YARPRefCountedPtr()          { ptr = NULL; }
  YARPRefCountedPtr(T *nptr)   { ptr = NULL;  Take(nptr); }

  virtual ~YARPRefCountedPtr() { Reset();    }

  T *Ptr() { return ptr; }

  void Set(T *nptr)
    {
      Reset();
      ptr = nptr;
      assert(ptr!=NULL);
      ptr->AddRef();
    }

  void Take(T *nptr)
    {
      Reset();
      ptr = nptr;
    }

  void Reset()
    { 
      if (ptr!=NULL) ptr->RemoveRef(); 
      ptr = NULL; 
    }

  /*
  void Switch(CountedPtr<T>& peer)
    {
      T *tmp = ptr;
      ptr = peer.ptr;
      peer.ptr = tmp;
    }
  */

  void MakeIndependent()
    {
      int needed;
      T *nptr;
      if (ptr!=NULL)
	{
	  nptr = (T*)ptr->Clone(false);
	  if (nptr!=NULL)
	    {
	      Take(nptr);
	    }
	}
    }
};

#endif
