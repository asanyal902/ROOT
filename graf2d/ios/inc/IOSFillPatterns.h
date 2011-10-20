// @(#)root/graf2d:$Id$
// Author: Timur Pocheptsov, 14/8/2011

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_IOSFillPatterns
#define ROOT_IOSFillPatterns

#include <CoreGraphics/CGPattern.h>

namespace ROOT {
namespace iOS {
namespace GraphicUtils {

//
//Predefined fill styles (patterns).
//Must be 25, now only 15. To be added.
//

//TODO: remaining patterns are required.
enum {
   kPredefinedFillPatterns = 18
};

//Pattern generator function type. Parameter of type float *
//is an rgb tuple. Attention! rgb pointer should be valid while
//you are using pattern - it will be passed into pattern drawing callback
//funciton.
typedef CGPatternRef (*PatternGenerator_t)(float *);
//Array of pointers to functions, generating patterns.
extern PatternGenerator_t gPatternGenerators[kPredefinedFillPatterns];


//Due to some reason(s), ROOT likes to use hardcoded constants, never
//defines named constants and you have to know, what does 3000 or 1001 means.
//NIZKII POKLON za takoe. I have to fix it.

extern const unsigned solidFillStyle; //1001
extern const unsigned stippleBase; //3000

}//GraphicUtils
}//namespace iOS
}//namespace ROOT

#endif
