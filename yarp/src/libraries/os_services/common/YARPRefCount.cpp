#include "YARPRefCount.h"

#ifdef YARPREFCOUNT_SINGLE_MUTEX
#include "YARPSemaphore.h"
YARPSemaphore yref_sema(1);
#define YR_WAIT yref_sema.Wait()
#define YR_POST yref_sema.Post()
#endif

PYARPRefCount AddYarpRefCount(PYARPRefCount& ref)
{
  PYARPRefCount result = NULL;
  YR_WAIT;
  if (ref==NULL)
    {
      ref = new YARPRefCount;
    }
  assert(ref!=NULL);
  ref->ref_count++;
  result = ref;
  YR_POST;
  return result;
}

YARPRefCount::YARPRefCount()
{
  ref_count = 1;
}

YARPRefCount::~YARPRefCount()
{
}

void YARPRefCount::AddRef()
{
  YR_WAIT;
  ref_count++;
  YR_POST;
}

void YARPRefCount::ZeroRef()
{
  YR_WAIT;
  ref_count = 0;
  YR_POST;
}

void YARPRefCount::RemoveRef()
{
  YR_WAIT;
  assert(ref_count>0);
  ref_count--;
  if (ref_count==0)
    {
      YR_POST;
      Destroy();
    }
  else
    {
      YR_POST;
    }
}

/*
void *YARPRefCount::Clone(int always_clone)
{
  // Need to be careful to do everything atomically
  YR_WAIT;

  Buffer *ptr = NULL;
  int copy = (GetReferenceCount()>1);

  if (needed!=NULL)
    {
      *needed = copy;
    }
  if (copy)
    {
      assert(memory!=NULL);
      ptr = new Buffer(GetLength());
      assert(ptr!=NULL);
      memcpy(ptr->memory,memory,GetLength());
      ref_count--;
    }

  YR_POST;
  return ptr;
}
*/


