// @(#)root/gui:$Name:  $:$Id: TGStatusBar.h,v 1.6 2003/05/28 11:55:31 rdm Exp $
// Author: Fons Rademakers   23/01/98

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGStatusBar
#define ROOT_TGStatusBar


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGStatusBar                                                          //
//                                                                      //
// Provides a StatusBar widget.                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TGFrame
#include "TGFrame.h"
#endif

class TGStatusBarPart;


class TGStatusBar : public TGFrame {

friend class TGStatusBarPart;

protected:
   TGStatusBarPart **fStatusPart; // frames containing statusbar text
   Int_t            *fParts;      // size of parts (in percent of total width)
   Int_t             fNpart;      // number of parts
   Int_t             fYt;         // y drawing position (depending on font)
   Int_t            *fXt;         // x position for each part
   Bool_t            f3DCorner;   // draw 3D corner (drawn by default)

   static const TGFont *fgDefaultFont;
   static TGGC         *fgDefaultGC;

   virtual void DoRedraw();

   static FontStruct_t  GetDefaultFontStruct();
   static const TGGC   &GetDefaultGC();

public:
   TGStatusBar(const TGWindow *p, UInt_t w, UInt_t h,
               UInt_t options = kSunkenFrame,
               Pixel_t back = GetDefaultFrameBackground());
   virtual ~TGStatusBar();

   virtual void DrawBorder();
   virtual void SetText(TGString *text, Int_t partidx = 0);
   virtual void SetText(const char *text, Int_t partidx = 0);
   virtual void SetParts(Int_t *parts, Int_t npart);
   virtual void SetParts(Int_t npart);
   void         Draw3DCorner(Bool_t corner) { f3DCorner = corner; }
   
   TGCompositeFrame *GetBarPart(Int_t npart) const;

   ClassDef(TGStatusBar,0)  // Status bar widget
};

#endif
