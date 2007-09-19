// @(#)root/gui:$Id: TRootHelpDialog.cxx,v 1.9 2006/05/26 09:16:29 brun Exp $
// Author: Fons Rademakers   24/02/98

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TRootHelpDialog                                                      //
//                                                                      //
// A TRootHelpDialog is used to display help text (or any text in a     //
// dialog window). There is on OK button to popdown the dialog.         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TRootHelpDialog.h"
#include "TGButton.h"
#include "TGTextView.h"


ClassImp(TRootHelpDialog)

//______________________________________________________________________________
TRootHelpDialog::TRootHelpDialog(const TGWindow *main,
    const char *title, UInt_t w, UInt_t h) :
    TGTransientFrame(gClient->GetRoot(), main, w, h)
{
   // Create a help text dialog.

   fView = new TGTextView(this, w, h, kSunkenFrame | kDoubleBorder);
   fL1 = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 3, 3, 3, 3);
   AddFrame(fView, fL1);

   fOK = new TGTextButton(this, "  &OK  ");
   fL2 = new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 0, 0, 5, 5);
   AddFrame(fOK, fL2);

   SetWindowName(title);
   SetIconName(title);

   MapSubwindows();

   Resize(GetDefaultSize());

   // position relative to the parent's window
   CenterOnParent();
}

//______________________________________________________________________________
TRootHelpDialog::TRootHelpDialog(const TRootHelpDialog& rd) :
  TGTransientFrame(rd),
  fView(rd.fView),
  fOK(rd.fOK),
  fL1(rd.fL1),
  fL2(rd.fL2)
{ 
   //copy constructor
}

//______________________________________________________________________________
TRootHelpDialog& TRootHelpDialog::operator=(const TRootHelpDialog& rd)
{
   //assignment operator
   if(this!=&rd) {
      TGTransientFrame::operator=(rd);
      fView=rd.fView;
      fOK=rd.fOK;
      fL1=rd.fL1;
      fL2=rd.fL2;
   } 
   return *this;
}

//______________________________________________________________________________
TRootHelpDialog::~TRootHelpDialog()
{
   // Delete help text dialog.

   delete fView;
   delete fOK;
   delete fL1;
   delete fL2;
}

//______________________________________________________________________________
void TRootHelpDialog::Popup()
{
   // Show help dialog.

   MapWindow();
}

//______________________________________________________________________________
void TRootHelpDialog::SetText(const char *helpText)
{
   // Set help text from helpText buffer in TGTextView.

   fView->LoadBuffer(helpText);
}

//______________________________________________________________________________
void TRootHelpDialog::AddText(const char *helpText)
{
   // Add help text from helpText buffer to already existing text in TGTextView.

   TGText tt;
   tt.LoadBuffer(helpText);
   fView->AddText(&tt);
}

//______________________________________________________________________________
void TRootHelpDialog::CloseWindow()
{
   // Called when closed via window manager action.

   DeleteWindow();
}

//______________________________________________________________________________
Bool_t TRootHelpDialog::ProcessMessage(Long_t msg, Long_t, Long_t)
{
   // Process OK button.

   switch (GET_MSG(msg)) {
      case kC_COMMAND:
         switch (GET_SUBMSG(msg)) {
            case kCM_BUTTON:
               // Only one button and one action...
               DeleteWindow();
               break;
            default:
               break;
         }
      default:
         break;
   }

   return kTRUE;
}

