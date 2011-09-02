// YARPSafeNew.h
// Pasa - June 2001
//	- this is something needed for real use of the libraries.
//		I wasn't sure at the moment whether to commit to YARP os_services
//		I would like to keep the img proc stuff as independent as possible

#ifndef __YARPSafeNewh__
#define __YARPSafeNewh__

#include <assert.h>

//
// There's no serialization. This was switched off at a given point.
//	- we put a loop around the "almost" thread safe "malloc".
//
#define SERIALIZED_NEW_WAIT
#define SERIALIZED_NEW_POST

#ifdef __QNX__
#include <sys/kernel.h>
#endif

#include <new.h>
#include <iostream.h>
#include <malloc.h>

// remove this to print debug messages.
#define DONOTCOMPLAIN 1

#ifdef __QNX__
#ifndef for
#define for if (1) for
#endif
#endif

inline void *
operator new (size_t size)
{
  SERIALIZED_NEW_WAIT;
  void * result = NULL;
  int i = 0;
  while (result == NULL && i < 15)
  {
//	  cout << ">>> allocating memory " << size << endl;
	  result = calloc(1, size);
	  if (result==NULL)
		{
	      Yield();
		  cout << "*** serialized new failed " << i << " requested " << size << endl;
		  i++;
		}
  }
 SERIALIZED_NEW_POST;
  return result;
}

inline void *
operator new[] (size_t size)
{
  SERIALIZED_NEW_WAIT;
  void * result = NULL;
  int i = 0;
	while (result == NULL && i < 15)
	{
//	  cout << ">>> allocating memory " << size << endl;
	  result = calloc(1, size);
	  if (result==NULL)
		{
	      Yield();
			  cout << "*** serialized new failed " << i << " requested " << size << endl;
			  i++;
		}
	}
  SERIALIZED_NEW_POST;

  return result;
}

// Should also serialize delete and delete[]
inline void operator delete (void * ptr)
{
  SERIALIZED_NEW_WAIT;
	if (ptr != NULL)
		free (ptr);
	else
#ifndef DONOTCOMPLAIN
		cout << "*** deleting a NULL ptr, ignoring that" << endl;
#endif
  SERIALIZED_NEW_POST;
}

inline void operator delete[] (void * ptr)
{
  SERIALIZED_NEW_WAIT;

	if (ptr != NULL)
		free (ptr);
	else
#ifdef DONOTCOMPLAIN
		cout << "*** deleting a NULL ptr, ignoring that" << endl;
#endif
  SERIALIZED_NEW_POST;
}

#undef DONOTCOMPLAIN

#endif

