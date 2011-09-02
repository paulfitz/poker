#include <stdio.h>
#include <string.h>
#include "RefCounted.h"

#ifdef SINGLE_MUTEX_FOR_REFCOUNTED
Sema refcounted_sema(1);
#endif
Sema allocation_sema(1);

void *Buffer::Clone(int *needed)
{
  // Need to be careful to do everything atomically
  mutex.Wait();

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

  mutex.Post();
  return ptr;
}
