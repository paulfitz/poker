#ifndef YARPIMAGEPORTCONTENT_INC
#define YARPIMAGEPORTCONTENT_INC

#include "YARPImage.h"
#include "YARPPort.h"
#include "YARPPortContent.h"

#include "begin_pack_for_net.h"

class YARPImagePortContentHeader
{
public:
  NetInt32 len;
  NetInt32 w;
  NetInt32 id;
  NetInt32 h;
  NetInt32 depth;
  double timestamp;
} PACKED_FOR_NET;

#include "end_pack_for_net.h"

class YARPImagePortContent : public YARPGenericImage, public YARPPortContent
{
public:
  YARPImagePortContentHeader header;

  virtual int Read(YARPPortReader& reader)
    {
      if (reader.Read((char*)(&header),sizeof(header)))
	{
	  //	  cout.flush();
	  SetID(header.id);
	  //SetPixelSize(header.depth);
	  int r = GetWidth();
	  if (GetWidth()!=header.w || GetHeight()!=header.h)
	    {
//	      cout << "CREATING!! " << header.w << " " << GetWidth() << endl;
	      Resize(header.w,header.h);
	    }
	  char *mem = GetRawBuffer();
	  assert(mem!=NULL);
	  assert(GetWidth()==header.w && GetHeight()==header.h &&
		 GetPixelSize()==header.depth);
	  reader.Read(mem,header.len);
	  //timestamp = header.timestamp;
	}
      return 1;
    }
  
  virtual int Write(YARPPortWriter& writer)
    {
//      cout << "HIT Write called" << endl;
      header.h = GetHeight();
      header.w = GetWidth();
      header.depth = GetPixelSize();
      header.id = GetID();
      header.len = header.h*header.w*header.depth;      
      header.timestamp = 0;
      writer.Write((char*)(&header),sizeof(header));
      char *mem = GetRawBuffer();
      assert(mem!=NULL);
      writer.Write(mem,header.len);
      return 1;
    }
  
  // Called when communications code is finished with the object, and
  // it will be passed back to the user.
  // Often fine to do nothing here.
  virtual int Recycle()
    {
      return 0;
    }
};


// The following has not been tested yet 

class YARPInputPortOf<YARPGenericImage> : public YARPBasicInputPort<YARPImagePortContent>
{
};

class YARPOutputPortOf<YARPGenericImage> : public YARPBasicOutputPort<YARPImagePortContent>
{
};




#endif
