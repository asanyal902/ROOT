// @(#)root/ged:$Name:  $:$Id: TGedPropertyFrame.cxx,v 1.6 2004/04/22 16:28:28 brun Exp $
// Author: Marek Biskup, Ilka Antcheva 15/08/2003

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGedPropertiesFrame                                                  //
//                                                                      //
// TGedPropertiesFrame is a window that allows user to change object    //
// properties.                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "TGedPropertyFrame.h"
#include "TGedToolBox.h"
#include "TList.h"
#include "TGButton.h"
#include "TGPicture.h"
#include "TROOT.h"
#include "TRootCanvas.h"
#include "TCanvas.h"
#include "TGedEditor.h"
#include "TGedAttFrame.h"
#include "TGClient.h"
#include "TH1.h"
#include "TF1.h"
#include "TGDoubleSlider.h"
#include "Hoption.h"
#include "ctype.h"
#include "TGTab.h"

ClassImp(TGedPropertyFrame)

//______________________________________________________________________________
TGedPropertyFrame::TGedPropertyFrame(const TGWindow *p, TCanvas* canvas) :
      TGCompositeFrame(p, 110, 20, 0)
{
   Build();
   gROOT->GetListOfCleanups()->Add(this);
   if (canvas)
      ConnectToCanvas(canvas);
}

//______________________________________________________________________________
TGedPropertyFrame::~TGedPropertyFrame()
{
   gROOT->GetListOfCleanups()->Remove(this);
   Cleanup();
}

//______________________________________________________________________________
void TGedPropertyFrame::Build()
{
   TGTab* tab = new TGTab(this, 110, 30);
   AddFrame(tab, new TGLayoutHints(kLHintsTop));

   TGCompositeFrame *tab1 = tab->AddTab("Style");

   fAttFrame[0] = new TGedAttNameFrame(tab1, 1);
   fAttFrame[1] = new TGedAttFillFrame(tab1, 2);
   fAttFrame[2] = new TGedAttLineFrame(tab1, 3);
   fAttFrame[3] = new TGedAttTextFrame(tab1, 4);
   fAttFrame[4] = new TGedAttMarkerFrame(tab1, 5);
   fAttFrame[5] = new TGedAttAxisFrame(tab1, 6);
   fAttFrame[6] = new TGedAttAxisTitle(tab1, 7);
   fAttFrame[7] = new TGedAttAxisLabel(tab1, 8);
   
   for (int i = 0; i < kNPropertyFrames; i++)
      tab1->AddFrame(fAttFrame[i], 
                      new TGLayoutHints(kLHintsTop | kLHintsExpandX, 0, 0, 2, 2));
}

//______________________________________________________________________________
void TGedPropertyFrame::ConnectToCanvas(TCanvas *c)
{
   TQObject::Connect(c, "Selected(TPad*,TObject*,Int_t)", "TGedPropertyFrame",
                     this, "SetModel(TPad*,TObject*,Int_t)");

   // re-emit the last Selected() signal to have property frames
   // of the last selected object in the pad editor when the editor shows up
   c->Selected((TPad *)c->GetSelectedPad(), c->GetSelected(), c->GetEvent());
}

//______________________________________________________________________________
void TGedPropertyFrame::SetModel(TPad* pad, TObject* obj, Int_t event)
{
   // Slot connected to Selected() signal of TCanvas

   for (int i = 0; i < kNPropertyFrames; i++)
      fAttFrame[i]->SetModel(pad, obj, event);

   fModel = obj;
   fPad = pad;
}

//______________________________________________________________________________
Bool_t TGedPropertyFrame::ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2)
{
   if (parm1 + parm2 + msg )
      ;
/*
   switch (GET_MSG(msg)) {
      case kC_COMMAND:
         switch (GET_SUBMSG(msg)) {
            case kCM_RADIOBUTTON:
               switch (parm2) {
                  case kType:
                     SetRadio(fType, 12, parm1);
                     break;
                  case kCoords:
                     SetRadio(fCoords, 4, parm1);
                     break;
                  case kErrors:
                     SetRadio(fErrors, 5, parm1);
                     break;
                  default:
                     break;
               }
               break;
            case kCM_BUTTON:
               switch (parm1) {
                  case kDraw:
                     DrawHistogram();
                     break;
                  case kDefaults:
                     Reset();
                     break;
                  case kClose:
                     CloseFrame();
                     break;
                  default:
                     break;
               }
         }
         break;
      case kC_HSLIDER:
         ProcessSlider(GET_SUBMSG(msg));
         break;
   }
*/
   return kTRUE;
}

//______________________________________________________________________________
void TGedPropertyFrame::RecursiveRemove(TObject* obj)
{
   // Remove references to fModel in case the fModel is being deleted
   // Deactivate attribute frames if they point to obj

   if (fModel != obj) return;
   SetModel((TPad*)fPad,0,0);
}

