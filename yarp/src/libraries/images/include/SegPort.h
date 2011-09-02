#ifndef SEGPORT_INC

#include "YARPImagePort.h"

class ImagePairContent : public YARPPortContent
{
public:
  YARPImagePortContent c1;
  YARPImagePortContent c2;

  virtual int Read(YARPPortReader& reader)
    {
      c1.Read(reader);
      c2.Read(reader);
      return 1;
    }

  virtual int Write(YARPPortWriter& writer)
    {
      c1.Write(writer);
      c2.Write(writer);
      return 1;
    }

  virtual int Recycle()
    {
      c1.Recycle();
      c2.Recycle();
      return 0;
    }
};

typedef YARPBasicInputPort<ImagePairContent> InputSegPort;
typedef YARPBasicOutputPort<ImagePairContent> OutputSegPort;

#endif
