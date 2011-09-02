#ifndef YARPDAYSPEECH_INC
#define YARPDAYSPEECH_INC

#include <hash_map>

#include "YARPPhoneme.h"

class YARPDaySpeech
{
public:
  typedef hash_map<long int, int, hash<long int>, equal_to<long int> > hash_li;

  hash_li dict;
  long int dict_total;
  
  YARPDaySpeech()
    {
      dict_total = 0;
    }

  float Compare(const YARPPhonemes& phs1, const YARPPhonemes& phs2);
  float Compare(const char *phs1, const char *phs2);

  void Add(const YARPPhonemes& phs1);
  void Add(const char *phs1);


  float Compare(const YARPPhonemes& phs1);
  float Compare(const char *phs1);
};

#endif

