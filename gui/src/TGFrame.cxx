// @(#)root/gui:$Name:  $:$Id: TGFrame.cxx,v 1.27 2003/07/21 10:39:58 brun Exp $
// Author: Fons Rademakers   03/01/98

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
// TGFrame, TGCompositeFrame, TGVerticalFrame, TGHorizontalFrame,       //
// TGMainFrame, TGTransientFrame and TGGroupFrame                       //
//                                                                      //
// The frame classes describe the different "dressed" GUI windows.      //
//                                                                      //
// The TGFrame class is a subclasses of TGWindow, and is used as base   //
// class for some simple widgets (buttons, labels, etc.).               //
// It provides:                                                         //
//  - position & dimension fields                                       //
//  - an 'options' attribute (see constant above)                       //
//  - a generic event handler                                           //
//  - a generic layout mechanism                                        //
//  - a generic border                                                  //
//                                                                      //
// The TGCompositeFrame class is the base class for composite widgets   //
// (menu bars, list boxes, etc.).                                       //
// It provides:                                                         //
//  - a layout manager                                                  //
//  - a frame container (TList *)                                       //
//                                                                      //
// The TGVerticalFrame and TGHorizontalFrame are composite frame that   //
// layout their children in vertical or horizontal way.                 //
//                                                                      //
// The TGMainFrame class defines top level windows that interact with   //
// the system Window Manager.                                           //
//                                                                      //
// The TGTransientFrame class defines transient windows that typically  //
// are used for dialogs windows.                                        //
//                                                                      //
// The TGGroupFrame is a composite frame with a border and a title.     //
// It is typically used to group a number of logically related widgets  //
// visually together.                                                   //
//                                                                      //
//Begin_Html
/*
<img src="gif/tgcompositeframe_classtree.gif">
*/
//End_Html
//////////////////////////////////////////////////////////////////////////

#include "TGFrame.h"
#include "TGResourcePool.h"
#include "TGPicture.h"
#include "TList.h"
#include "TApplication.h"
#include "TTimer.h"
#include "Riostream.h"


Bool_t      TGFrame::fgInit = kFALSE;
Pixel_t     TGFrame::fgDefaultFrameBackground = 0;
Pixel_t     TGFrame::fgDefaultSelectedBackground = 0;
Pixel_t     TGFrame::fgWhitePixel = 0;
Pixel_t     TGFrame::fgBlackPixel = 0;
const TGGC *TGFrame::fgBlackGC = 0;
const TGGC *TGFrame::fgWhiteGC = 0;
const TGGC *TGFrame::fgHilightGC = 0;
const TGGC *TGFrame::fgShadowGC = 0;
const TGGC *TGFrame::fgBckgndGC = 0;
Time_t      TGFrame::fgLastClick = 0;
UInt_t      TGFrame::fgLastButton = 0;
Int_t       TGFrame::fgDbx = 0;
Int_t       TGFrame::fgDby = 0;
Window_t    TGFrame::fgDbw = 0;

const TGFont *TGGroupFrame::fgDefaultFont = 0;
const TGGC   *TGGroupFrame::fgDefaultGC = 0;


TGLayoutHints *TGCompositeFrame::fgDefaultHints = new TGLayoutHints;


ClassImp(TGFrame)
ClassImp(TGCompositeFrame)
ClassImp(TGVerticalFrame)
ClassImp(TGHorizontalFrame)
ClassImp(TGMainFrame)
ClassImp(TGTransientFrame)
ClassImp(TGGroupFrame)


//______________________________________________________________________________
TGFrame::TGFrame(const TGWindow *p, UInt_t w, UInt_t h,
                 UInt_t options, ULong_t back)
   : TGWindow(p, 0, 0, w, h, 0, 0, 0, 0, 0, options)
{
   // Create a TGFrame object. Options is an OR of the EFrameTypes.

   if (!fgInit) {
      TGFrame::GetDefaultFrameBackground();
      TGFrame::GetDefaultSelectedBackground();
      TGFrame::GetWhitePixel();
      TGFrame::GetBlackPixel();
      TGFrame::GetBlackGC();
      TGFrame::GetWhiteGC();
      TGFrame::GetHilightGC();
      TGFrame::GetShadowGC();
      TGFrame::GetBckgndGC();
      fgInit = kTRUE;
   }

   SetWindowAttributes_t wattr;

   fBackground = back;
   fOptions    = options;
   fWidth = w; fHeight = h; fX = fY = fBorderWidth = 0;

   if (fOptions & (kSunkenFrame | kRaisedFrame))
      fBorderWidth = (fOptions & kDoubleBorder) ? 2 : 1;

   wattr.fMask = kWABackPixel | kWAEventMask;
   wattr.fBackgroundPixel = back;
   wattr.fEventMask = kExposureMask;
   if (fOptions & kMainFrame) {
      wattr.fEventMask |= kStructureNotifyMask;
      gVirtualX->ChangeWindowAttributes(fId, &wattr);
      //if (fgDefaultBackgroundPicture)
      //   SetBackgroundPixmap(fgDefaultBackgroundPicture->GetPicture());
   } else {
      gVirtualX->ChangeWindowAttributes(fId, &wattr);
      //if (!(fOptions & kOwnBackground))
      //   SetBackgroundPixmap(kParentRelative);
   }
   fEventMask = (UInt_t) wattr.fEventMask;
}

//______________________________________________________________________________
TGFrame::TGFrame(TGClient *c, Window_t id, const TGWindow *parent)
   : TGWindow(c, id, parent)
{
   // Create a frame using an externally created window. For example
   // to register the root window (called by TGClient), or a window
   // created via TVirtualX::InitWindow() (id is obtained with
   // TVirtualX::GetWindowID()).

   if (!fgInit && gClient) {
      TGFrame::GetDefaultFrameBackground();
      TGFrame::GetDefaultSelectedBackground();
      TGFrame::GetWhitePixel();
      TGFrame::GetBlackPixel();
      TGFrame::GetBlackGC();
      TGFrame::GetWhiteGC();
      TGFrame::GetHilightGC();
      TGFrame::GetShadowGC();
      TGFrame::GetBckgndGC();
      fgInit = kTRUE;
   }

   WindowAttributes_t attributes;
   gVirtualX->GetWindowAttributes(id, attributes);

   fX           = attributes.fX;
   fY           = attributes.fY;
   fWidth       = attributes.fWidth;
   fHeight      = attributes.fHeight;
   fBorderWidth = attributes.fBorderWidth;
   fEventMask   = (UInt_t) attributes.fYourEventMask;
   fBackground  = 0;
   fOptions     = 0;
}

//______________________________________________________________________________
void TGFrame::DeleteWindow()
{
   // Delete window. Use single shot timer to call final delete method.
   // We use this inderect way since deleting the window in its own
   // execution "thread" can cause side effects because frame methods
   // can still be called while the window object has already been deleted.

   TTimer::SingleShot(50, IsA()->GetName(), this, "ReallyDelete()");
}

//______________________________________________________________________________
void TGFrame::ChangeBackground(ULong_t back)
{
   // Change frame background color.

   fBackground = back;
   gVirtualX->SetWindowBackground(fId, back);
}

//______________________________________________________________________________
void TGFrame::SetBackgroundColor(Pixel_t back)
{
   // Set background color (override from TGWindow base class).
   // Same effect as ChangeBackground().

   fBackground = back;
   TGWindow::SetBackgroundColor(back);
}

//______________________________________________________________________________
void TGFrame::ChangeOptions(UInt_t options)
{
   // Change frame options. Options is an OR of the EFrameTypes.

   if ((options & (kDoubleBorder | kSunkenFrame | kRaisedFrame)) !=
      (fOptions & (kDoubleBorder | kSunkenFrame | kRaisedFrame))) {
      if (options & (kSunkenFrame | kRaisedFrame))
         fBorderWidth = (options & kDoubleBorder) ? 2 : 1;
      else
         fBorderWidth = 0;
   }

   fOptions = options;
}

//______________________________________________________________________________
void TGFrame::AddInput(UInt_t emask)
{
   // Add events specified in the emask to the events the frame should handle.

   fEventMask |= emask;
   gVirtualX->SelectInput(fId, fEventMask);
}

//______________________________________________________________________________
void TGFrame::RemoveInput(UInt_t emask)
{
   // Remove events specified in emask from the events the frame should handle.

   fEventMask &= ~emask;
   gVirtualX->SelectInput(fId, fEventMask);
}

//________________________________________________________________________________
void TGFrame::Draw3dRectangle(UInt_t type, Int_t x, Int_t y,
                                     UInt_t w, UInt_t h)
{
   switch (type) {
      case kSunkenFrame:
         gVirtualX->DrawLine(fId, GetShadowGC()(),  x,     y,     x+w-2, y);
         gVirtualX->DrawLine(fId, GetShadowGC()(),  x,     y,     x,     y+h-2);
         gVirtualX->DrawLine(fId, GetHilightGC()(), x,     y+h-1, x+w-1, y+h-1);
         gVirtualX->DrawLine(fId, GetHilightGC()(), x+w-1, y+h-1, x+w-1, y);
         break;

      case kSunkenFrame | kDoubleBorder:
         gVirtualX->DrawLine(fId, GetShadowGC()(), x,     y,     x+w-2, y);
         gVirtualX->DrawLine(fId, GetShadowGC()(), x,     y,     x,     y+h-2);
         gVirtualX->DrawLine(fId, GetBlackGC()(),  x+1,   y+1,   x+w-3, y+1);
         gVirtualX->DrawLine(fId, GetBlackGC()(),  x+1,   y+1,   x+1,   y+h-3);

         gVirtualX->DrawLine(fId, GetHilightGC()(), x,     y+h-1, x+w-1, y+h-1);
         gVirtualX->DrawLine(fId, GetHilightGC()(), x+w-1, y+h-1, x+w-1, y);
         gVirtualX->DrawLine(fId, GetBckgndGC()(),  x+1,   y+h-2, x+w-2, y+h-2);
         gVirtualX->DrawLine(fId, GetBckgndGC()(),  x+w-2, y+1,   x+w-2, y+h-2);
         break;

      case kRaisedFrame:
         gVirtualX->DrawLine(fId, GetHilightGC()(), x,     y,     x+w-2, y);
         gVirtualX->DrawLine(fId, GetHilightGC()(), x,     y,     x,     y+h-2);
         gVirtualX->DrawLine(fId, GetShadowGC()(),  x,     y+h-1, x+w-1, y+h-1);
         gVirtualX->DrawLine(fId, GetShadowGC()(),  x+w-1, y+h-1, x+w-1, y);
         break;

      case kRaisedFrame | kDoubleBorder:
         gVirtualX->DrawLine(fId, GetHilightGC()(), x,     y,     x+w-2, y);
         gVirtualX->DrawLine(fId, GetHilightGC()(), x,     y,     x,     y+h-2);
         gVirtualX->DrawLine(fId, GetBckgndGC()(),  x+1,   y+1,   x+w-3, y+1);
         gVirtualX->DrawLine(fId, GetBckgndGC()(),  x+1,   y+1,   x+1,   y+h-3);

         gVirtualX->DrawLine(fId, GetShadowGC()(),  x+1,   y+h-2, x+w-2, y+h-2);
         gVirtualX->DrawLine(fId, GetShadowGC()(),  x+w-2, y+h-2, x+w-2, y+1);
         gVirtualX->DrawLine(fId, GetBlackGC()(),   x,     y+h-1, x+w-1, y+h-1);
         gVirtualX->DrawLine(fId, GetBlackGC()(),   x+w-1, y+h-1, x+w-1, y);
         break;

      default:
         break;
   }
}

//______________________________________________________________________________
void TGFrame::DrawBorder()
{
   // Draw frame border.

   Draw3dRectangle(fOptions & (kSunkenFrame | kRaisedFrame | kDoubleBorder),
                   0, 0, fWidth, fHeight);
}

//______________________________________________________________________________
void TGFrame::DoRedraw()
{
   // Redraw the frame.

   gVirtualX->ClearArea(fId, fBorderWidth, fBorderWidth,
                   fWidth - (fBorderWidth << 1), fHeight - (fBorderWidth << 1));

   // border will only be drawn if we have a 3D option hint
   // (kRaisedFrame or kSunkenFrame)
   DrawBorder();
}

//______________________________________________________________________________
Bool_t TGFrame::HandleConfigureNotify(Event_t *event)
{
   // This event is generated when the frame is resized.

   if ((event->fWidth != fWidth) || (event->fHeight != fHeight)) {
      fWidth  = event->fWidth;
      fHeight = event->fHeight;
      Layout();
   }
   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGFrame::HandleEvent(Event_t *event)
{
   // Handle all frame events. Events are dispatched to the specific
   // event handlers.

   switch (event->fType) {

      case kExpose:
         HandleExpose(event);
         break;

      case kConfigureNotify:
         while (gVirtualX->CheckEvent(fId, kConfigureNotify, *event))
            ;
         HandleConfigureNotify(event);
         break;

      case kGKeyPress:
      case kKeyRelease:
         HandleKey(event);
         break;

      case kFocusIn:
      case kFocusOut:
         HandleFocusChange(event);
         break;

     case kButtonPress:
         {
            Int_t dbl_clk = kFALSE;

            if ((event->fTime - fgLastClick < 350) &&
                (event->fCode == fgLastButton) &&
                (TMath::Abs(event->fXRoot - fgDbx) < 6) &&
                (TMath::Abs(event->fYRoot - fgDby) < 6) &&
                (event->fWindow == fgDbw)) dbl_clk = kTRUE;

             fgLastClick = event->fTime;
             fgLastButton = event->fCode;
             fgDbx = event->fXRoot;
             fgDby = event->fYRoot;
             fgDbw = event->fWindow;

             if (dbl_clk) {
                if (!HandleDoubleClick(event))
                   HandleButton(event);
             } else {
                HandleButton(event);
             }
         }
         break;

      case kButtonRelease:
         HandleButton(event);
         break;

      case kEnterNotify:
      case kLeaveNotify:
         HandleCrossing(event);
         break;

      case kMotionNotify:
         while (gVirtualX->CheckEvent(fId, kMotionNotify, *event))
            ;
         HandleMotion(event);
         break;

      case kClientMessage:
         HandleClientMessage(event);
         break;

      case kSelectionNotify:
         HandleSelection(event);
         break;

      case kSelectionRequest:
         HandleSelectionRequest(event);
         break;

      case kSelectionClear:
         HandleSelectionClear(event);
         break;

      case kColormapNotify:
         HandleColormapChange(event);
         break;

      default:
         //Warning("HandleEvent", "unknown event (%#x) for (%#x)", event->fType, fId);
         break;
   }

   if (TestBit(kNotDeleted))
      ProcessedEvent(event);  // emit signal

   return kTRUE;
}

//______________________________________________________________________________
void TGFrame::Move(Int_t x, Int_t y)
{
   // Move frame.

   if (x != fX || y != fY) {
      TGWindow::Move(x, y);
      fX = x; fY = y;
   }
}

//______________________________________________________________________________
void TGFrame::Resize(UInt_t w, UInt_t h)
{
   // Resize the frame. 
   // If w=0 && h=0 - Resize to deafult size 

   if (w != fWidth || h != fHeight) {
      TGDimension siz = GetDefaultSize();
      fWidth = w ? w : siz.fWidth; 
      fHeight = h ? h : siz.fHeight;
      TGWindow::Resize(fWidth, fHeight);
      Layout();
   }
}

//______________________________________________________________________________
void TGFrame::Resize(TGDimension size)
{
   // Resize the frame.

   Resize(size.fWidth, size.fHeight);
}

//______________________________________________________________________________
void TGFrame::MoveResize(Int_t x, Int_t y, UInt_t w, UInt_t h)
{
   // Move and/or resize the frame.
   // If w=0 && h=0 - Resize to deafult size 

   // we do it anyway as we don't know if it's only a move or only a resize
   TGDimension siz = GetDefaultSize();
   fWidth = w ? w : siz.fWidth; 
   fHeight = h ? h : siz.fHeight;
   fX = x; fY = y;
   TGWindow::MoveResize(x, y, fWidth, fHeight);
   Layout();
}

//______________________________________________________________________________
void TGFrame::SendMessage(const TGWindow *w, Long_t msg, Long_t parm1, Long_t parm2)
{
   // Send message (i.e. event) to window w. Message is encoded in one long
   // as message type and up to two long parameters.

   Event_t event;

   if (w) {
      event.fType   = kClientMessage;
      event.fFormat = 32;
      event.fHandle = gROOT_MESSAGE;

      event.fWindow  = w->GetId();
      event.fUser[0] = msg;
      event.fUser[1] = parm1;
      event.fUser[2] = parm2;
      event.fUser[3] = 0;
      event.fUser[4] = 0;

      gVirtualX->SendEvent(w->GetId(), &event);
   }
}

//______________________________________________________________________________
Bool_t TGFrame::HandleClientMessage(Event_t *event)
{
   // Handle a client message. Client messages are the ones sent via
   // TGFrame::SendMessage (typically by widgets).

   if (event->fHandle == gROOT_MESSAGE) {
      ProcessMessage(event->fUser[0], event->fUser[1], event->fUser[2]);
   }

   return kTRUE;
}

//______________________________________________________________________________
ULong_t TGFrame::GetDefaultFrameBackground()
{
   //

   static Bool_t init = kFALSE;
   if (!init) {
      fgDefaultFrameBackground = gClient->GetResourcePool()->GetFrameBgndColor();
      init = kTRUE;
   }
   return fgDefaultFrameBackground;
}

//______________________________________________________________________________
ULong_t TGFrame::GetDefaultSelectedBackground()
{
   //

   static Bool_t init = kFALSE;
   if (!init) {
      fgDefaultSelectedBackground = gClient->GetResourcePool()->GetSelectedBgndColor();
      init = kTRUE;
   }
   return fgDefaultSelectedBackground;
}

//______________________________________________________________________________
ULong_t TGFrame::GetWhitePixel()
{
   //

   static Bool_t init = kFALSE;
   if (!init) {
      fgWhitePixel = gClient->GetResourcePool()->GetWhiteColor();
      init  = kTRUE;
   }
   return fgWhitePixel;
}

//______________________________________________________________________________
ULong_t TGFrame::GetBlackPixel()
{
   //

   static Bool_t init = kFALSE;
   if (!init) {
      fgBlackPixel = gClient->GetResourcePool()->GetBlackColor();
      init = kTRUE;
   }
    return fgBlackPixel;
}

//______________________________________________________________________________
const TGGC &TGFrame::GetBlackGC()
{
   //

   if (!fgBlackGC)
      fgBlackGC = gClient->GetResourcePool()->GetBlackGC();
   return *fgBlackGC;
}

//______________________________________________________________________________
const TGGC &TGFrame::GetWhiteGC()
{
   //

   if (!fgWhiteGC)
      fgWhiteGC = gClient->GetResourcePool()->GetWhiteGC();
   return *fgWhiteGC;
}

//______________________________________________________________________________
const TGGC &TGFrame::GetHilightGC()
{
   //

   if (!fgHilightGC)
      fgHilightGC = gClient->GetResourcePool()->GetFrameHiliteGC();
   return *fgHilightGC;
}

//______________________________________________________________________________
const TGGC &TGFrame::GetShadowGC()
{
   if (!fgShadowGC)
      fgShadowGC = gClient->GetResourcePool()->GetFrameShadowGC();
   return *fgShadowGC;
}

//______________________________________________________________________________
const TGGC &TGFrame::GetBckgndGC()
{
   //

   if (!fgBckgndGC)
      fgBckgndGC = gClient->GetResourcePool()->GetFrameBckgndGC();
   return *fgBckgndGC;
}

//______________________________________________________________________________
Time_t TGFrame::GetLastClick()
{ 
   //

   return fgLastClick; 
}

//______________________________________________________________________________
void TGFrame::Print(Option_t *option) const
{
   // print window id

   cout <<  option << ClassName() << ":\tid=" << fId << " parent=" << fParent->GetId();
   cout <<" x=" << fX << " y=" << fY;
   cout << " w=" << fWidth << " h=" << fHeight << endl;
}

//______________________________________________________________________________
TGCompositeFrame::TGCompositeFrame(const TGWindow *p, UInt_t w, UInt_t h,
         UInt_t options, ULong_t back) : TGFrame(p, w, h, options, back)
{
   // Create a composite frame. A composite frame has in addition to a TGFrame
   // also a layout manager and a list of child frames.

   fLayoutManager = 0;
   fList          = new TList;

   if (fOptions & kHorizontalFrame)
      SetLayoutManager(new TGHorizontalLayout(this));
   else
      SetLayoutManager(new TGVerticalLayout(this));
}

//______________________________________________________________________________
TGCompositeFrame::TGCompositeFrame(TGClient *c, Window_t id, const TGWindow *parent)
   : TGFrame(c, id, parent)
{
   // Create a frame using an externally created window. For example
   // to register the root window (called by TGClient), or a window
   // created via TVirtualX::InitWindow() (id is obtained with TVirtualX::GetWindowID()).

   fLayoutManager = 0;
   fList          = new TList;

   SetLayoutManager(new TGVerticalLayout(this));
}

//______________________________________________________________________________
TGCompositeFrame::~TGCompositeFrame()
{
   // Delete a composite frame.

   if (fList) fList->Delete();
   delete fList;
   delete fLayoutManager;
}

//______________________________________________________________________________
void TGCompositeFrame::Cleanup()
{
   // Cleanup and delete all objects contained in this composite frame.
   // This will delete all objects added via AddFrame().
   // CAUTION: all objects (frames and layout hints) must be unique, i.e.
   // cannot be shared.

   if (!fList) return;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next())) {
      delete el->fFrame;
      if (el->fLayout != fgDefaultHints)
         delete el->fLayout;
   }
   fList->Delete();
}

//______________________________________________________________________________
void TGCompositeFrame::SetLayoutManager(TGLayoutManager *l)
{
   // Set the layout manager for the composite frame.
   // The layout manager is adopted by the frame and will be deleted
   // by the frame.

   if (l) {
      delete fLayoutManager;
      fLayoutManager = l;
   } else
      Error("SetLayoutManager", "no layout manager specified");
}

//______________________________________________________________________________
void TGCompositeFrame::ChangeOptions(UInt_t options)
{
   // Change composite frame options. Options is an OR of the EFrameTypes.

  TGFrame::ChangeOptions(options);

  if (options & kHorizontalFrame)
     SetLayoutManager(new TGHorizontalLayout(this));
  else
     SetLayoutManager(new TGVerticalLayout(this));
}

//______________________________________________________________________________
void TGCompositeFrame::AddFrame(TGFrame *f, TGLayoutHints *l)
{
   // Add frame to the composite frame using the specified layout hints.
   // If no hints are specified default hints TGLayoutHints(kLHintsNormal,0,0,0,0)
   // will be used. Most of the time, however, you will want to provide
   // specific hints. User specified hints can be reused many times
   // and need to be destroyed by the user. The added frames cannot not be
   // added to different composite frames but still need to be deleted by
   // the user.

   TGFrameElement *nw;

   nw = new TGFrameElement;
   nw->fFrame  = f;
   nw->fLayout = l ? l : fgDefaultHints;
   nw->fState  = 1;
   fList->Add(nw);
}

//______________________________________________________________________________
void TGCompositeFrame::RemoveFrame(TGFrame *f)
{
   // Remove frame from composite frame.

   if (!fList) return;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      if (el->fFrame == f) {
         fList->Remove(el);
         delete el;
         break;
      }
}

//______________________________________________________________________________
void TGCompositeFrame::MapSubwindows()
{
   // Map all sub windows that are part of the composite frame.

   TGWindow::MapSubwindows();

   if (!fList) return;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      el->fFrame->MapSubwindows();
}

//______________________________________________________________________________
void TGCompositeFrame::HideFrame(TGFrame *f)
{
   // Hide sub frame.

   if (!fList) return;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      if (el->fFrame == f) {
         el->fState = 0;
         el->fFrame->UnmapWindow();
         Layout();
         break;
      }
}

//______________________________________________________________________________
void TGCompositeFrame::ShowFrame(TGFrame *f)
{
   // Show sub frame.

   if (!fList) return;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      if (el->fFrame == f) {
         el->fState = 1;
         el->fFrame->MapWindow();
         Layout();
         break;
      }
}

//______________________________________________________________________________
Int_t TGCompositeFrame::GetState(TGFrame *f) const
{
   // Get state of sub frame.

   if (!fList) return 0;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      if (el->fFrame == f)
         return el->fState;

   return 0;
}

//______________________________________________________________________________
Bool_t TGCompositeFrame::IsVisible(TGFrame *f) const
{
   // Get state of sub frame.

   if (!fList) return kFALSE;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      if (el->fFrame == f)
         return (el->fState & kIsVisible);

   return kFALSE;
}

//______________________________________________________________________________
Bool_t TGCompositeFrame::IsArranged(TGFrame *f) const
{
   // Get state of sub frame.

   if (!fList) return kFALSE;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next()))
      if (el->fFrame == f)
         return (el->fState & kIsArranged);

   return kFALSE;
}

//______________________________________________________________________________
void TGCompositeFrame::Layout()
{
   // Layout the elements of the composite frame.

   fLayoutManager->Layout();
}

//______________________________________________________________________________
void TGCompositeFrame::Print(Option_t *option) const
{
   // Print all frames in this composite frame.

   TGFrameElement *el;
   TIter next(fList);
   TString tab = option;

   TGFrame::Print(tab.Data());
   tab += "   ";
   while ((el = (TGFrameElement*)next())) {
      el->fFrame->Print(tab.Data());
   }
}

//______________________________________________________________________________
TGFrame *TGCompositeFrame::GetFrameFromPoint(Int_t x, Int_t y)
{
   // Get frame located at specified point.

   if (!Contains(x, y)) return 0;

   if (!fList) return this;

   TGFrame *f;
   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next())) {
      //if (el->fFrame->IsVisible()) { //for this need to move IsVisible to TGFrame
      if (el->fState & kIsVisible) {
         f = el->fFrame->GetFrameFromPoint(x - el->fFrame->GetX(),
                                           y - el->fFrame->GetY());
         if (f) return f;
      }
   }
   return this;
}

//______________________________________________________________________________
Bool_t TGCompositeFrame::TranslateCoordinates(TGFrame *child, Int_t x, Int_t y,
                                              Int_t &fx, Int_t &fy)
{
   // Translate coordinates to child frame.

   if (child == this) {
      fx = x;
      fy = y;
      return kTRUE;
   }

   if (!Contains(x, y)) return kFALSE;

   if (!fList) return kFALSE;

   TGFrameElement *el;
   TIter next(fList);

   while ((el = (TGFrameElement *) next())) {
      if (el->fFrame == child) {
         fx = x - el->fFrame->GetX();
         fy = y - el->fFrame->GetY();
         return kTRUE;
      } else if (el->fFrame->IsComposite()) {
         if (((TGCompositeFrame *)el->fFrame)->TranslateCoordinates(child,
              x - el->fFrame->GetX(), y - el->fFrame->GetY(), fx, fy))
            return kTRUE;
      }
   }
   return kFALSE;
}


class TGMapKey : public TObject {
public:
   UInt_t     fKeyCode;
   TGWindow  *fWindow;
   TGMapKey(UInt_t keycode, TGWindow *w) { fKeyCode = keycode; fWindow = w; }
};

//______________________________________________________________________________
TGMainFrame::TGMainFrame(const TGWindow *p, UInt_t w, UInt_t h,
        UInt_t options) : TGCompositeFrame(p, w, h, options | kMainFrame)
{
   // Create a top level main frame. A main frame interacts
   // with the window manager.

   // WMDeleteNotify causes the system to send a kClientMessage to the
   // window with fFormat=32 and fUser[0]=gWM_DELETE_WINDOW when window
   // closed via WM
   gVirtualX->WMDeleteNotify(fId);

   fBindList = new TList;

   fMWMValue    = 0;
   fMWMFuncs    = 0;
   fMWMInput    = 0;
   fWMX         = -1;
   fWMY         = -1;
   fWMWidth     = (UInt_t) -1;
   fWMHeight    = (UInt_t) -1;
   fWMMinWidth  = (UInt_t) -1;
   fWMMinHeight = (UInt_t) -1;
   fWMMaxWidth  = (UInt_t) -1;
   fWMMaxHeight = (UInt_t) -1;
   fWMWidthInc  = (UInt_t) -1;
   fWMHeightInc = (UInt_t) -1;
   fWMInitState = (EInitialState) 0;
}

//______________________________________________________________________________
TGMainFrame::~TGMainFrame()
{
   // TGMainFrame destructor.

   if (fBindList) {
      fBindList->Delete();
      delete fBindList;
   }
   DestroyWindow();
}

//______________________________________________________________________________
Bool_t TGMainFrame::HandleKey(Event_t *event)
{
   // Handle keyboard events.

   if (event->fType == kGKeyPress)
      gVirtualX->SetKeyAutoRepeat(kFALSE);
   else
      gVirtualX->SetKeyAutoRepeat(kTRUE);

   if (!fBindList) return kFALSE;

   TIter next(fBindList);
   TGMapKey *m;
   TGFrame  *w;

   while ((m = (TGMapKey *) next())) {
      if (m->fKeyCode == event->fCode) {
         w = (TGFrame *) m->fWindow;
         return w->HandleKey(event);
      }
   }
   return kFALSE;
}

//______________________________________________________________________________
Bool_t TGMainFrame::BindKey(const TGWindow *w, Int_t keycode, Int_t modifier) const
{
   // Bind key to a window.

   if (fBindList) {
      TGMapKey *m = new TGMapKey(keycode, (TGWindow *)w);
      fBindList->Add(m);
      gVirtualX->GrabKey(fId, keycode, modifier, kTRUE);
      return kTRUE;
   }
   return kFALSE;
}

//______________________________________________________________________________
void TGMainFrame::RemoveBind(const TGWindow *, Int_t keycode, Int_t modifier) const
{
   // Remove key binding.

   if (fBindList) {
      TIter next(fBindList);
      TGMapKey *m;
      while ((m = (TGMapKey *) next())) {
         if (m->fKeyCode == (UInt_t) keycode) {
            fBindList->Remove(m);
            delete m;
            gVirtualX->GrabKey(fId, keycode, modifier, kFALSE);
            return;
         }
      }
   }
}

//______________________________________________________________________________
Bool_t TGMainFrame::HandleClientMessage(Event_t *event)
{
   // Handle client messages sent to this frame.

   TGCompositeFrame::HandleClientMessage(event);

   if ((event->fFormat == 32) && ((Atom_t)event->fUser[0] == gWM_DELETE_WINDOW) &&
       (event->fHandle != gROOT_MESSAGE)) {
      Emit("CloseWindow()");
      if (TestBit(kNotDeleted) && !TestBit(kDontCallClose))
         CloseWindow();
   }
   return kTRUE;
}

//______________________________________________________________________________
void TGMainFrame::SendCloseMessage()
{
   // Send close message to self. This method should be called from
   // a button to close this window.

   Event_t event;

   event.fType   = kClientMessage;
   event.fFormat = 32;
   event.fHandle = gWM_DELETE_WINDOW;

   event.fWindow  = GetId();
   event.fUser[0] = (Long_t) gWM_DELETE_WINDOW;
   event.fUser[1] = 0;
   event.fUser[2] = 0;
   event.fUser[3] = 0;
   event.fUser[4] = 0;

   gVirtualX->SendEvent(GetId(), &event);
}

//______________________________________________________________________________
void TGMainFrame::CloseWindow()
{
   // Close main frame. We get here in response to ALT+F4 or a window
   // manager close command. To terminate the application when this
   // happens override this method and call gApplication->Terminate(0) or
   // make a connection to this signal. If not the window will be just
   // destroyed and can not be used anymore.

   DestroyWindow();
}

//______________________________________________________________________________
void TGMainFrame::DontCallClose()
{
   // Typically call this method in the slot connected to the CloseWindow()
   // signal to prevent the calling of the default or any derived CloseWindow()
   // methods to prevent premature or double deletion of this window.

   SetBit(kDontCallClose);
}

//______________________________________________________________________________
void TGMainFrame::SetWindowName(const char *name)
{
   // Set window name. This is typically done via the window manager.

   fWindowName = name;
   gVirtualX->SetWindowName(fId, (char *)name);
}

//______________________________________________________________________________
void TGMainFrame::SetIconName(const char *name)
{
   // Set window icon name. This is typically done via the window manager.

   fIconName = name;
   gVirtualX->SetIconName(fId, (char *)name);
}

//______________________________________________________________________________
void TGMainFrame::SetIconPixmap(const char *iconName)
{
   // Set window icon pixmap by name. This is typically done via the window
   // manager.

   fIconPixmap = iconName;
   const TGPicture *iconPic = fClient->GetPicture(iconName);
   if (iconPic) {
      Pixmap_t pic = iconPic->GetPicture();
      gVirtualX->SetIconPixmap(fId, pic);
   }
}

//______________________________________________________________________________
void TGMainFrame::SetClassHints(const char *className, const char *resourceName)
{
   // Set the windows class and resource name. Used to get the right
   // resources from the resource database. However, ROOT applications
   // will typically use the .rootrc file for this.

   fClassName    = className;
   fResourceName = resourceName;
   gVirtualX->SetClassHints(fId, (char *)className, (char *)resourceName);
}

//______________________________________________________________________________
void TGMainFrame::SetMWMHints(UInt_t value, UInt_t funcs, UInt_t input)
{
   // Set decoration style for MWM-compatible wm (mwm, ncdwm, fvwm?).

   fMWMValue = value;
   fMWMFuncs = funcs;
   fMWMInput = input;
   gVirtualX->SetMWMHints(fId, value, funcs, input);
}

//______________________________________________________________________________
void TGMainFrame::SetWMPosition(Int_t x, Int_t y)
{
   // Give the window manager a window position hint.

   fWMX = x;
   fWMY = y;
   gVirtualX->SetWMPosition(fId, x, y);
}

//______________________________________________________________________________
void TGMainFrame::SetWMSize(UInt_t w, UInt_t h)
{
   // Give the window manager a window size hint.

   fWMWidth  = w;
   fWMHeight = h;
   gVirtualX->SetWMSize(fId, w, h);
}

//______________________________________________________________________________
void TGMainFrame::SetWMSizeHints(UInt_t wmin, UInt_t hmin,
                                 UInt_t wmax, UInt_t hmax,
                                 UInt_t winc, UInt_t hinc)
{
   // Give the window manager minimum and maximum size hints. Also
   // specify via winc and hinc the resize increments.

   fWMMinWidth  = wmin;
   fWMMinHeight = hmin;
   fWMMaxWidth  = wmax;
   fWMMaxHeight = hmax;
   fWMWidthInc  = winc;
   fWMHeightInc = hinc;
   gVirtualX->SetWMSizeHints(fId, wmin, hmin, wmax, hmax, winc, hinc);
}

//______________________________________________________________________________
void TGMainFrame::SetWMState(EInitialState state)
{
   // Set the initial state of the window. Either kNormalState or kIconicState.

   fWMInitState = state;
   gVirtualX->SetWMState(fId, state);
}

//______________________________________________________________________________
TGTransientFrame::TGTransientFrame(const TGWindow *p, const TGWindow *main,
                                   UInt_t w, UInt_t h, UInt_t options)
   : TGMainFrame(p, w, h, options | kTransientFrame)
{
   // Create a transient window. A transient window is typically used for
   // dialog boxes.

   fMain = main;

   if (fMain)
      gVirtualX->SetWMTransientHint(fId, fMain->GetId());
}


//______________________________________________________________________________
TGGroupFrame::TGGroupFrame(const TGWindow *p, TGString *title,
                           UInt_t options, GContext_t norm,
                           FontStruct_t font, ULong_t back) :
   TGCompositeFrame(p, 1, 1, options, back)
{
   // Create a group frame. The title will be adopted and deleted by the
   // group frame.

   fText       = title;
   fFontStruct = font;
   fNormGC     = norm;

   int max_ascent, max_descent;
   gVirtualX->GetFontProperties(fFontStruct, max_ascent, max_descent);
   fBorderWidth = max_ascent + max_descent + 1;
}

//______________________________________________________________________________
TGGroupFrame::TGGroupFrame(const TGWindow *p, const char *title,
                           UInt_t options, GContext_t norm,
                           FontStruct_t font, ULong_t back) :
   TGCompositeFrame(p, 1, 1, options, back)
{
   // Create a group frame.

   fText       = new TGString(title);
   fFontStruct = font;
   fNormGC     = norm;
   fTitlePos   = kLeft;

   int max_ascent, max_descent;
   gVirtualX->GetFontProperties(fFontStruct, max_ascent, max_descent);
   fBorderWidth = max_ascent + max_descent + 1;
}

//______________________________________________________________________________
TGGroupFrame::~TGGroupFrame()
{
   // Delete a group frame.

   delete fText;
}

//______________________________________________________________________________
TGDimension TGGroupFrame::GetDefaultSize() const
{
   // Returns default size.

   UInt_t tw = gVirtualX->TextWidth(fFontStruct, fText->GetString(),
                                    fText->GetLength()) + 24;

   TGDimension dim = TGCompositeFrame::GetDefaultSize();

   return  tw>dim.fWidth ? TGDimension(tw, dim.fHeight) : dim;
}

//______________________________________________________________________________
void TGGroupFrame::DoRedraw()
{
   // Redraw the group frame. Need special DoRedraw() since we need to
   // redraw with fBorderWidth=0.

   gVirtualX->ClearArea(fId, 0, 0, fWidth, fHeight);

   DrawBorder();
}

//______________________________________________________________________________
void TGGroupFrame::DrawBorder()
{
   // Draw border of around the group frame.
   //
   // if frame is kRaisedFrame  - a frame border is of "wall style",
   // otherwise of "groove style".

   Int_t x, y, l, t, r, b, gl, gr, sep, max_ascent, max_descent;

   UInt_t tw = gVirtualX->TextWidth(fFontStruct, fText->GetString(), fText->GetLength());
   gVirtualX->GetFontProperties(fFontStruct, max_ascent, max_descent);

   l = 0;
   t = (max_ascent + max_descent + 2) >> 1;
   r = fWidth - 1;
   b = fHeight - 1;

   sep = 3;
   UInt_t rr = 5 + (sep << 1) + tw;

   switch (fTitlePos) {
      case kRight:
         gl = fWidth>rr ? fWidth - rr : 5 + sep;
         break;
      case kCenter:
         gl = fWidth>tw ? ((fWidth - tw)>>1) - sep : 5 + sep;
         break;
      case kLeft:
      default:
         gl = 5 + sep;
   }
   gr = gl + tw + (sep << 1);

   switch (fOptions & (kSunkenFrame | kRaisedFrame)) {
      case kRaisedFrame:
         gVirtualX->DrawLine(fId, GetHilightGC()(),  l,   t,   gl,  t);
         gVirtualX->DrawLine(fId, GetShadowGC()(), l+1, t+1, gl,  t+1);

         gVirtualX->DrawLine(fId, GetHilightGC()(),  gr,  t,   r-1, t);
         gVirtualX->DrawLine(fId, GetShadowGC()(), gr,  t+1, r-2, t+1);

         gVirtualX->DrawLine(fId, GetHilightGC()(),  r-1, t,   r-1, b-1);
         gVirtualX->DrawLine(fId, GetShadowGC()(), r,   t,   r,   b);

         gVirtualX->DrawLine(fId, GetHilightGC()(),  r-1, b-1, l,   b-1);
         gVirtualX->DrawLine(fId, GetShadowGC()(), r,   b,   l,   b);

         gVirtualX->DrawLine(fId, GetHilightGC()(),  l,   b-1, l,   t);
         gVirtualX->DrawLine(fId, GetShadowGC()(), l+1, b-2, l+1, t+1);
         break;
      case kSunkenFrame:
      default:
         gVirtualX->DrawLine(fId, GetShadowGC()(),  l,   t,   gl,  t);
         gVirtualX->DrawLine(fId, GetHilightGC()(), l+1, t+1, gl,  t+1);

         gVirtualX->DrawLine(fId, GetShadowGC()(),  gr,  t,   r-1, t);
         gVirtualX->DrawLine(fId, GetHilightGC()(), gr,  t+1, r-2, t+1);

         gVirtualX->DrawLine(fId, GetShadowGC()(),  r-1, t,   r-1, b-1);
         gVirtualX->DrawLine(fId, GetHilightGC()(), r,   t,   r,   b);

         gVirtualX->DrawLine(fId, GetShadowGC()(),  r-1, b-1, l,   b-1);
         gVirtualX->DrawLine(fId, GetHilightGC()(), r,   b,   l,   b);

         gVirtualX->DrawLine(fId, GetShadowGC()(),  l,   b-1, l,   t);
         gVirtualX->DrawLine(fId, GetHilightGC()(), l+1, b-2, l+1, t+1);
         break;
   }

   x = gl + sep;
   y = 1;

   fText->Draw(fId, fNormGC, x, y + max_ascent);
}

//______________________________________________________________________________
void TGGroupFrame::SetTitle(TGString *title)
{
   // Set or change title of the group frame. Titlte TGString is adopted
   // by the TGGroupFrame.

   if (!title) {
      Warning("SetTitle", "title cannot be 0, try \"\"");
      title = new TGString("");
   }

   delete fText;

   fText = title;
   fClient->NeedRedraw(this);
}

//______________________________________________________________________________
void TGGroupFrame::SetTitle(const char *title)
{
   // Set or change title of the group frame.

   if (!title) {
      Error("SetTitle", "title cannot be 0, try \"\"");
      return;
   }

   SetTitle(new TGString(title));
}

//______________________________________________________________________________
FontStruct_t TGGroupFrame::GetDefaultFontStruct()
{
   if (!fgDefaultFont)
      fgDefaultFont = gClient->GetResourcePool()->GetDefaultFont();
   return fgDefaultFont->GetFontStruct();
}

//______________________________________________________________________________
const TGGC &TGGroupFrame::GetDefaultGC()
{
   if (!fgDefaultGC)
      fgDefaultGC = gClient->GetResourcePool()->GetFrameGC();
   return *fgDefaultGC;
}
