// @(#)root/eve:$Id$
// Author: Alja Mrak-Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveCaloLegoOverlay.h"

#include "TAxis.h"

#include "TGLRnrCtx.h"
#include "TGLIncludes.h"
#include "TGLSelectRecord.h"
#include "TGLUtil.h"
#include "TGLCamera.h"
#include "TGLAxisPainter.h"

#include "TEveCalo.h"
#include "TEveCaloData.h"
#include <KeySymbols.h>


//______________________________________________________________________________
//
//
// GL-overaly control GUI for TEveCaloLego.
//
//

ClassImp(TEveCaloLegoOverlay);

//______________________________________________________________________________
TEveCaloLegoOverlay::TEveCaloLegoOverlay() :
   TGLCameraOverlay(),

   fCalo(0),

   fShowScales(kTRUE),
   fScaleColor(kWhite), fScaleTransparency(0),
   fScaleCoordX(0.8), fScaleCoordY(0.2),
   fCellX(-1), fCellY(-1),

   fFrameColor(kGray), fFrameLineTransp(0), fFrameBgTransp(90),

   fMouseX(0),  fMouseY(0),
   fInDrag(kFALSE),

   fHeaderSelected(kFALSE),

   fPlaneAxis(0), fAxisPlaneColor(kGray),
   fShowPlane(kFALSE),

   fMenuW(0.08),
   fButtonW(0.5),
   fShowSlider(kFALSE),
   fSliderH(0.6),
   fSliderPosY(0.15),
   fSliderVal(0),

   fActiveID(-1), fActiveCol(kRed-4)
{
   // Constructor.

   fPlaneAxis = new TAxis();
}

/******************************************************************************/
// Virtual event handlers from TGLOverlayElement
/******************************************************************************/

Bool_t TEveCaloLegoOverlay::SetSliderVal(Event_t* event, TGLRnrCtx &rnrCtx)
{
   // Set height of horizontal plane in the calorimeter.

   TGLRect& wprt = rnrCtx.RefCamera().RefViewport();
   fSliderVal = (1 -event->fY*1./wprt.Height() -fSliderPosY)/fSliderH;

   if (fSliderVal < 0 )
      fSliderVal = 0;
   else if (fSliderVal > 1)
      fSliderVal = 1;

   fCalo->SetHPlaneVal(fSliderVal);

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TEveCaloLegoOverlay::Handle(TGLRnrCtx          & rnrCtx,
                                   TGLOvlSelectRecord & selRec,
                                   Event_t            * event)
{
   // Handle overlay event.
   // Return TRUE if event was handled.

   if (selRec.GetN() < 2) return kFALSE;

   switch (event->fType)
   {
      case kButtonPress:
      {
         fMouseX = event->fX;
         fMouseY = event->fY;
         fInDrag = kTRUE;
         return kTRUE;
      }
      case kButtonRelease:
      {
         fInDrag = kFALSE;
         return kTRUE;
      }
      case kMotionNotify:
      {
         if (fInDrag)
         {
            const TGLRect& vp = rnrCtx.RefCamera().RefViewport();
            fScaleCoordX += (Float_t)(event->fX - fMouseX) / vp.Width();
            fScaleCoordY -= (Float_t)(event->fY - fMouseY) / vp.Height();
            fMouseX = event->fX;
            fMouseY = event->fY;
         }
         return kTRUE;
      }
      default:
      {
         return kFALSE;
      }
   }
}

//______________________________________________________________________________
Bool_t TEveCaloLegoOverlay::MouseEnter(TGLOvlSelectRecord& /*rec*/)
{
   // Mouse has entered overlay area.

   return kTRUE;
}

 //______________________________________________________________________________
void TEveCaloLegoOverlay::MouseLeave()
{
   // Mouse has left overlay area.

   fActiveID = -1;
}

//______________________________________________________________________________
void TEveCaloLegoOverlay::SetScaleColorTransparency(Color_t colIdx, UChar_t transp)
{
   // Set color and transparency of scales.

   fScaleColor = colIdx;
   fScaleTransparency = transp;
}

//______________________________________________________________________________
void TEveCaloLegoOverlay::SetScalePosition(Double_t x, Double_t y)
{
   // Set scale coordinates in range [0,1]. 

   fScaleCoordX = x;
   fScaleCoordY = y;
}

//______________________________________________________________________________
void TEveCaloLegoOverlay:: SetFrameAttribs(Color_t frameColor, UChar_t lineTransp, UChar_t bgTransp)
{
   // Set frame attribs.
   
   fFrameColor = frameColor;
   fFrameLineTransp = lineTransp;
   fFrameBgTransp = bgTransp;
}

//==============================================================================
void TEveCaloLegoOverlay::RenderHeader(TGLRnrCtx& rnrCtx)
{
   // Render text on top right corner of the screen.

   TGLRect &vp = rnrCtx.GetCamera()->RefViewport();

   TGLFont font;
   Int_t fs = TGLFontManager::GetFontSize(vp.Height()*0.035, 12, 36);
   rnrCtx.RegisterFont(fs, "arial", TGLFont::kPixmap, font);
   font.PreRender();
   Float_t off = fs*0.2;
   Float_t bb[6];
   font.BBox(fHeaderTxt.Data(), bb[0], bb[1], bb[2], bb[3], bb[4], bb[5]);
   Float_t x = vp.Width()  -bb[3] -off;
   Float_t y = vp.Height() -bb[4] -off;
   if (rnrCtx.Selection())
   {
      glPushName(0);
      glLoadName(3);
      glBegin(GL_QUADS);
      glVertex2f(x/vp.Width(), y/ vp.Height());
      glVertex2f(1,  y/ vp.Height());
      glVertex2f(1, 1);
      glVertex2f(x/vp.Width(), 1);
      glEnd();
      glPopName();
   }
   else
   {
      TGLUtil::Color(fHeaderSelected ? fActiveCol : fCalo->GetFontColor());
      glRasterPos2i(0, 0);
      glBitmap(0, 0, 0, 0, x, y, 0);
      font.Render(fHeaderTxt.Data());
   }
   font.PostRender();
}

//______________________________________________________________________________
void TEveCaloLegoOverlay::RenderPlaneInterface(TGLRnrCtx &rnrCtx)
{
   // Render menu for plane-value and the plane if marked.

   TGLCapabilitySwitch lights_off(GL_LIGHTING, kFALSE);
   glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_POINT_BIT);
   glEnable(GL_POINT_SMOOTH);
   glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(0.1, 1);
   glDisable(GL_CULL_FACE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glPushName(0);

   // move to the center of menu
   Double_t maxVal = fCalo->GetMaxVal();
   glScalef(fSliderH/(maxVal), fSliderH/maxVal, 1.);

   // button
   glPushMatrix();
   glTranslatef(0, (1-fButtonW )*fMenuW*0.8, 0);

   glLoadName(1);
   Float_t a=0.6;
   (fActiveID == 1) ? TGLUtil::Color(fActiveCol):TGLUtil::Color4f(0, 1, 0, a);
   Float_t bw = fButtonW*fMenuW*0.5;
   Float_t bwt = bw*0.8;
   Float_t bh = fButtonW*fMenuW;
   glBegin(GL_QUADS);
   glVertex2f(-bw, 0);
   glVertex2f( bw, 0);
   glVertex2f( bwt, bh);
   glVertex2f(-bwt, bh);
   glEnd();

   TGLUtil::Color(4);

   TGLUtil::LineWidth(1);
   glBegin(GL_LINES);
   glVertex2f(0, 0); glVertex2f(0, bh);
   glVertex2f((bw+bwt)*0.5, bh*0.5); glVertex2f(-(bw+bwt)*0.5, bh*0.5);
   glEnd();

   TGLUtil::LineWidth(2);
   glBegin(GL_LINE_LOOP);
   glVertex2f(-bw, 0);
   glVertex2f( bw, 0);
   glVertex2f( bwt, bh);
   glVertex2f(-bwt, bh);
   glEnd();
   TGLUtil::LineWidth(1);

   glTranslatef(0, fSliderPosY, 0.5);

   if (fShowSlider)
   {
      // event handler
      if (rnrCtx.Selection())
      {
         glLoadName(2);
         Float_t w = fButtonW*fMenuW*0.5f;
         glBegin(GL_QUADS);
         glVertex2f(-w, 0);
         glVertex2f( w, 0);
         glVertex2f( w, fSliderH);
         glVertex2f(-w, fSliderH);
         glEnd();
      }

      // slider axis
      glPushMatrix();
      fAxisPainter->SetLabelPixelFontSize(TMath::CeilNint(rnrCtx.GetCamera()->RefViewport().Height()*GetAttAxis()->GetLabelSize()));
      fAxisPainter->RefDir().Set(0, 1, 0);
      fAxisPainter->RefTMOff(0).Set(1, 0, 0);
      fAxisPainter->SetLabelAlign(TGLFont::kLeft);
      fPlaneAxis->SetRangeUser(0, maxVal);
      fPlaneAxis->SetLimits(0, maxVal);
      fPlaneAxis->SetNdivisions(710);
      fPlaneAxis->SetTickLength(0.02*maxVal);
      fPlaneAxis->SetLabelOffset(0.02*maxVal);
      fPlaneAxis->SetLabelSize(0.05);
      fPlaneAxis->SetAxisColor(fAxisPlaneColor);
      fPlaneAxis->SetLabelColor(fAxisPlaneColor);

      glPushMatrix();
      glScalef(fSliderH/(maxVal), fSliderH/maxVal, 1.);
      fAxisPainter->PaintAxis(rnrCtx, fPlaneAxis);
      glPopMatrix();

      // marker
      TGLUtil::Color((fActiveID == 2) ? fActiveCol : 3);
      TGLUtil::PointSize(8);
      glBegin(GL_POINTS);
      glVertex3f(0, fSliderVal*fSliderH, -0.1);
      glEnd();
   }

   glPopName();
   glPopAttrib();
   glPopMatrix();
}

/******************************************************************************/
void TEveCaloLegoOverlay::RenderScales(TGLRnrCtx& rnrCtx)
{
   // Draw slider of calo 2D.

   // scale position

   TGLRect &vp = rnrCtx.GetCamera()->RefViewport();

   Double_t maxVal = fCalo->GetMaxVal();
   Int_t maxe = TMath::CeilNint(TMath::Log10(maxVal+1)); // max round exponent
   Double_t sqv = TMath::Power(10, maxe)+1; // max square value
   Double_t fc = TMath::Log10(sqv)/TMath::Log10(fCalo->GetMaxVal()+1);
   Double_t cellX =  fCellX * fc;
   Double_t cellY =  fCellY * fc;

   Double_t scaleStepY = 0.1; // step is 10% of screen
   Double_t scaleStepX =  scaleStepY*vp.Height()/vp.Width(); // step is 10% of screen

   if (cellY < scaleStepY)
   {
      glPushMatrix();
      glTranslatef(fScaleCoordX, fScaleCoordY, 0); // translate to lower left corner

      glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_POINT_BIT);
      glEnable(GL_BLEND);
      glDisable(GL_CULL_FACE);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0.1, 1);

      // draw cells
      glBegin(GL_QUADS);
      TGLUtil::ColorTransparency(fScaleColor, fScaleTransparency);
      Int_t ne = 0;
      Float_t valFac, pos, dx, dy, z;
      for (Int_t i=0; i < 4; ++i)
      { 
         ne = i;
         valFac = TMath::Log10(TMath::Power(10, maxe-i)+1)/TMath::Log10(sqv);
         pos = i* scaleStepY;
         dx = 0.5* cellX * valFac;
         dy = 0.5* cellY * valFac;
         z = -0.1;
         if (dy*vp.Height() < 0.5)
         {
            break;
         }
         glVertex3f( - dx, pos - dy, z);
         glVertex3f( - dx, pos + dy, z);
         glVertex3f( + dx, pos + dy, z);
         glVertex3f( + dx, pos - dy, z);
      }
      glEnd();

      // draw numbers 
      TGLUtil::Color(fScaleColor);
      TGLFont fontB;
      Int_t fsb = TGLFontManager::GetFontSize(vp.Height()*0.03, 12, 36);
      rnrCtx.RegisterFont(fsb, "arial", TGLFont::kPixmap, fontB);
      TGLFont fontE;
      Int_t fsE = TGLFontManager::GetFontSize(vp.Height()*0.008, 8, 36);
      rnrCtx.RegisterFont(fsE, "arial", TGLFont::kPixmap, fontE);

      Float_t llx, lly, llz, urx, ury, urz;
      fontB.BBox("10", llx, lly, llz, urx, ury, urz);
      Float_t expX = urx/vp.Width();
      Float_t expY = (ury-lly)*0.5/vp.Height();
      Float_t expOff = 1;
      {
         Float_t x = 0.5*scaleStepX;
         Float_t z = -0.1;
         fontB.PreRender();
         fontE.PreRender();
         for (Int_t i = 0; i < ne; ++i)
         {
            fontB.RenderBitmap("10", x, i*scaleStepY, z, TGLFont::kLeft);
            if (i != maxe)
            {
               fontB.BBox(Form("%d",  maxe-i), llx, lly, llz, urx, ury, urz);
               if (expOff >  urx/vp.Width()) expOff = urx/vp.Width();
               fontE.RenderBitmap(Form("%d",  maxe-i), x+expX , i*scaleStepY+expY, z, TGLFont::kLeft );
            }
         }
         fontB.PostRender();
         fontE.PostRender();
         if (expOff < 1)  expX += expOff;
      }

      // draw frame
      {
         Double_t off = 0.1;
         Double_t x0 = -(0.5+off) * scaleStepX;
         Double_t x1 = (0.5+off) * scaleStepX + expX;
         Double_t y0 = -(0.5+off) * scaleStepY; 
         Double_t y1 = scaleStepY*(ne - 0.5 + off);
         Double_t z = -0.2;

         TGLUtil::ColorTransparency(fFrameColor, fFrameLineTransp);
         glBegin(GL_LINE_LOOP);
         glVertex3f(x0, y0, z); glVertex3f(x1, y0, z);
         glVertex3f(x1, y1, z); glVertex3f(x0, y1, z);
         glEnd();

         TGLUtil::ColorTransparency(fFrameColor, fFrameBgTransp);
         glPushName(0);
         glLoadName(1);
         glBegin(GL_QUADS);
         glVertex2f(x0, y0); glVertex2f(x1, y0);
         glVertex2f(x1, y1); glVertex2f(x0, y1);
         glEnd();
         glPopName();
      }
   
      glPopMatrix();
      glPopAttrib();
   }

} // end draw scales

/******************************************************************************/

void TEveCaloLegoOverlay::Render(TGLRnrCtx& rnrCtx)
{
   // Draw calorimeter scale info and plane interface.

   if ( fCalo == 0 || fCalo->GetData()->Empty()) return;

   Float_t old_depth_range[2];
   glGetFloatv(GL_DEPTH_RANGE, old_depth_range);
   glDepthRange(0, 0.001);

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   if (rnrCtx.Selection())
   {
      TGLRect rect(*rnrCtx.GetPickRectangle());
      rnrCtx.GetCamera()->WindowToViewport(rect);
      gluPickMatrix(rect.X(), rect.Y(), rect.Width(), rect.Height(),
                    (Int_t*) rnrCtx.GetCamera()->RefViewport().CArr());;
   }
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glTranslatef(-1, -1, 0);
   glScalef(2, 2, 1);

   TGLCamera& cam = rnrCtx.RefCamera();
   Bool_t drawOverlayAxis = kFALSE;

   if (cam.IsOrthographic() && fShowScales)
   {
      // in 2D need pixel cell dimension
      // project lego eta-phi boundraries
      TGLVector3 rng(fCalo->GetEtaRng(), fCalo->GetPhiRng(), 0);
      TGLVertex3 p;
      TGLVector3 res = cam.WorldDeltaToViewport(p, rng);

      // get smallest bin
      Double_t sq = 1e4;
      if (fCalo->fBinStep == 1)
      {
         TEveCaloData::CellData_t cellData;
         for ( TEveCaloData::vCellId_t::iterator i = fCalo->fCellList.begin(); i != fCalo->fCellList.end(); ++i)
         {
            fCalo->fData->GetCellData(*i, cellData);
            if (sq > cellData.EtaDelta()) sq = cellData.EtaDelta();
            if (sq > cellData.PhiDelta()) sq = cellData.PhiDelta();
         }
      }
      else
      {
         TAxis* a;
         Int_t nb;
         a = fCalo->GetData()->GetEtaBins();
         nb = a->GetNbins();
         for (Int_t i=1 ; i<=nb; i++)
         {
            if (sq > a->GetBinWidth(i)) sq = a->GetBinWidth(i);
         }

         a = fCalo->GetData()->GetPhiBins();
         nb = a->GetNbins();
         for (Int_t i=1 ; i<=nb; i++)
         {
            if (sq > a->GetBinWidth(i)) sq = a->GetBinWidth(i);
         }

         sq *= fCalo->fBinStep;
      }
      fCellX = (res.X()*sq)/(fCalo->GetEtaRng()*1.*cam.RefViewport().Width());
      fCellY = (res.Y()*sq)/(fCalo->GetPhiRng()*1.*cam.RefViewport().Height());
      // printf("bin width %f cells size %f %f\n", sq, fCellX, fCellY);

      RenderScales(rnrCtx);

      // draw camera overlay if projected lego bbox to large
      if (res.X() > cam.RefViewport().Width()*0.8 || res.Y() > cam.RefViewport().Height()*0.8)
         drawOverlayAxis = kTRUE;
   }

   if (cam.IsPerspective() && fShowPlane)
   {
      RenderPlaneInterface(rnrCtx);
   }

   // draw info text on yop right corner
   if (fHeaderTxt.Length())
   {
      RenderHeader(rnrCtx);
   }

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);

   glDepthRange(old_depth_range[0], old_depth_range[1]);

   if (drawOverlayAxis) TGLCameraOverlay::Render(rnrCtx);
}
