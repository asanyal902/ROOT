// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveCaloVizEditor.h"
#include "TEveCalo.h"
#include "TEveGValuators.h"
#include "TEveRGBAPaletteEditor.h"

#include "TGLabel.h"
#include "TGNumberEntry.h"
#include "TGDoubleSlider.h"
#include "TGNumberEntry.h"
#include "TG3DLine.h"

#include "TMathBase.h"
#include "TMath.h"
//______________________________________________________________________________
// GUI editor for TEveCaloEditor.
//

ClassImp(TEveCaloVizEditor);

//______________________________________________________________________________
TEveCaloVizEditor::TEveCaloVizEditor(const TGWindow *p, Int_t width, Int_t height,
                                     UInt_t options, Pixel_t back) :
   TGedFrame(p, width, height, options | kVerticalFrame, back),
   fM(0),

   fEtaRng(0),
   fPhi(0),
   fPhiRng(0),
   fTower(0),
   fPalette(0),
   fTowerHeight(0)
{
   // Constructor.

   MakeTitle("TEveCaloVizEditor");

   Int_t  labelW = 45;

   // eta
   fEtaRng = new TEveGDoubleValuator(this,"Eta rng:", 40, 0);
   fEtaRng->SetNELength(6);
   fEtaRng->SetLabelWidth(labelW);
   fEtaRng->Build();
   fEtaRng->GetSlider()->SetWidth(195);
   fEtaRng->SetLimits(-5, 5, TGNumberFormat::kNESRealTwo);
   fEtaRng->Connect("ValueSet()", "TEveCaloVizEditor", this, "DoEtaRange()");
   AddFrame(fEtaRng, new TGLayoutHints(kLHintsTop, 1, 1, 4, 5));

   // phi
   fPhi = new TEveGValuator(this, "Phi:", 90, 0);
   fPhi->SetLabelWidth(labelW);
   fPhi->SetNELength(6);
   fPhi->Build();
   fPhi->SetLimits(-180, 180);
   fPhi->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoPhi()");
   AddFrame(fPhi, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

   fPhiRng = new TEveGValuator(this, "PhiRng:", 90, 0);
   fPhiRng->SetLabelWidth(labelW);
   fPhiRng->SetNELength(6);
   fPhiRng->Build();
   fPhiRng->SetLimits(0, 180);
   fPhiRng->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoPhi()");
   AddFrame(fPhiRng, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

   CreateTowerTab();
}

//______________________________________________________________________________
void TEveCaloVizEditor::CreateTowerTab()
{
   // Create second tab in the editor.

   fTower = CreateEditorTabSubFrame("Tower");

   TGHorizontalFrame *title1 = new TGHorizontalFrame(fTower, 145, 10,
                                                     kLHintsExpandX | kFixedWidth);
   title1->AddFrame(new TGLabel(title1, "Tower"),
                    new TGLayoutHints(kLHintsLeft, 1, 1, 0, 0));
   title1->AddFrame(new TGHorizontal3DLine(title1),
                    new TGLayoutHints(kLHintsExpandX, 5, 5, 7, 7));
   fTower->AddFrame(title1, new TGLayoutHints(kLHintsTop, 0, 0, 2, 0));



   Int_t  labelW = 45;
   fTowerHeight = new TEveGValuator(fTower, "Height:", 90, 0);
   fTowerHeight->SetLabelWidth(labelW);
   fTowerHeight->SetNELength(6);
   fTowerHeight->Build();
   fTowerHeight->SetLimits(0, 1, 100, TGNumberFormat::kNESRealTwo);
   fTowerHeight->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoTowerHeight()");
   fTower->AddFrame(fTowerHeight, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

   TGHorizontalFrame *title2 = new TGHorizontalFrame(fTower, 145, 10, kLHintsExpandX| kFixedWidth);
   title2->AddFrame(new TGLabel(title2, "Palette Controls"),
                    new TGLayoutHints(kLHintsLeft, 1, 1, 0, 0));
   title2->AddFrame(new TGHorizontal3DLine(title2),
                    new TGLayoutHints(kLHintsExpandX, 5, 5, 7, 7));
   fTower->AddFrame(title2, new TGLayoutHints(kLHintsTop, 0, 0, 5, 0));

   fPalette = new TEveRGBAPaletteSubEditor(fTower);
   fTower->AddFrame(fPalette, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 0, 0, 0));
   fPalette->Connect("Changed()", "TEveCaloVizEditor", this, "DoPalette()");


}

//______________________________________________________________________________
void TEveCaloVizEditor::SetModel(TObject* obj)
{
   // Set model object.

   fM = dynamic_cast<TEveCaloViz*>(obj);

   fEtaRng->SetValues(fM->fEtaMin, fM->fEtaMax);

   fPhi->SetValue(fM->fPhi*TMath::RadToDeg());
   fPhiRng->SetValue(fM->fPhiRng*TMath::RadToDeg());

   fPalette->SetModel(fM->fPalette);

   fTowerHeight->SetValue(fM->fTowerHeight);

}

//______________________________________________________________________________
void TEveCaloVizEditor::DoEtaRange()
{
   // Slot for setting eta range.

   fM->fEtaMin = fEtaRng->GetMin();
   fM->fEtaMax = fEtaRng->GetMax();
   fM->fCacheOK = kFALSE;
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoPhi()
{
  // Slot for setting phi range.

   fM->fPhi    = fPhi->GetValue()*TMath::DegToRad();
   fM->fPhiRng = fPhiRng->GetValue()*TMath::DegToRad();
   fM->fCacheOK = kFALSE;
   Update();
}


//______________________________________________________________________________
void TEveCaloVizEditor::DoTowerHeight()
{
  // Slot for setting tower height.

   fM->SetTowerHeight(fTowerHeight->GetValue());
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoPalette()
{
   // Slot for palette changed.

   fM->fCacheOK = kFALSE;
   Update();
}
