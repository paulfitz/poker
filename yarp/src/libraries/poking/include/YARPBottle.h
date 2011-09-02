#ifndef YARPBOTTLE_INC
#define YARPBOTTLE_INC

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "YARPPort.h"
#include "YARPPortContent.h"

#include "YARPNetworkTypes.h"

#include "YARPBottleCodes.h"

#include <vector>
#ifndef __QNX__
using namespace std;
#endif

//#define for if(1) for

class YARPBottle : public YARPPortContent
{
public:
  vector<char> text;
  NetInt32 id;
  NetInt32 len;
  NetInt32 top;
  
  YARPBottle() { id = 0; top = 0; }

  /*
  YARPBottle(const YARPBottle& bottle)
    {
      id = bottle.id;
      len = bottle.len;
      top = bottle.top;
      //text = bottle.text;
      text.erase(text.begin(), text.end());
      for (vector<char>::const_iterator it=bottle.text.begin();
	   it != bottle.text.end(); it++)
      {
	//text.push_back(*it);
      }
      //text.insert(text.begin(), bottle.text.begin(), bottle.text.end());
    }

  YARPBottle& operator = (const YARPBottle& bottle)
  {
      id = bottle.id;
      len = bottle.len;
      top = bottle.top;
    //text = bottle.text;
      text.erase(text.begin(), text.end());
    for (vector<char>::const_iterator it=bottle.text.begin();
	 it != bottle.text.end(); it++)
      {
	//text.push_back(*it);
      }
    //text.insert(text.begin(), bottle.text.begin(), bottle.text.end());
    return *this;
  }
   */
  
  virtual int Read(YARPPortReader& reader)
    {
      reader.Read((char*)(&id),sizeof(id));
//      printf("read id %d\n", id);
      reader.Read((char*)(&len),sizeof(len));
//      printf("read len %d\n", len);

      text.reserve(len);
#ifdef __QNX__
      while (text.size()<len)
	{
	  text.push_back(0);
	}
#else
      text.resize(len);
#endif

      top = len;
      int result = reader.Read((char*)(&text[0]),len);
      return result;
    }
  
  virtual int Write(YARPPortWriter& writer)
    {
      len = top; //text.size();
      writer.Write((char*)(&id),sizeof(id));
      writer.Write((char*)(&len),sizeof(len));
      return writer.Write((char*)(&text[0]),len);
    }
  
  virtual int Recycle()
    { top = 0; return 0; }
  
  vector<char>& Text() { return text; }
};

class YARPInputPortOf<YARPBottle> : public YARPBasicInputPort<YARPBottle>
{
public:
  YARPInputPortOf(int id) : YARPBasicInputPort<YARPBottle>(id) { }
  YARPInputPortOf() : YARPBasicInputPort<YARPBottle>() { }
};

class YARPOutputPortOf<YARPBottle> : public YARPBasicOutputPort<YARPBottle>
{
};

class YARPBottleIterator
{
public:
  YARPBottle *bottle;

  int index;

  YARPBottleIterator() { bottle = NULL; index = 0; }

  YARPBottleIterator(YARPBottle& n_bottle) { Attach(n_bottle); }
  
  void Attach(YARPBottle& n_bottle) { bottle = &n_bottle; index = 0; }
  
  YARPBottle& Bottle() { assert(bottle!=NULL); return *bottle; } 
  
  void SetID(int id) { Bottle().id = id; }
  
  int GetID() { return Bottle().id; }
  
  int GetSize() { return Bottle().Text().size(); }
  
  
  char *ReadRawBlock(int len)
    {
      assert(GetSize()-index>=len);
      char *result = (&(Bottle().Text())[index]);
      index += len;
      return result;
    }

  void ReadRawBlock(char *buf, int len)
    {
      char *src = ReadRawBlock(len);
      memcpy(buf,src,len);
    }
  
  void WriteRawBlock(char *buf, int len)
    {
      vector<char> &text = Bottle().Text();
#ifdef __QNX__
      while (text.size()<index+len)
	{
	  text.push_back(0);
	}
#else
      text.resize(index+len);
#endif
      memcpy(&text[index],buf,len);
      index+=len;
      Bottle().top = index;
    }
  
  char ReadRawChar()
    {
      char result;  
      ReadRawBlock((char*)(&result),sizeof(result));
      return result;
    }
  void WriteRawChar(char result)
    {
      WriteRawBlock((char*)(&result),sizeof(result));
    }

  NetInt32 ReadRawInt()
    {
      NetInt32 result;  
      ReadRawBlock((char*)(&result),sizeof(result));
      return result;
    }
  void WriteRawInt(NetInt32 result)
    {
      WriteRawBlock((char*)(&result),sizeof(result));
    }

  double ReadRawFloat()
    {
      double result;  
      ReadRawBlock((char*)(&result),sizeof(result));
      return result;
    }
  void WriteRawFloat(double result)
    {
      WriteRawBlock((char*)(&result),sizeof(result));
    }
  
  const char *ReadRawText()
    {
      NetInt32 len = ReadRawInt();
      return ReadRawBlock(len);
    }
  void WriteRawText(const char *text)
    {
      WriteRawInt(strlen(text)+1);
      WriteRawBlock((char*)(&text[0]),strlen(text)+1);      
    }

  
  void AssertType(char ch)
    {
      char ch_compare;
      ch_compare = ReadRawChar();
      assert(ch==ch_compare);
    }
  
  void WriteInt(NetInt32 result)
    { WriteRawChar('d');  WriteRawInt(result); }
  void WriteFloat(double result)
    { WriteRawChar('f');  WriteRawFloat(result); }
  void WriteText(const char *result)
    { WriteRawChar('s');  WriteRawText(result); }
  void WriteVocab(NetInt32 result)
    { WriteRawChar('v');  WriteRawInt(result); }
  
  NetInt32 ReadInt()
    { AssertType('d');  NetInt32 result = ReadRawInt();  return result; }
  double ReadFloat()
    { AssertType('f');  double result = ReadRawFloat();  return result; }
  const char *ReadText()
    { AssertType('s');  return ReadRawText(); }
  NetInt32 ReadVocab()
    { AssertType('v');  NetInt32 result = ReadRawInt();  return result; }
  
  void Rewind()
    {
      index = 0;
    }
  
  void Reset()
    {
      Rewind();
      Bottle().top = 0;
    }
  
  int More()
    {
      return (index<Bottle().top-1);
//      return (index<Bottle().Text().size()-1);
    }
  
  void Display()
    {
      Rewind();
      int first = 1;
      while (More())
	{
	  char ch = ReadRawChar();
	  if (!first)
	    {
	      printf(" ");
	    }
	  first = 0;
	  switch(ch)
	    {
	    case 'd':
	      printf("%d", ReadRawInt());
	      break;
	    case 'v':
	      printf("%s", ybc_label(ReadRawInt()));
	      break;
	    case 'f':
	      printf("%g", ReadRawFloat());
	      break;
	    case 's':
	      printf("(%s)", ReadRawText());
	      break;
	    default:
	      printf("???");
	      break;
	    }
	}
      printf("\n");
    }
};

extern void SendBottle(YARPBottle& bottle);

#endif
