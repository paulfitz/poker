#ifndef YARPVITERBI_INC
#define YARPVITERBI_INC

#ifdef __QNX__
#include "YARPSafeNew.h"
#endif

#include <assert.h>
#include "YARPImage.h"

class YARPViterbi
{
public:
  int K;
  int T;
  int index;
  int mode;
  int path_valid;
  float best_cost;
  YARPImageOf<YarpPixelFloat> cost;
  YARPImageOf<YarpPixelInt> src;
  YARPImageOf<YarpPixelInt> path;

  YARPViterbi()
    {
      K = T = 0;
      Reset();
    }

  void SetSize(int states, int sequence_length);

  void Reset()
    {
      index = 0;
      mode = 0;
      path_valid = 0;
      best_cost = 0;
    }

  void AssertMode(int n_mode);

  void AddTransition(int s0, int s1, float c);

  void EndTransitions()
    {
      path_valid = 0;
      AssertMode(0);
    }

  void BeginTransitions()
    {
      path_valid = 0;
      AssertMode(1);
    }

  void CalculatePath();

  void ShowPath();

  int length()
    {
      if (index>0)
	{
	  CalculatePath();
	}
      return index;
    }

  int GetPath(int i)
    {
      CalculatePath();
      assert(i<index);
      return path(0,i);
    }

  int operator() (int i)
    {
      return GetPath(i);
    }

  float GetCost()
    {
      CalculatePath();
      return best_cost;
    }

  void CopyPath(YARPGenericImage& out_path)
  {
	  CalculatePath();
	  out_path.CastCopy(path);
  }
};


#endif
