// @(#)root/hist:$Name:  $:$Id: TF2.cxx,v 1.2 2000/06/13 10:37:48 brun Exp $
// Author: Rene Brun   23/08/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TROOT.h"
#include "TF2.h"
#include "TMath.h"
#include "TRandom.h"
#include "TH2.h"
#include "TVirtualPad.h"
#include "TStyle.h"

ClassImp(TF2)

//______________________________________________________________________________
//
// a 2-Dim function with parameters
// TF2 graphics function is via the TH1 drawing functions.
//
//      Example of a function
//
//   TF2 *f2 = new TF2("f2","sin(x)*sin(y)/(x*y)",0,5,0,5);
//   f2->Draw();
//Begin_Html
/*
<img src="gif/function2.gif">
*/
//End_Html
//
//      See TF1 class for the list of functions formats
//

//______________________________________________________________________________
TF2::TF2(): TF1()
{
//*-*-*-*-*-*-*-*-*-*-*F2 default constructor*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ======================

}


//______________________________________________________________________________
TF2::TF2(const char *name,const char *formula, Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax)
      :TF1(name,formula,xmin,xmax)
{
//*-*-*-*-*-*-*F2 constructor using a formula definition*-*-*-*-*-*-*-*-*-*-*
//*-*          =========================================
//*-*
//*-*  See TFormula constructor for explanation of the formula syntax.
//*-*
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   fYmin   = ymin;
   fYmax   = ymax;
   fNpx    = 30;
   fNpy    = 30;
   fContour.Set(0);
}

//______________________________________________________________________________
TF2::TF2(const char *name, void *fcn, Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax, Int_t npar)
      : TF1(name, fcn, xmin, xmax, npar)
{
//*-*-*-*-*-*-*F2 constructor using a pointer to an interpreted function*-*-*
//*-*          =========================================================
//*-*
//*-*   npar is the number of free parameters used by the function
//*-*
//*-*  Creates a function of type C between xmin and xmax and ymin,ymax.
//*-*  The function is defined with npar parameters
//*-*  fcn must be a function of type:
//*-*     Double_t fcn(Double_t *x, Double_t *params)
//*-*
//*-*  This constructor is called for functions of type C by CINT.
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   fYmin   = ymin;
   fYmax   = ymax;
   fNpx    = 30;
   fNpy    = 30;
   fNdim   = 2;
   fContour.Set(0);

}

//______________________________________________________________________________
TF2::TF2(const char *name, Double_t (*fcn)(Double_t *, Double_t *), Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax, Int_t npar)
      : TF1(name, fcn, xmin, xmax, npar)
{
//*-*-*-*-*-*-*F2 constructor using a pointer to a compiled function*-*-*-*-*
//*-*          =====================================================
//*-*
//*-*   npar is the number of free parameters used by the function
//*-*
//*-*   This constructor creates a function of type C when invoked
//*-*   with the normal C++ compiler.
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   fYmin   = ymin;
   fYmax   = ymax;
   fNpx    = 30;
   fNpy    = 30;
   fNdim   = 2;
   fContour.Set(0);

}

//______________________________________________________________________________
TF2::~TF2()
{
//*-*-*-*-*-*-*-*-*-*-*F2 default destructor*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  =====================

}

//______________________________________________________________________________
TF2::TF2(const TF2 &f2)
{
   ((TF2&)f2).Copy(*this);
}

//______________________________________________________________________________
void TF2::Copy(TObject &obj)
{
//*-*-*-*-*-*-*-*-*-*-*Copy this F2 to a new F2*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ========================

   TF1::Copy(obj);
   ((TF2&)obj).fYmin    = fYmin;
   ((TF2&)obj).fYmax    = fYmax;
   ((TF2&)obj).fNpy     = fNpy;
   fContour.Copy(((TF2&)obj).fContour);
}

//______________________________________________________________________________
Int_t TF2::DistancetoPrimitive(Int_t px, Int_t py)
{
//*-*-*-*-*-*-*-*-*-*-*Compute distance from point px,py to a function*-*-*-*-*
//*-*                  ===============================================
//*-*  Compute the closest distance of approach from point px,py to this function.
//*-*  The distance is computed in pixels units.
//*-*
//*-*  Algorithm:
//*-*
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   return TF1::DistancetoPrimitive(px, py);

}

//______________________________________________________________________________
void TF2::Draw(Option_t *option)
{
//*-*-*-*-*-*-*-*-*-*-*Draw this function with its current attributes*-*-*-*-*
//*-*                  ==============================================
//*-* NB. You must use DrawCopy if you want to draw several times the same
//*-*     function in the current canvas.
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   TString opt = option;
   opt.ToLower();
   if (gPad && !opt.Contains("same")) gPad->Clear();

   AppendPad(option);
}

//______________________________________________________________________________
TF1 *TF2::DrawCopy(Option_t *option)
{
//*-*-*-*-*-*-*-*Draw a copy of this function with its current attributes*-*-*
//*-*            ========================================================
//*-*
//*-*  This function MUST be used instead of Draw when you want to draw
//*-*  the same function with different parameters settings in the same canvas.
//*-*
//*-* Possible option values are:
//*-*   "SAME"  superimpose on top of existing picture
//*-*   "L"     connect all computed points with a straight line
//*-*   "C"     connect all computed points with a smooth curve.
//*-*
//*-* Note that the default value is "F". Therefore to draw on top
//*-* of an existing picture, specify option "SL"
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   TF2 *newf2 = new TF2();
   Copy(*newf2);
   newf2->AppendPad(option);
   return newf2;
}

//______________________________________________________________________________
void TF2::DrawF2(const char *formula, Double_t xmin, Double_t ymin, Double_t xmax, Double_t ymax, Option_t *option)
{
//*-*-*-*-*-*-*-*-*-*Draw formula between xmin,ymin and xmax,ymax*-*-*-*-*-*-*-*
//*-*                ============================================
//*-*

   if (Compile((char*)formula)) return;

   SetRange(xmin, ymin, xmax, ymax);

   Draw(option);

}

//______________________________________________________________________________
void TF2::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
//*-*-*-*-*-*-*-*-*-*-*Execute action corresponding to one event*-*-*-*
//*-*                  =========================================
//*-*  This member function is called when a F2 is clicked with the locator
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   TF1::ExecuteEvent(event, px, py);
}

//______________________________________________________________________________
Int_t TF2::GetContour(Double_t *levels)
{
//*-*-*-*-*-*-*-*Return contour values into array levels*-*-*-*-*-*-*-*-*-*
//*-*            =======================================
//*-*
//*-*  The number of contour levels can be returned by getContourLevel
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Int_t nlevels = fContour.fN;
  if (levels) {
     for (Int_t level=0; level<nlevels; level++) levels[level] = GetContourLevel(level);
  }
  return nlevels;
}

//______________________________________________________________________________
Double_t TF2::GetContourLevel(Int_t level)
{
//*-*-*-*-*-*-*-*Return the number of contour levels*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*            ===================================
  if (level <0 || level >= fContour.fN) return 0;
  if (fContour.fArray[0] != -9999) return fContour.fArray[level];
  if (fHistogram == 0) return 0;
  return fHistogram->GetContourLevel(level);
}

//______________________________________________________________________________
char *TF2::GetObjectInfo(Int_t px, Int_t py)
{
//   Redefines TObject::GetObjectInfo.
//   Displays the function value
//   corresponding to cursor position px,py
//
   const char *snull = "";
   if (!gPad) return (char*)snull;
   static char info[64];
   Double_t x = gPad->PadtoX(gPad->AbsPixeltoX(px));
   Double_t y = gPad->PadtoY(gPad->AbsPixeltoY(py));
   const char *drawOption = GetDrawOption();
   Double_t uxmin,uxmax;
   Double_t uymin,uymax;
   if (gPad->GetView() || strncmp(drawOption,"cont",4) == 0
                       || strncmp(drawOption,"CONT",4) == 0) {
      uxmin=gPad->GetUxmin();
      uxmax=gPad->GetUxmax();
      x = fXmin +(fXmax-fXmin)*(x-uxmin)/(uxmax-uxmin);
      uymin=gPad->GetUymin();
      uymax=gPad->GetUymax();
      y = fYmin +(fYmax-fYmin)*(y-uymin)/(uymax-uymin);
   }
   sprintf(info,"(x=%g, y=%g, f=%.18g)",x,y,Eval(x,y));
   return info;
}

//______________________________________________________________________________
Double_t TF2::GetRandom()
{
//*-*-*-*-*-*Return a random number following this function shape*-*-*-*-*-*-*
//*-*        ====================================================
//*-*
   printf("GetRandom cannot be called for TF2/3, use GetRandom2/3 instead\n");
   return 0;  // not yet implemented
}

//______________________________________________________________________________
void TF2::GetRandom2(Double_t &xrandom, Double_t &yrandom)
{
//*-*-*-*-*-*Return 2 random numbers following this function shape*-*-*-*-*-*
//*-*        =====================================================
//*-*
//*-*   The distribution contained in this TF2 function is integrated
//*-*   over the cell contents.
//*-*   It is normalized to 1.
//*-*   Getting the two random numbers implies:
//*-*     - Generating a random number between 0 and 1 (say r1)
//*-*     - Look in which cell in the normalized integral r1 corresponds to
//*-*     - make a linear interpolation in the returned cell
//*-*

   //  Check if integral array must be build
   Int_t i,j,cell;
   Double_t dx   = (fXmax-fXmin)/fNpx;
   Double_t dy   = (fYmax-fYmin)/fNpy;
   Int_t ncells = fNpx*fNpy;
   if (fIntegral == 0) {
      fIntegral = new Double_t[ncells+1];
      fIntegral[0] = 0;
      Double_t integ;
      Int_t intNegative = 0;
      cell = 0;
      for (j=0;j<fNpy;j++) {
         for (i=0;i<fNpx;i++) {
            integ = Integral(fXmin+i*dx,fXmin+i*dx+dx,fYmin+j*dy,fYmin+j*dy+dy);
            if (integ < 0) {intNegative++; integ = -integ;}
            fIntegral[cell+1] = fIntegral[cell] + integ;
            cell++;
         }
      }
      if (intNegative > 0) {
         Warning("GetRandom2","function:%s has %d negative values: abs assumed",GetName(),intNegative);
      }
      if (fIntegral[ncells] == 0) {
         Error("GetRandom2","Integral of function is zero");
         return;
      }
      for (i=1;i<=ncells;i++) {  // normalize integral to 1
         fIntegral[i] /= fIntegral[ncells];
      }
   }

// return random numbers
   Double_t r,ddx,ddy,dxint;
   r     = gRandom->Rndm();
   cell  = TMath::BinarySearch(ncells,fIntegral,r);
   dxint = fIntegral[cell+1] - fIntegral[cell];
   if (dxint > 0) ddx = dx*(r - fIntegral[cell])/dxint;
   else           ddx = 0;
   ddy = dy*gRandom->Rndm();
   j   = cell/fNpx;
   i   = cell%fNpx;
   xrandom = fXmin +dx*i +ddx;
   yrandom = fYmin +dy*j +ddy;
}

//______________________________________________________________________________
void TF2::GetRange(Double_t &xmin, Double_t &ymin,  Double_t &xmax, Double_t &ymax)
{
//*-*-*-*-*-*-*-*-*-*-*Return range of a 2-D function*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ==============================

   xmin = fXmin;
   xmax = fXmax;
   ymin = fYmin;
   ymax = fYmax;
}

//______________________________________________________________________________
void TF2::GetRange(Double_t &xmin, Double_t &ymin, Double_t &zmin, Double_t &xmax, Double_t &ymax, Double_t &zmax)
{
//*-*-*-*-*-*-*-*-*-*-*Return range of function*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ========================

   xmin = fXmin;
   xmax = fXmax;
   ymin = fYmin;
   ymax = fYmax;
   zmin = 0;
   zmax = 0;
}

//______________________________________________________________________________
Double_t TF2::Integral(Double_t ax, Double_t bx, Double_t ay, Double_t by, Double_t epsilon)
{
// Return Integral of a 2d function in range [ax,bx],[ay,by]
//
   Double_t a[2], b[2];
   a[0] = ax;
   b[0] = bx;
   a[1] = ay;
   b[1] = by;
   Double_t relerr  = 0;
   Double_t result = IntegralMultiple(2,a,b,epsilon,relerr);
   return result;
}

//______________________________________________________________________________
void TF2::Paint(Option_t *option)
{
//*-*-*-*-*-*-*-*-*Paint this 2-D function with its current attributes*-*-*-*-*
//*-*              ===================================================

   Int_t i,j,bin;
   Double_t dx, dy;
   Double_t xv[2];

   TString opt = option;
   opt.ToLower();
   if (!opt.Contains("same")) gPad->Clear();

//*-*-  Create a temporary histogram and fill each channel with the function value
   if (!fHistogram) {
      fHistogram = new TH2F("Func",(char*)GetTitle(),fNpx,fXmin,fXmax,fNpy,fYmin,fYmax);
      if (!fHistogram) return;
      fHistogram->SetDirectory(0);
   }
   InitArgs(xv,fParams);
   dx = (fXmax - fXmin)/Double_t(fNpx);
   dy = (fYmax - fYmin)/Double_t(fNpy);
   for (i=1;i<=fNpx;i++) {
      xv[0] = fXmin + (Double_t(i) - 0.5)*dx;
      for (j=1;j<=fNpy;j++) {
         xv[1] = fYmin + (Double_t(j) - 0.5)*dy;
         bin   = j*(fNpx + 2) + i;
         fHistogram->SetBinContent(bin,EvalPar(xv,fParams));
      }
   }
   ((TH2F*)fHistogram)->Fill(fXmin-1,fYmin-1,0);  //This call to force fNentries non zero

//*-*- Copy Function attributes to histogram attributes
   Double_t *levels = fContour.GetArray();
   if (levels && levels[0] == -9999) levels = 0;
   fHistogram->SetMinimum(fMinimum);
   fHistogram->SetMaximum(fMaximum);
   fHistogram->SetContour(fContour.fN, levels);
   fHistogram->SetLineColor(GetLineColor());
   fHistogram->SetLineStyle(GetLineStyle());
   fHistogram->SetLineWidth(GetLineWidth());
   fHistogram->SetFillColor(GetFillColor());
   fHistogram->SetFillStyle(GetFillStyle());
   fHistogram->SetMarkerColor(GetMarkerColor());
   fHistogram->SetMarkerStyle(GetMarkerStyle());
   fHistogram->SetMarkerSize(GetMarkerSize());

//*-*-  Draw the histogram
   Int_t optStat = gStyle->GetOptStat();
   gStyle->SetOptStat(0);
   if (opt.Length() == 0)  fHistogram->Paint("cont3");
   else if (opt == "same") fHistogram->Paint("cont2same");
   else                    fHistogram->Paint(option);
   gStyle->SetOptStat(optStat);

}


//______________________________________________________________________________
void TF2::SetContour(Int_t  nlevels, Double_t *levels)
{
//*-*-*-*-*-*-*-*Set the number and values of contour levels*-*-*-*-*-*-*-*-*
//*-*            ===========================================
//
//  By default the number of contour levels is set to 20.
//
//  if argument levels = 0 or missing, equidistant contours are computed
//

  Int_t level;
  if (nlevels <=0 ) {
     fContour.Set(0);
     return;
  }
  fContour.Set(nlevels);

//*-*-  Contour levels are specified
  if (levels) {
     for (level=0; level<nlevels; level++) fContour.fArray[level] = levels[level];
  } else {
     fContour.fArray[0] = -9999; // means not defined at this point
  }
}


//______________________________________________________________________________
void TF2::SetContourLevel(Int_t level, Double_t value)
{
//*-*-*-*-*-*-*-*-*-*-*Set value for one contour level*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  ===============================
  if (level <0 || level >= fContour.fN) return;
  fContour.fArray[level] = value;
}

//______________________________________________________________________________
void TF2::SetNpy(Int_t npy)
{
//*-*-*-*-*-*-*-*Set the number of points used to draw the function*-*-*-*-*-*
//*-*            ==================================================

   if(npy > 4 && npy < 1000) fNpy = npy;
   Update();
}

//______________________________________________________________________________
void TF2::SetRange(Double_t xmin, Double_t ymin, Double_t xmax, Double_t ymax)
{
//*-*-*-*-*-*Initialize the upper and lower bounds to draw the function*-*-*-*
//*-*        ==========================================================

   fXmin = xmin;
   fXmax = xmax;
   fYmin = ymin;
   fYmax = ymax;
   Update();
}

//______________________________________________________________________________
void TF2::Streamer(TBuffer &R__b)
{
   // Stream an object of class TF2.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 3) {
         TF2::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      Int_t nlevels;
      TF1::Streamer(R__b);
      if (R__v < 3) {
         Float_t ymin,ymax;
         R__b >> ymin; fYmin = ymin;
         R__b >> ymax; fYmax = ymax;
      } else {
         R__b >> fYmin;
         R__b >> fYmax;
      }
      R__b >> fNpy;
      R__b >> nlevels;
      if (R__v < 3) {
         Float_t *contour = 0;
         Int_t n = R__b.ReadArray(contour);
         fContour.Set(n);
         for (Int_t i=0;i<n;i++) fContour.fArray[i] = contour[i];
         delete [] contour;
      } else {
         fContour.Streamer(R__b);
      }
      R__b.CheckByteCount(R__s, R__c, TF2::IsA());
      //====end of old versions
      
   } else {
      TF2::Class()->WriteBuffer(R__b,this);
   }
}
