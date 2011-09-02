//
// YARPImageServices.h
//

// just a global include for image processing stuff.


#ifndef __YARPImageServicesh__
#define __YARPImageServicesh__

#ifdef __QNX__
#include "YARPSafeNew.h"
#endif

#include "YARPImage.h"
#include "YARPImageFile.h"
#include "YARPImageDraw.h"
#include "YARPFilters.h"
#include "YARPImageUtils.h"
#include "YARPColorConverter.h"
#include "YARPSimpleOperations.h"

#include "YARPlogpolar.h"
#include "YARPBinarizeFilter.h"
#include "YARPColorSaliency.h"
#include "YARPBlobFinder.h"
#include "YARPBlurFilter.h"
#include "YARPFirstDerivativeT.h"
#include "YARPLpShifter.h"
#include "YARPDisparity.h"
#include "YARPKernelFilter.h"
#include "YARPLpKernelFilter.h"
#include "YARPLpSeparableFilter.h"
#include "YARPSkinFilter.h"
#include "YARPTemporalSmooth.h"
#include "YARPTagFilter.h"
#include "YARPBoxer.h"

#include "YARPSusan.h"

// extra services (require external libraries).
#include "YARPFirstOrderFlow.h"
#include "YARPFlowVerify.h"
#include "YARPColorSegmentation.h"
#include "YARPClrHistogram.h"
#include "YARPFftFeatures.h"
#include "YARPGaussianFeatures.h"

// this is win specific (I don't know why!)
#include "YARPDIBConverter.h"

#endif
