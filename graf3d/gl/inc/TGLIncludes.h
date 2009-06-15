// @(#)root/gl:$Id$
// Author:  Richard Maunder  25/05/2005

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGLIncludes
#define ROOT_TGLIncludes

#ifndef G__DICTIONARY

#include <GL/glew.h>
#ifdef WIN32
#include <GL/wglew.h>
#include "Windows4Root.h"
#else //WIN32
#include <GL/glxew.h>
#endif //WIN32

#else //G__DICTIONARY

#ifdef WIN32
#ifndef ROOT_Windows4Root
#include "Windows4Root.h"
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#endif //G__DICTIONARY

#endif // ROOT_TGLIncludes

