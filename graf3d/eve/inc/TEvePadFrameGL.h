// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEvePadFrameGL
#define ROOT_TEvePadFrameGL

#include "TGLObject.h"

class TGLViewer;
class TGLScene;
class TGLFBO;
class TGLPadPainter;
class TGLPadPainter3D;

class TEvePadFrame;

class TEvePadFrameGL : public TGLObject
{
private:
   TEvePadFrameGL(const TEvePadFrameGL&);            // Not implemented
   TEvePadFrameGL& operator=(const TEvePadFrameGL&); // Not implemented

   static TGLPadPainter   *fgPainter;
   static TGLPadPainter3D *fgPainter3D;

protected:
   TEvePadFrame    *fM; // Model object.

   mutable TGLFBO  *fFBO;
   mutable Int_t    fRTS;
   
   void DirectDrawIntoFBO(const TGLRnrCtx& rnrCtx) const;

   void DirectDrawFBO(const TGLRnrCtx& rnrCtx) const;
   void DirectDraw3D(const TGLRnrCtx& rnrCtx) const;

public:
   TEvePadFrameGL();
   virtual ~TEvePadFrameGL();

   virtual Bool_t SetModel(TObject* obj, const Option_t* opt=0);
   virtual void   SetBBox();

   virtual Bool_t ShouldDLCache(const TGLRnrCtx& rnrCtx) const;

   virtual void   DirectDraw(TGLRnrCtx & rnrCtx) const;

   // To support two-level selection
   // virtual Bool_t SupportsSecondarySelect() const { return kTRUE; }
   // virtual void ProcessSelection(TGLRnrCtx & rnrCtx, TGLSelectRecord & rec);

   ClassDef(TEvePadFrameGL, 0); // GL renderer class for TEvePadFrame.
};

#endif
