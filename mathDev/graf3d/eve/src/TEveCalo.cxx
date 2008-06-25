// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveCalo.h"
#include "TEveCaloData.h"
#include "TEveProjections.h"
#include "TEveProjectionManager.h"
#include "TEveRGBAPalette.h"
#include "TEveText.h"

#include "TClass.h"
#include "TMathBase.h"
#include "TMath.h"
#include "TBuffer3D.h"
#include "TBuffer3DTypes.h"
#include "TVirtualPad.h"
#include "TVirtualViewer3D.h"
#include "TAxis.h"

#include "TGLUtil.h"


//==============================================================================
// TEveCaloViz
//==============================================================================

//______________________________________________________________________________
//
// Base class for calorimeter data visualization.
// See TEveCalo2D and TEveCalo3D for concrete implementations.

ClassImp(TEveCaloViz);

//______________________________________________________________________________
TEveCaloViz::TEveCaloViz(const Text_t* n, const Text_t* t) :
   TEveElement(),
   TNamed(n, t),
   TEveProjectable(),

   fData(0),

   fEtaMin(-10),
   fEtaMax(10),

   fPhi(0.),
   fPhiOffset(TMath::Pi()),

   fBarrelRadius(-1.f),
   fEndCapPos(-1.f),

   fPlotEt(kTRUE),

   fMaxTowerH(100),
   fScaleAbs(kFALSE),
   fMaxValAbs(100),

   fValueIsColor(kTRUE),
   fPalette(0),

   fCellIdCacheOK(kFALSE)
{
   // Constructor.

   SetElementName("TEveCaloViz");
}

//______________________________________________________________________________
TEveCaloViz::TEveCaloViz(TEveCaloData* data, const Text_t* n, const Text_t* t) :
   TEveElement(),
   TNamed(n, t),

   fData(0),

   fEtaMin(-10),
   fEtaMax(10),

   fPhi(0.),
   fPhiOffset(TMath::Pi()),

   fBarrelRadius(-1.f),
   fEndCapPos(-1.f),

   fPlotEt(kTRUE),

   fMaxTowerH(100),
   fScaleAbs(kFALSE),
   fMaxValAbs(100),

   fValueIsColor(kTRUE),
   fPalette(0),

   fCellIdCacheOK(kFALSE)
{
   // Constructor.

   SetElementName("TEveCaloViz");
   SetData(data);
}

//______________________________________________________________________________
TEveCaloViz::~TEveCaloViz()
{
   // Destructor.

   if (fPalette) fPalette->DecRefCount();
   if (fData) fData->DecRefCount();
}

//______________________________________________________________________________
Float_t TEveCaloViz::GetDataSliceThreshold(Int_t slice) const
{
   // Get threshold for given slice.

   return fData->RefSliceInfo(slice).fThreshold;
}

//______________________________________________________________________________
void TEveCaloViz::SetDataSliceThreshold(Int_t slice, Float_t val)
{
   // Set threshold for given slice.

   fData->SetSliceThreshold(slice, val);
}

//______________________________________________________________________________
Color_t TEveCaloViz::GetDataSliceColor(Int_t slice) const
{
   // Get slice color from data.

   return fData->RefSliceInfo(slice).fColor;
}

//______________________________________________________________________________
void TEveCaloViz::SetDataSliceColor(Int_t slice, Color_t col)
{
   // Set slice color in data.
  
   fData->SetSliceColor(slice, col);
}

//______________________________________________________________________________
void TEveCaloViz::SetEta(Float_t l, Float_t u)
{
   // Set eta range.

   fEtaMin=l;
   fEtaMax=u;

   if(fData && fData->GetEtaBins())
         fData->GetEtaBins()->SetRangeUser(l, u);

   InvalidateCellIdCache();
}

//______________________________________________________________________________
void TEveCaloViz::SetPlotEt(Bool_t isEt)
{
   // Set E/Et plot.

  fPlotEt=isEt;
  fPalette->SetLimits(0, TMath::CeilNint(fData->GetMaxVal(fPlotEt)));

  InvalidateCellIdCache();
}

//______________________________________________________________________________
Float_t TEveCaloViz::GetMaxVal() const
{

   // Get maximum plotted value.

   if (fScaleAbs)
      return fMaxValAbs;
   else
      return fData->GetMaxVal(fPlotEt);

}

//______________________________________________________________________________
void TEveCaloViz::SetPhiWithRng(Float_t phi, Float_t rng)
{
   // Set phi range.

   using namespace TMath;

   fPhi = phi;
   fPhiOffset = rng;

   InvalidateCellIdCache();
}

//______________________________________________________________________________
Float_t TEveCaloViz::GetTransitionTheta() const
{
   // Get transition angle between barrel and end-cap cells.

   return TMath::ATan(fBarrelRadius/fEndCapPos);
}

//______________________________________________________________________________
Float_t TEveCaloViz::GetTransitionEta() const
{
   // Get transition eta between barrel and end-cap cells.

   using namespace TMath;
   Float_t t = GetTransitionTheta()*0.5f;
   return -Log(Tan(t));
}

//______________________________________________________________________________
void TEveCaloViz::SetData(TEveCaloData* data)
{
   // Set calorimeter event data.

   if (data == fData) return;
   if (fData) fData->DecRefCount(this);
   fData = data;
   if (fData) fData->IncRefCount(this);

   DataChanged();
}

//______________________________________________________________________________
void TEveCaloViz::DataChanged()
{
   // Update setting and cache on data changed.
   // Called from TEvecaloData::BroadcastDataChange()

   Double_t min, max, delta;

   fData->GetEtaLimits(min, max);
   if (fEtaMin < min) fEtaMin = min;
   if (fEtaMax > max) fEtaMax = max;

   fData->GetPhiLimits(min, max);
   delta = 0.5*(max - min);
   if (fPhi < min || fPhi > max) {
      fPhi       = 0.5*(max + min);
      fPhiOffset = delta;
   } else {
      if (fPhiOffset > delta) fPhiOffset = delta;
   }

   if (fPalette)
   {
      Int_t hlimit = TMath::CeilNint(fScaleAbs ? fMaxValAbs : fData->GetMaxVal(fPlotEt));
      fPalette->SetLimits(0, hlimit);
      fPalette->SetMin(0);
      fPalette->SetMax(hlimit);
   }

   InvalidateCellIdCache();
}

//______________________________________________________________________________
void TEveCaloViz::AssignCaloVizParameters(TEveCaloViz* m)
{
   // Assign paramteres from given model.

   SetData(m->fData);

   fEtaMin    = m->fEtaMin;
   fEtaMax    = m->fEtaMax;

   fPhi       = m->fPhi;
   fPhiOffset = m->fPhiOffset;

   fBarrelRadius = m->fBarrelRadius;
   fEndCapPos    = m->fEndCapPos;

   if (m->fPalette)
   {
      TEveRGBAPalette& mp = * m->fPalette;
      TEveRGBAPalette* p = new TEveRGBAPalette(mp.GetMinVal(), mp.GetMaxVal(), mp.GetInterpolate());
      p->SetDefaultColor(mp.GetDefaultColor());
   }
}

//______________________________________________________________________________
void TEveCaloViz::SetPalette(TEveRGBAPalette* p)
{
   // Set TEveRGBAPalette object pointer.

   if ( fPalette == p) return;
   if (fPalette) fPalette->DecRefCount();
   fPalette = p;
   if (fPalette) fPalette->IncRefCount();
}

//______________________________________________________________________________
Float_t TEveCaloViz::GetValToHeight() const
{
   // Get transformation factor from E/Et to height

   if (fScaleAbs)
   {
      return fMaxTowerH/fMaxValAbs;
   }
   else
   {
      return fMaxTowerH/fData->GetMaxVal(fPlotEt);
   }
}

//______________________________________________________________________________
TEveRGBAPalette* TEveCaloViz::AssertPalette()
{
   // Make sure the TEveRGBAPalette pointer is not null.
   // If it is not set, a new one is instantiated and the range is set
   // to current min/max signal values.

   if (fPalette == 0) {
      fPalette = new TEveRGBAPalette;
      fPalette->SetDefaultColor((Color_t)4); 

      Int_t hlimit = TMath::CeilNint(fScaleAbs ? fMaxValAbs : fData->GetMaxVal(fPlotEt));
      fPalette->SetLimits(0, hlimit);
      fPalette->SetMin(0);
      fPalette->SetMax(hlimit);

   }
   return fPalette;
}

//______________________________________________________________________________
void TEveCaloViz::Paint(Option_t* /*option*/)
{
   // Paint this object. Only direct rendering is supported.

   static const TEveException eH("TEvecaloViz::Paint ");

   if (!fData)
      return;

   TBuffer3D buff(TBuffer3DTypes::kGeneric);

   // Section kCore
   buff.fID           = this;
   buff.fTransparency = 0;
   if (HasMainTrans())
      RefMainTrans().SetBuffer3D(buff);
   buff.SetSectionsValid(TBuffer3D::kCore);

   Int_t reqSections = gPad->GetViewer3D()->AddObject(buff);
   if (reqSections != TBuffer3D::kNone)
      Error(eH, "only direct GL rendering supported.");
}

//______________________________________________________________________________
TClass* TEveCaloViz::ProjectedClass() const
{
   // Virtual from TEveProjectable, returns TEveCalo2D class.

   return TEveCalo2D::Class();
}

//______________________________________________________________________________
void TEveCaloViz::SetupColorHeight(Float_t value, Int_t slice, Float_t& outH) const
{
   // Set color and height for a given value and slice using slice color or TEveRGBAPalette.

   if (fValueIsColor)
   {
      outH = GetValToHeight()*fData->GetMaxVal(fPlotEt);
      UChar_t c[4];
      fPalette->ColorFromValue((Int_t)value, c);
      TGLUtil::Color4ubv(c);
   }
   else 
   {
      TGLUtil::Color(fData->RefSliceInfo(slice).fColor);
      outH = GetValToHeight()*value;
   }
}


//==============================================================================
// TEveCalo3D
//==============================================================================

//______________________________________________________________________________
//
// Visualization of a calorimeter event data in 3D.

ClassImp(TEveCalo3D);

//______________________________________________________________________________
void TEveCalo3D::BuildCellIdCache()
{
   // Build list of drawn cell IDs. See TEveCalo3DGL::DirectDraw().

   fCellList.clear();

   fData->GetCellList(GetEta(), GetEtaRng(), GetPhi(), GetPhiRng(), fCellList);
   fCellIdCacheOK = kTRUE;
}

//______________________________________________________________________________
void TEveCalo3D::ComputeBBox()
{
   // Fill bounding-box information of the base-class TAttBBox (virtual method).
   // If member 'TEveFrameBox* fFrame' is set, frame's corners are used as bbox.

   BBoxInit();

   Float_t th = GetValToHeight() * fData->GetMaxVal(fPlotEt);

   fBBox[0] = -fBarrelRadius - th;
   fBBox[1] =  fBarrelRadius + th;
   fBBox[2] =  fBBox[0];
   fBBox[3] =  fBBox[1];
   fBBox[4] = -fEndCapPos - th;
   fBBox[5] =  fEndCapPos + th;
}


//==============================================================================
// TEveCalo2D
//==============================================================================

//______________________________________________________________________________
//
// Visualization of a calorimeter event data in 2D.

ClassImp(TEveCalo2D);

//______________________________________________________________________________
TEveCalo2D::TEveCalo2D(const Text_t* n, const Text_t* t):
   TEveCaloViz(n, t),
   TEveProjected(),
   fOldProjectionType(TEveProjection::kPT_Unknown)
{
   // Constructor.
}

//______________________________________________________________________________
void TEveCalo2D::UpdateProjection()
{
   // This is virtual method from base-class TEveProjected.

   if (fManager->GetProjection()->GetType() != fOldProjectionType)
   {
      fCellIdCacheOK=kFALSE;
      fOldProjectionType = fManager->GetProjection()->GetType();
   }
   ComputeBBox();
}

//______________________________________________________________________________
void TEveCalo2D::SetProjection(TEveProjectionManager* mng, TEveProjectable* model)
{
   // Set projection manager and model object.

   TEveProjected::SetProjection(mng, model);
   TEveCaloViz* viz = dynamic_cast<TEveCaloViz*>(model);
   AssignCaloVizParameters(viz);
}

//______________________________________________________________________________
void TEveCalo2D::BuildCellIdCache()
{
   // Build lists of drawn cell IDs. See TEveCalo2DGL::DirecDraw().

   // clear old cache 
   for (std::vector<TEveCaloData::vCellId_t*>::iterator it = fCellLists.begin(); it != fCellLists.end(); it++)
      delete *it;

   fCellLists.clear();


   TEveProjection::EPType_e pt = fManager->GetProjection()->GetType();
   TEveCaloData::vCellId_t*  clv; // ids per phi bin in r-phi projection else ids per eta bins in rho-z projection  
   if (pt == TEveProjection::kPT_RhoZ)
   {
      // build list on basis of phi bins
      const TAxis* ax = fData->GetEtaBins();
      Int_t nBins = ax->GetNbins();
      for (Int_t ibin = 1; ibin <= nBins; ++ibin)
      {
         if (ax->GetBinLowEdge(ibin) > fEtaMin && ax->GetBinUpEdge(ibin) <= fEtaMax)
         {
            clv = new TEveCaloData::vCellId_t();
            fData->GetCellList(ax->GetBinCenter(ibin), ax->GetBinWidth(ibin)+1e-5, fPhi, GetPhiRng(), *clv);
           
            if (clv->size()) 
               fCellLists.push_back(clv);
            else 
               delete clv;
         }
      }
   }
   else if (pt == TEveProjection::kPT_RPhi)
   {
      // build list on basis of phi bins
      const TAxis* ay = fData->GetPhiBins();
      Int_t nBins = ay->GetNbins();
      for (Int_t ibin = 1; ibin <= nBins; ++ibin)
      {
         if (TEveUtil::IsU1IntervalOverlappingByMinMax
             (GetPhiMin(), GetPhiMax(), ay->GetBinLowEdge(ibin), ay->GetBinUpEdge(ibin)))
         {
            clv = new TEveCaloData::vCellId_t();
            fData->GetCellList(GetEta(), GetEtaRng(), ay->GetBinCenter(ibin), ay->GetBinWidth(ibin),*clv);

            if (clv->size()) 
               fCellLists.push_back(clv);
            else
               delete clv;
         }
      }
   }

   fCellIdCacheOK= kTRUE;
}

//______________________________________________________________________________
void TEveCalo2D::ComputeBBox()
{
   // Fill bounding-box information of the base-class TAttBBox (virtual method).
   // If member 'TEveFrameBox* fFrame' is set, frame's corners are used as bbox.

   BBoxZero();

   Float_t x, y, z;
   Float_t th = GetValToHeight()*fData->GetMaxVal(fPlotEt);
   Float_t r  = fBarrelRadius + th;
   Float_t ze = fEndCapPos + th;

   x = r, y = 0, z = 0;
   fManager->GetProjection()->ProjectPoint(x, y, z);
   BBoxCheckPoint(x, y, z);

   x = 0, y = 0, z = 0;
   fManager->GetProjection()->ProjectPoint(x, y, z);
   BBoxCheckPoint(x, y, z);

   x = 0, y = 0, z = ze;
   fManager->GetProjection()->ProjectPoint(x, y, z);
   BBoxCheckPoint(x, y, z);

   x = 0, y = 0, z = -ze;
   fManager->GetProjection()->ProjectPoint(x, y, z);
   BBoxCheckPoint(x, y, z);

   x = 0, y = r, z = 0;
   fManager->GetProjection()->ProjectPoint(x, y, z);
   BBoxCheckPoint(x, y, z);

   x = 0, y = -r, z = 0;
   fManager->GetProjection()->ProjectPoint(x, y, z);
   BBoxCheckPoint(x, y, z);

   AssertBBoxExtents(0.1);
}


//==============================================================================
// TEveCaloLego
//==============================================================================

//______________________________________________________________________________
//
// Visualization of calorimeter data as eta/phi histogram.

ClassImp(TEveCaloLego);

//______________________________________________________________________________
TEveCaloLego::TEveCaloLego(const Text_t* n, const Text_t* t):
   TEveCaloViz(n, t),

   fFontColor(0),
   fGridColor(kGray+2),
   fPlaneColor(kRed-5),
   fPlaneTransparency(60),

   fNZSteps(6),
   fZAxisStep(0.f),

   fBinWidth(4),

   fProjection(kAuto),
   f2DMode(kValColor),

   fDrawHPlane(kFALSE),
   fHPlaneVal(0)
{
   // Constructor.

   fMaxTowerH = 1;
   SetElementNameTitle("TEveCaloLego", "TEveCaloLego");
}

//______________________________________________________________________________
TEveCaloLego::TEveCaloLego(TEveCaloData* data):
   TEveCaloViz(),

   fFontColor(0),
   fGridColor(kGray+2),
   fPlaneColor(kRed-5),
   fPlaneTransparency(60),

   fNZSteps(6),
   fZAxisStep(0.f),

   fBinWidth(4),

   fProjection(kAuto),
   f2DMode(kValColor),

   fBoxMode(kBack),

   fDrawHPlane(kFALSE),
   fHPlaneVal(0)
{
   // Constructor.

   fMaxTowerH = 1;
   SetElementNameTitle("TEveCaloLego", "TEveCaloLego");
   SetData(data);
}

//______________________________________________________________________________
void TEveCaloLego::BuildCellIdCache()
{
   // Build list of drawn cell IDs. For more information see TEveCaloLegoGL:DirectDraw().

   fCellList.clear();

   fData->GetCellList(GetEta(), GetEtaRng(), GetPhi(), GetPhiRng(), fCellList);
   fCellIdCacheOK = kTRUE;
}

//______________________________________________________________________________
void TEveCaloLego::ComputeBBox()
{
   // Fill bounding-box information of the base-class TAttBBox (virtual method).
   // If member 'TEveFrameBox* fFrame' is set, frame's corners are used as bbox.

   BBoxInit();

   // Float_t[6] X(min,max), Y(min,max), Z(min,max)

   if (fData)
   {
      Float_t ex = 1.2;

      Float_t a = 0.5*ex;

      fBBox[0] = -a; 
      fBBox[1] =  a;
      fBBox[2] = -a; 
      fBBox[3] =  a;

      // scaling is relative to shortest XY axis
      Double_t em, eM, pm, pM;
      fData->GetEtaLimits(em, eM);
      fData->GetPhiLimits(pm, pM);
      Double_t r = (eM-em)/(pM-pm);
      if (r<1)
      {
         fBBox[2] /= r;
         fBBox[3] /= r;
      }
      else 
      {
         fBBox[0] *= r;
         fBBox[1] *= r; 
      }

      fBBox[4] =  fMaxTowerH*(1-ex);
      fBBox[5] =  fMaxTowerH*ex;
   } 
}
