
#ifndef YARP_IMAGE_HASH_INC
#define YARP_IMAGE_HASH_INC

#include "YARPImage.h"
#include "YARPNetworkTypes.h"

class YARPImageHash
{
public:
  static NetInt32 GetHash(YARPImageOf<YarpPixelBGR>& src);
};

#endif

