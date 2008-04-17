// @(#)root/gl:$Id$
// Author:  Timur Pocheptsov, Matevz Tadel, June 2007

#ifndef ROOT_TGLContextPrivate
#define ROOT_TGLContextPrivate

#include <map>

#ifndef ROOT_TGLIncludes
#include "TGLIncludes.h"
#endif
#ifndef ROOT_TGLContext
#include "TGLContext.h"
#endif

#ifdef WIN32

class TGLContextPrivate {
public:
   HWND        fHWND;
   HDC         fHDC;
   HGLRC       fGLContext;

   TGLContextPrivate()
      : fHWND(0),
        fHDC(0),
        fGLContext(0)
   {
   }
   static void RegisterContext(TGLContext *ctx);
   static void RemoveContext(TGLContext *ctx);
   static TGLContext *GetCurrentContext();


private:
   TGLContextPrivate(const TGLContextPrivate &);
   TGLContextPrivate &operator = (const TGLContextPrivate &);

   static std::map<HGLRC, TGLContext *> fgContexts;
};


#else

class TGLContextPrivate {
public:
   Display     *fDpy;
   XVisualInfo *fVisualInfo;
   GLXContext   fGLContext;
   Int_t        fWindowIndex;
   //GLXPbuffer   fPBDC;

   TGLContextPrivate()
      : fDpy(0),
        fVisualInfo(0),
        fGLContext(0),
        fWindowIndex(-1)
   {
   }

   static void RegisterContext(TGLContext *ctx);
   static void RemoveContext(TGLContext *ctx);
   static TGLContext *GetCurrentContext();

private:
   TGLContextPrivate(const TGLContextPrivate &);
   TGLContextPrivate &operator = (const TGLContextPrivate &);

   static std::map<GLXContext, TGLContext *> fgContexts;
};

#endif

#endif
