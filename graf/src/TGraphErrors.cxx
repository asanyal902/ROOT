// @(#)root/graf:$Name:  $:$Id: TGraphErrors.cxx,v 1.5 2000/10/13 07:32:07 brun Exp $
// Author: Rene Brun   15/09/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <string.h>
#include <fstream.h>

#include "TROOT.h"
#include "TGraphErrors.h"
#include "TStyle.h"
#include "TMath.h"
#include "TVirtualPad.h"

ClassImp(TGraphErrors)

//______________________________________________________________________________
//
//   A TGraphErrors is a TGraph with error bars.
//   The various format options to draw a TGraphErrors are explained in
//     TGraphErrors::Paint.
//
//  The picture below has been generated by the following macro:
//------------------------------------------------------------------------
//{
//   gROOT->Reset();
//   c1 = new TCanvas("c1","A Simple Graph with error bars",200,10,700,500);
//
//   c1->SetFillColor(42);
//   c1->SetGrid();
//   c1->GetFrame()->SetFillColor(21);
//   c1->GetFrame()->SetBorderSize(12);
//
//   Int_t n = 10;
//   Double_t x[n]  = {-0.22, 0.05, 0.25, 0.35, 0.5, 0.61,0.7,0.85,0.89,0.95};
//   Double_t y[n]  = {1,2.9,5.6,7.4,9,9.6,8.7,6.3,4.5,1};
//   Double_t ex[n] = {.05,.1,.07,.07,.04,.05,.06,.07,.08,.05};
//   Double_t ey[n] = {.8,.7,.6,.5,.4,.4,.5,.6,.7,.8};
//   gr = new TGraphErrors(n,x,y,ex,ey);
//   gr->SetTitle("TGraphErrors Example");
//   gr->SetMarkerColor(4);
//   gr->SetMarkerStyle(21);
//   gr->Draw("ALP");
//
//   c1->Update();
//}
//Begin_Html
/*
<img src="gif/gerrors.gif">
*/
//End_Html
//

//______________________________________________________________________________
TGraphErrors::TGraphErrors(): TGraph()
{
//*-*-*-*-*-*-*-*-*-*-*TGraphErrors default constructor*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ================================
   fEX       = 0;
   fEY       = 0;
}


//______________________________________________________________________________
TGraphErrors::TGraphErrors(Int_t n)
             : TGraph(n)
{
//*-*-*-*-*-*-*-*-*-*-*TGraphErrors normal constructor*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ===============================
//
//  the arrays are preset to zero

   if (n <= 0) {
      Error("TGraphErrors", "illegal number of points (%d)", n);
      return;
   }

   fEX       = new Double_t[n];
   fEY       = new Double_t[n];

   for (Int_t i=0;i<n;i++) {
      fEX[i] = 0;
      fEY[i] = 0;
   }
}


//______________________________________________________________________________
TGraphErrors::TGraphErrors(Int_t n, Float_t *x, Float_t *y, Float_t *ex, Float_t *ey)
       : TGraph(n,x,y)
{
//*-*-*-*-*-*-*-*-*-*-*TGraphErrors normal constructor*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ===============================
//
//  if ex or ey are null, the corresponding arrays are preset to zero

   if (n <= 0) {
      Error("TGraphErrors", "illegal number of points (%d)", n);
      return;
   }

   fEX       = new Double_t[n];
   fEY       = new Double_t[n];

   for (Int_t i=0;i<n;i++) {
      if (ex) fEX[i] = ex[i];
      else    fEX[i] = 0;
      if (ey) fEY[i] = ey[i];
      else    fEY[i] = 0;
   }
}


//______________________________________________________________________________
TGraphErrors::TGraphErrors(Int_t n, Double_t *x, Double_t *y, Double_t *ex, Double_t *ey)
       : TGraph(n,x,y)
{
//*-*-*-*-*-*-*-*-*-*-*TGraphErrors normal constructor*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ===============================
//
//  if ex or ey are null, the corresponding arrays are preset to zero

   if (n <= 0) {
      Error("TGraphErrors", "illegal number of points (%d)", n);
      return;
   }

   fEX       = new Double_t[n];
   fEY       = new Double_t[n];

   for (Int_t i=0;i<n;i++) {
      if (ex) fEX[i] = ex[i];
      else    fEX[i] = 0;
      if (ey) fEY[i] = ey[i];
      else    fEY[i] = 0;
   }
}

//______________________________________________________________________________
TGraphErrors::~TGraphErrors()
{
//*-*-*-*-*-*-*-*-*-*-*TGraphErrors default destructor*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ===============================

   delete [] fEX;
   delete [] fEY;
}

//______________________________________________________________________________
void TGraphErrors::ComputeRange(Double_t &xmin, Double_t &ymin, Double_t &xmax, Double_t &ymax)
{
  for (Int_t i=0;i<fNpoints;i++) {
     if (fX[i] -fEX[i] < xmin) xmin = fX[i]-fEX[i];
     if (fX[i] +fEX[i] > xmax) xmax = fX[i]+fEX[i];
     if (fY[i] -fEY[i] < ymin) ymin = fY[i]-fEY[i];
     if (fY[i] +fEY[i] > ymax) ymax = fY[i]+fEY[i];
  }
}

//______________________________________________________________________________
Double_t TGraphErrors::GetErrorX(Int_t i)
{
//    This function is called by GraphFitChisquare.
//    It returns the error along X at point i.

   if (i < 0 || i >= fNpoints) return -1;
   if (fEX) return fEX[i];
   return -1;
}

//______________________________________________________________________________
Double_t TGraphErrors::GetErrorY(Int_t i)
{
//    This function is called by GraphFitChisquare.
//    It returns the error along Y at point i.

   if (i < 0 || i >= fNpoints) return -1;
   if (fEY) return fEY[i];
   return -1;
}

//______________________________________________________________________________
void TGraphErrors::Paint(Option_t *option)
{
   // Paint this TGraphErrors with its current attributes
   //
   // by default horizonthal and vertical small lines are drawn at
   // the end of the error bars. if option "z" or "Z" is specified,
   // these lines are not drawn.
   
   const Int_t BASEMARKER=8;
   Double_t s2x, s2y, symbolsize;
   Double_t x, y, ex, ey, xl1, xl2, xr1, xr2, yup1, yup2, ylow1, ylow2, tx, ty;
   static Float_t cxx[11] = {1,1,0.6,0.6,1,1,0.6,0.5,1,0.6,0.6};
   static Float_t cyy[11] = {1,1,1,1,1,1,1,1,1,0.5,0.6};

   Bool_t endLines = kTRUE;
   if (strchr(option,'z')) endLines = kFALSE;
   if (strchr(option,'Z')) endLines = kFALSE;

   TGraph::Paint(option);

   TAttLine::Modify();

   symbolsize  = GetMarkerSize();
   Int_t mark  = GetMarkerStyle();
   Double_t cx  = 0;
   Double_t cy  = 0;
   if (mark >= 20 && mark < 31) {
      cx = cxx[mark-20];
      cy = cyy[mark-20];
   }

//*-*-      define the offset of the error bars due to the symbol size
   s2x  = gPad->PixeltoX(Int_t(0.5*symbolsize*BASEMARKER)) - gPad->PixeltoX(0);
   s2y  =-gPad->PixeltoY(Int_t(0.5*symbolsize*BASEMARKER)) + gPad->PixeltoY(0);
   tx   = 0.50*s2x;
   ty   = 0.50*s2y;

   gPad->SetBit(kClipFrame, TestBit(kClipFrame));
   for (Int_t i=0;i<fNpoints;i++) {
      x  = gPad->XtoPad(fX[i]);
      y  = gPad->YtoPad(fY[i]);
      if (x < gPad->GetUxmin()) continue;
      if (x > gPad->GetUxmax()) continue;
      if (y < gPad->GetUymin()) continue;
      if (y > gPad->GetUymax()) continue;
      ex = fEX[i];
      ey = fEY[i];
      xl1 = x - s2x*cx;
      xl2 = gPad->XtoPad(fX[i] - ex);
      if (xl1 > xl2) {
         gPad->PaintLine(xl1,y,xl2,y);
         if (endLines) gPad->PaintLine(xl2,y-ty,xl2,y+ty);
      }
      xr1 = x + s2x*cx;
      xr2 = gPad->XtoPad(fX[i] + ex);
      if (xr1 < xr2) {
         gPad->PaintLine(xr1,y,xr2,y);
         if (endLines) gPad->PaintLine(xr2,y-ty,xr2,y+ty);
      }
      yup1 = y + s2y*cy;
      yup2 = gPad->YtoPad(fY[i] + ey);
      if (yup2 > gPad->GetUymax()) yup2 =  gPad->GetUymax();
      if (yup2 > yup1) {
         gPad->PaintLine(x,yup1,x,yup2);
         if (endLines) gPad->PaintLine(x-tx,yup2,x+tx,yup2);
      }
      ylow1 = y - s2y*cy;
      ylow2 = gPad->YtoPad(fY[i] - ey);
      if (ylow2 < gPad->GetUymin()) ylow2 =  gPad->GetUymin();
      if (ylow2 < ylow1) {
         gPad->PaintLine(x,ylow1,x,ylow2);
         if (endLines) gPad->PaintLine(x-tx,ylow2,x+tx,ylow2);
      }
   }
   gPad->ResetBit(kClipFrame);
}


//______________________________________________________________________________
void TGraphErrors::Print(Option_t *)
{
//*-*-*-*-*-*-*-*-*-*-*Print graph and errors values*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  =============================
//

   for (Int_t i=0;i<fNpoints;i++) {
      printf("x[%d]=%g, y[%d]=%g, ex[%d]=%g, ey[%d]=%g\n",i,fX[i],i,fY[i],i,fEX[i],i,fEY[i]);
   }
}

//______________________________________________________________________________
void TGraphErrors::SavePrimitive(ofstream &out, Option_t *option)
{
    // Save primitive as a C++ statement(s) on output stream out

   char quote = '"';
   out<<"   "<<endl;
   if (gROOT->ClassSaved(TGraphErrors::Class())) {
       out<<"   ";
   } else {
       out<<"   TGraphErrors *";
   }
   out<<"gre = new TGraphErrors("<<fNpoints<<");"<<endl;
   out<<"   gre->SetName("<<quote<<GetName()<<quote<<");"<<endl;
   out<<"   gre->SetTitle("<<quote<<GetTitle()<<quote<<");"<<endl;

   SaveFillAttributes(out,"gre",0,1001);
   SaveLineAttributes(out,"gre",1,1,1);
   SaveMarkerAttributes(out,"gre",1,1,1);

   for (Int_t i=0;i<fNpoints;i++) {
      out<<"   gre->SetPoint("<<i<<","<<fX[i]<<","<<fY[i]<<");"<<endl;
      out<<"   gre->SetPointError("<<i<<","<<fEX[i]<<","<<fEY[i]<<");"<<endl;
   }
   if (strstr(option,"multigraph")) {
      out<<"   multigraph->Add(gre);"<<endl;
      return;
   }
   out<<"   gre->Draw("
      <<quote<<option<<quote<<");"<<endl;
}

//______________________________________________________________________________
void TGraphErrors::Set(Int_t n)
{
// Set number of points in the graph
// Existing coordinates are preserved 
// New coordinates and errors above fNpoints are preset to 0.
   
   if (n < 0) n = 0;
   if (n == fNpoints) return;
   Double_t *x=0, *y=0, *ex=0, *ey=0;
   if (n > 0) {
      x  = new Double_t[n];
      y  = new Double_t[n];
      ex = new Double_t[n];
      ey = new Double_t[n];
   }
   Int_t i;
   for (i=0; i<fNpoints;i++) {
      if (fX)   x[i] = fX[i];
      if (fY)   y[i] = fY[i];
      if (fEX) ex[i] = fEX[i];
      if (fEY) ey[i] = fEY[i];
   }
   for (i=fNpoints; i<n;i++) {
      x[i]  = 0;
      y[i]  = 0;
      ex[i] = 0;
      ey[i] = 0;
   }
   delete [] fX;
   delete [] fY;
   delete [] fEX;
   delete [] fEY;
   fNpoints =n;
   fX  = x;
   fY  = y;
   fEX = ex;
   fEY = ey;
}

//______________________________________________________________________________
void TGraphErrors::SetPoint(Int_t i, Double_t x, Double_t y)
{
//*-*-*-*-*-*-*-*-*-*-*Set x and y values for point number i*-*-*-*-*-*-*-*-*
//*-*                  =====================================

   if (i < 0) return;
   if (i >= fNpoints) {
   // re-allocate the object
      Double_t *savex  = new Double_t[i+1];
      Double_t *savey  = new Double_t[i+1];
      Double_t *saveex = new Double_t[i+1];
      Double_t *saveey = new Double_t[i+1];
      if (fNpoints > 0) {
         memcpy(savex, fX, fNpoints*sizeof(Double_t));
         memcpy(savey, fY, fNpoints*sizeof(Double_t));
         memcpy(saveex,fEX,fNpoints*sizeof(Double_t));
         memcpy(saveey,fEY,fNpoints*sizeof(Double_t));
      }
      if (fX)  delete [] fX;
      if (fY)  delete [] fY;
      if (fEX) delete [] fEX;
      if (fEY) delete [] fEY;
      fX  = savex;
      fY  = savey;
      fEX = saveex;
      fEY = saveey;
      fNpoints = i+1;
   }
   fX[i] = x;
   fY[i] = y;
}

//______________________________________________________________________________
void TGraphErrors::SetPointError(Int_t i, Double_t ex, Double_t ey)
{
//*-*-*-*-*-*-*-*-*-*-*Set ex and ey values for point number i*-*-*-*-*-*-*-*
//*-*                  =======================================

   if (i < 0) return;
   if (i >= fNpoints) {
   // re-allocate the object
      TGraphErrors::SetPoint(i,0,0);
   }
   fEX[i] = ex;
   fEY[i] = ey;
}

//______________________________________________________________________________
void TGraphErrors::Streamer(TBuffer &b)
{
   // Stream an object of class TGraphErrors.

   if (b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = b.ReadVersion(&R__s, &R__c);
      if (R__v > 1) {
         TGraphErrors::Class()->ReadBuffer(b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      TGraph::Streamer(b);
      fEX = new Double_t[fNpoints];
      fEY = new Double_t[fNpoints];
      Float_t *ex = new Float_t[fNpoints];
      Float_t *ey = new Float_t[fNpoints];
      b.ReadFastArray(ex,fNpoints);
      b.ReadFastArray(ey,fNpoints);
      for (Int_t i=0;i<fNpoints;i++) {
         fEX[i] = ex[i];
         fEY[i] = ey[i];
      }
      delete [] ey;
      b.CheckByteCount(R__s, R__c, TGraphErrors::IsA());
      //====end of old versions
      
   } else {
      TGraphErrors::Class()->WriteBuffer(b,this);
   }
}
