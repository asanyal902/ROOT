// @(#)root/hist:$Name:  $:$Id: TH2.cxx,v 1.44 2003/07/02 21:18:21 brun Exp $
// Author: Rene Brun   26/12/94

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TROOT.h"
#include "TH2.h"
#include "TVirtualPad.h"
#include "TF2.h"
#include "TProfile.h"
#include "TRandom.h"
#include "TMatrix.h"
#include "TMatrixD.h"
#include "THLimitsFinder.h"

ClassImp(TH2)

//______________________________________________________________________________
//
// Service class for 2-Dim histogram classes
//
//  TH2C a 2-D histogram with one byte per cell (char)
//  TH2S a 2-D histogram with two bytes per cell (short integer)
//  TH2I a 2-D histogram with four bytes per cell (32 bits integer)
//  TH2F a 2-D histogram with four bytes per cell (float)
//  TH2D a 2-D histogram with eight bytes per cell (double)
//

//______________________________________________________________________________
TH2::TH2()
{
   fDimension   = 2;
   fScalefactor = 1;
   fTsumwy      = fTsumwy2 = fTsumwxy = 0;
}

//______________________________________________________________________________
TH2::TH2(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH1(name,title,nbinsx,xlow,xup)
{
   // see comments in the TH1 base class constructors
   
   fDimension   = 2;
   fScalefactor = 1;
   fTsumwy      = fTsumwy2 = fTsumwxy = 0;
   if (nbinsy <= 0) nbinsy = 1;
   fYaxis.Set(nbinsy,ylow,yup);
   fNcells      = (nbinsx+2)*(nbinsy+2);
}

//______________________________________________________________________________
TH2::TH2(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH1(name,title,nbinsx,xbins)
{
   // see comments in the TH1 base class constructors
   fDimension   = 2;
   fScalefactor = 1;
   fTsumwy      = fTsumwy2 = fTsumwxy = 0;
   if (nbinsy <= 0) nbinsy = 1;
   fYaxis.Set(nbinsy,ylow,yup);
   fNcells      = (nbinsx+2)*(nbinsy+2);
}

//______________________________________________________________________________
TH2::TH2(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,const Double_t *ybins)
     :TH1(name,title,nbinsx,xlow,xup)
{
   // see comments in the TH1 base class constructors
   fDimension   = 2;
   fScalefactor = 1;
   fTsumwy      = fTsumwy2 = fTsumwxy = 0;
   if (nbinsy <= 0) nbinsy = 1;
   if (ybins) fYaxis.Set(nbinsy,ybins);
   else       fYaxis.Set(nbinsy,0,1);
   fNcells      = (nbinsx+2)*(nbinsy+2);
}

//______________________________________________________________________________
TH2::TH2(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                           ,Int_t nbinsy,const Double_t *ybins)
     :TH1(name,title,nbinsx,xbins)
{
   // see comments in the TH1 base class constructors
   fDimension   = 2;
   fScalefactor = 1;
   fTsumwy      = fTsumwy2 = fTsumwxy = 0;
   if (nbinsy <= 0) nbinsy = 1;
   if (ybins) fYaxis.Set(nbinsy,ybins);
   else       fYaxis.Set(nbinsy,0,1);
   fNcells      = (nbinsx+2)*(nbinsy+2);
}

//______________________________________________________________________________
TH2::TH2(const char *name,const char *title,Int_t nbinsx,const Float_t *xbins
                                           ,Int_t nbinsy,const Float_t *ybins)
     :TH1(name,title,nbinsx,xbins)
{
   // see comments in the TH1 base class constructors
   fDimension   = 2;
   fScalefactor = 1;
   fTsumwy      = fTsumwy2 = fTsumwxy = 0;
   if (nbinsy <= 0) nbinsy = 1;
   if (ybins) fYaxis.Set(nbinsy,ybins);
   else       fYaxis.Set(nbinsy,0,1);
   fNcells      = (nbinsx+2)*(nbinsy+2);
}

//______________________________________________________________________________
TH2::~TH2()
{
}

//______________________________________________________________________________
Int_t TH2::BufferEmpty(Bool_t deleteBuffer)
{
// Fill histogram with all entries in the buffer.
// The buffer is deleted if deleteBuffer is true.

   // do we need to compute the bin size?
   Int_t nbentries = (Int_t)fBuffer[0];
   if (!nbentries) return 0;
   if (fXaxis.GetXmax() <= fXaxis.GetXmin() || fYaxis.GetXmax() <= fYaxis.GetXmin()) {
      //find min, max of entries in buffer
      Double_t xmin = fBuffer[2];
      Double_t xmax = xmin;
      Double_t ymin = fBuffer[3];
      Double_t ymax = ymin;
      for (Int_t i=1;i<nbentries;i++) {
         Double_t x = fBuffer[3*i+2];
         if (x < xmin) xmin = x;
         if (x > xmax) xmax = x;
         Double_t y = fBuffer[3*i+3];
         if (y < ymin) ymin = y;
         if (y > ymax) ymax = y;
      }
      THLimitsFinder::GetLimitsFinder()->FindGoodLimits(this,xmin,xmax,ymin,ymax);
   }
   Double_t *buffer = fBuffer; fBuffer = 0;
   
   for (Int_t i=0;i<nbentries;i++) {
      Fill(buffer[3*i+2],buffer[3*i+3],buffer[3*i+1]);
   }
   
   if (deleteBuffer) { delete buffer;    fBufferSize = 0;}
   else              { fBuffer = buffer; fBuffer[0] = 0;}
   return nbentries;
}
 
//______________________________________________________________________________
Int_t TH2::BufferFill(Axis_t x, Axis_t y, Stat_t w)
{
// accumulate arguments in buffer. When buffer is full, empty the buffer
// fBuffer[0] = number of entries in buffer
// fBuffer[1] = w of first entry
// fBuffer[2] = x of first entry
// fBuffer[3] = y of first entry

   Int_t nbentries = (Int_t)fBuffer[0];
   if (3*nbentries+3 >= fBufferSize) {
      BufferEmpty(kTRUE);
      return Fill(x,y,w);
   }
   fBuffer[3*nbentries+1] = w;
   fBuffer[3*nbentries+2] = x;
   fBuffer[3*nbentries+3] = y;
   fBuffer[0] += 1;
   return -3;
}

//______________________________________________________________________________
void TH2::Copy(TObject &obj) const
{
   TH1::Copy(obj);
   ((TH2&)obj).fScalefactor = fScalefactor;
   ((TH2&)obj).fTsumwy      = fTsumwy;
   ((TH2&)obj).fTsumwy2     = fTsumwy2;
   ((TH2&)obj).fTsumwxy     = fTsumwxy;
}

//______________________________________________________________________________
Int_t TH2::Fill(Axis_t x,Axis_t y)
{
//*-*-*-*-*-*-*-*-*-*-*Increment cell defined by x,y by 1*-*-*-*-*-*-*-*-*-*
//*-*                  ==================================
//*-*
//*-* if x or/and y is less than the low-edge of the corresponding axis first bin,
//*-*   the Underflow cell is incremented.
//*-* if x or/and y is greater than the upper edge of corresponding axis last bin,
//*-*   the Overflow cell is incremented.
//*-*
//*-* If the storage of the sum of squares of weights has been triggered,
//*-* via the function Sumw2, then the sum of the squares of weights is incremented
//*-* by 1in the cell corresponding to x,y.
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   if (fBuffer) BufferFill(x,y,1);
   
   Int_t binx, biny, bin;
   fEntries++;
   binx = fXaxis.FindBin(x);
   biny = fYaxis.FindBin(y);
   bin  = biny*(fXaxis.GetNbins()+2) + binx;
   AddBinContent(bin);
   if (fSumw2.fN) ++fSumw2.fArray[bin];
   if (binx == 0 || binx > fXaxis.GetNbins()) {
      if (!fgStatOverflows) return -1;
   }
   if (biny == 0 || biny > fYaxis.GetNbins()) {
      if (!fgStatOverflows) return -1;
   }
   ++fTsumw;
   ++fTsumw2;
   fTsumwx  += x;
   fTsumwx2 += x*x;
   fTsumwy  += y;
   fTsumwy2 += y*y;
   fTsumwxy += x*y;
   return bin;
}

//______________________________________________________________________________
Int_t TH2::Fill(Axis_t x, Axis_t y, Stat_t w)
{
//*-*-*-*-*-*-*-*-*-*-*Increment cell defined by x,y by a weight w*-*-*-*-*-*
//*-*                  ===========================================
//*-*
//*-* if x or/and y is less than the low-edge of the corresponding axis first bin,
//*-*   the Underflow cell is incremented.
//*-* if x or/and y is greater than the upper edge of corresponding axis last bin,
//*-*   the Overflow cell is incremented.
//*-*
//*-* If the storage of the sum of squares of weights has been triggered,
//*-* via the function Sumw2, then the sum of the squares of weights is incremented
//*-* by w^2 in the cell corresponding to x,y.
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   if (fBuffer) BufferFill(x,y,w);

   Int_t binx, biny, bin;
   fEntries++;
   binx = fXaxis.FindBin(x);
   biny = fYaxis.FindBin(y);
   bin  = biny*(fXaxis.GetNbins()+2) + binx;
   AddBinContent(bin,w);
   if (fSumw2.fN) fSumw2.fArray[bin] += w*w;
   if (binx == 0 || binx > fXaxis.GetNbins()) {
      if (!fgStatOverflows) return -1;
   }
   if (biny == 0 || biny > fYaxis.GetNbins()) {
      if (!fgStatOverflows) return -1;
   }      
   Stat_t z= (w > 0 ? w : -w);
   fTsumw   += z;
   fTsumw2  += z*z;
   fTsumwx  += z*x;
   fTsumwx2 += z*x*x;
   fTsumwy  += z*y;
   fTsumwy2 += z*y*y;
   fTsumwxy += z*x*y;
   return bin;
}

//______________________________________________________________________________
Int_t TH2::Fill(const char *namex, const char *namey, Stat_t w)
{
// Increment cell defined by namex,namey by a weight w
//
// if x or/and y is less than the low-edge of the corresponding axis first bin,
//   the Underflow cell is incremented.
// if x or/and y is greater than the upper edge of corresponding axis last bin,
//   the Overflow cell is incremented.
//
// If the storage of the sum of squares of weights has been triggered,
// via the function Sumw2, then the sum of the squares of weights is incremented
// by w^2 in the cell corresponding to x,y.
//

   Int_t binx, biny, bin;
   fEntries++;
   binx = fXaxis.FindBin(namex);
   biny = fYaxis.FindBin(namey);
   bin  = biny*(fXaxis.GetNbins()+2) + binx;
   AddBinContent(bin,w);
   if (fSumw2.fN) fSumw2.fArray[bin] += w*w;
   if (binx == 0 || binx > fXaxis.GetNbins()) return -1;
   if (biny == 0 || biny > fYaxis.GetNbins()) return -1;
   Axis_t x = fXaxis.GetBinCenter(binx);
   Axis_t y = fYaxis.GetBinCenter(biny);
   Stat_t z= (w > 0 ? w : -w);
   fTsumw   += z;
   fTsumw2  += z*z;
   fTsumwx  += z*x;
   fTsumwx2 += z*x*x;
   fTsumwy  += z*y;
   fTsumwy2 += z*y*y;
   fTsumwxy += z*x*y;
   return bin;
}

//______________________________________________________________________________
Int_t TH2::Fill(const char *namex, Axis_t y, Stat_t w)
{
// Increment cell defined by namex,y by a weight w
//
// if x or/and y is less than the low-edge of the corresponding axis first bin,
//   the Underflow cell is incremented.
// if x or/and y is greater than the upper edge of corresponding axis last bin,
//   the Overflow cell is incremented.
//
// If the storage of the sum of squares of weights has been triggered,
// via the function Sumw2, then the sum of the squares of weights is incremented
// by w^2 in the cell corresponding to x,y.
//

   Int_t binx, biny, bin;
   fEntries++;
   binx = fXaxis.FindBin(namex);
   biny = fYaxis.FindBin(y);
   bin  = biny*(fXaxis.GetNbins()+2) + binx;
   AddBinContent(bin,w);
   if (fSumw2.fN) fSumw2.fArray[bin] += w*w;
   if (binx == 0 || binx > fXaxis.GetNbins()) return -1;
   if (biny == 0 || biny > fYaxis.GetNbins()) {
      if (!fgStatOverflows) return -1;
   }
   Axis_t x = fXaxis.GetBinCenter(binx);
   Stat_t z= (w > 0 ? w : -w);
   fTsumw   += z;
   fTsumw2  += z*z;
   fTsumwx  += z*x;
   fTsumwx2 += z*x*x;
   fTsumwy  += z*y;
   fTsumwy2 += z*y*y;
   fTsumwxy += z*x*y;
   return bin;
}

//______________________________________________________________________________
Int_t TH2::Fill(Axis_t x, const char *namey, Stat_t w)
{
// Increment cell defined by x,namey by a weight w
//
// if x or/and y is less than the low-edge of the corresponding axis first bin,
//   the Underflow cell is incremented.
// if x or/and y is greater than the upper edge of corresponding axis last bin,
//   the Overflow cell is incremented.
//
// If the storage of the sum of squares of weights has been triggered,
// via the function Sumw2, then the sum of the squares of weights is incremented
// by w^2 in the cell corresponding to x,y.
//

   Int_t binx, biny, bin;
   fEntries++;
   binx = fXaxis.FindBin(x);
   biny = fYaxis.FindBin(namey);
   bin  = biny*(fXaxis.GetNbins()+2) + binx;
   AddBinContent(bin,w);
   if (fSumw2.fN) fSumw2.fArray[bin] += w*w;
   if (binx == 0 || binx > fXaxis.GetNbins()) {
      if (!fgStatOverflows) return -1;
   }
   if (biny == 0 || biny > fYaxis.GetNbins()) return -1;
   Axis_t y = fYaxis.GetBinCenter(biny);
   Stat_t z= (w > 0 ? w : -w);
   fTsumw   += z;
   fTsumw2  += z*z;
   fTsumwx  += z*x;
   fTsumwx2 += z*x*x;
   fTsumwy  += z*y;
   fTsumwy2 += z*y*y;
   fTsumwxy += z*x*y;
   return bin;
}

//______________________________________________________________________________
void TH2::FillN(Int_t ntimes, const Axis_t *x, const Axis_t *y, const Double_t *w, Int_t stride)
{
//*-*-*-*-*-*-*Fill a 2-D histogram with an array of values and weights*-*-*-*
//*-*          ========================================================
//*-*
//*-* ntimes:  number of entries in arrays x and w (array size must be ntimes*stride)
//*-* x:       array of x values to be histogrammed
//*-* y:       array of y values to be histogrammed
//*-* w:       array of weights
//*-* stride:  step size through arrays x, y and w
//*-*
//*-* If the storage of the sum of squares of weights has been triggered,
//*-* via the function Sumw2, then the sum of the squares of weights is incremented
//*-* by w[i]^2 in the cell corresponding to x[i],y[i].
//*-* if w is NULL each entry is assumed a weight=1
//*-*
//*-* NB: function only valid for a TH2x object
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Int_t binx, biny, bin, i;
   fEntries += ntimes;
   Double_t ww = 1;
   ntimes *= stride;
   for (i=0;i<ntimes;i+=stride) {
      binx = fXaxis.FindBin(x[i]);
      biny = fYaxis.FindBin(y[i]);
      bin  = biny*(fXaxis.GetNbins()+2) + binx;
      if (w) ww = w[i];
      AddBinContent(bin,ww);
      if (fSumw2.fN) fSumw2.fArray[bin] += ww*ww;
      if (binx == 0 || binx > fXaxis.GetNbins()) {
         if (!fgStatOverflows) continue;
      }
      if (biny == 0 || biny > fYaxis.GetNbins()) {
         if (!fgStatOverflows) continue;
      }
      Stat_t z= (ww > 0 ? ww : -ww);
      fTsumw   += z;
      fTsumw2  += z*z;
      fTsumwx  += z*x[i];
      fTsumwx2 += z*x[i]*x[i];
      fTsumwy  += z*y[i];
      fTsumwy2 += z*y[i]*y[i];
      fTsumwxy += z*x[i]*y[i];
   }
}

//______________________________________________________________________________
void TH2::FillRandom(const char *fname, Int_t ntimes)
{
//*-*-*-*-*-*-*Fill histogram following distribution in function fname*-*-*-*
//*-*          =======================================================
//*-*
//*-*   The distribution contained in the function fname (TF2) is integrated
//*-*   over the channel contents.
//*-*   It is normalized to 1.
//*-*   Getting one random number implies:
//*-*     - Generating a random number between 0 and 1 (say r1)
//*-*     - Look in which bin in the normalized integral r1 corresponds to
//*-*     - Fill histogram channel
//*-*   ntimes random numbers are generated
//*-*
//*-*  One can also call TF2::GetRandom2 to get a random variate from a function.
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*

   Int_t bin, binx, biny, ibin, loop;
   Double_t r1, x, y, xv[2];
//*-*- Search for fname in the list of ROOT defined functions
   TF1 *f1 = (TF1*)gROOT->GetFunction(fname);
   if (!f1) { Error("FillRandom", "Unknown function: %s",fname); return; }

//*-*- Allocate temporary space to store the integral and compute integral
   Int_t nbinsx = GetNbinsX();
   Int_t nbinsy = GetNbinsY();
   Int_t nbins  = nbinsx*nbinsy;

   Double_t *integral = new Double_t[nbins+1];
   ibin = 0;
   integral[ibin] = 0;
   for (biny=1;biny<=nbinsy;biny++) {
      xv[1] = fYaxis.GetBinCenter(biny);
      for (binx=1;binx<=nbinsx;binx++) {
         xv[0] = fXaxis.GetBinCenter(binx);
         ibin++;
         integral[ibin] = integral[ibin-1] + f1->Eval(xv[0],xv[1]);
      }
   }

//*-*- Normalize integral to 1
   if (integral[nbins] == 0 ) {
      Error("FillRandom", "Integral = zero"); return;
   }
   for (bin=1;bin<=nbins;bin++)  integral[bin] /= integral[nbins];

//*-*--------------Start main loop ntimes
   for (loop=0;loop<ntimes;loop++) {
      r1 = gRandom->Rndm(loop);
      ibin = TMath::BinarySearch(nbins,&integral[0],r1);
      biny = ibin/nbinsx;
      binx = 1 + ibin - nbinsx*biny;
      biny++;
      x    = fXaxis.GetBinCenter(binx);
      y    = fYaxis.GetBinCenter(biny);
      Fill(x,y, 1.);
  }
  delete [] integral;
}

//______________________________________________________________________________
void TH2::FillRandom(TH1 *h, Int_t ntimes)
{
//*-*-*-*-*-*-*Fill histogram following distribution in histogram h*-*-*-*
//*-*          ====================================================
//*-*
//*-*   The distribution contained in the histogram h (TH2) is integrated
//*-*   over the channel contents.
//*-*   It is normalized to 1.
//*-*   Getting one random number implies:
//*-*     - Generating a random number between 0 and 1 (say r1)
//*-*     - Look in which bin in the normalized integral r1 corresponds to
//*-*     - Fill histogram channel
//*-*   ntimes random numbers are generated
//*-*
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*

   if (!h) { Error("FillRandom", "Null histogram"); return; }
   if (fDimension != h->GetDimension()) {
      Error("FillRandom", "Histograms with different dimensions"); return;
   }

   if (h->ComputeIntegral() == 0) return;

   Int_t loop;
   Axis_t x,y;
   TH2 *h2 = (TH2*)h;
   for (loop=0;loop<ntimes;loop++) {
      h2->GetRandom2(x,y);
      Fill(x,y,1.);
   }
}


//______________________________________________________________________________
void TH2::FitSlicesX(TF1 *f1, Int_t binmin, Int_t binmax, Int_t cut, Option_t *option)
{
// Project slices along X in case of a 2-D histogram, then fit each slice
// with function f1 and make a histogram for each fit parameter
// Only bins along Y between binmin and binmax are considered.
// if f1=0, a gaussian is assumed
// Before invoking this function, one can set a subrange to be fitted along X
// via f1->SetRange(xmin,xmax)
// The argument option (default="QNR") can be used to change the fit options.
//     "Q" means Quiet mode
//     "N" means do not show the result of the fit
//     "R" means fit the function in the specified function range
//
// Note that the generated histograms are added to the list of objects
// in the current directory. It is the user's responsability to delete
// these histograms.
//
//  Example: Assume a 2-d histogram h2
//   Root > h2->FitSlicesX(); produces 4 TH1D histograms
//          with h2_0 containing parameter 0(Constant) for a Gaus fit
//                    of each bin in Y projected along X
//          with h2_1 containing parameter 1(Mean) for a gaus fit
//          with h2_2 containing parameter 2(RMS)  for a gaus fit
//          with h2_chi2 containing the chisquare/number of degrees of freedom for a gaus fit
//
//   Root > h2->FitSlicesX(0,15,22,10);
//          same as above, but only for bins 15 to 22 along Y
//          and only for bins in Y for which the corresponding projection
//          along X has more than cut bins filled.
//
//  NOTE: To access the generated histograms in the current directory, do eg:
//     TH1D *h2_1 = (TH1D*)gDirectory->Get("h2_1");

   Int_t nbins  = fYaxis.GetNbins();
   if (binmin < 1) binmin = 1;
   if (binmax > nbins) binmax = nbins;
   if (binmax < binmin) {binmin = 1; binmax = nbins;}

   //default is to fit with a gaussian
   if (f1 == 0) {
      f1 = (TF1*)gROOT->GetFunction("gaus");
      if (f1 == 0) f1 = new TF1("gaus","gaus",fXaxis.GetXmin(),fXaxis.GetXmax());
      else         f1->SetRange(fXaxis.GetXmin(),fXaxis.GetXmax());
   }
   Int_t npar = f1->GetNpar();
   Double_t *parsave = new Double_t[npar];
   f1->GetParameters(parsave);

   //Create one histogram for each function parameter
   Int_t ipar;
   char name[80], title[80];
   TH1D *hlist[25];
   const TArrayD *bins = fYaxis.GetXbins();
   for (ipar=0;ipar<npar;ipar++) {
      sprintf(name,"%s_%d",GetName(),ipar);
      sprintf(title,"Fitted value of par[%d]=%s",ipar,f1->GetParName(ipar));
      if (bins->fN == 0) {
         hlist[ipar] = new TH1D(name,title, nbins, fYaxis.GetXmin(), fYaxis.GetXmax());
      } else {
         hlist[ipar] = new TH1D(name,title, nbins,bins->fArray);
      }
      hlist[ipar]->GetXaxis()->SetTitle(fYaxis.GetTitle());
   }
   sprintf(name,"%s_chi2",GetName());
   TH1D *hchi2 = new TH1D(name,"chisquare", nbins, fYaxis.GetXmin(), fYaxis.GetXmax());
   hchi2->GetXaxis()->SetTitle(fYaxis.GetTitle());

   //Loop on all bins in Y, generate a projection along X
   Int_t bin;
   Int_t nentries;
   for (bin=binmin;bin<=binmax;bin++) {
      TH1D *hpx = ProjectionX("_temp",bin,bin,"e");
      if (hpx == 0) continue;
      nentries = Int_t(hpx->GetEntries());
      if (nentries == 0 || nentries < cut) {delete hpx; continue;}
      f1->SetParameters(parsave);
      hpx->Fit(f1,option);
      Int_t npfits = f1->GetNumberFitPoints();
      if (npfits > npar && npfits >= cut) {
         for (ipar=0;ipar<npar;ipar++) {
            hlist[ipar]->Fill(fYaxis.GetBinCenter(bin),f1->GetParameter(ipar));
            hlist[ipar]->SetBinError(bin,f1->GetParError(ipar));
         }
         hchi2->Fill(fYaxis.GetBinCenter(bin),f1->GetChisquare()/(npfits-npar));
      }
      delete hpx;
   }
   delete [] parsave;
}

//______________________________________________________________________________
void TH2::FitSlicesY(TF1 *f1, Int_t binmin, Int_t binmax, Int_t cut, Option_t *option)
{
// Project slices along Y in case of a 2-D histogram, then fit each slice
// with function f1 and make a histogram for each fit parameter
// Only bins along X between binmin and binmax are considered.
// if f1=0, a gaussian is assumed
// Before invoking this function, one can set a subrange to be fitted along Y
// via f1->SetRange(ymin,ymax)
// The argument option (default="QNR") can be used to change the fit options.
//     "Q" means Quiet mode
//     "N" means do not show the result of the fit
//     "R" means fit the function in the specified function range
//
// Note that the generated histograms are added to the list of objects
// in the current directory. It is the user's responsability to delete
// these histograms.
//
//  Example: Assume a 2-d histogram h2
//   Root > h2->FitSlicesY(); produces 4 TH1D histograms
//          with h2_0 containing parameter 0(Constant) for a Gaus fit
//                    of each bin in X projected along Y
//          with h2_1 containing parameter 1(Mean) for a gaus fit
//          with h2_2 containing parameter 2(RMS)  for a gaus fit
//          with h2_chi2 containing the chisquare/number of degrees of freedom for a gaus fit
//
//   Root > h2->FitSlicesY(0,15,22,10);
//          same as above, but only for bins 15 to 22 along X
//          and only for bins in X for which the corresponding projection
//          along Y has more than cut bins filled.
//
//  NOTE: To access the generated histograms in the current directory, do eg:
//     TH1D *h2_1 = (TH1D*)gDirectory->Get("h2_1");
//
// A complete example of this function is given in begin_html <a href="examples/fitslicesy.C.html">tutorial:fitslicesy.C</a> end_html
// with the following output:
//Begin_Html
/*
<img src="gif/fitslicesy.gif">
*/
//End_Html

   Int_t nbins  = fXaxis.GetNbins();
   if (binmin < 1) binmin = 1;
   if (binmax > nbins) binmax = nbins;
   if (binmax < binmin) {binmin = 1; binmax = nbins;}

   //default is to fit with a gaussian
   if (f1 == 0) {
      f1 = (TF1*)gROOT->GetFunction("gaus");
      if (f1 == 0) f1 = new TF1("gaus","gaus",fYaxis.GetXmin(),fYaxis.GetXmax());
      else         f1->SetRange(fYaxis.GetXmin(),fYaxis.GetXmax());
   }
   Int_t npar = f1->GetNpar();
   Double_t *parsave = new Double_t[npar];
   f1->GetParameters(parsave);

   //Create one histogram for each function parameter
   Int_t ipar;
   char name[80], title[80];
   TH1D *hlist[25];
   const TArrayD *bins = fXaxis.GetXbins();
   for (ipar=0;ipar<npar;ipar++) {
      sprintf(name,"%s_%d",GetName(),ipar);
      sprintf(title,"Fitted value of par[%d]=%s",ipar,f1->GetParName(ipar));
      if (bins->fN == 0) {
         hlist[ipar] = new TH1D(name,title, nbins, fXaxis.GetXmin(), fXaxis.GetXmax());
      } else {
         hlist[ipar] = new TH1D(name,title, nbins,bins->fArray);
      }
      hlist[ipar]->GetXaxis()->SetTitle(fXaxis.GetTitle());
   }
   sprintf(name,"%s_chi2",GetName());
   TH1D *hchi2 = new TH1D(name,"chisquare", nbins, fXaxis.GetXmin(), fXaxis.GetXmax());
   hchi2->GetXaxis()->SetTitle(fXaxis.GetTitle());

   //Loop on all bins in X, generate a projection along Y
   Int_t bin;
   Int_t nentries;
   for (bin=binmin;bin<=binmax;bin++) {
      TH1D *hpy = ProjectionY("_temp",bin,bin,"e");
      if (hpy == 0) continue;
      nentries = Int_t(hpy->GetEntries());
      if (nentries == 0 || nentries < cut) {delete hpy; continue;}
      f1->SetParameters(parsave);
      hpy->Fit(f1,option);
      Int_t npfits = f1->GetNumberFitPoints();
      if (npfits > npar && npfits >= cut) {
         for (ipar=0;ipar<npar;ipar++) {
            hlist[ipar]->Fill(fXaxis.GetBinCenter(bin),f1->GetParameter(ipar));
            hlist[ipar]->SetBinError(bin,f1->GetParError(ipar));
         }
         hchi2->Fill(fXaxis.GetBinCenter(bin),f1->GetChisquare()/(npfits-npar));
      }
      delete hpy;
   }
   delete [] parsave;
}

//______________________________________________________________________________
Stat_t TH2::GetCorrelationFactor(Int_t axis1, Int_t axis2) const
{
//*-*-*-*-*-*-*-*Return correlation factor between axis1 and axis2*-*-*-*-*
//*-*            ====================================================
  if (axis1 < 1 || axis2 < 1 || axis1 > 2 || axis2 > 2) {
     Error("GetCorrelationFactor","Wrong parameters");
     return 0;
  }
  if (axis1 == axis2) return 1;
  Stat_t rms1 = GetRMS(axis1);
  if (rms1 == 0) return 0;
  Stat_t rms2 = GetRMS(axis2);
  if (rms2 == 0) return 0;
  return GetCovariance(axis1,axis2)/rms1/rms2;
}

//______________________________________________________________________________
Stat_t TH2::GetCovariance(Int_t axis1, Int_t axis2) const
{
//*-*-*-*-*-*-*-*Return covariance between axis1 and axis2*-*-*-*-*
//*-*            ====================================================

  if (axis1 < 1 || axis2 < 1 || axis1 > 2 || axis2 > 2) {
     Error("GetCovariance","Wrong parameters");
     return 0;
  }
  Stat_t stats[7];
  GetStats(stats);
  Stat_t sumw   = stats[0];
  Stat_t sumw2  = stats[1];
  Stat_t sumwx  = stats[2];
  Stat_t sumwx2 = stats[3];
  Stat_t sumwy  = stats[4];
  Stat_t sumwy2 = stats[5];
  Stat_t sumwxy = stats[6];

  if (sumw == 0) return 0;
  if (axis1 == 1 && axis2 == 1) {
     return TMath::Abs(sumwx2/sumw - sumwx*sumwx/sumw2);
  }
  if (axis1 == 2 && axis2 == 2) {
     return TMath::Abs(sumwy2/sumw - sumwy*sumwy/sumw2);
  }
  return sumwxy/sumw - sumwx/sumw*sumwy/sumw;
}

//______________________________________________________________________________
void TH2::GetRandom2(Axis_t &x, Axis_t &y)
{
// return 2 random numbers along axis x and y distributed according
// the cellcontents of a 2-dim histogram

   Int_t nbinsx = GetNbinsX();
   Int_t nbinsy = GetNbinsY();
   Int_t nbins  = nbinsx*nbinsy;
   Double_t integral;
   if (fIntegral) {
      if (fIntegral[nbins+1] != fEntries) integral = ComputeIntegral();
   } else {
      integral = ComputeIntegral();
      if (integral == 0 || fIntegral == 0) return;
   }
   Float_t r1 = gRandom->Rndm();
   Int_t ibin = TMath::BinarySearch(nbins,fIntegral,r1);
   Int_t biny = ibin/nbinsx;
   Int_t binx = ibin - nbinsx*biny;
   x = fXaxis.GetBinLowEdge(binx+1)
      +fXaxis.GetBinWidth(binx+1)*(r1-fIntegral[ibin])/(fIntegral[ibin+1] - fIntegral[ibin]);
   y = fYaxis.GetBinLowEdge(biny+1) + fYaxis.GetBinWidth(biny+1)*gRandom->Rndm();
}

//______________________________________________________________________________
void TH2::GetStats(Stat_t *stats) const
{
   // fill the array stats from the contents of this histogram
   // The array stats must be correctly dimensionned in the calling program.
   // stats[0] = sumw
   // stats[1] = sumw2
   // stats[2] = sumwx
   // stats[3] = sumwx2
   // stats[4] = sumwy
   // stats[5] = sumwy2
   // stats[6] = sumwxy

   if (fBuffer) ((TH2*)this)->BufferEmpty();
   
   Int_t bin, binx, biny;
   Stat_t w;
   Float_t x,y;
   if (fTsumw == 0 || fXaxis.TestBit(TAxis::kAxisRange) || fYaxis.TestBit(TAxis::kAxisRange)) {
      for (bin=0;bin<7;bin++) stats[bin] = 0;
      for (biny=fYaxis.GetFirst();biny<=fYaxis.GetLast();biny++) {
         y = fYaxis.GetBinCenter(biny);
         for (binx=fXaxis.GetFirst();binx<=fXaxis.GetLast();binx++) {
            bin = GetBin(binx,biny);
            x = fXaxis.GetBinCenter(binx);
            w = TMath::Abs(GetBinContent(bin));
            stats[0] += w;
            stats[1] += w*w;
            stats[2] += w*x;
            stats[3] += w*x*x;
            stats[4] += w*y;
            stats[5] += w*y*y;
            stats[6] += w*x*y;
         }
      }
   } else {
      stats[0] = fTsumw;
      stats[1] = fTsumw2;
      stats[2] = fTsumwx;
      stats[3] = fTsumwx2;
      stats[4] = fTsumwy;
      stats[5] = fTsumwy2;
      stats[6] = fTsumwxy;
   }
}

//______________________________________________________________________________
Stat_t TH2::Integral(Option_t *option) const
{
//Return integral of bin contents. Only bins in the bins range are considered.
// By default the integral is computed as the sum of bin contents in the range.
// if option "width" is specified, the integral is the sum of
// the bin contents multiplied by the bin width in x and in y.

   return Integral(fXaxis.GetFirst(),fXaxis.GetLast(),
                   fYaxis.GetFirst(),fYaxis.GetLast(),option);
}

//______________________________________________________________________________
Stat_t TH2::Integral(Int_t binx1, Int_t binx2, Int_t biny1, Int_t biny2, Option_t *option) const
{
//Return integral of bin contents in range [binx1,binx2],[biny1,biny2]
// for a 2-D histogram
// By default the integral is computed as the sum of bin contents in the range.
// if option "width" is specified, the integral is the sum of
// the bin contents multiplied by the bin width in x and in y.

   if (fBuffer) ((TH2*)this)->BufferEmpty();
   
   Int_t nbinsx = GetNbinsX();
   Int_t nbinsy = GetNbinsY();
   if (binx1 < 0) binx1 = 0;
   if (binx2 > nbinsx+1) binx2 = nbinsx+1;
   if (binx2 < binx1)    binx2 = nbinsx;
   if (biny1 < 0) biny1 = 0;
   if (biny2 > nbinsy+1) biny2 = nbinsy+1;
   if (biny2 < biny1)    biny2 = nbinsy;
   Stat_t integral = 0;

//*-*- Loop on bins in specified range
   TString opt = option;
   opt.ToLower();
   Bool_t width = kFALSE;
   if (opt.Contains("width")) width = kTRUE;
   Int_t bin, binx, biny;
   for (biny=biny1;biny<=biny2;biny++) {
      for (binx=binx1;binx<=binx2;binx++) {
         bin = binx +(nbinsx+2)*biny;
         if (width) integral += GetBinContent(bin)*fXaxis.GetBinWidth(binx)*fYaxis.GetBinWidth(biny);
         else       integral += GetBinContent(bin);
      }
   }
   return integral;
}
        
//______________________________________________________________________________
Double_t TH2::KolmogorovTest(TH1 *h2, Option_t *option) const
{
//  Statistical test of compatibility in shape between
//  THIS histogram and h2, using Kolmogorov test.
//     Default: Ignore under- and overflow bins in comparison
//
//     option is a character string to specify options
//         "U" include Underflows in test
//         "O" include Overflows 
//         "N" include comparison of normalizations
//         "D" Put out a line of "Debug" printout
//
//   The returned function value is the probability of test
//       (much less than one means NOT compatible)
//
//  Code adapted by Rene Brun from original HBOOK routine HDIFF

   TString opt = option;
   opt.ToUpper();
   
   Double_t prb = 0;
   TH1 *h1 = (TH1*)this;
   if (h2 == 0) return 0;
   TAxis *xaxis1 = h1->GetXaxis();
   TAxis *xaxis2 = h2->GetXaxis();
   TAxis *yaxis1 = h1->GetYaxis();
   TAxis *yaxis2 = h2->GetYaxis();
   Int_t ncx1   = xaxis1->GetNbins();
   Int_t ncx2   = xaxis2->GetNbins();
   Int_t ncy1   = yaxis1->GetNbins();
   Int_t ncy2   = yaxis2->GetNbins();

     // Check consistency of dimensions
   if (h1->GetDimension() != 2 || h2->GetDimension() != 2) {
      Error("KolmogorovTest","Histograms must be 2-D\n");
      return 0;
   }

     // Check consistency in number of channels
   if (ncx1 != ncx2) {
      Error("KolmogorovTest","Number of channels in X is different, %d and %d\n",ncx1,ncx2);
      return 0;
   }
   if (ncy1 != ncy2) {
      Error("KolmogorovTest","Number of channels in Y is different, %d and %d\n",ncy1,ncy2);
      return 0;
   }
    
     // Check consistency in channel edges
   Bool_t afunc1 = kFALSE;
   Bool_t afunc2 = kFALSE;
   Double_t difprec = 1e-5;
   Double_t diff1 = TMath::Abs(xaxis1->GetXmin() - xaxis2->GetXmin());
   Double_t diff2 = TMath::Abs(xaxis1->GetXmax() - xaxis2->GetXmax());
   if (diff1 > difprec || diff2 > difprec) {
      Error("KolmogorovTest","histograms with different binning along X");
      return 0;
   }
   diff1 = TMath::Abs(yaxis1->GetXmin() - yaxis2->GetXmin());
   diff2 = TMath::Abs(yaxis1->GetXmax() - yaxis2->GetXmax());
   if (diff1 > difprec || diff2 > difprec) {
      Error("KolmogorovTest","histograms with different binning along Y");
      return 0;
   }

   //   Should we include Uflows, Oflows?
   Int_t ibeg = 1, jbeg = 1;
   Int_t iend = ncx1, jend = ncy1;
   if (opt.Contains("U")) {ibeg = 0; jbeg = 0;}
   if (opt.Contains("O")) {iend = ncx1+1; jend = ncy1+1;}
   
   Int_t i,j;
   Double_t hsav;
   Double_t sum1  = 0;
   Double_t tsum1 = 0;
   for (i=0;i<=ncx1+1;i++) {
      for (j=0;j<=ncy1+1;j++) {
         hsav = h1->GetCellContent(i,j);
         tsum1 += hsav;
         if (i >= ibeg && i <= iend && j >= jbeg && j <= jend) sum1 += hsav;
      }
   }
   Double_t sum2  = 0;
   Double_t tsum2 = 0;
   for (i=0;i<=ncx1+1;i++) {
      for (j=0;j<=ncy1+1;j++) {
         hsav = h2->GetCellContent(i,j);
         tsum2 += hsav;
         if (i >= ibeg && i <= iend && j >= jbeg && j <= jend) sum2 += hsav;
      }
   }

   //    Check that both scatterplots contain events
   if (sum1 == 0) {
      Error("KolmogorovTest","Integral is zero for h1=%s\n",h1->GetName());
      return 0;
   }
   if (sum2 == 0) {
      Error("KolmogorovTest","Integral is zero for h2=%s\n",h2->GetName());
      return 0;
   }

   //    Check that scatterplots are not weighted or saturated
   Double_t num1 = h1->GetEntries();
   Double_t num2 = h2->GetEntries();
   if (num1 != tsum1) {
      Warning("KolmogorovTest","Saturation or weighted events for h1=%s, num1=%g, tsum1=%g\n",h1->GetName(),num1,tsum1);
   }
   if (num2 != tsum2) {
      Warning("KolmogorovTest","Saturation or weighted events for h2=%s, num2=%g, tsum2=%g\n",h2->GetName(),num2,tsum2);
   }

   //   Find first Kolmogorov distance
   Double_t s1 = 1/sum1;
   Double_t s2 = 1/sum2;
   Double_t dfmax = 0;
   Double_t rsum1=0, rsum2=0;
   for (i=ibeg;i<=iend;i++) {
      for (j=jbeg;j<=jend;j++) {
         rsum1 += s1*h1->GetCellContent(i,j);
         rsum2 += s2*h2->GetCellContent(i,j);
         dfmax  = TMath::Max(dfmax, TMath::Abs(rsum1-rsum2));
      }
   }

   //   Find second Kolmogorov distance 
   Double_t dfmax2 = 0;
   rsum1=0, rsum2=0;
   for (j=jbeg;j<=jend;j++) {
      for (i=ibeg;i<=iend;i++) {
         rsum1 += s1*h1->GetCellContent(i,j);
         rsum2 += s2*h2->GetCellContent(i,j);
         dfmax2 = TMath::Max(dfmax2, TMath::Abs(rsum1-rsum2));
      }
   }

   //    Get Kolmogorov probability
   Double_t factnm;
   if (afunc1)      factnm = TMath::Sqrt(sum2);
   else if (afunc2) factnm = TMath::Sqrt(sum1);
   else             factnm = TMath::Sqrt(sum1*sum2/(sum1+sum2));
   Double_t z  = dfmax*factnm;
   Double_t z2 = dfmax2*factnm;
   
   prb = TMath::KolmogorovProb(0.5*(z+z2));

   Double_t prb1=0, prb2=0;
   Double_t resum1, resum2, chi2, d12;
   if (opt.Contains("N")) { //Combine probabilities for shape and normalization,
      prb1   = prb;
      resum1 = sum1; if (afunc1) resum1 = 0;
      resum2 = sum2; if (afunc2) resum2 = 0;
      d12    = sum1-sum2;
      chi2   = d12*d12/(resum1+resum2);
      prb2   = TMath::Prob(chi2,1);
      //     see Eadie et al., section 11.6.2
      if (prb > 0 && prb2 > 0) prb = prb*prb2*(1-TMath::Log(prb*prb2));
      else                     prb = 0;
   }

   //    debug printout
   if (opt.Contains("D")) {
      printf(" Kolmo Prob  h1 = %s, sum1=%g\n",h1->GetName(),sum1);
      printf(" Kolmo Prob  h2 = %s, sum2=%g\n",h2->GetName(),sum2);
      printf(" Kolmo Probabil = %f, Max Dist = %g\n",prb,dfmax);
      if (opt.Contains("N")) 
      printf(" Kolmo Probabil = %f for shape alone, =%f for normalisation alone\n",prb1,prb2);
   }
      // This numerical error condition should never occur:
   if (TMath::Abs(rsum1-1) > 0.002) Warning("KolmogorovTest","Numerical problems with h1=%s\n",h1->GetName());
   if (TMath::Abs(rsum2-1) > 0.002) Warning("KolmogorovTest","Numerical problems with h2=%s\n",h2->GetName());

   return prb;
}   
   
//______________________________________________________________________________
Int_t TH2::Merge(TCollection *list)
{
   //Add all histograms in the collection to this histogram.
   //This function computes the min/max for the axes,
   //compute a new number of bins, if necessary,
   //add bin contents, errors and statistics.
   //The function returns the merged number of entries if the merge is 
   //successfull, -1 otherwise.
   //
   //IMPORTANT remark. The 2 axis x and y may have different number
   //of bins and different limits, BUT the largest bin width must be
   //a multiple of the smallest bin width.
   
   if (!list) return 0;
   TIter next(list);
   Double_t umin,umax,vmin,vmax;
   Int_t nx,ny;
   Double_t xmin  = fXaxis.GetXmin();
   Double_t xmax  = fXaxis.GetXmax();
   Double_t ymin  = fYaxis.GetXmin();
   Double_t ymax  = fYaxis.GetXmax();
   Double_t bwix  = fXaxis.GetBinWidth(1);
   Double_t bwiy  = fYaxis.GetBinWidth(1);
   Int_t    nbix  = fXaxis.GetNbins();
   Int_t    nbiy  = fYaxis.GetNbins();

   const Int_t kNstat = 7;
   Stat_t stats[kNstat], totstats[kNstat];
   TH2 *h;
   Int_t i, nentries=(Int_t)fEntries;
   for (i=0;i<kNstat;i++) {totstats[i] = stats[i] = 0;}
   GetStats(totstats);
   Bool_t same = kTRUE;
   while ((h=(TH2*)next())) {     
      if (!h->InheritsFrom(TH2::Class())) {
         Error("Add","Attempt to add object of class: %s to a %s",h->ClassName(),this->ClassName());
         return -1;
      }
      //import statistics
      h->GetStats(stats);
      for (i=0;i<kNstat;i++) totstats[i] += stats[i];
      nentries += (Int_t)h->GetEntries();
      
      // find min/max of the axes
      umin = h->GetXaxis()->GetXmin();
      umax = h->GetXaxis()->GetXmax();
      vmin = h->GetYaxis()->GetXmin();
      vmax = h->GetYaxis()->GetXmax();
      nx   = h->GetXaxis()->GetNbins();
      ny   = h->GetYaxis()->GetNbins();
      if (nx != nbix || ny != nbiy ||
              umin != xmin || umax != xmax || vmin != ymin || vmax != ymax) {
         same = kFALSE;
         if (umin < xmin) xmin = umin;  
         if (umax > xmax) xmax = umax;  
         if (vmin < ymin) ymin = vmin;  
         if (vmax > ymax) ymax = vmax;  
         if (h->GetXaxis()->GetBinWidth(1) > bwix) bwix = h->GetXaxis()->GetBinWidth(1);     
         if (h->GetYaxis()->GetBinWidth(1) > bwiy) bwiy = h->GetYaxis()->GetBinWidth(1);     
      }
   }
   
   //  if different binning compute best binning
   if (!same) {
      nbix = (Int_t) ((xmax-xmin)/bwix +0.1); while(nbix > 100) nbix /= 2;
      nbiy = (Int_t) ((ymax-ymin)/bwiy +0.1); while(nbiy > 100) nbiy /= 2;
      SetBins(nbix,xmin,xmax,nbiy,ymin,ymax);
   }
   
   //merge bin contents and errors
   next.Reset();
   Int_t ibin, bin, binx, biny, ix, iy;
   Double_t cu;
   while ((h=(TH2*)next())) {     
      nx   = h->GetXaxis()->GetNbins();
      ny   = h->GetYaxis()->GetNbins();
      for (biny=0;biny<=ny+1;biny++) {
         iy = fYaxis.FindBin(h->GetYaxis()->GetBinCenter(biny));
         for (binx=0;binx<=nx+1;binx++) {
            ix = fXaxis.FindBin(h->GetXaxis()->GetBinCenter(binx));
            bin = binx +(nx+2)*biny;
            ibin = ix +(nbix+2)*iy;
            cu  = h->GetBinContent(bin);
            AddBinContent(ibin,cu);
            if (fSumw2.fN) {
               Double_t error1 = h->GetBinError(bin);
               fSumw2.fArray[ibin] += error1*error1;
            }
         }
      }
   }
   
   //copy merged stats
   PutStats(totstats);
   SetEntries(nentries);
   
   return nentries;
}   
   
//______________________________________________________________________________
TProfile *TH2::ProfileX(const char *name, Int_t firstybin, Int_t lastybin, Option_t *option) const
{
//*-*-*-*-*Project a 2-D histogram into a profile histogram along X*-*-*-*-*-*
//*-*      ========================================================
//
//   The projection is made from the channels along the Y axis
//   ranging from firstybin to lastybin included.
//   By default, bins 1 to ny are included
//   When all bins are included, the number of entries in the projection
//   is set to the number of entries of the 2-D histogram, otherwise
//   the number of entries is incremented by 1 for all non empty cells.
//
//   if option "d" is specified, the profile is drawn in the current pad.
//
//   NOTE that if a TProfile named name exists in the current directory or pad,
//   the histogram is reset and filled again with the current contents of the TH2.

  TString opt = option;
  opt.ToLower();
  Int_t nx = fXaxis.GetNbins();
  Int_t ny = fYaxis.GetNbins();
  if (firstybin < 0) firstybin = 1;
  if (lastybin  < 0) lastybin  = ny;
  if (lastybin  > ny+1) lastybin  = ny;

// Create the profile histogram
  char *pname = (char*)name;
  if (strcmp(name,"_pfx") == 0) {
     Int_t nch = strlen(GetName()) + 5;
     pname = new char[nch];
     sprintf(pname,"%s%s",GetName(),name);
  }
  TProfile *h1=0;
  //check if a profile with identical name exist
  TObject *h1obj = gROOT->FindObject(pname);
  if (h1obj && h1obj->InheritsFrom("TProfile")) {
     h1 = (TProfile*)h1obj;
     h1->Reset();
  }

  if (!h1) {
     const TArrayD *bins = fXaxis.GetXbins();
     if (bins->fN == 0) {
        h1 = new TProfile(pname,GetTitle(),nx,fXaxis.GetXmin(),fXaxis.GetXmax(),option);
     } else {
        h1 = new TProfile(pname,GetTitle(),nx,bins->fArray,option);
     }
  }
  if (pname != name)  delete [] pname;

// Fill the profile histogram
  Double_t cont;
  for (Int_t binx =0;binx<=nx+1;binx++) {
     for (Int_t biny=firstybin;biny<=lastybin;biny++) {
        cont =  GetCellContent(binx,biny);
        if (cont) {
           h1->Fill(fXaxis.GetBinCenter(binx),fYaxis.GetBinCenter(biny), cont);
        }
     }
  }
  if (firstybin <=1 && lastybin >= ny) h1->SetEntries(fEntries);
  
  if (opt.Contains("d")) {
     TVirtualPad *padsav = gPad;
     TVirtualPad *pad = gROOT->GetSelectedPad();
     if (pad) pad->cd();
     char optin[100];
     strcpy(optin,opt.Data());
     char *d = (char*)strstr(optin,"d"); if (d) {*d = ' '; if (*(d+1) == 0) *d=0;}
     char *e = (char*)strstr(optin,"e"); if (e) {*e = ' '; if (*(e+1) == 0) *e=0;}        
     if (!gPad->FindObject(h1)) {
        h1->Draw(optin);
     } else {
        h1->Paint(optin);
     }
     if (padsav) padsav->cd();
  }
  return h1;
}

//______________________________________________________________________________
TProfile *TH2::ProfileY(const char *name, Int_t firstxbin, Int_t lastxbin, Option_t *option) const
{
//*-*-*-*-*Project a 2-D histogram into a profile histogram along Y*-*-*-*-*-*
//*-*      ========================================================
//
//   The projection is made from the channels along the X axis
//   ranging from firstxbin to lastxbin included.
//   By default, bins 1 to nx are included
//   When all bins are included, the number of entries in the projection
//   is set to the number of entries of the 2-D histogram, otherwise
//   the number of entries is incremented by 1 for all non empty cells.
//
//   if option "d" is specified, the profile is drawn in the current pad.
//
//   NOTE that if a TProfile named name exists in the current directory or pad,
//   the histogram is reset and filled again with the current contents of the TH2.

  TString opt = option;
  opt.ToLower();
  Int_t nx = fXaxis.GetNbins();
  Int_t ny = fYaxis.GetNbins();
  if (firstxbin < 0) firstxbin = 1;
  if (lastxbin  < 0) lastxbin  = nx;
  if (lastxbin  > nx+1) lastxbin  = nx;

// Create the projection histogram
  char *pname = (char*)name;
  if (strcmp(name,"_pfy") == 0) {
     Int_t nch = strlen(GetName()) + 5;
     pname = new char[nch];
     sprintf(pname,"%s%s",GetName(),name);
  }
  TProfile *h1=0;
  //check if a profile with identical name exist
  TObject *h1obj = gROOT->FindObject(pname);
  if (h1obj && h1obj->InheritsFrom("TProfile")) {
     h1 = (TProfile*)h1obj;
     h1->Reset();
  }

  if (!h1) {
     const TArrayD *bins = fYaxis.GetXbins();
     if (bins->fN == 0) {
        h1 = new TProfile(pname,GetTitle(),ny,fYaxis.GetXmin(),fYaxis.GetXmax(),option);
     } else {
        h1 = new TProfile(pname,GetTitle(),ny,bins->fArray,option);
     }
  }
  if (pname != name)  delete [] pname;

// Fill the profile histogram
  Double_t cont;
  for (Int_t biny =0;biny<=ny+1;biny++) {
     for (Int_t binx=firstxbin;binx<=lastxbin;binx++) {
        cont =  GetCellContent(binx,biny);
        if (cont) {
           h1->Fill(fYaxis.GetBinCenter(biny),fXaxis.GetBinCenter(binx), cont);
        }
     }
  }
  if (firstxbin <=1 && lastxbin >= nx) h1->SetEntries(fEntries);
  
  if (opt.Contains("d")) {
     TVirtualPad *padsav = gPad;
     TVirtualPad *pad = gROOT->GetSelectedPad();
     if (pad) pad->cd();
     char optin[100];
     strcpy(optin,opt.Data());
     char *d = (char*)strstr(optin,"d"); if (d) {*d = ' '; if (*(d+1) == 0) *d=0;}
     char *e = (char*)strstr(optin,"e"); if (e) {*e = ' '; if (*(e+1) == 0) *e=0;}        
     if (!gPad->FindObject(h1)) {
        h1->Draw(optin);
     } else {
        h1->Paint(optin);
     }
     if (padsav) padsav->cd();
  }
  return h1;
}

//______________________________________________________________________________
TH1D *TH2::ProjectionX(const char *name, Int_t firstybin, Int_t lastybin, Option_t *option) const
{
//*-*-*-*-*Project a 2-D histogram into a 1-D histogram along X*-*-*-*-*-*-*
//*-*      ====================================================
//
//   The projection is always of the type TH1D.
//   The projection is made from the channels along the Y axis
//   ranging from firstybin to lastybin included.
//   By default, bins 1 to ny are included
//   When all bins are included, the number of entries in the projection
//   is set to the number of entries of the 2-D histogram, otherwise
//   the number of entries is incremented by 1 for all non empty cells.
//
//   To make the projection in X of the underflow bin in Y, use firstybin=lastybin=0;
//   To make the projection in X of the overflow bin in Y, use firstybin=lastybin=ny+1;
//
//   if option "e" is specified, the errors are computed.
//   if option "d" is specified, the projection is drawn in the current pad.
//
//   NOTE that if a TH1D named name exists in the current directory or pad,
//   the histogram is reset and filled again with the current contents of the TH2.

  TString opt = option;
  opt.ToLower();
  Int_t nx = fXaxis.GetNbins();
  Int_t ny = fYaxis.GetNbins();
  if (firstybin < 0) firstybin = 1;
  if (lastybin  < 0) lastybin  = ny;
  if (lastybin  > ny+1) lastybin  = ny;

// Create the projection histogram
  char *pname = (char*)name;
  if (strcmp(name,"_px") == 0) {
     Int_t nch = strlen(GetName()) + 4;
     pname = new char[nch];
     sprintf(pname,"%s%s",GetName(),name);
  }
  TH1D *h1=0;
  //check if histogram with identical name exist
  TObject *h1obj = gROOT->FindObject(pname);
  if (h1obj && h1obj->InheritsFrom("TH1D")) {
     h1 = (TH1D*)h1obj;
     h1->Reset();
  }

  if (!h1) {
     const TArrayD *bins = fXaxis.GetXbins();
     if (bins->fN == 0) {
        h1 = new TH1D(pname,GetTitle(),nx,fXaxis.GetXmin(),fXaxis.GetXmax());
     } else {
        h1 = new TH1D(pname,GetTitle(),nx,bins->fArray);
     }
     if (opt.Contains("e")) h1->Sumw2();
  }
  if (pname != name)  delete [] pname;

// Fill the projected histogram
  Double_t cont,err,err2;
  for (Int_t binx =0;binx<=nx+1;binx++) {
     err2 = 0;
     for (Int_t biny=firstybin;biny<=lastybin;biny++) {
        cont  = GetCellContent(binx,biny);
        err   = GetCellError(binx,biny);
        err2 += err*err;
        if (cont) {
           h1->Fill(fXaxis.GetBinCenter(binx), cont);
        }
     }
     if (h1->GetSumw2N()) h1->SetBinError(binx,TMath::Sqrt(err2));
  }
  if (firstybin <=1 && lastybin >= ny) h1->SetEntries(fEntries);
  
  if (opt.Contains("d")) {
     TVirtualPad *padsav = gPad;
     TVirtualPad *pad = gROOT->GetSelectedPad();
     if (pad) pad->cd();
     char optin[100];
     strcpy(optin,opt.Data());
     char *d = (char*)strstr(optin,"d"); if (d) {*d = ' '; if (*(d+1) == 0) *d=0;}
     char *e = (char*)strstr(optin,"e"); if (e) {*e = ' '; if (*(e+1) == 0) *e=0;}        
     if (!gPad->FindObject(h1)) {
        h1->Draw(optin);
     } else {
        h1->Paint(optin);
     }
     if (padsav) padsav->cd();
  }
  
  return h1;
}

//______________________________________________________________________________
TH1D *TH2::ProjectionY(const char *name, Int_t firstxbin, Int_t lastxbin, Option_t *option) const
{
//*-*-*-*-*Project a 2-D histogram into a 1-D histogram along Y*-*-*-*-*-*-*
//*-*      ====================================================
//
//   The projection is always of the type TH1D.
//   The projection is made from the channels along the X axis
//   ranging from firstxbin to lastxbin included.
//   By default, bins 1 to nx are included
//   When all bins are included, the number of entries in the projection
//   is set to the number of entries of the 2-D histogram, otherwise
//   the number of entries is incremented by 1 for all non empty cells.
//
//   To make the projection in Y of the underflow bin in X, use firstxbin=lastxbin=0;
//   To make the projection in Y of the overflow bin in X, use firstxbin=lastxbin=nx+1;
//
//   if option "e" is specified, the errors are computed.
//   if option "d" is specified, the projection is drawn in the current pad.
//
//   NOTE that if a TH1D named name exists in the current directory or pad,
//   the histogram is reset and filled again with the current contents of the TH2.

  TString opt = option;
  opt.ToLower();
  Int_t nx = fXaxis.GetNbins();
  Int_t ny = fYaxis.GetNbins();
  if (firstxbin < 0) firstxbin = 1;
  if (lastxbin  < 0) lastxbin  = nx;
  if (lastxbin  > nx+1) lastxbin  = nx;

// Create the projection histogram
  char *pname = (char*)name;
  if (strcmp(name,"_py") == 0) {
     Int_t nch = strlen(GetName()) + 4;
     pname = new char[nch];
     sprintf(pname,"%s%s",GetName(),name);
  }
  TH1D *h1=0;
  //check if histogram with identical name exist
  TObject *h1obj = gROOT->FindObject(pname);
  if (h1obj && h1obj->InheritsFrom("TH1D")) {
     h1 = (TH1D*)h1obj;
     h1->Reset();
  }

  if (!h1) {
     const TArrayD *bins = fYaxis.GetXbins();
     if (bins->fN == 0) {
        h1 = new TH1D(pname,GetTitle(),ny,fYaxis.GetXmin(),fYaxis.GetXmax());
     } else {
     h1 = new TH1D(pname,GetTitle(),ny,bins->fArray);
     }
     if (opt.Contains("e")) h1->Sumw2();
  }
  if (pname != name)  delete [] pname;

// Fill the projected histogram
  Double_t cont,err,err2;
  for (Int_t biny =0;biny<=ny+1;biny++) {
     err2 = 0;
     for (Int_t binx=firstxbin;binx<=lastxbin;binx++) {
        cont  = GetCellContent(binx,biny);
        err   = GetCellError(binx,biny);
        err2 += err*err;
        if (cont) {
           h1->Fill(fYaxis.GetBinCenter(biny), cont);
        }
     }
     if (h1->GetSumw2N()) h1->SetBinError(biny,TMath::Sqrt(err2));
  }
  if (firstxbin <=1 && lastxbin >= nx) h1->SetEntries(fEntries);
  
  if (opt.Contains("d")) {
     TVirtualPad *padsav = gPad;
     TVirtualPad *pad = gROOT->GetSelectedPad();
     if (pad) pad->cd();
     char optin[100];
     strcpy(optin,opt.Data());
     char *d = (char*)strstr(optin,"d"); if (d) {*d = ' '; if (*(d+1) == 0) *d=0;}
     char *e = (char*)strstr(optin,"e"); if (e) {*e = ' '; if (*(e+1) == 0) *e=0;}        
     if (!gPad->FindObject(h1)) {
        h1->Draw(optin);
     } else {
        h1->Paint(optin);
     }
     if (padsav) padsav->cd();
  }
  
  return h1;
}

//______________________________________________________________________________
void TH2::PutStats(Stat_t *stats)
{
   // Replace current statistics with the values in array stats

   TH1::PutStats(stats);
   fTsumwy  = stats[4];
   fTsumwy2 = stats[5];
   fTsumwxy = stats[6];
}

//______________________________________________________________________________
void TH2::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*Reset this histogram: contents, errors, etc*-*-*-*-*-*-*-*
//*-*            ===========================================

   TH1::Reset(option);
   fTsumwy  = 0;
   fTsumwy2 = 0;
   fTsumwxy = 0;
}

//______________________________________________________________________________
void TH2::Streamer(TBuffer &R__b)
{ 
   // Stream an object of class TH2.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TH2::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      TH1::Streamer(R__b);
      R__b >> fScalefactor;
      R__b >> fTsumwy;
      R__b >> fTsumwy2;
      R__b >> fTsumwxy;
      //====end of old versions
      
   } else {
      TH2::Class()->WriteBuffer(R__b,this);
   }
}

ClassImp(TH2C)

//______________________________________________________________________________
//                     TH2C methods
//______________________________________________________________________________
TH2C::TH2C(): TH2()
{
}

//______________________________________________________________________________
TH2C::~TH2C()
{
}

//______________________________________________________________________________
TH2C::TH2C(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ylow,yup)
{
   TArrayC::Set(fNcells);
}

//______________________________________________________________________________
TH2C::TH2C(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xbins,nbinsy,ylow,yup)
{
   TArrayC::Set(fNcells);
}

//______________________________________________________________________________
TH2C::TH2C(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ybins)
{
   TArrayC::Set(fNcells);
}

//______________________________________________________________________________
TH2C::TH2C(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                             ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayC::Set(fNcells);
}

//______________________________________________________________________________
TH2C::TH2C(const char *name,const char *title,Int_t nbinsx,const Float_t *xbins
                                             ,Int_t nbinsy,const Float_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayC::Set(fNcells);
}

//______________________________________________________________________________
TH2C::TH2C(const TH2C &h2c) : TH2(), TArrayC()
{
   ((TH2C&)h2c).Copy(*this);
}

//______________________________________________________________________________
void TH2C::AddBinContent(Int_t bin)
{
//*-*-*-*-*-*-*-*-*-*Increment bin content by 1*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                ==========================

   if (fArray[bin] < 127) fArray[bin]++;
}

//______________________________________________________________________________
void TH2C::AddBinContent(Int_t bin, Stat_t w)
{
//*-*-*-*-*-*-*-*-*-*Increment bin content by w*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                ==========================

   Int_t newval = fArray[bin] + Int_t(w);
   if (newval > -128 && newval < 128) {fArray[bin] = Char_t(newval); return;}
   if (newval < -127) fArray[bin] = -127;
   if (newval >  127) fArray[bin] =  127;
}

//______________________________________________________________________________
void TH2C::Copy(TObject &newth2) const
{
   TH2::Copy((TH2C&)newth2);
   TArrayC::Copy((TH2C&)newth2);
}

//______________________________________________________________________________
TH1 *TH2C::DrawCopy(Option_t *option) const
{
   TString opt = option;
   opt.ToLower();
   if (gPad && !opt.Contains("same")) gPad->Clear();
   TH2C *newth2 = (TH2C*)Clone();
   newth2->SetDirectory(0);
   newth2->SetBit(kCanDelete);
   newth2->AppendPad(option);
   return newth2;
}

//______________________________________________________________________________
Stat_t TH2C::GetBinContent(Int_t bin) const
{
   if (fBuffer) ((TH2C*)this)->BufferEmpty();
   if (bin < 0) bin = 0;
   if (bin >= fNcells) bin = fNcells-1;
   if (!fArray) return 0;
   return Stat_t (fArray[bin]);
}

//______________________________________________________________________________
void TH2C::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*Reset this histogram: contents, errors, etc*-*-*-*-*-*-*-*
//*-*            ===========================================

   TH2::Reset(option);
   TArrayC::Reset();
}

//______________________________________________________________________________
void TH2C::SetBinContent(Int_t bin, Stat_t content)
{
// Set bin content
   if (bin < 0) return;
   if (bin >= fNcells) return;
   fArray[bin] = Char_t (content);
   fEntries++;
}

//______________________________________________________________________________
void TH2C::SetBinsLength(Int_t n)
{
// Set total number of bins including under/overflow
// Reallocate bin contents array
   
   if (n < 0) n = (fXaxis.GetNbins()+2)*(fYaxis.GetNbins()+2);
   fNcells = n;
   TArrayC::Set(n);
}

//______________________________________________________________________________
void TH2C::Streamer(TBuffer &R__b)
{
   // Stream an object of class TH2C.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TH2C::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      if (R__v < 2) {
         R__b.ReadVersion();
         TH1::Streamer(R__b);
         TArrayC::Streamer(R__b);
         R__b.ReadVersion();
         R__b >> fScalefactor;
         R__b >> fTsumwy;
         R__b >> fTsumwy2;
         R__b >> fTsumwxy;
      } else {
         TH2::Streamer(R__b);
         TArrayC::Streamer(R__b);
         R__b.CheckByteCount(R__s, R__c, TH2C::IsA());
      }
      //====end of old versions
      
   } else {
      TH2C::Class()->WriteBuffer(R__b,this);
   }
}

//______________________________________________________________________________
TH2C& TH2C::operator=(const TH2C &h1)
{
   if (this != &h1)  ((TH2C&)h1).Copy(*this);
   return *this;
}


//______________________________________________________________________________
TH2C operator*(Float_t c1, TH2C &h1)
{
   TH2C hnew = h1;
   hnew.Scale(c1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2C operator+(TH2C &h1, TH2C &h2)
{
   TH2C hnew = h1;
   hnew.Add(&h2,1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2C operator-(TH2C &h1, TH2C &h2)
{
   TH2C hnew = h1;
   hnew.Add(&h2,-1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2C operator*(TH2C &h1, TH2C &h2)
{
   TH2C hnew = h1;
   hnew.Multiply(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2C operator/(TH2C &h1, TH2C &h2)
{
   TH2C hnew = h1;
   hnew.Divide(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

ClassImp(TH2S)

//______________________________________________________________________________
//                     TH2S methods
//______________________________________________________________________________
TH2S::TH2S(): TH2()
{
}

//______________________________________________________________________________
TH2S::~TH2S()
{

}

//______________________________________________________________________________
TH2S::TH2S(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ylow,yup)
{
   TArrayS::Set(fNcells);
}

//______________________________________________________________________________
TH2S::TH2S(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xbins,nbinsy,ylow,yup)
{
   TArrayS::Set(fNcells);
}

//______________________________________________________________________________
TH2S::TH2S(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ybins)
{
   TArrayS::Set(fNcells);
}

//______________________________________________________________________________
TH2S::TH2S(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                             ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayS::Set(fNcells);
}

//______________________________________________________________________________
TH2S::TH2S(const char *name,const char *title,Int_t nbinsx,const Float_t *xbins
                                             ,Int_t nbinsy,const Float_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayS::Set(fNcells);
}

//______________________________________________________________________________
TH2S::TH2S(const TH2S &h2s) : TH2(), TArrayS()
{
   ((TH2S&)h2s).Copy(*this);
}

//______________________________________________________________________________
void TH2S::AddBinContent(Int_t bin)
{
//*-*-*-*-*-*-*-*-*-*Increment bin content by 1*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                ==========================

   if (fArray[bin] < 32767) fArray[bin]++;
}

//______________________________________________________________________________
void TH2S::AddBinContent(Int_t bin, Stat_t w)
{
//*-*-*-*-*-*-*-*-*-*Increment bin content by w*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                ==========================

   Int_t newval = fArray[bin] + Int_t(w);
   if (newval > -32768 && newval < 32768) {fArray[bin] = Short_t(newval); return;}
   if (newval < -32767) fArray[bin] = -32767;
   if (newval >  32767) fArray[bin] =  32767;
}

//______________________________________________________________________________
void TH2S::Copy(TObject &newth2) const
{
   TH2::Copy((TH2S&)newth2);
   TArrayS::Copy((TH2S&)newth2);
}

//______________________________________________________________________________
TH1 *TH2S::DrawCopy(Option_t *option) const
{
   TString opt = option;
   opt.ToLower();
   if (gPad && !opt.Contains("same")) gPad->Clear();
   TH2S *newth2 = (TH2S*)Clone();
   newth2->SetDirectory(0);
   newth2->SetBit(kCanDelete);
   newth2->AppendPad(option);
   return newth2;
}

//______________________________________________________________________________
Stat_t TH2S::GetBinContent(Int_t bin) const
{
   if (fBuffer) ((TH2C*)this)->BufferEmpty();
   if (bin < 0) bin = 0;
   if (bin >= fNcells) bin = fNcells-1;
   if (!fArray) return 0;
   return Stat_t (fArray[bin]);
}

//______________________________________________________________________________
void TH2S::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*Reset this histogram: contents, errors, etc*-*-*-*-*-*-*-*
//*-*            ===========================================

   TH2::Reset(option);
   TArrayS::Reset();
}

//______________________________________________________________________________
void TH2S::SetBinContent(Int_t bin, Stat_t content)
{
// Set bin content
   if (bin < 0) return;
   if (bin >= fNcells) return;
   fArray[bin] = Short_t (content);
   fEntries++;
}

//______________________________________________________________________________
void TH2S::SetBinsLength(Int_t n)
{
// Set total number of bins including under/overflow
// Reallocate bin contents array
   
   if (n < 0) n = (fXaxis.GetNbins()+2)*(fYaxis.GetNbins()+2);
   fNcells = n;
   TArrayS::Set(n);
}

//______________________________________________________________________________
void TH2S::Streamer(TBuffer &R__b)
{
   // Stream an object of class TH2S.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TH2S::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      if (R__v < 2) {
         R__b.ReadVersion();
         TH1::Streamer(R__b);
         TArrayS::Streamer(R__b);
         R__b.ReadVersion();
         R__b >> fScalefactor;
         R__b >> fTsumwy;
         R__b >> fTsumwy2;
         R__b >> fTsumwxy;
      } else {
         TH2::Streamer(R__b);
         TArrayS::Streamer(R__b);
         R__b.CheckByteCount(R__s, R__c, TH2S::IsA());
      }
      //====end of old versions
      
   } else {
      TH2S::Class()->WriteBuffer(R__b,this);
   }
}

//______________________________________________________________________________
TH2S& TH2S::operator=(const TH2S &h1)
{
   if (this != &h1)  ((TH2S&)h1).Copy(*this);
   return *this;
}


//______________________________________________________________________________
TH2S operator*(Float_t c1, TH2S &h1)
{
   TH2S hnew = h1;
   hnew.Scale(c1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2S operator+(TH2S &h1, TH2S &h2)
{
   TH2S hnew = h1;
   hnew.Add(&h2,1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2S operator-(TH2S &h1, TH2S &h2)
{
   TH2S hnew = h1;
   hnew.Add(&h2,-1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2S operator*(TH2S &h1, TH2S &h2)
{
   TH2S hnew = h1;
   hnew.Multiply(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2S operator/(TH2S &h1, TH2S &h2)
{
   TH2S hnew = h1;
   hnew.Divide(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

ClassImp(TH2I)

//______________________________________________________________________________
//                     TH2I methods
//______________________________________________________________________________
TH2I::TH2I(): TH2()
{
}

//______________________________________________________________________________
TH2I::~TH2I()
{

}

//______________________________________________________________________________
TH2I::TH2I(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ylow,yup)
{
   TArrayI::Set(fNcells);
}

//______________________________________________________________________________
TH2I::TH2I(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xbins,nbinsy,ylow,yup)
{
   TArrayI::Set(fNcells);
}

//______________________________________________________________________________
TH2I::TH2I(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ybins)
{
   TArrayI::Set(fNcells);
}

//______________________________________________________________________________
TH2I::TH2I(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                             ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayI::Set(fNcells);
}

//______________________________________________________________________________
TH2I::TH2I(const char *name,const char *title,Int_t nbinsx,const Float_t *xbins
                                             ,Int_t nbinsy,const Float_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayI::Set(fNcells);
}

//______________________________________________________________________________
TH2I::TH2I(const TH2I &h2i) : TH2(), TArrayI()
{
   ((TH2I&)h2i).Copy(*this);
}

//______________________________________________________________________________
void TH2I::AddBinContent(Int_t bin)
{
//*-*-*-*-*-*-*-*-*-*Increment bin content by 1*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                ==========================

   if (fArray[bin] < 2147483647) fArray[bin]++;
}

//______________________________________________________________________________
void TH2I::AddBinContent(Int_t bin, Stat_t w)
{
//*-*-*-*-*-*-*-*-*-*Increment bin content by w*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                ==========================

   Int_t newval = fArray[bin] + Int_t(w);
   if (newval > -2147483647 && newval < 2147483647) {fArray[bin] = Short_t(newval); return;}
   if (newval < -2147483647) fArray[bin] = -2147483647;
   if (newval >  2147483647) fArray[bin] =  2147483647;
}

//______________________________________________________________________________
void TH2I::Copy(TObject &newth2) const
{
   TH2::Copy((TH2I&)newth2);
   TArrayI::Copy((TH2I&)newth2);
}

//______________________________________________________________________________
TH1 *TH2I::DrawCopy(Option_t *option) const
{
   TString opt = option;
   opt.ToLower();
   if (gPad && !opt.Contains("same")) gPad->Clear();
   TH2I *newth2 = (TH2I*)Clone();
   newth2->SetDirectory(0);
   newth2->SetBit(kCanDelete);
   newth2->AppendPad(option);
   return newth2;
}

//______________________________________________________________________________
Stat_t TH2I::GetBinContent(Int_t bin) const
{
   if (fBuffer) ((TH2C*)this)->BufferEmpty();
   if (bin < 0) bin = 0;
   if (bin >= fNcells) bin = fNcells-1;
   if (!fArray) return 0;
   return Stat_t (fArray[bin]);
}

//______________________________________________________________________________
void TH2I::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*Reset this histogram: contents, errors, etc*-*-*-*-*-*-*-*
//*-*            ===========================================

   TH2::Reset(option);
   TArrayI::Reset();
}

//______________________________________________________________________________
void TH2I::SetBinContent(Int_t bin, Stat_t content)
{
// Set bin content
   if (bin < 0) return;
   if (bin >= fNcells) return;
   fArray[bin] = Short_t (content);
   fEntries++;
}

//______________________________________________________________________________
void TH2I::SetBinsLength(Int_t n)
{
// Set total number of bins including under/overflow
// Reallocate bin contents array
   
   if (n < 0) n = (fXaxis.GetNbins()+2)*(fYaxis.GetNbins()+2);
   fNcells = n;
   TArrayI::Set(n);
}

//______________________________________________________________________________
TH2I& TH2I::operator=(const TH2I &h1)
{
   if (this != &h1)  ((TH2I&)h1).Copy(*this);
   return *this;
}


//______________________________________________________________________________
TH2I operator*(Float_t c1, TH2I &h1)
{
   TH2I hnew = h1;
   hnew.Scale(c1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2I operator+(TH2I &h1, TH2I &h2)
{
   TH2I hnew = h1;
   hnew.Add(&h2,1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2I operator-(TH2I &h1, TH2I &h2)
{
   TH2I hnew = h1;
   hnew.Add(&h2,-1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2I operator*(TH2I &h1, TH2I &h2)
{
   TH2I hnew = h1;
   hnew.Multiply(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2I operator/(TH2I &h1, TH2I &h2)
{
   TH2I hnew = h1;
   hnew.Divide(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

ClassImp(TH2F)

//______________________________________________________________________________
//                     TH2F methods
//______________________________________________________________________________
TH2F::TH2F(): TH2()
{
}

//______________________________________________________________________________
TH2F::~TH2F()
{
}

//______________________________________________________________________________
TH2F::TH2F(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ylow,yup)
{
   TArrayF::Set(fNcells);
}

//______________________________________________________________________________
TH2F::TH2F(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xbins,nbinsy,ylow,yup)
{

   TArrayF::Set(fNcells);
}

//______________________________________________________________________________
TH2F::TH2F(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ybins)
{

   TArrayF::Set(fNcells);
}

//______________________________________________________________________________
TH2F::TH2F(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                             ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayF::Set(fNcells);
}

//______________________________________________________________________________
TH2F::TH2F(const char *name,const char *title,Int_t nbinsx,const Float_t *xbins
                                             ,Int_t nbinsy,const Float_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayF::Set(fNcells);
}

//______________________________________________________________________________
TH2F::TH2F(const TMatrix &m)
     :TH2("TMatrix","",m.GetNcols(),m.GetColLwb(),1+m.GetColUpb(),m.GetNrows(),m.GetRowLwb(),1+m.GetRowUpb())
{
   TArrayF::Set(fNcells);
   Int_t ilow = m.GetRowLwb();
   Int_t iup  = m.GetRowUpb();
   Int_t jlow = m.GetColLwb();
   Int_t jup  = m.GetColUpb();
   for (Int_t i=ilow;i<=iup;i++) {
      for (Int_t j=jlow;j<=jup;j++) {
         SetCellContent(j-jlow+1,i-ilow+1,m(i,j));
      }
   }     
}

//______________________________________________________________________________
TH2F::TH2F(const TH2F &h2f) : TH2(), TArrayF()
{
   ((TH2F&)h2f).Copy(*this);
}

//______________________________________________________________________________
void TH2F::Copy(TObject &newth2) const
{
   TH2::Copy((TH2F&)newth2);
   TArrayF::Copy((TH2F&)newth2);
}

//______________________________________________________________________________
TH1 *TH2F::DrawCopy(Option_t *option) const
{
   TString opt = option;
   opt.ToLower();
   if (gPad && !opt.Contains("same")) gPad->Clear();
   TH2F *newth2 = (TH2F*)Clone();
   newth2->SetDirectory(0);
   newth2->SetBit(kCanDelete);
   newth2->AppendPad(option);
   return newth2;
}

//______________________________________________________________________________
Stat_t TH2F::GetBinContent(Int_t bin) const
{
   if (fBuffer) ((TH2C*)this)->BufferEmpty();
   if (bin < 0) bin = 0;
   if (bin >= fNcells) bin = fNcells-1;
   if (!fArray) return 0;
   return Stat_t (fArray[bin]);
}

//______________________________________________________________________________
void TH2F::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*Reset this histogram: contents, errors, etc*-*-*-*-*-*-*-*
//*-*            ===========================================

   TH2::Reset(option);
   TArrayF::Reset();
}

//______________________________________________________________________________
void TH2F::SetBinContent(Int_t bin, Stat_t content)
{
// Set bin content
   if (bin < 0) return;
   if (bin >= fNcells) return;
   fArray[bin] = Float_t (content);
   fEntries++;
}

//______________________________________________________________________________
void TH2F::SetBinsLength(Int_t n)
{
// Set total number of bins including under/overflow
// Reallocate bin contents array
   
   if (n < 0) n = (fXaxis.GetNbins()+2)*(fYaxis.GetNbins()+2);
   fNcells = n;
   TArrayF::Set(n);
}

//______________________________________________________________________________
void TH2F::Streamer(TBuffer &R__b)
{
   // Stream an object of class TH2F.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TH2F::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      if (R__v < 2) {
         R__b.ReadVersion();
         TH1::Streamer(R__b);
         TArrayF::Streamer(R__b);
         R__b.ReadVersion();
         R__b >> fScalefactor;
         R__b >> fTsumwy;
         R__b >> fTsumwy2;
         R__b >> fTsumwxy;
      } else {
         TH2::Streamer(R__b);
         TArrayF::Streamer(R__b);
         R__b.CheckByteCount(R__s, R__c, TH2F::IsA());
      }
      //====end of old versions
      
   } else {
      TH2F::Class()->WriteBuffer(R__b,this);
   }
}

//______________________________________________________________________________
TH2F& TH2F::operator=(const TH2F &h1)
{
   if (this != &h1)  ((TH2F&)h1).Copy(*this);
   return *this;
}


//______________________________________________________________________________
TH2F operator*(Float_t c1, TH2F &h1)
{
   TH2F hnew = h1;
   hnew.Scale(c1);
   hnew.SetDirectory(0);
   return hnew;
}


//______________________________________________________________________________
TH2F operator*(TH2F &h1, Float_t c1)
{
   TH2F hnew = h1;
   hnew.Scale(c1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2F operator+(TH2F &h1, TH2F &h2)
{
   TH2F hnew = h1;
   hnew.Add(&h2,1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2F operator-(TH2F &h1, TH2F &h2)
{
   TH2F hnew = h1;
   hnew.Add(&h2,-1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2F operator*(TH2F &h1, TH2F &h2)
{
   TH2F hnew = h1;
   hnew.Multiply(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2F operator/(TH2F &h1, TH2F &h2)
{
   TH2F hnew = h1;
   hnew.Divide(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

ClassImp(TH2D)

//______________________________________________________________________________
//                     TH2D methods
//______________________________________________________________________________
TH2D::TH2D(): TH2()
{
}

//______________________________________________________________________________
TH2D::~TH2D()
{
}

//______________________________________________________________________________
TH2D::TH2D(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ylow,yup)
{
   TArrayD::Set(fNcells);
}

//______________________________________________________________________________
TH2D::TH2D(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                     ,Int_t nbinsy,Axis_t ylow,Axis_t yup)
     :TH2(name,title,nbinsx,xbins,nbinsy,ylow,yup)
{
   TArrayD::Set(fNcells);
}

//______________________________________________________________________________
TH2D::TH2D(const char *name,const char *title,Int_t nbinsx,Axis_t xlow,Axis_t xup
                                     ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xlow,xup,nbinsy,ybins)
{
   TArrayD::Set(fNcells);
}

//______________________________________________________________________________
TH2D::TH2D(const char *name,const char *title,Int_t nbinsx,const Double_t *xbins
                                             ,Int_t nbinsy,const Double_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayD::Set(fNcells);
}

//______________________________________________________________________________
TH2D::TH2D(const char *name,const char *title,Int_t nbinsx,const Float_t *xbins
                                             ,Int_t nbinsy,const Float_t *ybins)
     :TH2(name,title,nbinsx,xbins,nbinsy,ybins)
{
   TArrayD::Set(fNcells);
}

//______________________________________________________________________________
TH2D::TH2D(const TMatrixD &m)
     :TH2("TMatrixD","",m.GetNcols(),m.GetColLwb(),1+m.GetColUpb(),m.GetNrows(),m.GetRowLwb(),1+m.GetRowUpb())
{
   TArrayD::Set(fNcells);
   Int_t ilow = m.GetRowLwb();
   Int_t iup  = m.GetRowUpb();
   Int_t jlow = m.GetColLwb();
   Int_t jup  = m.GetColUpb();
   for (Int_t i=ilow;i<=iup;i++) {
      for (Int_t j=jlow;j<=jup;j++) {
         SetCellContent(j-jlow+1,i-ilow+1,m(i,j));
      }
   }     
}

//______________________________________________________________________________
TH2D::TH2D(const TH2D &h2d) : TH2(), TArrayD()
{
   ((TH2D&)h2d).Copy(*this);
}

//______________________________________________________________________________
void TH2D::Copy(TObject &newth2) const
{
   TH2::Copy((TH2D&)newth2);
   TArrayD::Copy((TH2D&)newth2);
}

//______________________________________________________________________________
TH1 *TH2D::DrawCopy(Option_t *option) const
{
   TString opt = option;
   opt.ToLower();
   if (gPad && !opt.Contains("same")) gPad->Clear();
   TH2D *newth2 = (TH2D*)Clone();
   newth2->SetDirectory(0);
   newth2->SetBit(kCanDelete);
   newth2->AppendPad(option);
   return newth2;
}

//______________________________________________________________________________
Stat_t TH2D::GetBinContent(Int_t bin) const
{
   if (fBuffer) ((TH2C*)this)->BufferEmpty();
   if (bin < 0) bin = 0;
   if (bin >= fNcells) bin = fNcells-1;
   if (!fArray) return 0;
   return Stat_t (fArray[bin]);
}

//______________________________________________________________________________
void TH2D::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*Reset this histogram: contents, errors, etc*-*-*-*-*-*-*-*
//*-*            ===========================================

   TH2::Reset(option);
   TArrayD::Reset();
}

//______________________________________________________________________________
void TH2D::SetBinContent(Int_t bin, Stat_t content)
{
// Set bin content
   if (bin < 0) return;
   if (bin >= fNcells) return;
   fArray[bin] = Double_t (content);
   fEntries++;
}

//______________________________________________________________________________
void TH2D::SetBinsLength(Int_t n)
{
// Set total number of bins including under/overflow
// Reallocate bin contents array
   
   if (n < 0) n = (fXaxis.GetNbins()+2)*(fYaxis.GetNbins()+2);
   fNcells = n;
   TArrayD::Set(n);
}

//______________________________________________________________________________
void TH2D::Streamer(TBuffer &R__b)
{
   // Stream an object of class TH2D.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TH2D::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      if (R__v < 2) {
         R__b.ReadVersion();
         TH1::Streamer(R__b);
         TArrayD::Streamer(R__b);
         R__b.ReadVersion();
         R__b >> fScalefactor;
         R__b >> fTsumwy;
         R__b >> fTsumwy2;
         R__b >> fTsumwxy;
      } else {
         TH2::Streamer(R__b);
         TArrayD::Streamer(R__b);
         R__b.CheckByteCount(R__s, R__c, TH2D::IsA());
      }
      //====end of old versions
      
   } else {
      TH2D::Class()->WriteBuffer(R__b,this);
   }
}

//______________________________________________________________________________
TH2D& TH2D::operator=(const TH2D &h1)
{
   if (this != &h1)  ((TH2D&)h1).Copy(*this);
   return *this;
}


//______________________________________________________________________________
TH2D operator*(Float_t c1, TH2D &h1)
{
   TH2D hnew = h1;
   hnew.Scale(c1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2D operator+(TH2D &h1, TH2D &h2)
{
   TH2D hnew = h1;
   hnew.Add(&h2,1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2D operator-(TH2D &h1, TH2D &h2)
{
   TH2D hnew = h1;
   hnew.Add(&h2,-1);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2D operator*(TH2D &h1, TH2D &h2)
{
   TH2D hnew = h1;
   hnew.Multiply(&h2);
   hnew.SetDirectory(0);
   return hnew;
}

//______________________________________________________________________________
TH2D operator/(TH2D &h1, TH2D &h2)
{
   TH2D hnew = h1;
   hnew.Divide(&h2);
   hnew.SetDirectory(0);
   return hnew;
}
