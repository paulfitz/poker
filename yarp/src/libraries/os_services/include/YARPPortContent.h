/*
paulfitz Sat May 26 22:27:07 EDT 2001
*/
#ifndef YARPPortContent_INC
#define YARPPortContent_INC

#include "YARPAll.h"

class YARPPortReader
{
public:
  virtual int Read(char *buffer, int length) = 0;
};

class YARPPortWriter
{
public:
  virtual int Write(char *buffer, int length) = 0;
};

/*
  Instances of this class know how to read or write themselves, and are
  can be kept in a pool of objects that circulate from the user to the 
  communications code and back to the user.  Necessary for efficiency
  when transmitting to multiple targets that read data at different rates.
 */


template <class T>
struct HierarchyRoot
{
  // HierarchyId is a nested class
  struct HierarchyId {};
};

class YARPPortContent : public HierarchyRoot<YARPPortContent>
{
public:
  virtual int Read(YARPPortReader& reader) = 0;
  
  virtual int Write(YARPPortWriter& writer) = 0;
  
  // Called when communications code is finished with the object, and
  // it will be passed back to the user.
  // Often fine to do nothing here.
  virtual int Recycle() = 0;
};


template <class T>
class YARPPortContentOf : public YARPPortContent
{
public:
  T datum;

  T& Content() { return datum; }

  virtual int Read(YARPPortReader& reader)
    { return reader.Read((char*)(&datum),sizeof(datum)); }
  
  virtual int Write(YARPPortWriter& writer)
    { return writer.Write((char*)(&datum),sizeof(datum)); }

  virtual int Recycle()
    { return 0; }
};

#endif
