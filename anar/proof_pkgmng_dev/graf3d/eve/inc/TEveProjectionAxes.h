// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveProjectionAxes
#define ROOT_TEveProjectionAxes

#include "TNamed.h"
#include "TAtt3D.h"
#include "TAttBBox.h"

#include "TEveElement.h"

class TEveProjectionManager;

class TEveProjectionAxes : public TEveElement,
                           public TNamed,
                           public TAtt3D,
                           public TAttBBox
{
   friend class TEveProjectionAxesGL;

public:
   enum ELabMode      { kPosition, kValue };
   enum EAxesMode     { kHorizontal, kVertical, kAll};

private:
   TEveProjectionAxes(const TEveProjectionAxes&);            // Not implemented
   TEveProjectionAxes& operator=(const TEveProjectionAxes&); // Not implemented

protected:
   TEveProjectionManager*  fManager;  // model object
   
   Float_t         fBoxOffsetX;     // offset X of bounding Box
   Float_t         fBoxOffsetY;     // offset Y  of bounding Box

   Float_t         fLabelSize;       // relative font size

   Color_t         fColor;

   ELabMode        fLabMode;       // tick-mark positioning
   EAxesMode       fAxesMode;
   Int_t           fNdiv;  // number of tick-mark on axis

   Bool_t          fDrawCenter;  // draw center of distortion
   Bool_t          fDrawOrigin;  // draw origin

public:
   TEveProjectionAxes(TEveProjectionManager* m);
   virtual ~TEveProjectionAxes();

   TEveProjectionManager* GetManager(){ return fManager; }

   void            SetLabMode(ELabMode x)   { fLabMode = x;     }
   ELabMode        GetLabMode()   const     { return fLabMode;  }
   void            SetAxesMode(EAxesMode x) { fAxesMode = x;    }
   EAxesMode       GetAxesMode()   const    { return fAxesMode; }

   void            SetNdiv(Int_t x) { fNdiv = x;    }
   Int_t           GetNdiv()  const { return fNdiv; }

   void            SetBoxOffsetX(Float_t x) {fBoxOffsetX=x;  }
   Float_t         GetBoxOffsetX() const    {return fBoxOffsetX; }

   void            SetBoxOffsetY(Float_t x) {fBoxOffsetY=x;  }
   Float_t         GetBoxOffsetY() const    {return fBoxOffsetY; }

   Float_t         GetLabelSize() const {return fLabelSize;}
   void            SetLabelSize(Float_t x) {fLabelSize=x;}

   void            SetDrawCenter(Bool_t x){ fDrawCenter = x;    }
   Bool_t          GetDrawCenter() const  { return fDrawCenter; }
   void            SetDrawOrigin(Bool_t x){ fDrawOrigin = x;    }
   Bool_t          GetDrawOrigin() const  { return fDrawOrigin; }

   virtual Bool_t  CanEditMainColor() const { return kTRUE; }

   virtual void    Paint(Option_t* option="");

   virtual void    ComputeBBox();

   virtual const TGPicture* GetListTreeIcon(Bool_t open=kFALSE);

   ClassDef(TEveProjectionAxes, 1); // Short description.
};

#endif
