// @(#)root/gui:$Name:  $:$Id: TGWindow.cxx,v 1.7 2003/11/25 15:57:34 rdm Exp $
// Author: Fons Rademakers   28/12/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
/**************************************************************************

    This source is based on Xclass95, a Win95-looking GUI toolkit.
    Copyright (C) 1996, 1997 David Barth, Ricky Ralston, Hector Peraza.

    Xclass95 is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

**************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGWindow                                                             //
//                                                                      //
// ROOT GUI Window base class.                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TGWindow.h"
#include "TMath.h"
#include "Riostream.h"

ClassImp(TGWindow)
ClassImp(TGUnknownWindowHandler)

Int_t TGWindow::fgCounter = 0;

//______________________________________________________________________________
TGWindow::TGWindow(const TGWindow *p, Int_t x, Int_t y, UInt_t w, UInt_t h,
                   UInt_t border, Int_t depth, UInt_t clss, void *visual,
                   SetWindowAttributes_t *attr, UInt_t wtype)
{
   // Create a new window. Parent p must exist. No specified arguments
   // result in values from parent to be taken (or defaults).

   UInt_t type = wtype;

   if (!p && gClient) p = gClient->GetRoot();
   if (p) {
      fClient = p->fClient;
      if (fClient->IsEditable()) type = wtype & ~1;
 
      fParent = p;
      fId = gVirtualX->CreateWindow(fParent->fId, x, y,
                                    TMath::Max(w, (UInt_t) 1),
                                    TMath::Max(h, (UInt_t) 1), border,
    			            depth, clss, visual, attr, type);
      fClient->RegisterWindow(this);
      fNeedRedraw = kFALSE;

      // name will be used in SavePrimitive methods
      fgCounter++;
      fName = "frame";
      fName += fgCounter;

   } else {
      Error("TGWindow", "no parent specified");
   }
}

//______________________________________________________________________________
TGWindow::TGWindow(TGClient *c, Window_t id, const TGWindow *parent)
{
   // Create a copy of a window.

   fClient = c;
   fId     = id;
   fParent = parent;
   fClient->RegisterWindow(this);
   fNeedRedraw = kFALSE;

   // name used in SavePrimitive methods
   fgCounter++;
   fName = "frame";
   fName += fgCounter;
}

//______________________________________________________________________________
TGWindow::~TGWindow()
{
   // Window destructor. Unregisters the window.

   if (fClient) fClient->UnregisterWindow(this);
}

//______________________________________________________________________________
void TGWindow::Move(Int_t x, Int_t y)
{
   // Move the window.

   gVirtualX->MoveWindow(fId, x, y);
}

//______________________________________________________________________________
void TGWindow::Resize(UInt_t w, UInt_t h)
{
   // Resize the window.

   gVirtualX->ResizeWindow(fId, TMath::Max(w, (UInt_t)1), TMath::Max(h, (UInt_t)1));
}

//______________________________________________________________________________
void TGWindow::MoveResize(Int_t x, Int_t y, UInt_t w, UInt_t h)
{
   // Move and resize the window.

   gVirtualX->MoveResizeWindow(fId, x, y, TMath::Max(w, (UInt_t)1), TMath::Max(h, (UInt_t)1));
}

//______________________________________________________________________________
Bool_t TGWindow::IsMapped()
{
   // Returns kTRUE if window is mapped on screen, kFALSE otherwise.

   WindowAttributes_t attr;

   gVirtualX->GetWindowAttributes(fId, attr);
   return (attr.fMapState != kIsUnmapped);
}

//______________________________________________________________________________
void TGWindow::Print(Option_t *) const
{
   // Print window id.

   cout << ClassName() << ":t" << fId << endl;
}

//______________________________________________________________________________
Int_t TGWindow::GetCounter()
{
   // Return global window counter (total number of created windows).

   return fgCounter;
}
