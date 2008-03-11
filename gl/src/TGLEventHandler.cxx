// @(#)root/gl:$Id$
// Author: Bertrand Bellenot   29/01/2008

/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGLEventHandler                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include "TGLEventHandler.h"
#include "TGEventHandler.h"
#include "TGLViewer.h"
#include "TGLWidget.h"
#include "TGWindow.h"
#include "TPoint.h"
#include "TVirtualPad.h" // Remove when pad removed - use signal
#include "TVirtualX.h"
#include "TGClient.h"
#include "TVirtualGL.h"
#include "TGLOverlay.h"
#include "TGLLogicalShape.h"
#include "TGLPhysicalShape.h"
#include "TContextMenu.h"
#include "KeySymbols.h"

//______________________________________________________________________________
//
// Base-class and default implementation of event-handler for TGLViewer.
//
// This allows for complete disentanglement of GL-viewer from GUI
// event handling. Further, alternative event-handlers can easily be
// designed and set at run-time.
//
// The signals about object being selected or hovered above are
// emitted via the TGLViewer itself.
//
// This class is still under development.

ClassImp(TGLEventHandler);

//______________________________________________________________________________
TGLEventHandler::TGLEventHandler(const char *name, TGWindow *w, TObject *obj,
                                 const char *title) :
   TGEventHandler(name, w, obj, title),
   fGLViewer           ((TGLViewer *)obj),
   fMouseTimer         (0),
   fLastPos            (-1, -1),
   fLastMouseOverPos   (-1, -1),
   fLastMouseOverShape (0),
   fActiveButtonID     (0),
   fLastEventState     (0)
{
   // Constructor.

   fMouseTimer = new TTimer(this, 250);
}

//______________________________________________________________________________
TGLEventHandler::~TGLEventHandler()
{
   // Destructor.

   delete fMouseTimer;
}

//______________________________________________________________________________
void TGLEventHandler::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
   // Process event of type 'event' - one of EEventType types,
   // occuring at window location px, py
   // This is provided for use when embedding GL viewer into pad

   /*enum EEventType {
   kNoEvent       =  0,
   kButton1Down   =  1, kButton2Down   =  2, kButton3Down   =  3, kKeyDown  =  4,
   kButton1Up     = 11, kButton2Up     = 12, kButton3Up     = 13, kKeyUp    = 14,
   kButton1Motion = 21, kButton2Motion = 22, kButton3Motion = 23, kKeyPress = 24,
   kButton1Locate = 41, kButton2Locate = 42, kButton3Locate = 43,
   kMouseMotion   = 51, kMouseEnter    = 52, kMouseLeave    = 53,
   kButton1Double = 61, kButton2Double = 62, kButton3Double = 63

   enum EGEventType {
   kGKeyPress, kKeyRelease, kButtonPress, kButtonRelease,
   kMotionNotify, kEnterNotify, kLeaveNotify, kFocusIn, kFocusOut,
   kExpose, kConfigureNotify, kMapNotify, kUnmapNotify, kDestroyNotify,
   kClientMessage, kSelectionClear, kSelectionRequest, kSelectionNotify,
   kColormapNotify, kButtonDoubleClick, kOtherEvent*/

   // Map our event EEventType (base/inc/Buttons.h) back to Event_t (base/inc/GuiTypes.h)
   // structure, and call appropriate HandleXXX() function
   Event_t eventSt;
   eventSt.fX = px;
   eventSt.fY = py;
   eventSt.fState = 0;

   if (event != kKeyPress) {
      eventSt.fY -= Int_t((1 - gPad->GetHNDC() - gPad->GetYlowNDC()) * gPad->GetWh());
      eventSt.fX -= Int_t(gPad->GetXlowNDC() * gPad->GetWw());
   }

   switch (event) {
      case kMouseMotion:
         eventSt.fCode = kMouseMotion;
         eventSt.fType = kMotionNotify;
         HandleMotion(&eventSt);
         break;
      case kButton1Down:
      case kButton1Up:
      {
         eventSt.fCode = kButton1;
         eventSt.fType = event == kButton1Down ? kButtonPress:kButtonRelease;
         HandleButton(&eventSt);
      }
      break;
      case kButton2Down:
      case kButton2Up:
      {
         eventSt.fCode = kButton2;
         eventSt.fType = event == kButton2Down ? kButtonPress:kButtonRelease;
         HandleButton(&eventSt);
      }
      break;
      case kButton3Down:
      {
         eventSt.fState = kKeyShiftMask;
         eventSt.fCode = kButton1;
         eventSt.fType = kButtonPress;
         HandleButton(&eventSt);
      }
      break;
      case kButton3Up:
      {
         eventSt.fCode = kButton3;
         eventSt.fType = kButtonRelease;//event == kButton3Down ? kButtonPress:kButtonRelease;
         HandleButton(&eventSt);
      }
      break;
      case kButton1Double:
      case kButton2Double:
      case kButton3Double:
      {
         eventSt.fCode = kButton1Double ? kButton1 : kButton2Double ? kButton2 : kButton3;
         eventSt.fType = kButtonDoubleClick;
         HandleDoubleClick(&eventSt);
      }
      break;
      case kButton1Motion:
      case kButton2Motion:
      case kButton3Motion:
      {

         eventSt.fCode = event == kButton1Motion ? kButton1 : event == kButton2Motion ? kButton2 : kButton3;
         eventSt.fType = kMotionNotify;
         HandleMotion(&eventSt);
      }
      break;
      case kKeyPress: // We only care about full key 'presses' not individual down/up
      {
         eventSt.fType = kGKeyPress;
         eventSt.fCode = py; // px contains key code - need modifiers from somewhere
         HandleKey(&eventSt);
      }
      break;
      case 6://trick :)
         if (fGLViewer->CurrentCamera().Zoom(+50, kFALSE, kFALSE)) { //TODO : val static const somewhere
            if (fGLViewer->fGLDevice != -1) {
               gGLManager->MarkForDirectCopy(fGLViewer->fGLDevice, kTRUE);
               gVirtualX->SetDrawMode(TVirtualX::kCopy);
            }
            fGLViewer->RequestDraw();
         }
         break;
      case 5://trick :)
         if (fGLViewer->CurrentCamera().Zoom(-50, kFALSE, kFALSE)) { //TODO : val static const somewhere
            if (fGLViewer->fGLDevice != -1) {
               gGLManager->MarkForDirectCopy(fGLViewer->fGLDevice, kTRUE);
               gVirtualX->SetDrawMode(TVirtualX::kCopy);
            }
            fGLViewer->RequestDraw();
         }
         break;
      case 7://trick :)
         eventSt.fState = kKeyShiftMask;
         eventSt.fCode = kButton1;
         eventSt.fType = kButtonPress;
         HandleButton(&eventSt);
         break;
      default:
      {
        // Error("TGLEventHandler::ExecuteEvent", "invalid event type");
      }
   }
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleEvent(Event_t *event)
{
   // Handle generic Event_t type 'event' - provided to catch focus changes
   // and terminate any interaction in viewer.

   if (event->fType == kFocusIn) {
      if (fGLViewer->fAction != kNone) {
         Error("TGLEventHandler::HandleEvent", "active action at focus in");
      }
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fMouseTimer) {
         fMouseTimer->Reset();
         fMouseTimer->TurnOn();
      }
   }
   if (event->fType == kFocusOut) {
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fMouseTimer) {
         fMouseTimer->Reset();
         fMouseTimer->TurnOff();
      }
   }

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleFocusChange(Event_t *event)
{
   // Handle generic Event_t type 'event' - provided to catch focus changes
   // and terminate any interaction in viewer.

   fGLViewer->MouseIdle(0, 0, 0);
   if (event->fType == kFocusIn) {
      if (fGLViewer->fAction != kNone) {
         Error("TGLEventHandler::HandleEvent", "active action at focus in");
      }
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fMouseTimer) {
         fMouseTimer->Reset();
         fMouseTimer->TurnOn();
      }
      fGLViewer->Activated();
   }
   if (event->fType == kFocusOut) {
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fMouseTimer) {
         fMouseTimer->Reset();
         fMouseTimer->TurnOff();
      }
   }

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleCrossing(Event_t *event)
{
   // Handle generic Event_t type 'event' - provided to catch focus changes
   // and terminate any interaction in viewer.

   fGLViewer->MouseIdle(0, 0, 0);
   if (event->fType == kEnterNotify) {
      if (fGLViewer->fAction != kNone) {
         Error("TGLEventHandler::HandleEvent", "active action at focus in");
      }
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fMouseTimer) {
         fMouseTimer->Reset();
         fMouseTimer->TurnOn();
      }
      // Maybe, maybe not...
      fGLViewer->Activated();
   }
   if (event->fType == kLeaveNotify) {
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fMouseTimer) {
         fMouseTimer->Reset();
         fMouseTimer->TurnOff();
      }
   }

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleButton(Event_t * event)
{
   // Handle mouse button 'event'.
   static Event_t eventSt = {kOtherEvent, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, kFALSE, 0, 0, {0,0,0,0,0}};

   if (fGLViewer->IsLocked()) {
      if (gDebug>2) {
         Info("TGLEventHandler::HandleButton", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return kFALSE;
   }

   // Button DOWN
   if (event->fType == kButtonPress)
   {
      // Allow a single action/button down/up pairing - block others
      fGLViewer->MouseIdle(0, 0, 0);
      fGLViewer->Activated();
      if (fGLViewer->GetAction() != kNone)
         return kFALSE;
      eventSt.fX = event->fX;
      eventSt.fY = event->fY;
      eventSt.fCode = event->fCode;

      if (fGLViewer->GetPushAction() == TGLViewer::kPushCamCenter)
      {
         fGLViewer->fPushAction = TGLViewer::kPushStd;
         fGLViewer->RequestSelect(event->fX, event->fY);
         if (fGLViewer->fSelRec.GetN() > 0)
         {
            TGLVector3 v(event->fX, event->fY, 0.5*fGLViewer->fSelRec.GetMinZ());
            fGLViewer->CurrentCamera().WindowToViewport(v);
            v = fGLViewer->CurrentCamera().ViewportToWorld(v);
            fGLViewer->CurrentCamera().SetExternalCenter(kTRUE);
            fGLViewer->CurrentCamera().SetCenterVec(v.X(), v.Y(), v.Z());
            fGLViewer->RequestDraw();
         }
         fGLViewer->RefreshPadEditor(this);
         return kTRUE;
      }

      Bool_t grabPointer = kFALSE;
      Bool_t handled     = kFALSE;

      // Record active button for release
      fActiveButtonID = event->fCode;

      if (fGLViewer->GetAction() == TGLViewer::kDragNone && fGLViewer->fCurrentOvlElm)
      {
         if (fGLViewer->fCurrentOvlElm->Handle(*fGLViewer->fRnrCtx, fGLViewer->fOvlSelRec, event))
         {
            handled     = kTRUE;
            grabPointer = kTRUE;
            fGLViewer->fAction = TGLViewer::kDragOverlay;
            fGLViewer->RequestDraw();
         }
      }
      if ( ! handled)
      {
         switch(event->fCode)
         {
            // LEFT mouse button
            case kButton1:
            {
               if (event->fState & kKeyShiftMask) {
                  if (fGLViewer->RequestSelect(event->fX, event->fY)) {
                     fGLViewer->ApplySelection();
                     handled = kTRUE;
                  } else {
                     fGLViewer->SelectionChanged(); // Just notify clients.
                  }
               } else if (event->fState & kKeyMod1Mask) {
                  fGLViewer->RequestSelect(event->fX, event->fY, kTRUE);
                  if (fGLViewer->fSecSelRec.GetPhysShape() != 0)
                  {
                     TGLLogicalShape& lshape = const_cast<TGLLogicalShape&>
                        (*fGLViewer->fSecSelRec.GetPhysShape()->GetLogical());
                     lshape.ProcessSelection(*fGLViewer->fRnrCtx, fGLViewer->fSecSelRec);
                     handled = kTRUE;
                  }
               }
               if ( ! handled) {
                  fGLViewer->fAction = TGLViewer::kDragCameraRotate;
                  grabPointer = kTRUE;
                  if (fMouseTimer) {
                     fMouseTimer->TurnOff();
                     fMouseTimer->Reset();
                  }
               }
               break;
            }
               // MID mouse button
            case kButton2:
            {
               fGLViewer->fAction = TGLViewer::kDragCameraTruck;
               grabPointer = kTRUE;
               break;
            }
               // RIGHT mouse button
            case kButton3:
            {
               // Shift + Right mouse - select+context menu
               if (event->fState & kKeyShiftMask)
               {
                  fGLViewer->RequestSelect(event->fX, event->fY);
                  const TGLPhysicalShape * selected = fGLViewer->fSelRec.GetPhysShape();
                  if (selected) {
                     if (!fGLViewer->fContextMenu) {
                        fGLViewer->fContextMenu = new TContextMenu("glcm", "GL Viewer Context Menu");
                     }
                     Int_t    x, y;
                     Window_t childdum;
                     gVirtualX->TranslateCoordinates(fGLViewer->fGLWindow->GetId(),
                                                     gClient->GetDefaultRoot()->GetId(),
                                                     event->fX, event->fY, x, y, childdum);
                     selected->InvokeContextMenu(*fGLViewer->fContextMenu, x, y);
                  }
               } else {
                  fGLViewer->fAction = TGLViewer::kDragCameraDolly;
                  grabPointer = kTRUE;
               }
               break;
            }
         }
      }
   }
   // Button UP
   else if (event->fType == kButtonRelease)
   {
      if (fGLViewer->fAction == TGLViewer::kDragOverlay)
      {
         fGLViewer->fCurrentOvlElm->Handle(*fGLViewer->fRnrCtx, fGLViewer->fOvlSelRec, event);
         fGLViewer->OverlayDragFinished();
         if (fGLViewer->RequestOverlaySelect(event->fX, event->fY))
            fGLViewer->RequestDraw();
      }
      else if (fGLViewer->fAction >= TGLViewer::kDragCameraRotate &&
               fGLViewer->fAction <= TGLViewer::kDragCameraDolly)
      {
         fGLViewer->RequestDraw(TGLRnrCtx::kLODHigh);
      }

      // TODO: Check on Linux - on Win32 only see button release events
      // for mouse wheel
      switch(event->fCode) {
         // Buttons 4/5 are mouse wheel
         // Note: Modifiers (ctrl/shift) disabled as fState doesn't seem to
         // have correct modifier flags with mouse wheel under Windows.
         case kButton5: {
            // Zoom out (adjust camera FOV)
            if (fGLViewer->CurrentCamera().Zoom(+50, kFALSE, kFALSE)) { //TODO : val static const somewhere
               fGLViewer->RequestDraw();
            }
            break;
         }
         case kButton4: {
            // Zoom in (adjust camera FOV)
            if (fGLViewer->CurrentCamera().Zoom(-50, kFALSE, kFALSE)) { //TODO : val static const somewhere
               fGLViewer->RequestDraw();
            }
            break;
         }
      }
      fGLViewer->fAction = TGLViewer::kDragNone;
      if (fGLViewer->fGLDevice != -1)
         gGLManager->MarkForDirectCopy(fGLViewer->fGLDevice, kFALSE);
      if ((event->fX == eventSt.fX) &&
          (event->fY == eventSt.fY) &&
          (eventSt.fCode == event->fCode)) {
         TObject *obj = 0;
         fGLViewer->RequestSelect(fLastPos.fX, fLastPos.fY, kFALSE);
         TGLPhysicalShape *phys_shape = fGLViewer->fSelRec.GetPhysShape();
         if (phys_shape) {
            obj = phys_shape->GetLogical()->GetExternal();
         }
         fGLViewer->Clicked(obj);
         fGLViewer->Clicked(obj, event->fCode, event->fState);
         eventSt.fX = 0;
         eventSt.fY = 0;
         eventSt.fCode = 0;
         eventSt.fState = 0;
      }
      if (event->fCode == kButton1 && fMouseTimer) {
         fMouseTimer->TurnOn();
      }

   }

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleDoubleClick(Event_t *event)
{
   // Handle mouse double click 'event'.

   if (fGLViewer->IsLocked()) {
      if (gDebug>3) {
         Info("TGLEventHandler::HandleDoubleClick", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return kFALSE;
   }

   fGLViewer->MouseIdle(0, 0, 0);
   // Reset interactive camera mode on button double
   // click (unless mouse wheel)
   if (event->fCode != kButton4 && event->fCode != kButton5) {
      if (fGLViewer->fResetCameraOnDoubleClick) {
         fGLViewer->ResetCurrentCamera();
         fGLViewer->RequestDraw();
      }
      fGLViewer->DoubleClicked();
   }
   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleConfigureNotify(Event_t *event)
{
   // Handle configure notify 'event' - a window resize/movement.

   if (fGLViewer->IsLocked()) {
      if (gDebug > 0) {
         Info("TGLEventHandler::HandleConfigureNotify", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return kFALSE;
   }

   if (event) {
      fGLViewer->SetViewport(event->fX, event->fY, event->fWidth, event->fHeight);
      fGLViewer->RequestDraw(TGLRnrCtx::kLODMed);
   }
   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleExpose(Event_t * event)
{
   // Handle window expose 'event' - show.

   if (event->fCount != 0) return kTRUE;

   if (fGLViewer->IsLocked()) {
      if (gDebug > 0) {
         Info("TGLViewer::HandleExpose", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return kFALSE;
   }

   fGLViewer->fRedrawTimer->RequestDraw(20, TGLRnrCtx::kLODHigh);
   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleKey(Event_t *event)
{
   // Handle keyboard 'event'.

   fLastEventState = event->fState;

   fGLViewer->MouseIdle(0, 0, 0);
   if (fGLViewer->IsLocked()) {
      if (gDebug>3) {
         Info("TGLEventHandler::HandleKey", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return kFALSE;
   }

   char tmp[10] = {0};
   UInt_t keysym = 0;

   if (fGLViewer->fGLDevice == -1)
      gVirtualX->LookupString(event, tmp, sizeof(tmp), keysym);
   else
      keysym = event->fCode;
   fGLViewer->fRnrCtx->SetEventKeySym(keysym);

   Bool_t redraw = kFALSE;
   if (fGLViewer->fCurrentOvlElm &&
       fGLViewer->fCurrentOvlElm->Handle(*fGLViewer->fRnrCtx, fGLViewer->fOvlSelRec, event))
   {
      redraw = kTRUE;
   }
   else
   {
      Bool_t mod1 = event->fState & kKeyControlMask;
      Bool_t mod2 = event->fState & kKeyShiftMask;

      switch (keysym)
      {
         case kKey_R:
         case kKey_r:
            fGLViewer->SetStyle(TGLRnrCtx::kFill);
            if (fGLViewer->fClearColor == 0) {
               fGLViewer->fClearColor = 1; // Black
               fGLViewer->RefreshPadEditor(this);
            }
            redraw = kTRUE;
            break;
         case kKey_W:
         case kKey_w:
            fGLViewer->SetStyle(TGLRnrCtx::kWireFrame);
            if (fGLViewer->fClearColor == 0) {
               fGLViewer->fClearColor = 1; // Black
               fGLViewer->RefreshPadEditor(this);
            }
            redraw = kTRUE;
            break;
         case kKey_T:
         case kKey_t:
            fGLViewer->SetStyle(TGLRnrCtx::kOutline);
            if (fGLViewer->fClearColor == 1) {
               fGLViewer->fClearColor = 0; // White
               fGLViewer->RefreshPadEditor(this);
            }
            redraw = kTRUE;
            break;

         case kKey_F1:
            fGLViewer->RequestSelect(fLastPos.fX, fLastPos.fY, kFALSE);
            fGLViewer->MouseIdle(fGLViewer->fSelRec.GetPhysShape(), (UInt_t)fLastPos.fX, (UInt_t)fLastPos.fY);
            break;

            // Camera
         case kKey_Plus:
         case kKey_J:
         case kKey_j:
            redraw = fGLViewer->CurrentCamera().Dolly(10, mod1, mod2);
            break;
         case kKey_Minus:
         case kKey_K:
         case kKey_k:
            redraw = fGLViewer->CurrentCamera().Dolly(-10, mod1, mod2);
            break;
         case kKey_Up:
            redraw = fGLViewer->CurrentCamera().Truck(0, 10, mod1, mod2);
            break;
         case kKey_Down:
            redraw = fGLViewer->CurrentCamera().Truck(0, -10, mod1, mod2);
            break;
         case kKey_Left:
            redraw = fGLViewer->CurrentCamera().Truck(-10, 0, mod1, mod2);
            break;
         case kKey_Right:
            redraw = fGLViewer->CurrentCamera().Truck(10, 0, mod1, mod2);
            break;
         case kKey_Home:
            fGLViewer->ResetCurrentCamera();
            redraw = kTRUE;
            break;

            // Toggle debugging mode
         case kKey_D:
         case kKey_d:
            fGLViewer->fDebugMode = !fGLViewer->fDebugMode;
            redraw = kTRUE;
            Info("OpenGL viewer debug mode : ", fGLViewer->fDebugMode ? "ON" : "OFF");
            break;
            // Forced rebuild for debugging mode
         case kKey_Space:
            if (fGLViewer->fDebugMode) {
               Info("OpenGL viewer FORCED rebuild", "");
               fGLViewer->UpdateScene();
            }
         default:;
      } // switch
   }

   if (redraw) {
      if (fGLViewer->fGLDevice != -1)
         gGLManager->MarkForDirectCopy(fGLViewer->fGLDevice, kTRUE);
      fGLViewer->RequestDraw();
   }

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleMotion(Event_t * event)
{
   // Handle mouse motion 'event'.

   fGLViewer->MouseIdle(0, 0, 0);
   if (fGLViewer->IsLocked()) {
      if (gDebug>3) {
         Info("TGLEventHandler::HandleMotion", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return kFALSE;
   }

   assert (event); // was if event==0 return

   Bool_t processed = kFALSE, changed = kFALSE;
   Short_t lod = TGLRnrCtx::kLODMed;

   // Camera interface requires GL coords - Y inverted
   Int_t  xDelta = event->fX - fLastPos.fX;
   Int_t  yDelta = event->fY - fLastPos.fY;
   Bool_t mod1   = event->fState & kKeyControlMask;
   Bool_t mod2   = event->fState & kKeyShiftMask;

   if (fGLViewer->fAction == TGLViewer::kDragNone)
   {
      changed = fGLViewer->RequestOverlaySelect(event->fX, event->fY);
      if (fGLViewer->fCurrentOvlElm)
         processed = fGLViewer->fCurrentOvlElm->Handle(*fGLViewer->fRnrCtx, fGLViewer->fOvlSelRec, event);
      lod = TGLRnrCtx::kLODHigh;
   } else if (fGLViewer->fAction == TGLViewer::kDragCameraRotate) {
      processed = fGLViewer->CurrentCamera().Rotate(xDelta, -yDelta, mod1, mod2);
   } else if (fGLViewer->fAction == TGLViewer::kDragCameraTruck) {
      processed = fGLViewer->CurrentCamera().Truck(xDelta, -yDelta, mod1, mod2);
   } else if (fGLViewer->fAction == TGLViewer::kDragCameraDolly) {
      processed = fGLViewer->CurrentCamera().Dolly(xDelta, mod1, mod2);
   } else if (fGLViewer->fAction == TGLViewer::kDragOverlay) {
      processed = fGLViewer->fCurrentOvlElm->Handle(*fGLViewer->fRnrCtx, fGLViewer->fOvlSelRec, event);
   }

   fLastPos.fX = event->fX;
   fLastPos.fY = event->fY;

   if (processed || changed) {
      if (fGLViewer->fGLDevice != -1) {
         gGLManager->MarkForDirectCopy(fGLViewer->fGLDevice, kTRUE);
         gVirtualX->SetDrawMode(TVirtualX::kCopy);
      }

      fGLViewer->RequestDraw(lod);
   }

   return processed;
}

//______________________________________________________________________________
Bool_t TGLEventHandler::HandleTimer(TTimer *t)
{
   // If mouse delay timer times out emit signal.

   if (t != fMouseTimer) return kTRUE;
   if (fGLViewer->fAction == TGLViewer::kDragNone) {
      if (fLastMouseOverPos != fLastPos) {
         fGLViewer->RequestSelect(fLastPos.fX, fLastPos.fY, kFALSE);
         if (fLastMouseOverShape != fGLViewer->fSelRec.GetPhysShape()) {
            fLastMouseOverShape = fGLViewer->fSelRec.GetPhysShape();
            fGLViewer->MouseOver(fLastMouseOverShape);
            fGLViewer->MouseOver(fLastMouseOverShape, fLastEventState);
         }
         fLastMouseOverPos = fLastPos;
      }
   }
   return kTRUE;
}

//______________________________________________________________________________
void TGLEventHandler::Repaint()
{
   // Handle window expose 'event' - show.

   if (fGLViewer->IsLocked()) {
      if (gDebug > 0) {
         Info("TGLViewer::HandleExpose", "ignored - viewer is %s",
            fGLViewer->LockName(fGLViewer->CurrentLock()));
      }
      return;
   }
   fGLViewer->fRedrawTimer->RequestDraw(20, TGLRnrCtx::kLODHigh);
}

