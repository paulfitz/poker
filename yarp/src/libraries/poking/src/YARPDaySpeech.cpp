#ifdef __LINUX__


#include "YARPAll.h"

#include <iostream>
#include <vector>
using namespace std;

#include "YARPPhoneme.h"
//#include "YARPBottle.h"
#include "YARPTime.h"

#include <fstream>
#include <string>

#include <assert.h>

#include "YARPDaySpeech.h"

#define dbg_printf if(0)printf

//typedef hash_map<long int, int, hash<long int>, equal_to<long int> > hash_li;
typedef YARPDaySpeech::hash_li hash_li;


static long int GetKey(const YARPPhoneme& ph)
{
  static hash_li table;
  static int id = 1;
  hash_li::iterator it = table.find(ph.key);
  if (it==table.end())
    {
      table[ph.key] = id;
      id++;
      it = table.find(ph.key);
    }
  return (*it).second;
}

static long int Combine(long int id1, long int id2)
{
  return id1*100+id2;
}

/*
static float Compare(YARPPhonemes& phs1, YARPPhonemes& phs2)
{
  int sz1 = phs1.size();
  int sz2 = phs2.size();
  int overlap1 = 0, overlap2 = 0;
  hash_li cmp, cmp2;
  for (YARPPhonemes::iterator ph=phs1.begin(); ph!=phs1.end(); ph++)
    {
      cmp[GetKey(*ph)] = 1;
    }
  for (YARPPhonemes::iterator ph=phs2.begin(); ph!=phs2.end(); ph++)
    {
      if (cmp.find(GetKey(*ph))!=cmp.end())
	{
	  overlap1++;
	}
      cmp2[GetKey(*ph)] = 1;
    }
  for (YARPPhonemes::iterator ph=phs1.begin(); ph!=phs1.end(); ph++)
    {
      if (cmp2.find(GetKey(*ph))!=cmp.end())
	{
	  overlap2++;
	}
    }
  if (sz1>=1 && sz2>=1)
    {
      if (GetKey(*phs1.begin())==GetKey(*phs2.begin()))
	{
	  overlap1++;
	}
    }
  if (sz1+sz2<1)
    {
      sz1 = 1;
    }
  //if (sz2<=3)
  //{
  //  overlap2++;
  //}
  float sim = (overlap1+overlap2)/((float)(sz1+sz2));
  //printf("     cheesy similarity %g (%d %d %d %d)\n", sim,
  //sz1, sz2, overlap1, overlap2);
  return sim;
}
*/

static int Fill(YARPPhonemes& phs1, hash_li& cmp)
{
  int total = 0;
  for (YARPPhonemes::iterator ph=phs1.begin(); ph!=phs1.end(); ph++)
    {
      long int id1 = GetKey(*ph);
      int ct = 8;
      for (YARPPhonemes::iterator ph2=ph; ph2!=phs1.end(); ph2++)
	//for (YARPPhonemes::iterator ph2=phs1.begin(); ph2!=phs1.end(); ph2++)
	{
	  if (ph2!=ph)
	    {
	      long int id2 = GetKey(*ph2);
	      long int id = Combine(id1,id2);
	      if (cmp.find(id)==cmp.end())
		{
		  cmp[id] = ct;
		}
	      else
		{
		  cmp[id] += ct;
		}
	      total += ct;
	      ct/=2;
	    }
	}
    }
  return total;
}


static int Inter(YARPPhonemes& phs1, hash_li& cmp)
{
  int total = 0;
  for (YARPPhonemes::iterator ph=phs1.begin(); ph!=phs1.end(); ph++)
    {
      long int id1 = GetKey(*ph);
      //for (YARPPhonemes::iterator ph2=phs1.begin(); ph2!=phs1.end(); ph2++)
      for (YARPPhonemes::iterator ph2=ph; ph2!=phs1.end(); ph2++)
	{
	  if (ph2!=ph)
	    {
	      long int id2 = GetKey(*ph2);
	      long int id = Combine(id1,id2);
	      if (cmp.find(id)!=cmp.end())
		{
		  total += cmp[id];
		}
	    }
	}
    }
  return total;
}

static float CompareBase(YARPPhonemes& phs1, YARPPhonemes& phs2)
{
  int sz1 = phs1.size();
  int sz2 = phs2.size();
  int overlap1 = 0, overlap2 = 0;
  hash_li cmp, cmp2;

  int total1 = Fill(phs1,cmp);
  int total2 = Fill(phs2,cmp2);

  overlap1 = Inter(phs1,cmp2);
  overlap2 = Inter(phs2,cmp);

  if (total1<5) total1 = 5;
  if (total2<5) total2 = 5;
  int total = total1+total2;

  float sim1 = (overlap1)/((float)(total1));
  float sim2 = (overlap2)/((float)(total2));
  float sim = sim1;
  if (sim2<sim) sim = sim2;
  //printf("     cheesy similarity %g (%d %d %d %d)\n", sim,
  //sz1, sz2, overlap1, overlap2);
  /*
  if (sz1<0) sz1 = 1;
  if (sz2<0) sz2 = 1;
  int sz0 = sz1;
  if (sz2<sz0) sz0 = sz2;
  if (fabs(sz1-sz2)
  sim *= 2;
  if (sim>1) sim = 1;
  */
  if (sim>1) sim = 1;
  //sim = 1 - (sqrt(fabs(1-sim)));
  //sim = sqrt(fabs(sim));
  return sim;
}

/*
static void old_main(int argc, char *argv[])
{
  assert(argc==3);
  argc--;
  argv++;
  const char *fname1 = argv[0];
  argc--;
  argv++;
  const char *fname2 = argv[0];

  fprintf(stderr,"Comparing %s and %s\n", 
	  fname1, fname2);
  double cmp = 0;

  char buf1[256] = "";
  char buf2[256] = "";
  {
    ifstream fin(fname1);
    fin.getline(buf1,sizeof(buf1));
  }
  {
    ifstream fin(fname2);
    fin.getline(buf2,sizeof(buf2));
  }
  
  YARPPhonemes phs1, phs2;
  YARPPhonemeConverter cvt;
  phs1 = cvt.ConvertMultiple(PHONEMES_VIAVOICE_ASR,buf1);
  phs2 = cvt.ConvertMultiple(PHONEMES_VIAVOICE_ASR,buf2);
  cmp = Compare2(phs1,phs2);
  if (cmp>1) cmp = 1;

  printf("%g\n", cmp);
}
*/


float YARPDaySpeech::Compare(const YARPPhonemes& phs1, 
			     const YARPPhonemes& phs2)
{
  YARPPhonemes phs1b, phs2b;
  // silly, silly
  phs1b = phs1;
  phs2b = phs2;
  float cmp = CompareBase(phs1b,phs2b);
  if (cmp>1) cmp = 1;
  return cmp;
}

float YARPDaySpeech::Compare(const char *phs1, const char *phs2)
{
  YARPPhonemes phs1b, phs2b;
  YARPPhonemeConverter cvt;
  phs1b = cvt.ConvertMultiple(PHONEMES_VIAVOICE_ASR,phs1);
  phs2b = cvt.ConvertMultiple(PHONEMES_VIAVOICE_ASR,phs2);
  return Compare(phs1b,phs2b);
}



void YARPDaySpeech::Add(const YARPPhonemes& phs1)
{
  YARPPhonemes phs1b;
  phs1b = phs1;
  dict_total += Fill(phs1b,dict);
}

void YARPDaySpeech::Add(const char *phs1)
{
  YARPPhonemes phs1b;
  YARPPhonemeConverter cvt;
  phs1b = cvt.ConvertMultiple(PHONEMES_VIAVOICE_ASR,phs1);
  Add(phs1b);
}



float YARPDaySpeech::Compare(const YARPPhonemes& phs1)
{
  YARPPhonemes phs1b;
  phs1b = phs1;

  long int overlap1 = 0, overlap2 = 0;
  hash_li cmp, cmp2;

  float total1 = dict_total;

  if (total1<5) total1 = 5;

  overlap1 = Inter(phs1b,dict);

  return overlap1/total1;
}

float YARPDaySpeech::Compare(const char *phs1)
{
  YARPPhonemes phs1b;
  YARPPhonemeConverter cvt;
  phs1b = cvt.ConvertMultiple(PHONEMES_VIAVOICE_ASR,phs1);
  return Compare(phs1b);
}



#endif
