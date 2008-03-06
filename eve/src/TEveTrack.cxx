// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveTrack.h"
#include "TEveTrackPropagator.h"
#include "TEvePointSet.h"

#include "TPolyLine3D.h"
#include "TMarker.h"
#include "TPolyMarker3D.h"
#include "TColor.h"
#include "TROOT.h"

// Updates
#include "TEveManager.h"
#include "TEveBrowser.h"
#include "TEveTrackProjected.h"
#include "TCanvas.h"

#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>

//==============================================================================
//==============================================================================
// TEveTrack
//==============================================================================

//______________________________________________________________________________
//
// Visual representation of a track.
//

ClassImp(TEveTrack);

//______________________________________________________________________________
TEveTrack::TEveTrack() :
   TEveLine(),

   fV(),
   fP(),
   fBeta(0),
   fPdg(0),
   fCharge(0),
   fLabel(kMinInt),
   fIndex(kMinInt),
   fPathMarks(),

   fPropagator(0)
{
   // Default constructor.
}

//______________________________________________________________________________
TEveTrack::TEveTrack(TParticle* t, Int_t label, TEveTrackPropagator* rs):
   TEveLine(),

   fV(t->Vx(), t->Vy(), t->Vz()),
   fP(t->Px(), t->Py(), t->Pz()),
   fBeta(t->P()/t->Energy()),
   fPdg(0),
   fCharge(0),
   fLabel(label),
   fIndex(kMinInt),
   fPathMarks(),

   fPropagator(0)
{
   // Constructor from TParticle.

   SetPropagator(rs);
   fMainColorPtr = &fLineColor;

   TParticlePDG* pdgp = t->GetPDG();
   if (pdgp) {
      fPdg    = pdgp->PdgCode();
      fCharge = (Int_t) TMath::Nint(pdgp->Charge()/3);
   }

   SetName(t->GetName());
}

//______________________________________________________________________________
TEveTrack::TEveTrack(TEveMCTrack* t, TEveTrackPropagator* rs):
   TEveLine(),

   fV(t->Vx(), t->Vy(), t->Vz()),
   fP(t->Px(), t->Py(), t->Pz()),
   fBeta(t->P()/t->Energy()),
   fPdg(0),
   fCharge(0),
   fLabel(t->fLabel),
   fIndex(t->fIndex),
   fPathMarks(),

   fPropagator(0)
{
   // Constructor from TEveUtil Monte Carlo track.

   SetPropagator(rs);
   fMainColorPtr = &fLineColor;

   TParticlePDG* pdgp = t->GetPDG();
   if (pdgp == 0) {
      t->ResetPdgCode(); pdgp = t->GetPDG();
   }
   fCharge = (Int_t) TMath::Nint(pdgp->Charge()/3);

   SetName(t->GetName());
}

//______________________________________________________________________________
TEveTrack::TEveTrack(TEveRecTrack* t, TEveTrackPropagator* rs) :
   TEveLine(),

   fV(t->fV),
   fP(t->fP),
   fBeta(t->fBeta),
   fPdg(0),
   fCharge(t->fSign),
   fLabel(t->fLabel),
   fIndex(t->fIndex),
   fPathMarks(),

   fPropagator(0)
{
   // Constructor from TEveUtil reconstructed track.

   SetPropagator(rs);
   fMainColorPtr = &fLineColor;

   SetName(t->GetName());
}

//______________________________________________________________________________
TEveTrack::TEveTrack(const TEveTrack& t) :
   TEveLine(),
   fV(t.fV),
   fP(t.fP),
   fBeta(t.fBeta),
   fPdg(t.fPdg),
   fCharge(t.fCharge),
   fLabel(t.fLabel),
   fIndex(t.fIndex),
   fPathMarks(),
   fPropagator(0)
{
   // Copy constructor.

   SetMainColor(t.GetMainColor());
   // TEveLine
   fRnrLine   = t.fRnrLine;
   fRnrPoints = t.fRnrPoints;
   // TLineAttrib
   fLineColor = t.fLineColor;
   fLineStyle = t.fLineStyle;
   fLineWidth = t.fLineWidth;
   SetPathMarks(t);
   SetPropagator (t.fPropagator);
}

//______________________________________________________________________________
TEveTrack::~TEveTrack()
{
   // Destructor.

   SetPropagator(0);
}


//______________________________________________________________________________
const TGPicture* TEveTrack::GetListTreeIcon(Bool_t)
{
   // Returns list-tree icon for TEveTrack.

   return fgListTreeIcons[4];
}


/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::SetStdTitle()
{
   // Set standard track title based on most data-member values.

   TString idx(fIndex == kMinInt ? "<undef>" : Form("%d", fIndex));
   TString lbl(fLabel == kMinInt ? "<undef>" : Form("%d", fLabel));
   SetTitle(Form("Index=%s, Label=%s\nChg=%d, Pdg=%d\n"
                 "pT=%.3f, pZ=%.3f\nV=(%.3f, %.3f, %.3f)",
                 idx.Data(), lbl.Data(), fCharge, fPdg,
                 fP.Perp(), fP.fZ, fV.fX, fV.fY, fV.fZ));
}

//______________________________________________________________________________
void TEveTrack::SetTrackParams(const TEveTrack& t)
{
   // Copy track parameters from t.
   // PathMarks are cleared.

   fV         = t.fV;
   fP         = t.fP;
   fBeta      = t.fBeta;
   fPdg       = t.fPdg;
   fCharge    = t.fCharge;
   fLabel     = t.fLabel;
   fIndex     = t.fIndex;

   SetMainColor(t.GetMainColor());
   // TEveLine
   fRnrLine   = t.fRnrLine;
   fRnrPoints = t.fRnrPoints;
   // TLineAttrib
   fLineColor = t.fLineColor;
   fLineStyle = t.fLineStyle;
   fLineWidth = t.fLineWidth;
   fPathMarks.clear();
   SetPropagator(t.fPropagator);
}

//______________________________________________________________________________
void TEveTrack::SetPathMarks(const TEveTrack& t)
{
   // Copy path-marks from t.

   const vPathMark_t& tpms = t.RefPathMarks();
   std::copy(t.RefPathMarks().begin(), t.RefPathMarks().end(),
             std::back_insert_iterator<vPathMark_t>(fPathMarks));
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::SetPropagator(TEveTrackPropagator* rs)
{
   // Set track's render style.
   // Reference counts of old and new render-style are updated.

   if (fPropagator == rs) return;
   if (fPropagator) fPropagator->DecRefCount(this);
   fPropagator = rs;
   if (fPropagator) rs->IncRefCount(this);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::SetAttLineAttMarker(TEveTrackList* tl)
{
   // Set line and marker attributes from TEveTrackList.

   SetRnrLine(tl->GetRnrLine());
   SetLineColor(tl->GetLineColor());
   SetLineStyle(tl->GetLineStyle());
   SetLineWidth(tl->GetLineWidth());

   SetRnrPoints(tl->GetRnrPoints());
   SetMarkerColor(tl->GetMarkerColor());
   SetMarkerStyle(tl->GetMarkerStyle());
   SetMarkerSize(tl->GetMarkerSize());
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::MakeTrack(Bool_t recurse)
{
   // Calculate track representation based on track data and current
   // settings of the render-style.
   // If recurse is true, descend into children.

   Reset(0);

   TEveTrackPropagator& rTP((fPropagator != 0) ? *fPropagator : TEveTrackPropagator::fgDefStyle);

   const Float_t maxRsq = rTP.GetMaxR() * rTP.GetMaxR();

   if (TMath::Abs(fV.fZ) < rTP.GetMaxZ() && fV.fX*fV.fX + fV.fY*fV.fY < maxRsq)
   {
      TEveVector currP = fP;
      Bool_t decay = kFALSE;
      fPropagator->InitTrack(fV, fP, fBeta, fCharge);
      for (vPathMark_i pm = fPathMarks.begin(); pm != fPathMarks.end(); ++pm)
      {
         if (rTP.GetFitReferences() && pm->fType == TEvePathMark::kReference)
         {
            if (TMath::Abs(pm->fV.fZ) > rTP.GetMaxZ() ||
               pm->fV.fX*pm->fV.fX + pm->fV.fY*pm->fV.fY > maxRsq)
               goto bounds;
            // printf("%s fit reference  \n", fName.Data());
            if (fPropagator->GoToVertex(pm->fV, currP)) {
               currP.fX = pm->fP.fX; currP.fY = pm->fP.fY; currP.fZ = pm->fP.fZ;
            }
         }
         else if (rTP.GetFitDaughters() &&  pm->fType == TEvePathMark::kDaughter)
         {
            if (TMath::Abs(pm->fV.fZ) > rTP.GetMaxZ() ||
                pm->fV.fX*pm->fV.fX + pm->fV.fY*pm->fV.fY > maxRsq)
               goto bounds;
            // printf("%s fit daughter  \n", fName.Data());
            if (fPropagator->GoToVertex(pm->fV, currP)) {
               currP.fX -= pm->fP.fX; currP.fY -= pm->fP.fY; currP.fZ -= pm->fP.fZ;
            }
         }
         else if (rTP.GetFitDecay() &&  pm->fType == TEvePathMark::kDecay)
         {
            if (TMath::Abs(pm->fV.fZ) > rTP.GetMaxZ() ||
                pm->fV.fX*pm->fV.fX + pm->fV.fY*pm->fV.fY > maxRsq)
               goto bounds;
            // printf("%s fit decay \n", fName.Data());
            fPropagator->GoToVertex(pm->fV, currP);
            decay = true;
            break;
         }
      } // loop path-marks

   bounds:
      if (!decay || rTP.GetFitDecay() == kFALSE)
      {
         // printf("%s loop to bounds  \n",fName.Data() );
         fPropagator->GoToBounds(currP);
      }
      //  make_polyline:
      fPropagator->FillPointSet(this);
      fPropagator->ResetTrack();
   }

   if (recurse)
   {
      for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i)
      {
         TEveTrack* t = dynamic_cast<TEveTrack*>(*i);
         if (t) t->MakeTrack(recurse);
      }
   }
}

/******************************************************************************/

//______________________________________________________________________________
TClass* TEveTrack::ProjectedClass() const
{
   // Virtual from TEveProjectable, return TEveTrackProjected class.

   return TEveTrackProjected::Class();
}

/******************************************************************************/

namespace {

struct Cmp_pathmark_t
{
   bool operator()(TEvePathMark const & a, TEvePathMark const & b)
   { return a.fTime < b.fTime; }
};

}

//______________________________________________________________________________
void TEveTrack::SortPathMarksByTime()
{
   // Sort registerd pat-marks by time.

   std::sort(fPathMarks.begin(), fPathMarks.end(), Cmp_pathmark_t());
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::ImportHits()
{
   // Import hits with same label as the track.
   // Uses macro "hits_from_label.C".

   TEveUtil::LoadMacro("hits_from_label.C");
   gROOT->ProcessLine(Form("hits_from_label(%d, (TEveElement*)%p);",
                           fLabel, this));
}

//______________________________________________________________________________
void TEveTrack::ImportClusters()
{
   // Import clusters with same label as the track.
   // Uses macro "clusters_from_label.C".

   TEveUtil::LoadMacro("clusters_from_label.C");
   gROOT->ProcessLine(Form("clusters_from_label(%d, (TEveElement*)%p);",
                           fLabel, this));
}

//______________________________________________________________________________
void TEveTrack::ImportClustersFromIndex()
{
   // Import clusters marked with same reconstructed track index as the track.
   // Uses macro "clusters_from_index.C".

   static const TEveException eh("TEveTrack::ImportClustersFromIndex ");

   if (fIndex == kMinInt)
      throw(eh + "index not set.");

   TEveUtil::LoadMacro("clusters_from_index.C");
   gROOT->ProcessLine(Form("clusters_from_index(%d, (TEveElement*)%p);",
                           fIndex, this));
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::ImportKine()
{
   // Import kinematics of the track's label recursively.
   // Uses macro "kine_tracks.C".

   static const TEveException eh("TEveTrack::ImportKine ");

   if (fLabel == kMinInt)
      throw(eh + "label not set.");

   Int_t label;
   if (fLabel < 0) {
      Warning(eh, "label negative, taking absolute value.");
      label = -fLabel;
   } else {
      label = fLabel;
   }

   TEveUtil::LoadMacro("kine_tracks.C");
   gROOT->ProcessLine(Form("kine_track(%d, kTRUE, kTRUE, kTRUE, kTRUE, (TEveElement*)%p);",
                           label, this));

}

//______________________________________________________________________________
void TEveTrack::ImportKineWithArgs(Bool_t importMother, Bool_t importDaugters,
                                   Bool_t colorPdg,     Bool_t recurse)
{
   // Import kinematics of the track's label. Arguments steer the
   // import process:
   //   importMother     import particle with track's label
   //   importDaugters   import direct daughters of label
   //   colorPdg         color kinematics by PDG code
   //   recurse          recursive import of daughters' daughters
   // Uses macro "kine_tracks.C".

   static const TEveException eh("TEveTrack::ImportKineWithArgs ");

   if (fLabel == kMinInt)
      throw(eh + "label not set.");

   Int_t label;
   if (fLabel < 0) {
      Warning(eh, "label negative, taking absolute value.");
      label = -fLabel;
   } else {
      label = fLabel;
   }

   TEveUtil::LoadMacro("kine_tracks.C");
   gROOT->ProcessLine(Form("kine_track(%d, %d, %d, %d, %d, (TEveElement*)%p);",
                           label, importMother, importDaugters, colorPdg, recurse, this));
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::PrintKineStack()
{
   // Print kinematics pertaining to track's label.
   // Uses macro "print_kine_from_label.C".

   static const TEveException eh("TEveTrack::PrintKineStack ");

   if (fLabel == kMinInt)
      throw(eh + "label not set.");

   Int_t label;
   if (fLabel < 0) {
      Warning(eh, "label negative, taking absolute value.");
      label = -fLabel;
   } else {
      label = fLabel;
   }

   TEveUtil::LoadMacro("print_kine_from_label.C");
   gROOT->ProcessLine(Form("print_kine_from_label(%d);", label));
}

//______________________________________________________________________________
void TEveTrack::PrintPathMarks()
{
   // Print registered path-marks.

   static const TEveException eh("TEveTrack::PrintPathMarks ");

   printf("TEveTrack '%s', number of path marks %d, label %d\n",
          GetName(), (Int_t)fPathMarks.size(), fLabel);

   for (vPathMark_i pm = fPathMarks.begin(); pm != fPathMarks.end(); ++pm)
   {
      printf("  %-9s  p: %8f %8f %8f Vertex: %8e %8e %8e %g \n",
             pm->TypeName(),
             pm->fP.fX,  pm->fP.fY, pm->fP.fZ,
             pm->fV.fX,  pm->fV.fY, pm->fV.fZ,
             pm->fTime);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrack::SecSelected(TEveTrack* track)
{
   // Emits "SecSelected(TEveTrack*)" signal.
   // Called from TEveTrackGL on secondary-selection.

   Emit("SecSelected(TEveTrack*)", (Long_t)track);
}

//______________________________________________________________________________
void TEveTrack::SetLineStyle(Style_t lstyle)
{
   // Set line-style of the track.
   // The style is propagated to projected tracks.

   TAttLine::SetLineStyle(lstyle);
   std::list<TEveProjected*>::iterator pi = fProjectedList.begin();
   while (pi != fProjectedList.end())
   {
      TEveTrack* pt = dynamic_cast<TEveTrack*>(*pi);
      if (pt)
      {
         pt->SetLineStyle(lstyle);
         pt->ElementChanged();
      }
      ++pi;
   }
}


//==============================================================================
//==============================================================================
// TEveTrackList
//==============================================================================

//______________________________________________________________________________
//
// A list of tracks supporting change of common attributes and
// selection based on track parameters.

ClassImp(TEveTrackList);

//______________________________________________________________________________
TEveTrackList::TEveTrackList(TEveTrackPropagator* rs) :
   TEveElementList(),
   TAttMarker(1, 20, 1),
   TAttLine(1,1,1),

   fPropagator(0),
   fRecurse(kTRUE),
   fRnrLine(kTRUE),
   fRnrPoints(kFALSE),

   fMinPt (0), fMaxPt (0), fLimPt (0),
   fMinP  (0), fMaxP  (0), fLimP  (0)
{
   // Constructor. If TrackRenderStyle argument is 0, a new default
   // render-style is created.

   fChildClass = TEveTrack::Class(); // override member from base TEveElementList

   fMainColorPtr = &fLineColor;
   if (fPropagator== 0) rs = new TEveTrackPropagator;
   SetPropagator(rs);
}

//______________________________________________________________________________
TEveTrackList::TEveTrackList(const Text_t* name, TEveTrackPropagator* rs) :
   TEveElementList(name),
   TAttMarker(1, 20, 1),
   TAttLine(1,1,1),

   fPropagator(0),
   fRecurse(kTRUE),
   fRnrLine(kTRUE),
   fRnrPoints(kFALSE),

   fMinPt (0), fMaxPt (0), fLimPt (0),
   fMinP  (0), fMaxP  (0), fLimP  (0)
{
   // Constructor. If TrackRenderStyle argument is 0, a new default
   // render-style is created.

   fChildClass = TEveTrack::Class(); // override member from base TEveElementList

   fMainColorPtr = &fLineColor;
   if (fPropagator== 0) rs = new TEveTrackPropagator;
   SetPropagator(rs);
}

//______________________________________________________________________________
TEveTrackList::~TEveTrackList()
{
   // Destructor.

   SetPropagator(0);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetPropagator(TEveTrackPropagator* rs)
{
   // Set default render-style for tracks.
   // This is not enforced onto the tracks themselves but this is the
   // render-style that is show in the TEveTrackListEditor.

   if (fPropagator == rs) return;
   if (fPropagator) fPropagator->DecRefCount();
   fPropagator = rs;
   if (fPropagator) rs->IncRefCount();
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::MakeTracks(Bool_t recurse)
{
   // Regenerate the visual representations of tracks.
   // The momentum limits are rescanned during the same traversal.

   fLimPt = fLimP = 0;

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      track->MakeTrack(recurse);

      fLimPt = TMath::Max(fLimPt, track->fP.Perp());
      fLimP  = TMath::Max(fLimP,  track->fP.Mag());
      if (recurse)
         FindMomentumLimits(*i, recurse);
   }

   fLimPt = RoundMomentumLimit(fLimPt);
   fLimP  = RoundMomentumLimit(fLimP);
   if (fMaxPt == 0) fMaxPt = fLimPt;
   if (fMaxP  == 0) fMaxP  = fLimP;
}

//______________________________________________________________________________
void TEveTrackList::FindMomentumLimits(TEveElement* el, Bool_t recurse)
{
   // Loop over track elements of argument el and find highest pT and p.
   // These are stored in members fLimPt and fLimP.

   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      TEveTrack* track = dynamic_cast<TEveTrack*>(*i);
      if (track)
      {
         fLimPt = TMath::Max(fLimPt, track->fP.Perp());
         fLimP  = TMath::Max(fLimP,  track->fP.Mag());
         if (recurse)
            FindMomentumLimits(*i, recurse);
      }
   }
}

//______________________________________________________________________________
Float_t TEveTrackList::RoundMomentumLimit(Float_t x)
{
   // Round the momentum limit up to a nice value.

   using namespace TMath;
   Double_t fac = Power(10, 1 - Floor(Log10(x)));
   return Ceil(fac*x) / fac;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetRnrLine(Bool_t rnr)
{
   // Set rendering of track as line for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetRnrLine() == fRnrLine)
         track->SetRnrLine(rnr);
      if (fRecurse)
         SetRnrLine(rnr, *i);
   }
   fRnrLine = rnr;
}

//______________________________________________________________________________
void TEveTrackList::SetRnrLine(Bool_t rnr, TEveElement* el)
{
   // Set rendering of track as line for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && (track->GetRnrLine() == fRnrLine))
         track->SetRnrLine(rnr);
      if (fRecurse)
         SetRnrLine(rnr, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetRnrPoints(Bool_t rnr)
{
   // Set rendering of track as points for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetRnrPoints() == fRnrPoints)
         track->SetRnrPoints(rnr);
      if (fRecurse)
         SetRnrPoints(rnr, *i);
   }
   fRnrPoints = rnr;
}

//______________________________________________________________________________
void TEveTrackList::SetRnrPoints(Bool_t rnr, TEveElement* el)
{
   // Set rendering of track as points for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track)
         if (track->GetRnrPoints() == fRnrPoints)
            track->SetRnrPoints(rnr);
      if (fRecurse)
         SetRnrPoints(rnr, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetMainColor(Color_t col)
{
   // Set main (line) color for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetLineColor() == fLineColor)
         track->SetLineColor(col);
      if (fRecurse)
         SetLineColor(col, *i);
   }
   TEveElement::SetMainColor(col);
}

//______________________________________________________________________________
void TEveTrackList::SetLineColor(Color_t col, TEveElement* el)
{
   // Set line color for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && track->GetLineColor() == fLineColor)
         track->SetLineColor(col);
      if (fRecurse)
         SetLineColor(col, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetLineWidth(Width_t width)
{
   // Set line width for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetLineWidth() == fLineWidth)
         track->SetLineWidth(width);
      if (fRecurse)
         SetLineWidth(width, *i);
   }
   fLineWidth=width;
}

//______________________________________________________________________________
void TEveTrackList::SetLineWidth(Width_t width, TEveElement* el)
{
   // Set line width for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && track->GetLineWidth() == fLineWidth)
         track->SetLineWidth(width);
      if (fRecurse)
         SetLineWidth(width, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetLineStyle(Style_t style)
{
   // Set line style for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetLineStyle() == fLineStyle)
         track->SetLineStyle(style);
      if (fRecurse)
         SetLineStyle(style, *i);
   }
   fLineStyle=style;
}

//______________________________________________________________________________
void TEveTrackList::SetLineStyle(Style_t style, TEveElement* el)
{
   // Set line style for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && track->GetLineStyle() == fLineStyle)
         track->SetLineStyle(style);
      if (fRecurse)
         SetLineStyle(style, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetMarkerStyle(Style_t style)
{
   // Set marker style for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetMarkerStyle() == fMarkerStyle)
         track->SetMarkerStyle(style);
      if (fRecurse)
         SetMarkerStyle(style, *i);
   }
   fMarkerStyle=style;
}

//______________________________________________________________________________
void TEveTrackList::SetMarkerStyle(Style_t style, TEveElement* el)
{
   // Set marker style for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && track->GetMarkerStyle() == fMarkerStyle)
         track->SetMarkerStyle(style);
      if (fRecurse)
         SetMarkerStyle(style, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetMarkerColor(Color_t col)
{
   // Set marker color for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetMarkerColor() == fMarkerColor)
         track->SetMarkerColor(col);
      if (fRecurse)
         SetMarkerColor(col, *i);
   }
   fMarkerColor=col;
}

//______________________________________________________________________________
void TEveTrackList::SetMarkerColor(Color_t col, TEveElement* el)
{
   // Set marker color for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && track->GetMarkerColor() == fMarkerColor)
         track->SetMarkerColor(col);
      if (fRecurse)
         SetMarkerColor(col, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SetMarkerSize(Size_t size)
{
   // Set marker size for the list and the elements.

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      TEveTrack* track = (TEveTrack*)(*i);
      if (track->GetMarkerSize() == fMarkerSize)
         track->SetMarkerSize(size);
      if (fRecurse)
         SetMarkerSize(size, *i);
   }
   fMarkerSize=size;
}

//______________________________________________________________________________
void TEveTrackList::SetMarkerSize(Size_t size, TEveElement* el)
{
   // Set marker size for children of el.

   TEveTrack* track;
   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      track = dynamic_cast<TEveTrack*>(*i);
      if (track && track->GetMarkerSize() == fMarkerSize)
         track->SetMarkerSize(size);
      if (fRecurse)
         SetMarkerSize(size, *i);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackList::SelectByPt(Float_t min_pt, Float_t max_pt)
{
   // Select visibility of tracks by transverse momentum.
   // If data-member fRecurse is set, the selection is applied
   // recursively to all children.

   fMinPt = min_pt;
   fMaxPt = max_pt;

   const Float_t minptsq = min_pt*min_pt;
   const Float_t maxptsq = max_pt*max_pt;

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      const Float_t ptsq = ((TEveTrack*)(*i))->fP.Perp2();
      Bool_t on = ptsq >= minptsq && ptsq <= maxptsq;
      (*i)->SetRnrState(on);
      if (on && fRecurse)
         SelectByPt(min_pt, max_pt, *i);
   }
}

//______________________________________________________________________________
void TEveTrackList::SelectByPt(Float_t min_pt, Float_t max_pt, TEveElement* el)
{
   // Select visibility of el's children tracks by transverse momentum.

   const Float_t minptsq = min_pt*min_pt;
   const Float_t maxptsq = max_pt*max_pt;

   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      TEveTrack* track = dynamic_cast<TEveTrack*>(*i);
      if (track)
      {
         const Float_t ptsq = track->fP.Perp2();
         Bool_t on = ptsq >= minptsq && ptsq <= maxptsq;
         track->SetRnrState(on);
         if (on && fRecurse)
            SelectByPt(min_pt, max_pt, *i);
      }
   }
}

//______________________________________________________________________________
void TEveTrackList::SelectByP(Float_t min_p, Float_t max_p)
{
   // Select visibility of tracks by momentum.
   // If data-member fRecurse is set, the selection is applied
   // recursively to all children.

   fMinP = min_p;
   fMaxP = max_p;

   const Float_t minpsq = min_p*min_p;
   const Float_t maxpsq = max_p*max_p;

   for (List_i i=BeginChildren(); i!=EndChildren(); ++i)
   {
      const Float_t psq  = ((TEveTrack*)(*i))->fP.Mag2();
      Bool_t on = psq >= minpsq && psq <= maxpsq;
      (*i)->SetRnrState(psq >= minpsq && psq <= maxpsq);
      if (on && fRecurse)
         SelectByP(min_p, max_p, *i);
   }
}

//______________________________________________________________________________
void TEveTrackList::SelectByP(Float_t min_p, Float_t max_p, TEveElement* el)
{
   // Select visibility of el's children tracks by momentum.

   const Float_t minpsq = min_p*min_p;
   const Float_t maxpsq = max_p*max_p;

   for (List_i i=el->BeginChildren(); i!=el->EndChildren(); ++i)
   {
      TEveTrack* track = dynamic_cast<TEveTrack*>(*i);
      if (track)
      {
         const Float_t psq  = ((TEveTrack*)(*i))->fP.Mag2();
         Bool_t on = psq >= minpsq && psq <= maxpsq;
         track->SetRnrState(on);
         if (on && fRecurse)
            SelectByP(min_p, max_p, *i);
      }
   }
}

/******************************************************************************/

//______________________________________________________________________________
TEveTrack* TEveTrackList::FindTrackByLabel(Int_t label)
{
   // Find track by label, select it and display it in the editor.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i) {
      if (((TEveTrack*)(*i))->GetLabel() == label) {
         TGListTree     *lt   = gEve->GetLTEFrame()->GetListTree();
         TGListTreeItem *mlti = lt->GetSelected();
         if (mlti->GetUserData() != this)
            mlti = FindListTreeItem(lt);
         TGListTreeItem *tlti = (*i)->FindListTreeItem(lt, mlti);
         lt->HighlightItem(tlti);
         lt->SetSelected(tlti);
         gEve->EditElement(*i);
         return (TEveTrack*) *i;
      }
   }
   return 0;
}

//______________________________________________________________________________
TEveTrack* TEveTrackList::FindTrackByIndex(Int_t index)
{
   // Find track by index, select it and display it in the editor.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i) {
      if (((TEveTrack*)(*i))->GetIndex() == index) {
         TGListTree     *lt   = gEve->GetLTEFrame()->GetListTree();
         TGListTreeItem *mlti = lt->GetSelected();
         if (mlti->GetUserData() != this)
            mlti = FindListTreeItem(lt);
         TGListTreeItem *tlti = (*i)->FindListTreeItem(lt, mlti);
         lt->HighlightItem(tlti);
         lt->SetSelected(tlti);
         gEve->EditElement(*i);
         return (TEveTrack*) *i;
      }
   }
   return 0;
}

//______________________________________________________________________________
void TEveTrackList::ImportHits()
{
   // Import hits for all track.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i) {
      ((TEveTrack*)(*i))->ImportHits();
   }
}

//______________________________________________________________________________
void TEveTrackList::ImportClusters()
{
   // Import clusters for all track.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i) {
      ((TEveTrack*)(*i))->ImportClusters();
   }
}

/******************************************************************************/

//______________________________________________________________________________
TClass* TEveTrackList::ProjectedClass() const
{
   // Virtual from TEveProjectable, returns TEveTrackListProjected class.

   return TEveTrackListProjected::Class();
}


//==============================================================================
//==============================================================================
// TEveTrackCounter
//==============================================================================

#include "TEveGedEditor.h"

//______________________________________________________________________________
//
// Provides event-based method for tagging of good / bad (or primary /
// secondary) tracks. A report can be written into a text file.
//
// TEveTrack status is toggled by using secondary-selection / ctrl-click
// functionality of the GL viewer.
//
// Some of the functionality is implemented in TEveTrackCounterEditor
// class.

ClassImp(TEveTrackCounter);

//______________________________________________________________________________
TEveTrackCounter* TEveTrackCounter::fgInstance = 0;

//______________________________________________________________________________
TEveTrackCounter::TEveTrackCounter(const Text_t* name, const Text_t* title) :
   TEveElement(),
   TNamed(name, title),

   fBadLineStyle (6),
   fClickAction  (kCA_ToggleTrack),
   fAllTracks    (0),
   fGoodTracks   (0),
   fTrackLists   ()
{
   // Constructor.
   // Connects to global signal "TEveTrack", "SecSelected(TEveTrack*)".

   if (fgInstance == 0) fgInstance = this;
   TQObject::Connect("TEveTrack", "SecSelected(TEveTrack*)",
                     "TEveTrackCounter", this, "DoTrackAction(TEveTrack*)");
}

//______________________________________________________________________________
TEveTrackCounter::~TEveTrackCounter()
{
   // Destructor.
   // Disconnect from the global track signals.

   TQObject::Disconnect("TEveTrack", "DoTrackAction(TEveTrack*)");
   if (fgInstance == this) fgInstance = 0;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackCounter::Reset()
{
   // Reset internal track-counters and track-list.

   printf("TEveTrackCounter::Reset()\n");
   fAllTracks  = 0;
   fGoodTracks = 0;
   TIter next(&fTrackLists);
   TEveTrackList* tlist;
   while ((tlist = dynamic_cast<TEveTrackList*>(next())))
      tlist->DecDenyDestroy();
   fTrackLists.Clear("nodelete");
}

//______________________________________________________________________________
void TEveTrackCounter::RegisterTracks(TEveTrackList* tlist, Bool_t goodTracks)
{
   // Register tracks from tlist and tlist itself.
   // If goodTracks is true, they are considered as primary/good
   // tracks.

   tlist->IncDenyDestroy();
   fTrackLists.Add(tlist);

   List_i i = tlist->BeginChildren();
   while (i != tlist->EndChildren())
   {
      TEveTrack* t = dynamic_cast<TEveTrack*>(*i);
      if (t != 0)
      {
         if (goodTracks)
         {
            ++fGoodTracks;
         } else {
            t->SetLineStyle(fBadLineStyle);
         }
         ++fAllTracks;
      }
      ++i;
   }
}

//______________________________________________________________________________
void TEveTrackCounter::DoTrackAction(TEveTrack* track)
{
   // Slot called when track is ctrl-clicked.
   //
   // No check is done if track actually belongs to one of the
   // registered track-lists.
   //
   // Probably it would be safer to copy good/bad tracks into special
   // sub-containers.
   // In this case one should also override RemoveElementLocal.

   static const TEveException eh("TEveTrackCounter::DoTrackAction ");

   switch (fClickAction)
   {

      case kCA_PrintTrackInfo:
      {
         printf("TEveTrack '%s'\n", track->GetObject(eh)->GetName());
         TEveVector &v = track->fV, &p = track->fP;
         printf("  Vx=%f, Vy=%f, Vz=%f; Pt=%f, Pz=%f, phi=%f)\n",
                v.fX, v.fY, v.fZ, p.Perp(), p.fZ, TMath::RadToDeg()*p.Phi());
         printf("  <other information should be printed ... full AliESDtrack>\n");
         break;
      }

      case kCA_ToggleTrack:
      {
         if (track->GetLineStyle() == 1)
         {
            track->SetLineStyle(fBadLineStyle);
            --fGoodTracks;
         } else {
            track->SetLineStyle(1);
            ++fGoodTracks;
         }
         track->ElementChanged();
         gEve->Redraw3D();

         printf("TEveTrackCounter::CountTrack All=%d, Good=%d, Bad=%d\n",
                fAllTracks, fGoodTracks, fAllTracks-fGoodTracks);

         if (gEve->GetEditor()->GetModel() == GetObject(eh))
            gEve->EditElement(this);

         break;
      }

   } // end switch fClickAction
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrackCounter::OutputEventTracks(FILE* out)
{
   // Print good-track summary into a plain-text file by iteration
   // through all registered track-lists.
   // State of each track is determined by its line-style, it is
   // considered a good track if it's line style is solid.

   if (out == 0)
   {
      out = stdout;
      fprintf(out, "TEveTrackCounter::FinalizeEvent()\n");
   }

   fprintf(out, "Event = %d  Ntracks = %d\n", fEventId, fGoodTracks);

   TIter tlists(&fTrackLists);
   TEveTrackList* tlist;
   Int_t cnt = 0;
   while ((tlist = (TEveTrackList*) tlists()) != 0)
   {
      List_i i = tlist->BeginChildren();
      while (i != tlist->EndChildren())
      {
         TEveTrack* t = dynamic_cast<TEveTrack*>(*i);
         if (t != 0 && t->GetLineStyle() == 1)
         {
            ++cnt;
            fprintf(out, " %2d: chg=%+2d  pt=%8.5f  eta=%+8.5f\n",
                    cnt, t->fCharge, t->fP.Perp(), t->fP.Eta());
         }
         ++i;
      }
   }
}
