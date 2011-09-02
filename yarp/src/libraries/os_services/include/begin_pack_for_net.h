#include "YARPNetworkTypes.h"

// pending -- need to deal with endian changes between machines

#ifdef __WIN__
// use packing and make no apologies about it
#pragma warning (disable:4103)
#pragma pack(push, 1)
#endif

#ifdef __LINUX__
#pragma align 1
#endif

#ifdef __QNX__
#pragma  pack (push) ;
#pragma  pack (1) ;
#endif
