// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveCalo2DGL.h"
#include "TEveCalo.h"
#include "TEveProjections.h"
#include "TEveProjectionManager.h"
#include "TEveRGBAPalette.h"

#include "TGLRnrCtx.h"
#include "TGLSelectRecord.h"
#include "TGLIncludes.h"
#include "TGLUtil.h"
#include "TAxis.h"

using namespace TMath;

//______________________________________________________________________________
// OpenGL renderer class for TEveCalo2D.
//

ClassImp(TEveCalo2DGL);

//______________________________________________________________________________
TEveCalo2DGL::TEveCalo2DGL() :
   TGLObject(),
   fM(0)
{
   // Constructor.

   // fDLCache = kFALSE; // Disable display list.
}

/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveCalo2DGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   // Set model object.

   if (SetModelCheckClass(obj, TEveCalo2D::Class())) {
      fM = dynamic_cast<TEveCalo2D*>(obj);
      return kTRUE;
   }
   return kFALSE;
}

//______________________________________________________________________________
void TEveCalo2DGL::SetBBox()
{
   // Set bounding box.

   // !! This ok if master sub-classed from TAttBBox
   SetAxisAlignedBBox(((TEveCalo2D*)fExternalObj)->AssertBBox());
}

/******************************************************************************/

//______________________________________________________________________________
Float_t TEveCalo2DGL::MakeRPhiCell(Float_t phiMin, Float_t phiMax, Float_t towerH, Float_t offset) const
{
   // Float_t towerH =  fM->fBarrelRadius*fM->fTowerHeight*(value -fMinVal)/(fMaxVal-fMinVal);
   Float_t r1 = fM->fBarrelRadius + offset;
   Float_t r2 = r1 + towerH;

   Float_t pnts[8];
   Float_t *p = pnts;

   p[0] = r1*Cos(phiMin);
   p[1] = r1*Sin(phiMin);
   p +=2;
   p[0] = r2*Cos(phiMin);
   p[1] = r2*Sin(phiMin);
   p +=2;
   p[0] = r2*Cos(phiMax);
   p[1] = r2*Sin(phiMax);
   p +=2;
   p[0] = r1*Cos(phiMax);
   p[1] = r1*Sin(phiMax);

   Float_t x, y, z;
   for (Int_t i=0; i<4; i++){
      x = pnts[2*i];
      y = pnts[2*i+1];
      z = 0.f;
      fM->fManager->GetProjection()->ProjectPoint(x, y, z);
      glVertex3f(x, y, fM->fDepth);
   }
   return offset+towerH;
}

//______________________________________________________________________________
void TEveCalo2DGL::DrawRPhi(TGLRnrCtx & rnrCtx) const
{
   TEveCaloData* data = fM->GetData();
   const TAxis* ax = data->GetPhiBins();

   if (fM->fCacheOK == kFALSE) {
      fM->ResetCache();
      Float_t eta = (fM->fEtaMax+fM->fEtaMin)*0.5f;
      Float_t etaRng = fM->fEtaMax-fM->fEtaMin;
      Float_t pr[4];
      // calculate the two intervals when circle is cut
      Float_t phi1 = fM->fPhi - fM->fPhiRng;
      Float_t phi2 = fM->fPhi + fM->fPhiRng;
      if (phi2 >TMath::Pi() && phi1<-Pi()) {
         pr[0] =  phi1;
         pr[1] =  Pi();
         pr[2] =  -Pi();
         pr[3] =  -TwoPi()+phi2;
      }
      else if (phi1<-TMath::Pi() && phi2<=Pi()) {
         pr[0] = -Pi();
         pr[1] =  phi2;
         pr[2] =  TwoPi()+phi1;
         pr[3] =  Pi();
      } else {
         pr[0] = pr[2] = phi1;
         pr[1] = pr[3] = phi2;
      }

      Int_t nBins = ax->GetNbins();
      for (Int_t ibin=0; ibin<nBins; ibin++) {
         if ( (   ax->GetBinLowEdge(ibin)>=pr[0] && ax->GetBinUpEdge(ibin)<pr[1])
              || (ax->GetBinLowEdge(ibin)>=pr[2] && ax->GetBinUpEdge(ibin)<pr[3])) 
         {
            TEveCaloData::vCellId_t* clv = new TEveCaloData::vCellId_t();
            Int_t nc = data->GetCellList(eta, etaRng, ax->GetBinCenter(ibin), ax->GetBinWidth(ibin),
                                         fM->fThreshold, *clv);
            if (nc) 
               fM->fCellLists.push_back(clv);
            else 
               delete clv;
         }
      }
      fM->fCacheOK= kTRUE;
   }

   Int_t nSlices = data->GetNSlices();
   Float_t *sliceVal = new Float_t[nSlices];
   TEveCaloData::CellData_t cellData;
   Float_t towerH;
   Bool_t visible;
   if (rnrCtx.SecSelection()) glPushName(0);
   for(UInt_t vi=0; vi<fM->fCellLists.size(); vi++) {
      // reset values
      Float_t off = 0;
      for (Int_t s=0; s<nSlices; s++)
         sliceVal[s]=0;
      // loop through eta bins
      TEveCaloData::vCellId_t* cids = fM->fCellLists[vi];
      for (TEveCaloData::vCellId_i it = cids->begin(); it != cids->end(); it++) {
         data->GetCellData(*it, cellData);
         sliceVal[(*it).fSlice] += cellData.Value();
      }
      // draw
      if (rnrCtx.SecSelection()) {
         glLoadName(vi);
         glPushName(0);
      }
      for (Int_t s=0; s<nSlices; s++) {
         fM->SetupColorHeight(sliceVal[s], s, towerH, visible);
         if (visible)
         {
            if (rnrCtx.SecSelection()) glLoadName(s);
            glBegin(GL_QUADS);
            off = MakeRPhiCell(cellData.PhiMin(), cellData.PhiMax(), towerH, off);
            glEnd();
         }
      }
      if (rnrCtx.SecSelection()) glPopName(); // slice
   } 
   if (rnrCtx.SecSelection()) glPopName(); // etha bin
}


/*******************************************************************************/
/*******************************************************************************/
//______________________________________________________________________________
Float_t TEveCalo2DGL::MakeRhoZBarrelCell(Float_t thetaMin, Float_t thetaMax, Bool_t phiPlus, Float_t towerH, Float_t offset) const
{
   //   Float_t towerH = fM->fBarrelRadius*fM->fTowerHeight*(value -fMinVal)/(fMaxVal-fMinVal);

   Float_t theta  = (thetaMin+thetaMax)*0.5;
   Float_t r1 = fM->fBarrelRadius/TMath::Abs(Sin(theta)) + offset;
   Float_t r2 = r1 + towerH;

   Float_t pnts[12];
   Float_t *p = pnts;

   p[0] = 0.f;
   p[1] = r1*Sin(thetaMin);
   p[2] = r1*Cos(thetaMin);
   p +=3;

   p[0] = 0.f;
   p[1] = r2*Sin(thetaMin);
   p[2] = r2*Cos(thetaMin);
   p +=3;

   p[0] = 0.f;
   p[1] = r2*Sin(thetaMax);
   p[2] = r2*Cos(thetaMax);

   p +=3;
   p[0] = 0.f;
   p[1] = r1*Sin(thetaMax);
   p[2] = r1*Cos(thetaMax);

   Float_t x, y, z;
   for (Int_t i=0; i<4; i++) {
      x = 0.f;
      y = phiPlus ? pnts[3*i+1] : -pnts[3*i+1];
      z = pnts[3*i+2];
      //      printf()
      fM->fManager->GetProjection()->ProjectPoint(x, y, z);
      glVertex3f(x, y, fM->fDepth);
   }
   return offset+towerH;
}

//______________________________________________________________________________
Float_t TEveCalo2DGL::MakeRhoZEndCapCell(Float_t thetaMin, Float_t thetaMax, Bool_t phiPlus, Float_t towerH, Float_t offset) const
{
   //   Float_t towerH =  fM->fEndCapPos*fM->fTowerHeight*(value - fMinVal)/(fMaxVal-fMinVal);
   Float_t theta  = (thetaMin+thetaMax)*0.5;
   Float_t r1 =Abs( fM->GetEndCapPos()/Cos(theta)) + offset;
   Float_t r2 = r1 + towerH;
   Float_t pnts[12];
   Float_t *p = pnts;

   p[0] = 0.f;
   p[1] = r1*Sin(thetaMin);
   p[2] = r1*Cos(thetaMin);
   p +=3;

   p[0] = 0.f;
   p[1] = r2*Sin(thetaMin);
   p[2] = r2*Cos(thetaMin);
   p +=3;

   p[0] = 0.f;
   p[1] = r2*Sin(thetaMax);
   p[2] = r2*Cos(thetaMax);

   p +=3;
   p[0] = 0.f;
   p[1] = r1*Sin(thetaMax);
   p[2] = r1*Cos(thetaMax);

   Float_t x, y, z;
   for (Int_t i=0; i<4; i++) {
      x = pnts[3*i];
      y = phiPlus ? pnts[3*i+1] : -pnts[3*i+1];
      z = pnts[3*i+2];
      fM->fManager->GetProjection()->ProjectPoint(x, y, z);
      glVertex3f(x, y, fM->fDepth);
   }
   return offset+towerH;
}

//______________________________________________________________________________
Float_t TEveCalo2DGL::MakeRhoZCell(Float_t thetaMin, Float_t thetaMax, Bool_t phiPlus, Float_t towerH, Float_t offset) const
{
   Float_t off;
   glBegin(GL_QUADS);
   {
      if (thetaMin>fM->GetTransitionTheta() && thetaMax<(TMath::Pi() - fM->GetTransitionTheta()))
         off = MakeRhoZBarrelCell(thetaMin, thetaMax, phiPlus, towerH, offset);
      else 
         off = MakeRhoZEndCapCell(thetaMin, thetaMax, phiPlus, towerH, offset);
   }
   glEnd();
   return off;
}

//______________________________________________________________________________
void TEveCalo2DGL::DrawRhoZ(TGLRnrCtx & rnrCtx) const
{
   TEveCaloData* data = fM->GetData();

   if (fM->fCacheOK == kFALSE) 
   {
      fM->ResetCache();
      const TAxis* ax = data->GetEtaBins();
      Int_t nBins = ax->GetNbins();
      for (Int_t ibin=0; ibin<nBins; ibin++){
         if (ax->GetBinLowEdge(ibin)<=fM->fEtaMin || ax->GetBinUpEdge(ibin)>fM->fEtaMax)
            continue;
         TEveCaloData::vCellId_t* aa = new TEveCaloData::vCellId_t();
         TEveCaloData::vCellId_t& aaref = * aa;
         Int_t nc = data->GetCellList(ax->GetBinCenter(ibin), ax->GetBinWidth(ibin),
                                      fM->fPhi, fM->fPhiRng, fM->fThreshold, aaref);
         if (nc) fM->fCellLists.push_back(aa);
      }
      fM->fCacheOK= kTRUE;
   }

   TEveCaloData::CellData_t cellData;
   Float_t towerH;
   Bool_t visible;
   Int_t nSlices = data->GetNSlices();
   Float_t *sliceValsUp  = new Float_t[nSlices];
   Float_t *sliceValsLow = new Float_t[nSlices];


   if (rnrCtx.SecSelection()) glPushName(0);
   for (UInt_t vi=0; vi<fM->fCellLists.size(); vi++) {
      // clear
      Float_t offUp  = 0;
      Float_t offLow = 0;
      for (Int_t s=0; s<nSlices; s++) {
         sliceValsUp[s]  = 0;
         sliceValsLow[s] = 0;
      }

      // RhoZ values
      for (TEveCaloData::vCellId_i it = fM->fCellLists[vi]->begin(); it !=  fM->fCellLists[vi]->end(); it++) {
         data->GetCellData(*it, cellData);
         if (cellData.Phi() > 0)
            sliceValsUp[(*it).fSlice]  += cellData.Value();
         else
            sliceValsLow[(*it).fSlice] += cellData.Value();
      }

      // draw
      if (rnrCtx.SecSelection())
      {
         glLoadName(vi); // phi bin
         glPushName(0); // slice
      }

      for (Int_t s=0; s<nSlices; s++) {
         if (rnrCtx.SecSelection())glLoadName(s); 
         // render phi positive slices
         fM->SetupColorHeight(sliceValsUp[s], s, towerH, visible);
         if (visible)
         {
            if (rnrCtx.SecSelection()) glPushName(kTRUE); // phi plus true    
            offUp = MakeRhoZCell(cellData.ThetaMin(kTRUE), cellData.ThetaMax(kTRUE), kTRUE , towerH, offUp);
            if (rnrCtx.SecSelection()) glPopName();
         }
         
         // render phi negative slices
         fM->SetupColorHeight(sliceValsLow[s], s, towerH, visible);
         if (visible)
         {
            if (rnrCtx.SecSelection()) glPushName(kFALSE); // phi plus false    
            offLow = MakeRhoZCell(cellData.ThetaMin(kTRUE), cellData.ThetaMax(kTRUE), kFALSE, towerH, offLow);
            if (rnrCtx.SecSelection()) glPopName();
         }
      }
      if (rnrCtx.SecSelection()) glPopName(); // slice
   }
   if (rnrCtx.SecSelection()) glPopName(); // phi bin
   delete [] sliceValsUp;
   delete [] sliceValsLow;
}

//______________________________________________________________________________
void TEveCalo2DGL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   // Render with OpenGL.
   // printf("TEveCalo2DGL::DirectDraw \n");

   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_POLYGON_BIT);
   glDisable(GL_LIGHTING);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glDisable(GL_CULL_FACE);
   glPointSize(3);

   fM->AssertPalette();

   TEveProjection::EPType_e pt = fM->fManager->GetProjection()->GetType();
   if (pt == TEveProjection::kPT_RhoZ) 
      DrawRhoZ(rnrCtx);
   else if (pt == TEveProjection::kPT_RPhi)
      DrawRPhi(rnrCtx);

   glPopAttrib();
}

//______________________________________________________________________________
void TEveCalo2DGL::ProcessSelection(TGLRnrCtx & /*rnrCtx*/, TGLSelectRecord & rec)
{
   // Processes secondary selection from TGLViewer.

   if (rec.GetN() < 2) return;

   Int_t id = rec.GetItem(1);
   Int_t slice = rec.GetItem(2);
   TEveCaloData::CellData_t cellData;

   Int_t n = 0;
   for (TEveCaloData::vCellId_i it =fM->fCellLists[id]->begin(); it!=fM->fCellLists[id]->end(); it++) 
   {
      if ((*it).fSlice == slice)
         n++;
   }

   printf("Tower selected in slice %d number of hits: %2d \n", slice, n);
   for (TEveCaloData::vCellId_i it =fM->fCellLists[id]->begin(); it!=fM->fCellLists[id]->end(); it++) 
   {
      if ((*it).fSlice == slice)
      {
         fM->fData->GetCellData(*it, cellData);
         cellData.Dump();
         
      }
   }

   // rho Z
   if (rec.GetN() == 4)
   {
      if(rec.GetItem(3))
      printf("Cell in selected positive phi half \n");
      else 
      printf("Cell in selected negative phi half \n");

      for (TEveCaloData::vCellId_i it =fM->fCellLists[id]->begin(); it!=fM->fCellLists[id]->end(); it++) 
      {
         fM->fData->GetCellData(*it, cellData);
         if ((*it).fSlice == slice) {
            if ( (rec.GetItem(3) && cellData.Phi()> 0)
                 || (rec.GetItem(3)== kFALSE && cellData.Phi()<0) )
               cellData.Dump();
         }
      }
   }
}
