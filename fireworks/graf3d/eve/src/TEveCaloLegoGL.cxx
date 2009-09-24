// @(#)root/eve:$Id$
// Author: Alja Mrak-Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TAxis.h"
#include "TH2.h"
#include "THLimitsFinder.h"

#include "TGLViewer.h"
#include "TGLIncludes.h"
#include "TGLRnrCtx.h"
#include "TGLSelectRecord.h"
#include "TGLScene.h"
#include "TGLCamera.h"
#include "TGLUtil.h"
#include "TColor.h"


#include "TEveCaloLegoGL.h"
#include "TEveCalo.h"
#include "TEveRGBAPalette.h"

#include <algorithm>

//______________________________________________________________________________
// OpenGL renderer class for TEveCaloLego.
//

ClassImp(TEveCaloLegoGL);

//______________________________________________________________________________
TEveCaloLegoGL::TEveCaloLegoGL() :
      TGLObject(),

      fDataMax(0),
      fGridColor(-1),
      fFontColor(-1),

      fEtaAxis(0),
      fPhiAxis(0),
      fZAxis(0),
      fM(0),
      fDLCacheOK(kFALSE),
      fCells3D(kTRUE)
{
   // Constructor.

   fDLCache = kFALSE;

   // need to set dummy parent, else loose settings in rebin

   fEtaAxis = new TAxis();
   fPhiAxis = new TAxis();
   fZAxis = new TAxis();

    fAxisPainter.SetFontMode(TGLFont::kPixmap);
}

//______________________________________________________________________________
TEveCaloLegoGL::~TEveCaloLegoGL()
{
   // Destructor.

   DLCachePurge();

   delete fEtaAxis;
   delete fPhiAxis;
   delete fZAxis;
}

//______________________________________________________________________________
Bool_t TEveCaloLegoGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   // Set model object.

   if (SetModelCheckClass(obj, TEveCaloLego::Class())) {
      fM = dynamic_cast<TEveCaloLego*>(obj);
      return kTRUE;
   }

   return kFALSE;
}

//______________________________________________________________________________
void TEveCaloLegoGL::SetBBox()
{
   // Set bounding box.

   SetAxisAlignedBBox(((TEveCaloLego*)fExternalObj)->AssertBBox());
}

//______________________________________________________________________________
void TEveCaloLegoGL::DLCacheDrop()
{
   // Drop all display-list definitions.

   fDLCacheOK = kFALSE;
   for (SliceDLMap_i i = fDLMap.begin(); i != fDLMap.end(); ++i)
      i->second = 0;

   TGLObject::DLCacheDrop();
}

//______________________________________________________________________________
void TEveCaloLegoGL::DLCachePurge()
{
   // Unregister all display-lists.

   fDLCacheOK = kFALSE;
   if (! fDLMap.empty()) {
      for (SliceDLMap_i i = fDLMap.begin(); i != fDLMap.end(); ++i) {
         if (i->second) {
            PurgeDLRange(i->second, 1);
            i->second = 0;
         }
      }
   }
   TGLObject::DLCachePurge();
}

//______________________________________________________________________________
void TEveCaloLegoGL::MakeQuad(Float_t x1, Float_t y1, Float_t z1,
      Float_t xw, Float_t yw, Float_t h) const
{
   // Draw an axis-aligned box using quads.

   //    z
   //    |
   //    |
   //    |________y
   //   /  6-------7
   //  /  /|      /|
   // x  5-------4 |
   //    | 2-----|-3
   //    |/      |/
   //    1-------0
   //

   Float_t x2 = x1 + xw;
   Float_t y2 = y1 + yw;
   Float_t z2 = z1 + h;

   if (x1 < fM->GetEtaMin()) x1 = fM->GetEtaMin();
   if (x2 > fM->GetEtaMax()) x2 = fM->GetEtaMax();

   if (y1 < fM->GetPhiMin()) y1 = fM->GetPhiMin();
   if (y2 > fM->GetPhiMax()) y2 = fM->GetPhiMax();

   glBegin(GL_QUADS);
   {
      // bottom 0123
      glNormal3f(0, 0, -1);
      glVertex3f(x2, y2, z1);
      glVertex3f(x2, y1, z1);
      glVertex3f(x1, y1, z1);
      glVertex3f(x1, y2, z1);
      // top 4765
      glNormal3f(0, 0, 1);
      glVertex3f(x2, y2, z2);
      glVertex3f(x1, y2, z2);
      glVertex3f(x1, y1, z2);
      glVertex3f(x2, y1, z2);

      // back 0451
      glNormal3f(1, 0, 0);
      glVertex3f(x2, y2, z1);
      glVertex3f(x2, y2, z2);
      glVertex3f(x2, y1, z2);
      glVertex3f(x2, y1, z1);
      // front 3267
      glNormal3f(-1, 0, 0);
      glVertex3f(x1, y2, z1);
      glVertex3f(x1, y1, z1);
      glVertex3f(x1, y1, z2);
      glVertex3f(x1, y2, z2);

      // left  0374
      glNormal3f(0, 1, 0);
      glVertex3f(x2, y2, z1);
      glVertex3f(x1, y2, z1);
      glVertex3f(x1, y2, z2);
      glVertex3f(x2, y2, z2);
      // right 1562
      glNormal3f(0, -1, 0);
      glVertex3f(x2, y1, z1);
      glVertex3f(x2, y1, z2);
      glVertex3f(x1, y1, z2);
      glVertex3f(x1, y1, z1);
   }
   glEnd();
}

//______________________________________________________________________________
void TEveCaloLegoGL::MakeDisplayList() const
{
   // Create display-list that draws histogram bars.
   // It is used for filled and outline passes.

   if (fM->fBinStep > 1)
   {
      Int_t nSlices = fM->fData->GetNSlices();
      Float_t *vals;
      Int_t bin;
      Float_t offset;
      Float_t y0, y1;
      for (Int_t s = 0; s < nSlices; ++s)
      {
         if (fDLMap.empty() || fDLMap[s] == 0)
            fDLMap[s] = glGenLists(1);

         glNewList(fDLMap[s], GL_COMPILE);
         glLoadName(s);
         glPushName(0);
         for (Int_t i=1; i<=fEtaAxis->GetNbins(); ++i)
         {
            for (Int_t j=1; j<=fPhiAxis->GetNbins(); ++j)
            {
               bin = (i)+(j)*(fEtaAxis->GetNbins()+2);

               if (fRebinData.fBinData[bin] !=-1)
               {
                  vals = fRebinData.GetSliceVals(bin);
                  offset =0;
                  for (Int_t t=0; t<s; t++)
                     offset+=vals[t];

                  y0 = fPhiAxis->GetBinLowEdge(j);
                  y1 = fPhiAxis->GetBinUpEdge(j);
                  WrapTwoPi(y0, y1);
                  {
                     glLoadName(bin);
                     MakeQuad(fEtaAxis->GetBinLowEdge(i), y0, offset,
                              fEtaAxis->GetBinWidth(i), y1-y0, vals[s]);
                  }
               }
            }
         }
         glPopName();
         glEndList();
      }
   }
   else {
      TEveCaloData::CellData_t cellData;
      Int_t   prevTower = 0;
      Float_t offset = 0;

      // ids in eta phi rng
      Int_t nSlices = fM->fData->GetNSlices();
      for (Int_t s = 0; s < nSlices; ++s) {
         if (fDLMap.empty() || fDLMap[s] == 0)
            fDLMap[s] = glGenLists(1);
         glNewList(fDLMap[s], GL_COMPILE);

         for (UInt_t i = 0; i < fM->fCellList.size(); ++i) {
            if (fM->fCellList[i].fSlice > s) continue;
            if (fM->fCellList[i].fTower != prevTower) {
               offset = 0;
               prevTower = fM->fCellList[i].fTower;
            }

            fM->fData->GetCellData(fM->fCellList[i], cellData);
            if (s == fM->fCellList[i].fSlice) {
               glLoadName(i);
               WrapTwoPi(cellData.fPhiMin, cellData.fPhiMax);
               MakeQuad(cellData.EtaMin(), cellData.PhiMin(), offset,
                        cellData.EtaDelta(), cellData.PhiDelta(), cellData.Value(fM->fPlotEt));
            }
            offset += cellData.Value(fM->fPlotEt);
         }
         glEndList();
      }
   }
   fDLCacheOK = kTRUE;
}

//______________________________________________________________________________
void TEveCaloLegoGL::SetAxis3DTitlePos(TGLRnrCtx &rnrCtx, Float_t x0, Float_t x1, Float_t y0, Float_t y1) const
{
   const GLdouble *pm = rnrCtx.RefCamera().RefLastNoPickProjM().CArr();
   GLdouble mm[16];
   GLint    vp[4];
   glGetDoublev(GL_MODELVIEW_MATRIX,  mm);
   glGetIntegerv(GL_VIEWPORT, vp);
   GLdouble projX[4], projY[4], projZ[4];

   GLdouble cornerX[4];
   GLdouble cornerY[4];
   cornerX[0] = x0; cornerY[0] = y0;
   cornerX[1] = x1; cornerY[1] = y0;
   cornerX[2] = x1; cornerY[2] = y1;
   cornerX[3] = x0; cornerY[3] = y1;

   gluProject(cornerX[0], cornerY[0], 0, mm, pm, vp, &projX[0], &projY[0], &projZ[0]);
   gluProject(cornerX[1], cornerY[1], 0, mm, pm, vp, &projX[1], &projY[1], &projZ[1]);
   gluProject(cornerX[2], cornerY[2], 0, mm, pm, vp, &projX[2], &projY[2], &projZ[2]);
   gluProject(cornerX[3], cornerY[3], 0, mm, pm, vp, &projX[3], &projY[3], &projZ[3]);


   // Z axis location (left most corner)
   //
   Int_t idxLeft = 0;
   Float_t xt = projX[0];
   for (Int_t i = 1; i < 4; ++i) {
      if (projX[i] < xt) {
         xt  = projX[i];
         idxLeft = i;
      }
   }
   fZAxisTitlePos.Set(cornerX[idxLeft], cornerY[idxLeft], fDataMax* 1.05);


   // XY axis location (closest to eye) first
   //
   Float_t zt = 1.f;
   Float_t zMin = 0.f;
   Int_t idxFront = 0;
   for (Int_t i = 0; i < 4; ++i) {
      if (projZ[i] < zt) {
         zt  = projZ[i];
         idxFront = i;
      }
      if (projZ[i] > zMin) zMin = projZ[i];
   }


   Int_t xyIdx = idxFront;
   if (zMin - zt < 1e-2) xyIdx = 0; // avoid flipping in front view


   switch (xyIdx) {
      case 0:
         fXAxisTitlePos.fX = x1;
         fXAxisTitlePos.fY = y0;
         fYAxisTitlePos.fX = x0;
         fYAxisTitlePos.fY = y1;
         break;
      case 1:
         fXAxisTitlePos.fX = x0;
         fXAxisTitlePos.fY = y0;
         fYAxisTitlePos.fX = x1;
         fYAxisTitlePos.fY = y1;
         break;
      case 2:
         fXAxisTitlePos.fX = x0;
         fXAxisTitlePos.fY = y1;
         fYAxisTitlePos.fX = x1;
         fYAxisTitlePos.fY = y0;
         break;
      case 3:
         fXAxisTitlePos.fX = x1;
         fXAxisTitlePos.fY = y1;
         fYAxisTitlePos.fX = x0;
         fYAxisTitlePos.fY = y0;
         break;
   }

   // move title 5% over the axis length
   Float_t off = 0.05;
   Float_t tOffX = (x1-x0) * off; if (fYAxisTitlePos.fX > x0) tOffX = -tOffX;
   Float_t tOffY = (y1-y0) * off; if (fXAxisTitlePos.fY > y0) tOffY = -tOffY;
   fXAxisTitlePos.fX += tOffX;
   fYAxisTitlePos.fY += tOffY;


   // frame box
   //
   if (fM->fBoxMode)
   {
      // get corner closest to eye excluding left corner
      Double_t zm = 1.f;
      Int_t idxDepthT = 0;
      for (Int_t i = 0; i < 4; ++i)
      {
         if (projZ[i] < zm && projZ[i] >= zt && i != idxFront )
         {
            zm  = projZ[i];
            idxDepthT = i;
         }
      }
      if (idxFront == idxLeft)  idxFront =idxDepthT;

      switch (idxFront)
      {
         case 0:
            fBackPlaneXConst[0].Set(x1, y0, 0); fBackPlaneXConst[1].Set(x1, y1, 0);
            fBackPlaneYConst[0].Set(x0, y1, 0); fBackPlaneYConst[1].Set(x1, y1, 0);
            break;
         case 1:
            fBackPlaneXConst[0].Set(x0, y0, 0); fBackPlaneXConst[1].Set(x0, y1, 0);
            fBackPlaneYConst[0].Set(x0, y1, 0); fBackPlaneYConst[1].Set(x1, y1, 0);
            break;
         case 2:
            fBackPlaneXConst[0].Set(x0, y0, 0); fBackPlaneXConst[1].Set(x0, y1, 0);
            fBackPlaneYConst[0].Set(x0, y0, 0); fBackPlaneYConst[1].Set(x1, y0, 0);
            break;
         case 3:
            fBackPlaneXConst[0].Set(x1, y0, 0); fBackPlaneXConst[1].Set(x1, y1, 0);
            fBackPlaneYConst[0].Set(x0, y0, 0); fBackPlaneYConst[1].Set(x1, y0, 0);
            break;
      }
   }
}

//______________________________________________________________________________
void TEveCaloLegoGL::DrawAxis3D(TGLRnrCtx & rnrCtx) const
{
   // Draw z-axis and z-box at the appropriate grid corner-point including
   // tick-marks and labels.

   // set font size first depending on size of projected axis

   TGLMatrix mm;
   GLdouble pm[16];
   GLint    vp[4];
   glGetDoublev(GL_MODELVIEW_MATRIX, mm.Arr());
   glGetDoublev(GL_PROJECTION_MATRIX, pm);
   glGetIntegerv(GL_VIEWPORT, vp);

   GLdouble dn[3];
   GLdouble up[3];
   gluProject(fZAxisTitlePos.fX, fZAxisTitlePos.fY, 0                , mm.Arr(), pm, vp, &dn[0], &dn[1], &dn[2]);
   gluProject(fZAxisTitlePos.fX, fZAxisTitlePos.fY, fZAxisTitlePos.fZ, mm.Arr(), pm, vp, &up[0], &up[1], &up[2]);
   Double_t len = TMath::Sqrt((up[0] - dn[0]) * (up[0] - dn[0])
                              + (up[1] - dn[1]) * (up[1] - dn[1])
                              + (up[2] - dn[2]) * (up[2] - dn[2]));

   TGLVertex3 worldRef(fZAxisTitlePos.fX, fZAxisTitlePos.fY, fZAxisTitlePos.fZ);

   fAxisPainter.RefTMOff(0) = rnrCtx.RefCamera().ViewportDeltaToWorld(worldRef, -len, 0,  &mm);
   fAxisPainter.SetLabelPixelFontSize(TMath::Nint(len*fM->GetData()->GetEtaBins()->GetLabelSize()));
   fAxisPainter.SetTitlePixelFontSize(TMath::Nint(len*fM->GetData()->GetEtaBins()->GetTitleSize()));

   // Z axis
   //
   if (fM->fData->Empty() == kFALSE)
   {
      fZAxis->SetAxisColor(fGridColor);
      fZAxis->SetLabelColor(fFontColor);
      fZAxis->SetTitleColor(fFontColor);
      fZAxis->SetNdivisions(fM->fNZSteps*100 + 10);
      fZAxis->SetLimits(0, fDataMax);
      fZAxis->SetTitle(fM->GetPlotEt() ? "Et[GeV]" : "E[GeV]");

      fAxisPainter.SetTMNDim(1);
      fAxisPainter.RefDir().Set(0., 0., 1.);
      fAxisPainter.SetLabelAlign(TGLFont::kRight, TGLFont::kCenterV);
      glPushMatrix();
      glTranslatef(fZAxisTitlePos.fX, fZAxisTitlePos.fY, 0);

      // tickmark vector = 10 pixels left
      fAxisPainter.RefTitlePos().Set(fAxisPainter.RefTMOff(0).X()*0.05,  fAxisPainter.RefTMOff(0).Y()*0.05, fZAxisTitlePos.fZ);
      fZAxis->SetLabelOffset(0.05);
      fZAxis->SetTickLength(0.05);
      fAxisPainter.PaintAxis(rnrCtx, fZAxis);
      glTranslated( fAxisPainter.RefTMOff(0).X(),  fAxisPainter.RefTMOff(0).Y(),  fAxisPainter.RefTMOff(0).Z());
      glPopMatrix();

      // repaint axis if tower dobule-clicked
      if (fM->fTowerPicked >= 0) {
         TEveCaloData::CellData_t cd;
         fM->fData->GetCellData(fM->fCellList[fM->fTowerPicked], cd);
         WrapTwoPi(cd.fPhiMin, cd.fPhiMax);
         glPushMatrix();
         glTranslatef(cd.EtaMin(), cd.PhiMin(), 0);
         fAxisPainter.RnrLines();
         fAxisPainter.RnrLabels();
         glPopMatrix();
      }

      // draw box frame
      //
      if (fM->fBoxMode) {

         glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);

         // box verticals
         TGLUtil::LineWidth(1);
         glBegin(GL_LINES);
         TGLUtil::Color(fGridColor);

         glVertex3f(fBackPlaneXConst[0].fX   ,fBackPlaneXConst[0].fY   ,0);
         glVertex3f(fBackPlaneXConst[0].fX   ,fBackPlaneXConst[0].fY   ,fDataMax);
         glVertex3f(fBackPlaneXConst[1].fX   ,fBackPlaneXConst[1].fY   ,0);
         glVertex3f(fBackPlaneXConst[1].fX   ,fBackPlaneXConst[1].fY   ,fDataMax);


         glVertex3f(fBackPlaneYConst[0].fX   ,fBackPlaneYConst[0].fY   ,0);
         glVertex3f(fBackPlaneYConst[0].fX   ,fBackPlaneYConst[0].fY   ,fDataMax);
         glVertex3f(fBackPlaneYConst[1].fX   ,fBackPlaneYConst[1].fY   ,0);
         glVertex3f(fBackPlaneYConst[1].fX   ,fBackPlaneYConst[1].fY   ,fDataMax);

         // box top
         glVertex3f(fBackPlaneXConst[0].fX   ,fBackPlaneXConst[0].fY   ,fDataMax);
         glVertex3f(fBackPlaneXConst[1].fX   ,fBackPlaneXConst[1].fY   ,fDataMax);
         glVertex3f(fBackPlaneYConst[0].fX   ,fBackPlaneYConst[0].fY   ,fDataMax);
         glVertex3f(fBackPlaneYConst[1].fX   ,fBackPlaneYConst[1].fY   ,fDataMax);

         glEnd();

         // box horizontals stippled
         glEnable(GL_LINE_STIPPLE);
         Int_t ondiv;
         Double_t omin, omax, bw1;
         THLimitsFinder::Optimize(0, fDataMax, fM->fNZSteps, omin, omax, ondiv, bw1);

         glLineStipple(1, 0x5555);
         glBegin(GL_LINES);
         Float_t hz  = bw1;
         for (Int_t i = 1; i <= ondiv; ++i, hz += bw1) {
            glVertex3f(fBackPlaneXConst[0].fX   ,fBackPlaneXConst[0].fY   ,hz);
            glVertex3f(fBackPlaneXConst[1].fX   ,fBackPlaneXConst[1].fY   ,hz);
            glVertex3f(fBackPlaneYConst[0].fX   ,fBackPlaneYConst[0].fY   ,hz);
            glVertex3f(fBackPlaneYConst[1].fX   ,fBackPlaneYConst[1].fY   ,hz);
         }
         glEnd();

         glPopAttrib();
      }
   }

   // XY Axis
   //

   Float_t yOff  = fM->GetPhiRng();
   if (fXAxisTitlePos.fY < fM->GetPhiMax()) yOff = -yOff;

   Float_t xOff  = fM->GetEtaRng();
   if (fYAxisTitlePos.fX < fM->GetEtaMax()) xOff = -xOff;

   TAxis ax;
   ax.SetAxisColor(fGridColor);
   ax.SetLabelColor(fFontColor);
   ax.SetTitleColor(fFontColor);
   ax.SetTitleFont(fM->GetData()->GetEtaBins()->GetTitleFont());
   ax.SetLabelOffset(0.02);
   ax.SetTickLength(0.05);
   fAxisPainter.SetTMNDim(2);
   fAxisPainter.RefTMOff(1).Set(0, 0, -fDataMax);
   fAxisPainter.SetLabelAlign(TGLFont::kCenterH, TGLFont::kBottom);

   // eta
   glPushMatrix();
   fAxisPainter.RefDir().Set(1, 0, 0);
   fAxisPainter.RefTMOff(0).Set(0, yOff, 0);
   glTranslatef(0, fXAxisTitlePos.fY, 0);
   ax.SetNdivisions(710);
   ax.SetLimits(fM->GetEtaMin(), fM->GetEtaMax());
   ax.SetTitle(fM->GetData()->GetEtaBins()->GetTitle());
   fAxisPainter.RefTitlePos().Set(fXAxisTitlePos.fX, yOff*1.5*ax.GetTickLength(), -fDataMax*ax.GetTickLength());
   fAxisPainter.PaintAxis(rnrCtx, &ax);
   glPopMatrix();

   // phi
   fAxisPainter.RefDir().Set(0, 1, 0);
   fAxisPainter.RefTMOff(0).Set(xOff, 0, 0);
   ax.SetNdivisions(510);
   ax.SetLimits(fM->GetPhiMin(), fM->GetPhiMax());
   ax.SetTitle(fM->GetData()->GetPhiBins()->GetTitle());
   glPushMatrix();
   glTranslatef(fYAxisTitlePos.fX, 0, 0);
   fAxisPainter.RefTitlePos().Set( xOff*1.5*ax.GetTickLength(), fYAxisTitlePos.fY,  -fDataMax*ax.GetTickLength());
   fAxisPainter.PaintAxis(rnrCtx, &ax);
   glPopMatrix();

} // DrawAxis3D

//______________________________________________________________________________
void TEveCaloLegoGL::DrawAxis2D(TGLRnrCtx & rnrCtx) const
{
   // Draw XY axis.

   TAxis ax;
   ax.SetAxisColor(fGridColor);
   ax.SetLabelColor(fFontColor);
   ax.SetTitleColor(fFontColor);
   ax.SetTitleFont(fM->GetData()->GetEtaBins()->GetTitleFont());
   ax.SetLabelOffset(0.01);
   ax.SetTickLength(0.05);

   // set fonts
   fAxisPainter.SetAttAxis(&ax);

   // get projected length of diagonal to determine
   TGLMatrix mm;
   GLdouble pm[16];
   GLint    vp[4];
   glGetDoublev(GL_MODELVIEW_MATRIX, mm.Arr());
   glGetDoublev(GL_PROJECTION_MATRIX, pm);
   glGetIntegerv(GL_VIEWPORT, vp);

   GLdouble dn[3];
   GLdouble up[3];
   gluProject(fM->GetEtaMin(), fM->GetPhiMin(), 0, mm.Arr(), pm, vp, &dn[0], &dn[1], &dn[2]);
   gluProject(fM->GetEtaMax(), fM->GetPhiMax(), 0, mm.Arr(), pm, vp, &up[0], &up[1], &up[2]);
   Double_t len = TMath::Sqrt((up[0] - dn[0]) * (up[0] - dn[0])
                              + (up[1] - dn[1]) * (up[1] - dn[1])
                              + (up[2] - dn[2]) * (up[2] - dn[2]));

   // eta
   fAxisPainter.SetLabelPixelFontSize(TMath::Nint(len*fM->GetData()->GetEtaBins()->GetLabelSize()));
   fAxisPainter.SetTitlePixelFontSize(TMath::Nint(len*fM->GetData()->GetEtaBins()->GetTitleSize()));
   ax.SetNdivisions(710);
   ax.SetLimits(fM->GetEtaMin(), fM->GetEtaMax());
   ax.SetTitle(fM->GetData()->GetEtaBins()->GetTitle());
   fAxisPainter.RefTitlePos().Set(fM->GetEtaMax(), -fM->GetPhiRng()*(ax.GetTickLength()+ ax.GetLabelOffset()), 0 );
   fAxisPainter.RefDir().Set(1, 0, 0);
   fAxisPainter.RefTMOff(0).Set(0,  -fM->GetPhiRng(), 0);
   fAxisPainter.SetLabelAlign(TGLFont::kCenterH, TGLFont::kBottom);

   glPushMatrix();
   glTranslatef(0, fM->GetPhiMin(), 0);
   fAxisPainter.PaintAxis(rnrCtx, &ax);
   glPopMatrix();

   // phi
   ax.SetNdivisions(510);
   ax.SetLimits(fM->GetPhiMin(), fM->GetPhiMax());
   ax.SetTitle(fM->GetData()->GetPhiBins()->GetTitle());
   fAxisPainter.RefTitlePos().Set(-fM->GetEtaRng()*(ax.GetTickLength()+ ax.GetLabelOffset()), fM->GetPhiMax(), 0);
   fAxisPainter.RefDir().Set(0, 1, 0);
   fAxisPainter.RefTMOff(0).Set(-fM->GetEtaRng(), 0, 0);
   fAxisPainter.SetLabelAlign(TGLFont::kRight, TGLFont::kCenterV);

   glPushMatrix();
   glTranslatef(fM->GetEtaMin(), 0, 0);
   fAxisPainter.PaintAxis(rnrCtx, &ax);
   glPopMatrix();
}

//______________________________________________________________________________
Int_t TEveCaloLegoGL::GetGridStep(TGLRnrCtx &rnrCtx) const
{
   // Calculate view-dependent grid density.

   using namespace TMath;

   GLdouble x0, y0, z0, x1, y1, z1;
   GLdouble mm[16];
   GLint    vp[4];
   glGetDoublev(GL_MODELVIEW_MATRIX,  mm);
   glGetIntegerv(GL_VIEWPORT, vp);
   const GLdouble *pmx = rnrCtx.RefCamera().RefLastNoPickProjM().CArr();

   GLdouble em, eM, pm, pM;
   fM->GetData()->GetEtaLimits(pm, pM);
   fM->GetData()->GetPhiLimits(em, eM);
   gluProject(em, pm, 0.f , mm, pmx, vp, &x0, &y0, &z0);
   gluProject(eM, pM, 0.f , mm, pmx, vp, &x1, &y1, &z1);
   Float_t d0 = Sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1) + (z0 - z1) * (z0 - z1));

   gluProject(em, pm, 0.f , mm, pmx, vp, &x0, &y0, &z0);
   gluProject(eM, pM, 0.f , mm, pmx, vp, &x1, &y1, &z1);
   Float_t d1 = Sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1) + (z0 - z1) * (z0 - z1));

   Float_t d = d1 > d0 ? d1 : d0;
   Int_t i0 = fM->fData->GetEtaBins()->FindBin(fM->GetEtaMin());
   Int_t i1 = fM->fData->GetEtaBins()->FindBin(fM->GetEtaMax());
   Int_t j0 = fM->fData->GetPhiBins()->FindBin(fM->GetPhiMin());
   Int_t j1 = fM->fData->GetPhiBins()->FindBin(fM->GetPhiMax());

   Int_t pixelsPerBin = TMath::Nint(d / Sqrt((i0 - i1) * (i0 - i1) + (j0 - j1) * (j0 - j1)));
   Int_t ngroup = 1;
   if (fM->fAutoRebin)
   {
      if (pixelsPerBin < fM->fPixelsPerBin*0.5) {
         ngroup = 4;
      } else if (pixelsPerBin < fM->fPixelsPerBin) {
         ngroup = 2;
      } else {
         ngroup = 1;
      }
   }
   return ngroup;
}

//___________________________________________________________________________
void TEveCaloLegoGL::RebinAxis(TAxis *orig, TAxis *curr) const
{
   // Rebin eta, phi axis.

   Double_t center = 0.5 * (orig->GetXmin() + orig->GetXmax());
   Int_t idx0 = orig->FindBin(center);
   Double_t bc = orig->GetBinCenter(idx0);
   if (bc > center) idx0--;

   Int_t nbR = TMath::FloorNint(idx0/fM->fBinStep) + TMath::FloorNint((orig->GetNbins() - idx0)/fM->fBinStep);
   Double_t *bins = new Double_t[nbR+1];
   Int_t off = idx0 - TMath::FloorNint(idx0/fM->fBinStep)*fM->fBinStep;
   for(Int_t i = 0; i <= nbR; i++)
      bins[i] = orig->GetBinUpEdge(off + i*fM->fBinStep);

   curr->Set(nbR, bins);
   delete [] bins;
}

//______________________________________________________________________________
void TEveCaloLegoGL::DrawHistBase(TGLRnrCtx &rnrCtx) const
{
   // Draw basic histogram components: x-y grid

   Float_t eta0 = fM->fEtaMin;
   Float_t eta1 = fM->fEtaMax;
   Float_t phi0 = fM->GetPhiMin();
   Float_t phi1 = fM->GetPhiMax();

   // XY grid
   //
   TGLUtil::Color(fGridColor);
   TGLUtil::LineWidth(1);
   glBegin(GL_LINES);
   glVertex2f(eta0, phi0);
   glVertex2f(eta0, phi1);
   glVertex2f(eta1, phi0);
   glVertex2f(eta1, phi1);

   glVertex2f(eta0, phi0);
   glVertex2f(eta1, phi0);
   glVertex2f(eta0, phi1);
   glVertex2f(eta1, phi1);

   // eta grid
   Float_t val;
   Int_t neb = fEtaAxis->GetNbins();
   for (Int_t i = 0; i<= neb; i++)
   {
      val = fEtaAxis->GetBinUpEdge(i);
      if (val > eta0 && val < eta1 )
      {
         glVertex2f(val, phi0);
         glVertex2f(val, phi1);
      }
   }

   // phi grid
   Int_t npb = fPhiAxis->GetNbins();
   for (Int_t i = 1; i <= npb; i++) {
       val = fPhiAxis->GetBinUpEdge(i);
      if (val > phi0 && val < phi1)
      {
         glVertex2f(eta0, val);
         glVertex2f(eta1, val);
      }
   }

   glEnd();

   // XYZ axes
   //
   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
   TGLUtil::LineWidth(2);
   if (fCells3D)
   {
      SetAxis3DTitlePos(rnrCtx, eta0, eta1, phi0, phi1);
      DrawAxis3D(rnrCtx);
   }
   else
   {
      DrawAxis2D(rnrCtx);
   }
   glPopAttrib();
}

//______________________________________________________________________________
void TEveCaloLegoGL::DrawCells3D(TGLRnrCtx & rnrCtx) const
{
   // Render the calo lego-plot with OpenGL.

   // quads
   {
      for (SliceDLMap_i i = fDLMap.begin(); i != fDLMap.end(); ++i) {
         TGLUtil::Color(fM->GetDataSliceColor(i->first));
         glCallList(i->second);
      }
   }
   // outlines
   {
      if (rnrCtx.SceneStyle() == TGLRnrCtx::kFill) {
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
         glDisable(GL_POLYGON_OFFSET_FILL);
         TGLUtil::Color(1);
         for (SliceDLMap_i i = fDLMap.begin(); i != fDLMap.end(); ++i)
            glCallList(i->second);
      }
   }
}

//______________________________________________________________________________
void TEveCaloLegoGL::DrawCells2D(TGLRnrCtx & rnrCtx) const
{
   // Draw projected histogram.

   std::vector<Double_t>  cellGeom;
   std::vector<Float_t>   sumVal;
   std::vector<Int_t>     maxSlice;
   std::vector<Int_t>     id;

   Int_t max_energy_slice, cellID=0;
   Float_t sum, max_energy, x1, x2, y1, y2;

   if (fM->fBinStep == 1)
   {
      TEveCaloData::vCellId_t::iterator currentCell = fM->fCellList.begin();
      TEveCaloData::vCellId_t::iterator nextCell    = currentCell;
      ++nextCell;

      while (currentCell != fM->fCellList.end()) {
         TEveCaloData::CellData_t currentCellData;
         TEveCaloData::CellData_t nextCellData;

         fM->fData->GetCellData(*currentCell, currentCellData);
         sum = max_energy = currentCellData.Value(fM->fPlotEt);
         max_energy_slice = currentCell->fSlice;
         while (nextCell != fM->fCellList.end() && currentCell->fTower == nextCell->fTower) {
            fM->fData->GetCellData(*nextCell, nextCellData);
            Float_t energy = nextCellData.Value(fM->fPlotEt);
            sum += energy;
            if (energy > max_energy) {
               max_energy       = energy;
               max_energy_slice = nextCell->fSlice;
            }
            ++nextCell;
            ++cellID;
         }

         WrapTwoPi(currentCellData.fPhiMin, currentCellData.fPhiMax);

         cellGeom.push_back(currentCellData.fEtaMin); cellGeom.push_back(currentCellData.fPhiMin);
         cellGeom.push_back(currentCellData.fEtaMax); cellGeom.push_back(currentCellData.fPhiMax);

         sumVal.push_back(sum);
         maxSlice.push_back(max_energy_slice);
         id.push_back(cellID);

         currentCell = nextCell;
         ++nextCell;
         ++cellID;
      }
   }
   else {
      // get sum value in agragated cells
      const Int_t nEta = fEtaAxis->GetNbins();
      const Int_t nPhi = fPhiAxis->GetNbins();
      std::vector<Float_t> vec;
      vec.assign((nEta + 2)*(nPhi + 2), 0.f);
      std::vector<Float_t> max_e;
      std::vector<Int_t>   max_e_slice;
      max_e.assign((nEta + 2) * (nPhi + 2), 0.f);
      max_e_slice.assign((nEta + 2) * (nPhi + 2), -1);

      for (UInt_t bin = 0; bin < fRebinData.fBinData.size(); ++bin) {
         Float_t ssum = 0;
         if (fRebinData.fBinData[bin] != -1) {
            Float_t *val = fRebinData.GetSliceVals(bin);
            for (Int_t s = 0; s < fRebinData.fNSlices; ++s) {
               ssum += val[s];
               if (val[s] > max_e[bin]) {
                  max_e[bin]       = val[s];
                  max_e_slice[bin] = s;
               }
            }
         }
         vec[bin] = ssum;
      }

      // take smallest threshold
      Float_t threshold = fM->GetDataSliceThreshold(0);
      for (Int_t s = 1; s < fM->fData->GetNSlices(); ++s) {
         if (threshold > fM->GetDataSliceThreshold(s))
            threshold = fM->GetDataSliceThreshold(s);
      }

      for (Int_t i = 1; i <= fEtaAxis->GetNbins(); ++i) {
         for (Int_t j = 1; j <= fPhiAxis->GetNbins(); ++j) {
            const Int_t bin = j * (nEta + 2) + i;
            if (vec[bin] > threshold && fRebinData.fBinData[bin] != -1) {
               x1 = fEtaAxis->GetBinLowEdge(i);
               x2 = fEtaAxis->GetBinUpEdge(i);
               y1 = fPhiAxis->GetBinLowEdge(j);
               y2 = fPhiAxis->GetBinUpEdge(j);
               cellGeom.push_back(x1); cellGeom.push_back(y1);
               cellGeom.push_back(x2); cellGeom.push_back(y2);
               sumVal.push_back(vec[bin]);
               maxSlice.push_back(max_e_slice[bin]);
               id.push_back(bin);
            }
         }
      }
   }

   //
   // drawcells
   //

   Float_t bws = -1; //smallest bin
   Float_t logMax = -1;

   if (fM->f2DMode == TEveCaloLego::kValColor ) {
      fM->AssertPalette();
      UChar_t col[4];

      for (UInt_t i=0; i < sumVal.size(); i++) {
         glLoadName(id[i]);
         glBegin(GL_POLYGON);

         fM->fPalette->ColorFromValue(TMath::FloorNint(sumVal[i]), col);
         TGLUtil::Color4ubv(col);

         x1 = cellGeom[4*i];
         y1 = cellGeom[4*i+1];
         x2 = cellGeom[4*i+2];
         y2 = cellGeom[4*i+3];

         glVertex3f(x1, y1, sumVal[i]);
         glVertex3f(x2, y1, sumVal[i]);
         glVertex3f(x2, y2, sumVal[i]);
         glVertex3f(x1, y2, sumVal[i]);
         glEnd();
      }
   }
   else {
      bws = 1e5;
      Float_t x, y;
      for (UInt_t i=0; i< cellGeom.size(); i += 4 ) {
         if ( cellGeom[i+2] -cellGeom[i] < bws)   bws =  cellGeom[i+2] -cellGeom[i];
         if ( cellGeom[i+3] -cellGeom[i+1] < bws) bws =  cellGeom[i+3] -cellGeom[i+1];
      }
      bws *= 0.5;

      Float_t maxv =0;
      glBegin(GL_POINTS);
      for (UInt_t i=0; i< sumVal.size(); i++) {
         TGLUtil::Color(fM->fData->GetSliceColor(maxSlice[i]));
         x = 0.5* (cellGeom[4*i] +cellGeom[4*i+2]);
         y = 0.5* (cellGeom[4*i+1] +cellGeom[4*i+3]);
         glVertex3f(x, y, sumVal[i]);
         if (sumVal[i] > maxv) maxv = sumVal[i];
      }
      glEnd();
      logMax = TMath::Log10(maxv + 1);
      // scale cells

      for (UInt_t i=0; i< sumVal.size(); i++) {
         glLoadName(id[i]);
         glBegin(GL_POLYGON);
         TGLUtil::Color(fM->fData->GetSliceColor(maxSlice[i]));

         Float_t bw = bws* TMath::Log10(sumVal[i]+1)/logMax;

         x = 0.5* (cellGeom[4*i] +cellGeom[4*i+2]) ;
         y = 0.5* (cellGeom[4*i+1] +cellGeom[4*i+3]) ;
         glVertex3f(x - bw, y - bw, sumVal[i]);
         glVertex3f(x + bw, y - bw, sumVal[i]);
         glVertex3f(x + bw, y + bw, sumVal[i]);
         glVertex3f(x - bw, y + bw, sumVal[i]);

         glEnd();

      }

   }

   // print values on towers
   if (rnrCtx.Selection() == kFALSE && rnrCtx.Highlight() == kFALSE) {
      // get projected length of diagonal to determine
      TGLMatrix mm;
      GLdouble pm[16];
      GLint    vp[4];
      glGetDoublev(GL_MODELVIEW_MATRIX, mm.Arr());
      glGetDoublev(GL_PROJECTION_MATRIX, pm);
      glGetIntegerv(GL_VIEWPORT, vp);

      GLdouble dn[3];
      GLdouble up[3];
      gluProject(fM->GetEtaMin(), 0, 0, mm.Arr(), pm, vp, &dn[0], &dn[1], &dn[2]);
      gluProject(fM->GetEtaMax(), 0, 0, mm.Arr(), pm, vp, &up[0], &up[1], &up[2]);
      Double_t etaLenPix = up[0]-dn[0];
      Float_t sx = etaLenPix/fM->GetEtaRng();

      TGLUtil::Color(rnrCtx.ColorSet().Markup().GetColorIndex());
      TGLFont font;
      Double_t cs;
      Float_t x, y;
      rnrCtx.RegisterFontNoScale(fM->fCellPixelFontSize, "arial", TGLFont::kPixmap, font);
      for (UInt_t i=0; i< sumVal.size(); i++) {
         if (fM->f2DMode == TEveCaloLego::kValColor )
            cs = TMath::Min(cellGeom[4*i+2] - cellGeom[4*i], cellGeom[4*i+3] - cellGeom[4*i+1]);
         else
            cs = bws*TMath::Log10(sumVal[i]+1)/logMax;

         if (cs*sx >  fM->fDrawNumberCellPixels)
         {
            x = 0.5* (cellGeom[4*i]   + cellGeom[4*i+2]);
            y = 0.5* (cellGeom[4*i+1] + cellGeom[4*i+3]);
            // can use same format as for axis
            // space on top of towers is limited
            const char* txt;
            if (sumVal[i] > 10)
               txt = Form("%d", TMath::Nint(sumVal[i]));
            else if (sumVal[i] > 1 )
               txt = Form("%.1f", sumVal[i]);
            else if (sumVal[i] > 0.01 )
               txt = Form("%.2f", 0.01*TMath::Nint(sumVal[i]*100));
            else
            {
               txt = Form("~1e%d", TMath::Nint(TMath::Log10(sumVal[i])));
            }
            font.Render(txt, x, y, sumVal[i]*1.2, TGLFont::kCenterH, TGLFont::kCenterV);
         }
      }
   }
} // end DrawCells2D

//______________________________________________________________________________
void TEveCaloLegoGL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   // Draw the object.

   if (! fM->fData || ! fM->fData->GetEtaBins() || ! fM->fData->GetPhiBins())
      return;

   // projection type
   if (fM->fProjection == TEveCaloLego::kAuto)
      fCells3D = (!(rnrCtx.RefCamera().IsOrthographic() && rnrCtx.RefCamera().GetCamBase().GetBaseVec(1).Z()));
   else if (fM->fProjection == TEveCaloLego::k2D)
      fCells3D = kFALSE;
   else
      fCells3D = kTRUE;

   // cache max val
   fDataMax = fM->GetMaxVal();

   // modelview matrix
   Double_t em, eM, pm, pM;
   fM->fData->GetEtaLimits(em, eM);
   fM->fData->GetPhiLimits(pm, pM);
   Double_t unit = ((eM - em) < (pM - pm)) ? (eM - em) : (pM - pm);
   glPushMatrix();
   Float_t sx = (eM - em) / fM->GetEtaRng();
   Float_t sy = (pM - pm) / fM->GetPhiRng();
   glScalef(sx / unit, sy / unit, fM->fData->Empty() ? 1 : fM->GetValToHeight());
   glTranslatef(-fM->GetEta(), -fM->fPhi, 0);

   // rebin axsis , check limits, fix TwoPi cycling
   Int_t oldBinStep = fM->fBinStep;
   fM->fBinStep = GetGridStep(rnrCtx);
   if (oldBinStep != fM->fBinStep) fDLCacheOK=kFALSE;

   RebinAxis(fM->fData->GetEtaBins(), fEtaAxis);
   RebinAxis(fM->fData->GetPhiBins(), fPhiAxis);

   // cache ids
   Bool_t idCacheChanged = kFALSE;
   if (fM->fCellIdCacheOK == kFALSE) {
      fM->BuildCellIdCache();
      idCacheChanged = kTRUE;
   }

   // rebin data
   if (fDLCacheOK==kFALSE || idCacheChanged ) {
      fRebinData.fSliceData.clear();
      fRebinData.fSliceData.clear();

      if (fM->fBinStep > 1) {
         fM->fData->Rebin(fEtaAxis, fPhiAxis, fM->fCellList, fM->fPlotEt, fRebinData);
         if (fM->fNormalizeRebin) {
            Double_t maxVal = 0;
            for (UInt_t i = 0; i < fRebinData.fSliceData.size(); i += fRebinData.fNSlices) {
               Double_t sum = 0;
               for (Int_t s = 0; s < fRebinData.fNSlices; s++)
                  sum += fRebinData.fSliceData[i+s];

               if (sum > maxVal) maxVal = sum;
            }

            const Float_t scale = fM->GetMaxVal() / maxVal;
            for (std::vector<Float_t>::iterator it = fRebinData.fSliceData.begin(); it != fRebinData.fSliceData.end(); it++)
               (*it) *= scale;
         }
      }
   }

   fFontColor = fM->fFontColor > -1 ? fM->fFontColor :  rnrCtx.ColorSet().Markup().GetColorIndex();
   fGridColor = fM->fGridColor;
   if (fM->fGridColor < 0)
   {
      TGLViewer* glV = (TGLViewer*)rnrCtx.GetViewer();
      if (glV->IsColorSetDark())
      {
         if (fM->fFontColor < 0) fFontColor = TColor::GetColorDark(fFontColor);
         fGridColor = TColor::GetColorDark(fFontColor);
      }
      else
      {
         if (fM->fFontColor < 0) fFontColor = TColor::GetColorBright(fFontColor);
         fGridColor = TColor::GetColorBright(fFontColor);
      }
   }

   if (!fM->fData->Empty()) {
      glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT);
      TGLUtil::LineWidth(1);
      glEnable(GL_NORMALIZE);
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0.8, 1);

      glPushName(0);
      glLoadName(0);
      if (fCells3D) {
         if (!fDLCacheOK) MakeDisplayList();
         DrawCells3D(rnrCtx);
      } else {
         glDisable(GL_LIGHTING);
         DrawCells2D(rnrCtx);
      }
      glPopName();
      glPopAttrib();
   }

   // draw histogram base
   if (rnrCtx.Selection() == kFALSE && rnrCtx.Highlight() == kFALSE) {
      glDisable(GL_LIGHTING);
      DrawHistBase(rnrCtx);
      if (fM->fDrawHPlane) {
         glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
         glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
         glDisable(GL_CULL_FACE);
         TGLUtil::ColorTransparency(fM->fPlaneColor, fM->fPlaneTransparency);
         Float_t zhp = fM->fHPlaneVal * fDataMax;
         glBegin(GL_POLYGON);
         glVertex3f(fM->fEtaMin, fM->GetPhiMin(), zhp);
         glVertex3f(fM->fEtaMax, fM->GetPhiMin(), zhp);
         glVertex3f(fM->fEtaMax, fM->GetPhiMax(), zhp);
         glVertex3f(fM->fEtaMin, fM->GetPhiMax(), zhp);
         glEnd();
         glPopAttrib();
      }
   }

   glPopMatrix();
}

//______________________________________________________________________________
void TEveCaloLegoGL::ProcessSelection(TGLRnrCtx & /*rnrCtx*/, TGLSelectRecord & rec)
{
   // Processes secondary selection from TGLViewer.

   if (rec.GetN() < 2) return;

   if (fM->fBinStep == 1) {
      Int_t cellID = rec.GetItem(1);
      TEveCaloData::CellData_t cellData;
      fM->fData->GetCellData(fM->fCellList[cellID], cellData);
      printf(">> Selected cell in eta %f phi %f \n", cellData.Eta(), cellData.Phi());

      if (fCells3D) {
         printf("Tower %d in slice %d val %f\n",
                fM->fCellList[cellID].fTower,
                fM->fCellList[cellID].fSlice, cellData.fValue);
      } else {
         printf("Tower %d in slice %d val %f\n",
                fM->fCellList[cellID].fTower,
                fM->fCellList[cellID].fSlice, cellData.fValue);
      }

   } else {
      Int_t cellID = fCells3D ? rec.GetItem(2) : rec.GetItem(1);
      if (cellID)
      {
         Int_t nEta   = fEtaAxis->GetNbins();
         Int_t phiBin = Int_t(cellID/(nEta+2));
         Int_t etaBin = cellID - phiBin*(nEta+2);
         Float_t* v = fRebinData.GetSliceVals(cellID);

         printf(">> Selected cell in eta %f phi %f \n", fEtaAxis->GetBinCenter(etaBin), fPhiAxis->GetBinCenter(phiBin));

         if (fCells3D) {
            Int_t s = rec.GetItem(1);
            printf("Tower %d slice %d val %f \n", rec.GetItem(2), s, v[s]);
         }
         else
         {
            printf("Tower %d vals: \n", cellID);
            for (Int_t s = 0; s < fRebinData.fNSlices; s++) {
               printf("slice %d val %f\n", s, v[s]);
            }
         }
      }
   }
}
