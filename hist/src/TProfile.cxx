// @(#)root/hist:$Name:  $:$Id: TProfile.cxx,v 1.25 2002/05/11 16:34:46 brun Exp $
// Author: Rene Brun   29/09/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TProfile.h"
#include "TMath.h"
#include "THLimitsFinder.h"
#include "Riostream.h"

ClassImp(TProfile)

//______________________________________________________________________________
//
//  Profile histograms are used to display the mean
//  value of Y and its RMS for each bin in X. Profile histograms are in many cases an
//  elegant replacement of two-dimensional histograms : the inter-relation of two
//  measured quantities X and Y can always be visualized by a two-dimensional
//  histogram or scatter-plot; its representation on the line-printer is not particularly
//  satisfactory, except for sparse data. If Y is an unknown (but single-valued)
//  approximate function of X, this function is displayed by a profile histogram with
//  much better precision than by a scatter-plot.
//
//  The following formulae show the cumulated contents (capital letters) and the values
//  displayed by the printing or plotting routines (small letters) of the elements for bin J.
//
//                                                    2
//      H(J)  =  sum Y                  E(J)  =  sum Y
//      l(J)  =  sum l                  L(J)  =  sum l
//      h(J)  =  H(J)/L(J)              s(J)  =  sqrt(E(J)/L(J)- h(J)**2)
//      e(J)  =  s(J)/sqrt(L(J))
//
//  In the special case where s(J) is zero (eg, case of 1 entry only in one bin)
//  e(J) is computed from the average of the s(J) for all bins.
//  This simple/crude approximation was suggested in order to keep the bin
//  during a fit operation.
//
//           Example of a profile histogram with its graphics output
//{
//  TCanvas *c1 = new TCanvas("c1","Profile histogram example",200,10,700,500);
//  hprof  = new TProfile("hprof","Profile of pz versus px",100,-4,4,0,20);
//  Float_t px, py, pz;
//  for ( Int_t i=0; i<25000; i++) {
//     gRandom->Rannor(px,py);
//     pz = px*px + py*py;
//     hprof->Fill(px,pz,1);
//  }
//  hprof->Draw();
//}
//Begin_Html
/*
<img src="gif/profile.gif">
*/
//End_Html
//

//______________________________________________________________________________
TProfile::TProfile() : TH1D()
{
//*-*-*-*-*-*Default constructor for Profile histograms*-*-*-*-*-*-*-*-*
//*-*        ==========================================

   fScaling = kFALSE;
}

//______________________________________________________________________________
TProfile::~TProfile()
{
//*-*-*-*-*-*Default destructor for Profile histograms*-*-*-*-*-*-*-*-*
//*-*        =========================================

}

//______________________________________________________________________________
TProfile::TProfile(const char *name,const char *title,Int_t nbins,Axis_t xlow,Axis_t xup,Option_t *option)
    : TH1D(name,title,nbins,xlow,xup)
{
//*-*-*-*-*-*Normal Constructor for Profile histograms*-*-*-*-*-*-*-*-*-*
//*-*        ==========================================
//
//  The first five parameters are similar to TH1D::TH1D.
//  All values of y are accepted at filling time.
//  To fill a profile histogram, one must use TProfile::Fill function.
//
//  Note that when filling the profile histogram the function Fill
//  checks if the variable y is betyween fYmin and fYmax.
//  If a minimum or maximum value is set for the Y scale before filling,
//  then all values below ymin or above ymax will be discarded.
//  Setting the minimum or maximum value for the Y scale before filling
//  has the same effect as calling the special TProfile constructor below
//  where ymin and ymax are specified.
//
//  H(J) is printed as the channel contents. The errors displayed are s(J) if CHOPT='S'
//  (spread option), or e(J) if CHOPT=' ' (error on mean).
//
//        See TProfile::BuildOptions for explanation of parameters
//

   BuildOptions(0,0,option);
}

//______________________________________________________________________________
TProfile::TProfile(const char *name,const char *title,Int_t nbins,const Float_t *xbins,Option_t *option)
    : TH1D(name,title,nbins,xbins)
{
//*-*-*-*-*-*Constructor for Profile histograms with variable bin size*-*-*-*-*
//*-*        =========================================================
//
//        See TProfile::BuildOptions for more explanations on errors
//

   BuildOptions(0,0,option);
}

//______________________________________________________________________________
TProfile::TProfile(const char *name,const char *title,Int_t nbins,const Double_t *xbins,Option_t *option)
    : TH1D(name,title,nbins,xbins)
{
//*-*-*-*-*-*Constructor for Profile histograms with variable bin size*-*-*-*-*
//*-*        =========================================================
//
//        See TProfile::BuildOptions for more explanations on errors
//

   BuildOptions(0,0,option);
}

//______________________________________________________________________________
TProfile::TProfile(const char *name,const char *title,Int_t nbins,const Double_t *xbins,Axis_t ylow,Axis_t yup,Option_t *option)
    : TH1D(name,title,nbins,xbins)
{
//*-*-*-*-*-*Constructor for Profile histograms with variable bin size*-*-*-*-*
//*-*        =========================================================
//
//        See TProfile::BuildOptions for more explanations on errors
//

   BuildOptions(ylow,yup,option);
}

//______________________________________________________________________________
TProfile::TProfile(const char *name,const char *title,Int_t nbins,Axis_t xlow,Axis_t xup,Axis_t ylow,Axis_t yup,Option_t *option)
    : TH1D(name,title,nbins,xlow,xup)
{
//*-*-*-*-*-*Constructor for Profile histograms with range in y*-*-*-*-*-*
//*-*        ==================================================
//  The first five parameters are similar to TH1D::TH1D.
//  Only the values of Y between YMIN and YMAX will be considered at filling time.
//  ymin and ymax will also be the maximum and minimum values
//  on the y scale when drawing the profile.
//
//        See TProfile::BuildOptions for more explanations on errors
//

   BuildOptions(ylow,yup,option);
}


//______________________________________________________________________________
void TProfile::BuildOptions(Double_t ymin, Double_t ymax, Option_t *option)
{
//*-*-*-*-*-*-*Set Profile histogram structure and options*-*-*-*-*-*-*-*-*
//*-*          ===========================================
//
//    If a bin has N data points all with the same value Y (especially
//    possible when dealing with integers), the spread in Y for that bin
//    is zero, and the uncertainty assigned is also zero, and the bin is
//    ignored in making subsequent fits. If SQRT(Y) was the correct error
//    in the case above, then SQRT(Y)/SQRT(N) would be the correct error here.
//    In fact, any bin with non-zero number of entries N but with zero spread
//    should have an uncertainty SQRT(Y)/SQRT(N).
//
//    Now, is SQRT(Y)/SQRT(N) really the correct uncertainty?
//    that it is only in the case where the Y variable is some sort
//    of counting statistics, following a Poisson distribution. This should
//    probably be set as the default case. However, Y can be any variable
//    from an original NTUPLE, not necessarily distributed "Poissonly".
//    The computation of errors is based on the parameter option:
//    option:
//     ' '  (Default) Errors are Spread/SQRT(N) for Spread.ne.0. ,
//                      "     "  SQRT(Y)/SQRT(N) for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//     's'            Errors are Spread  for Spread.ne.0. ,
//                      "     "  SQRT(Y)  for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//     'i'            Errors are Spread/SQRT(N) for Spread.ne.0. ,
//                      "     "  1./SQRT(12.*N) for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//
//    The third case above corresponds to Integer Y values for which the
//    uncertainty is +-0.5, with the assumption that the probability that Y
//    takes any value between Y-0.5 and Y+0.5 is uniform (the same argument
//    goes for Y uniformly distributed between Y and Y+1); this would be
//    useful if Y is an ADC measurement, for example. Other, fancier options
//    would be possible, at the cost of adding one more parameter to the PROFILE
//    command. For example, if all Y variables are distributed according to some
//    known Gaussian of standard deviation Sigma, then:
//     'G'            Errors are Spread/SQRT(N) for Spread.ne.0. ,
//                      "     "  Sigma/SQRT(N) for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//    For example, this would be useful when all Y's are experimental quantities
//    measured with the same instrument with precision Sigma.
//
//

   SetErrorOption(option);

   fBinEntries.Set(fNcells);  //*-* create number of entries per bin array

   Sumw2();                   //*-* create sum of squares of weights array

   fYmin = ymin;
   fYmax = ymax;
   fScaling = kFALSE;
}

//______________________________________________________________________________
TProfile::TProfile(const TProfile &profile) : TH1D(profile)
{
   ((TProfile&)profile).Copy(*this);
}


//______________________________________________________________________________
void TProfile::Add(TF1 *, Double_t )
{
   // Performs the operation: this = this + c1*f1

   Error("Add","Function not implemented for TProfile");
   return;
}


//______________________________________________________________________________
void TProfile::Add(const TH1 *h1, Double_t c1)
{
   // Performs the operation: this = this + c1*h1

   if (!h1) {
      Error("Add","Attempt to add a non-existing profile");
      return;
   }
   if (!h1->InheritsFrom("TProfile")) {
      Error("Add","Attempt to add a non-profile object");
      return;
   }
   TProfile *p1 = (TProfile*)h1;

   Int_t nbinsx = GetNbinsX();
//*-*- Check profile compatibility
   if (nbinsx != p1->GetNbinsX()) {
      Error("Add","Attempt to add profiles with different number of bins");
      return;
   }

//*-*- Add statistics
   Double_t ac1 = TMath::Abs(c1);
   fEntries += ac1*p1->GetEntries();
   fTsumw   += ac1*p1->fTsumw;
   fTsumw2  += ac1*p1->fTsumw2;
   fTsumwx  += ac1*p1->fTsumwx;
   fTsumwx2 += ac1*p1->fTsumwx2;

//*-*- Loop on bins (including underflows/overflows)
   Int_t bin;
   Double_t *cu1 = p1->GetW();
   Double_t *er1 = p1->GetW2();
   Double_t *en1 = p1->GetB();
   for (bin=0;bin<=nbinsx+1;bin++) {
      fArray[bin]             +=  c1*cu1[bin];
      fSumw2.fArray[bin]      += ac1*ac1*er1[bin];
      if (!fScaling) fBinEntries.fArray[bin] += ac1*en1[bin];
   }
}

//______________________________________________________________________________
void TProfile::Add(const TH1 *h1, const TH1 *h2, Double_t c1, Double_t c2)
{
//*-*-*-*-*Replace contents of this profile by the addition of h1 and h2*-*-*
//*-*      =============================================================
//
//   this = c1*h1 + c2*h2
//

   if (!h1 || !h2) {
      Error("Add","Attempt to add a non-existing profile");
      return;
   }
   if (!h1->InheritsFrom("TProfile")) {
      Error("Add","Attempt to add a non-profile object");
      return;
   }
   TProfile *p1 = (TProfile*)h1;
   if (!h2->InheritsFrom("TProfile")) {
      Error("Add","Attempt to add a non-profile object");
      return;
   }
   TProfile *p2 = (TProfile*)h2;

   Int_t nbinsx = GetNbinsX();
//*-*- Check profile compatibility
   if (nbinsx != p1->GetNbinsX() || nbinsx != p2->GetNbinsX()) {
      Error("Add","Attempt to add profiles with different number of bins");
      return;
   }

//*-*- Add statistics
   Double_t ac1 = TMath::Abs(c1);
   Double_t ac2 = TMath::Abs(c2);
   fEntries = ac1*p1->GetEntries() + ac2*p2->GetEntries();
   fTsumw   = ac1*p1->fTsumw       + ac2*p2->fTsumw;
   fTsumw2  = ac1*p1->fTsumw2      + ac2*p2->fTsumw2;
   fTsumwx  = ac1*p1->fTsumwx      + ac2*p2->fTsumwx;
   fTsumwx2 = ac1*p1->fTsumwx2     + ac2*p2->fTsumwx2;

//*-*- Loop on bins (including underflows/overflows)
   Int_t bin;
   Double_t *cu1 = p1->GetW();
   Double_t *cu2 = p2->GetW();
   Double_t *er1 = p1->GetW2();
   Double_t *er2 = p2->GetW2();
   Double_t *en1 = p1->GetB();
   Double_t *en2 = p2->GetB();
   for (bin=0;bin<=nbinsx+1;bin++) {
      fArray[bin]             =  c1*cu1[bin] +  c2*cu2[bin];
      fSumw2.fArray[bin]      = ac1*ac1*er1[bin] + ac2*ac2*er2[bin];
      if (fScaling) {
         fBinEntries.fArray[bin] = en1[bin];
      } else {
         fBinEntries.fArray[bin] = ac1*en1[bin] + ac2*en2[bin];
      }
   }
}


//______________________________________________________________________________
Int_t TProfile::BufferEmpty(Bool_t deleteBuffer)
{
// Fill histogram with all entries in the buffer.
// The buffer is deleted if deleteBuffer is true.

   // do we need to compute the bin size?
   Int_t nbentries = (Int_t)fBuffer[0];
   if (!nbentries) return 0;
   if (fXaxis.GetXmax() <= fXaxis.GetXmin()) {
      //find min, max of entries in buffer
     Double_t xmin = fBuffer[2];
     Double_t xmax = xmin;
     for (Int_t i=1;i<nbentries;i++) {
         Double_t x = fBuffer[3*i+2];
         if (x < xmin) xmin = x;
         if (x > xmax) xmax = x;
      }
      THLimitsFinder::GetLimitsFinder()->FindGoodLimits(this,xmin,xmax);
   }

   Double_t *buffer = fBuffer;  fBuffer = 0;

   for (Int_t i=0;i<nbentries;i++) {
      Fill(buffer[3*i+2],buffer[3*i+3],buffer[3*i+1]);
   }

   if (deleteBuffer) { delete buffer;    fBufferSize = 0;}
   else              { fBuffer = buffer; fBuffer[0] = 0;}
   return nbentries;
}

//______________________________________________________________________________
Int_t TProfile::BufferFill(Axis_t x, Axis_t y, Stat_t w)
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
   return -2;
}

//______________________________________________________________________________
void TProfile::Copy(TObject &obj)
{
//*-*-*-*-*-*-*-*Copy a Profile histogram to a new profile histogram*-*-*-*-*
//*-*            ===================================================

   TH1D::Copy(((TProfile&)obj));
   fBinEntries.Copy(((TProfile&)obj).fBinEntries);
   ((TProfile&)obj).fYmin = fYmin;
   ((TProfile&)obj).fYmax = fYmax;
   ((TProfile&)obj).fErrorMode = fErrorMode;
}


//______________________________________________________________________________
void TProfile::Divide(TF1 *, Double_t )
{
   // Performs the operation: this = this/(c1*f1)

   Error("Divide","Function not implemented for TProfile");
   return;
}

//______________________________________________________________________________
void TProfile::Divide(const TH1 *h1)
{
//*-*-*-*-*-*-*-*-*-*-*Divide this profile by h1*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  =========================
//
//   this = this/h1
//

   if (!h1) {
      Error("Divide","Attempt to divide a non-existing profile");
      return;
   }
   if (!h1->InheritsFrom("TProfile")) {
      Error("Divide","Attempt to divide a non-profile object");
      return;
   }
   TProfile *p1 = (TProfile*)h1;

   Int_t nbinsx = GetNbinsX();
//*-*- Check profile compatibility
   if (nbinsx != p1->GetNbinsX()) {
      Error("Divide","Attempt to divide profiles with different number of bins");
      return;
   }

//*-*- Reset statistics
   fEntries = fTsumw   = fTsumw2 = fTsumwx = fTsumwx2 = 0;

//*-*- Loop on bins (including underflows/overflows)
   Int_t bin;
   Double_t *cu1 = p1->GetW();
   Double_t *er1 = p1->GetW2();
   Double_t *en1 = p1->GetB();
   Double_t c0,c1,w,z,x;
   for (bin=0;bin<=nbinsx+1;bin++) {
      c0  = fArray[bin];
      c1  = cu1[bin];
      if (c1) w = c0/c1;
      else    w = 0;
      fArray[bin] = w;
      z = TMath::Abs(w);
      x = fXaxis.GetBinCenter(bin);
      fEntries++;
      fTsumw   += z;
      fTsumw2  += z*z;
      fTsumwx  += z*x;
      fTsumwx2 += z*x*x;
      Double_t e0 = fSumw2.fArray[bin];
      Double_t e1 = er1[bin];
      Double_t c12= c1*c1;
      if (!c1) fSumw2.fArray[bin] = 0;
      else     fSumw2.fArray[bin] = (e0*e0*c1*c1 + e1*e1*c0*c0)/(c12*c12);
      if (!en1[bin]) fBinEntries.fArray[bin] = 0;
      else           fBinEntries.fArray[bin] /= en1[bin];
   }
}


//______________________________________________________________________________
void TProfile::Divide(const TH1 *h1, const TH1 *h2, Double_t c1, Double_t c2, Option_t *option)
{
//*-*-*-*-*Replace contents of this profile by the division of h1 by h2*-*-*
//*-*      ============================================================
//
//   this = c1*h1/(c2*h2)
//

   TString opt = option;
   opt.ToLower();
   Bool_t binomial = kFALSE;
   if (opt.Contains("b")) binomial = kTRUE;
   if (!h1 || !h2) {
      Error("Divide","Attempt to divide a non-existing profile");
      return;
   }
   if (!h1->InheritsFrom("TProfile")) {
      Error("Divide","Attempt to divide a non-profile object");
      return;
   }
   TProfile *p1 = (TProfile*)h1;
   if (!h2->InheritsFrom("TProfile")) {
      Error("Divide","Attempt to divide a non-profile object");
      return;
   }
   TProfile *p2 = (TProfile*)h2;

   Int_t nbinsx = GetNbinsX();
//*-*- Check histogram compatibility
   if (nbinsx != p1->GetNbinsX() || nbinsx != p2->GetNbinsX()) {
      Error("Divide","Attempt to divide profiles with different number of bins");
      return;
   }
   if (!c2) {
      Error("Divide","Coefficient of dividing profile cannot be zero");
      return;
   }

//*-*- Reset statistics
   fEntries = fTsumw   = fTsumw2 = fTsumwx = fTsumwx2 = 0;

//*-*- Loop on bins (including underflows/overflows)
   Int_t bin;
   Double_t *cu1 = p1->GetW();
   Double_t *cu2 = p2->GetW();
   Double_t *er1 = p1->GetW2();
   Double_t *er2 = p2->GetW2();
   Double_t *en1 = p1->GetB();
   Double_t *en2 = p2->GetB();
   Double_t b1,b2,w,z,x,d1,d2;
   d1 = c1*c1;
   d2 = c2*c2;
   for (bin=0;bin<=nbinsx+1;bin++) {
      b1  = cu1[bin];
      b2  = cu2[bin];
      if (b2) w = c1*b1/(c2*b2);
      else    w = 0;
      fArray[bin] = w;
      z = TMath::Abs(w);
      x = fXaxis.GetBinCenter(bin);
      fEntries++;
      fTsumw   += z;
      fTsumw2  += z*z;
      fTsumwx  += z*x;
      fTsumwx2 += z*x*x;
      Double_t e1 = er1[bin];
      Double_t e2 = er2[bin];
      Double_t b22= b2*b2*d2;
      if (!b2) fSumw2.fArray[bin] = 0;
      else {
         if (binomial) {
            fSumw2.fArray[bin] = TMath::Abs(w*(1-w)/(c2*b2));
         } else {
         fSumw2.fArray[bin] = d1*d2*(e1*e1*b2*b2 + e2*e2*b1*b1)/(b22*b22);
         }
      }
      if (!en2[bin]) fBinEntries.fArray[bin] = 0;
      else           fBinEntries.fArray[bin] = en1[bin]/en2[bin];
   }
}

//______________________________________________________________________________
TH1 *TProfile::DrawCopy(Option_t *option)
{
//*-*-*-*-*-*-*-*Draw a copy of this profile histogram*-*-*-*-*-*-*-*-*-*-*-*
//*-*            =====================================
   TProfile *newpf = new TProfile();
   Copy(*newpf);
   newpf->SetDirectory(0);
   newpf->SetBit(kCanDelete);
   newpf->AppendPad(option);
   return newpf;
}

//______________________________________________________________________________
Int_t TProfile::Fill(Axis_t x, Axis_t y)
{
//*-*-*-*-*-*-*-*-*-*-*Fill a Profile histogram (no weights)*-*-*-*-*-*-*-*
//*-*                  =====================================

   if (fBuffer) return BufferFill(x,y,1);

   Int_t bin;
   if (fYmin != fYmax) {
      if (y <fYmin || y> fYmax) return -1;
   }

   fEntries++;
   bin =fXaxis.FindBin(x);
   AddBinContent(bin, y);
   fSumw2.fArray[bin] += (Stat_t)y*y;
   fBinEntries.fArray[bin] += 1;
   if (bin == 0 || bin > fXaxis.GetNbins()) return -1;
   fTsumw++;
   fTsumw2++;
   fTsumwx  += x;
   fTsumwx2 += x*x;
   return bin;
}

//______________________________________________________________________________
Int_t TProfile::Fill(const char *namex, Axis_t y)
{
// Fill a Profile histogram (no weights)
//
   Int_t bin;
   if (fYmin != fYmax) {
      if (y <fYmin || y> fYmax) return -1;
   }

   fEntries++;
   bin =fXaxis.FindBin(namex);
   AddBinContent(bin, y);
   fSumw2.fArray[bin] += (Stat_t)y*y;
   fBinEntries.fArray[bin] += 1;
   if (bin == 0 || bin > fXaxis.GetNbins()) return -1;
   Axis_t x = fXaxis.GetBinCenter(bin);
   fTsumw++;
   fTsumw2++;
   fTsumwx  += x;
   fTsumwx2 += x*x;
   return bin;
}

//______________________________________________________________________________
Int_t TProfile::Fill(Axis_t x, Axis_t y, Stat_t w)
{
//*-*-*-*-*-*-*-*-*-*-*Fill a Profile histogram with weights*-*-*-*-*-*-*-*
//*-*                  =====================================

   if (fBuffer) return BufferFill(x,y,w);

   Int_t bin;
   if (fYmin != fYmax) {
      if (y <fYmin || y> fYmax) return -1;
   }

   Stat_t z= (w > 0 ? w : -w);
   fEntries++;
   bin =fXaxis.FindBin(x);
   AddBinContent(bin, z*y);
   fSumw2.fArray[bin] += z*y*y;
   fBinEntries.fArray[bin] += w;
   if (bin == 0 || bin > fXaxis.GetNbins()) return -1;
   fTsumw   += z;
   fTsumw2  += z*z;
   fTsumwx  += z*x;
   fTsumwx2 += z*x*x;
   return bin;
}

//______________________________________________________________________________
Int_t TProfile::Fill(const char *namex, Axis_t y, Stat_t w)
{
// Fill a Profile histogram with weights
//
   Int_t bin;

   if (fYmin != fYmax) {
      if (y <fYmin || y> fYmax) return -1;
   }

   Stat_t z= (w > 0 ? w : -w);
   fEntries++;
   bin =fXaxis.FindBin(namex);
   AddBinContent(bin, z*y);
   fSumw2.fArray[bin] += z*y*y;
   fBinEntries.fArray[bin] += w;
   if (bin == 0 || bin > fXaxis.GetNbins()) return -1;
   Axis_t x = fXaxis.GetBinCenter(bin);
   fTsumw   += z;
   fTsumw2  += z*z;
   fTsumwx  += z*x;
   fTsumwx2 += z*x*x;
   return bin;
}


//______________________________________________________________________________
void TProfile::FillN(Int_t ntimes, const Axis_t *x, const Axis_t *y, const Stat_t *w, Int_t stride)
{
//*-*-*-*-*-*-*-*-*-*-*Fill a Profile histogram with weights*-*-*-*-*-*-*-*
//*-*                  =====================================
   Int_t bin,i;
   ntimes *= stride;
   for (i=0;i<ntimes;i+=stride) {
      if (fYmin != fYmax) {
         if (y[i] <fYmin || y[i]> fYmax) continue;
      }

      Stat_t z= (w[i] > 0 ? w[i] : -w[i]);
      fEntries++;
      bin =fXaxis.FindBin(x[i]);
      AddBinContent(bin, z*y[i]);
      fSumw2.fArray[bin] += z*y[i]*y[i];
      fBinEntries.fArray[bin] += w[i];
      fTsumw   += z;
      fTsumw2  += z*z;
      fTsumwx  += z*x[i];
      fTsumwx2 += z*x[i]*x[i];
   }
}

//______________________________________________________________________________
Stat_t TProfile::GetBinContent(Int_t bin) const
{
//*-*-*-*-*-*-*Return bin content of a Profile histogram*-*-*-*-*-*-*-*-*-*
//*-*          =========================================

   if (fBuffer) ((TProfile*)this)->BufferEmpty();

   if (bin < 0 || bin >= fNcells) return 0;
   if (fBinEntries.fArray[bin] == 0) return 0;
   return fArray[bin]/fBinEntries.fArray[bin];
}

//______________________________________________________________________________
Stat_t TProfile::GetBinEntries(Int_t bin) const
{
//*-*-*-*-*-*-*Return bin entries of a Profile histogram*-*-*-*-*-*-*-*-*-*
//*-*          =========================================

   if (fBuffer) ((TProfile*)this)->BufferEmpty();

   if (bin < 0 || bin >= fNcells) return 0;
   return fBinEntries.fArray[bin];
}

//______________________________________________________________________________
Stat_t TProfile::GetBinError(Int_t bin) const
{
//*-*-*-*-*-*-*Return bin error of a Profile histogram*-*-*-*-*-*-*-*-*-*
//*-*          =======================================

   if (fBuffer) ((TProfile*)this)->BufferEmpty();

   if (bin < 0 || bin >= fNcells) return 0;
   Stat_t cont = fArray[bin];
   Stat_t sum  = fBinEntries.fArray[bin];
   Stat_t err2 = fSumw2.fArray[bin];
   if (sum == 0) return 0;
   Stat_t eprim;
   Stat_t contsum = cont/sum;
   Stat_t eprim2  = TMath::Abs(err2/sum - contsum*contsum);
   eprim          = TMath::Sqrt(eprim2);
   Double_t test = 1;
   if (err2 != 0 && sum < 5) test = eprim2*sum/err2;
//printf("bin=%d, cont=%g, sum=%g, err2=%g, eprim2=%g, test=%g\n",bin,cont,sum,err2,eprim2,test);
   //if statistics is unsufficient, take approximation.
   // error is set to the (average error on all bins) * 2
   //if (eprim <= 0) {   test in version 2.25/03
   //if (test < 1.e-4) { test in version 3.01/06
   if (test < 1.e-4 || eprim2 < 1e-6) {
      Stat_t scont, ssum, serr2;
      scont = ssum = serr2 = 0;
      for (Int_t i=1;i<fNcells;i++) {
         scont += fArray[i];
         ssum  += fBinEntries.fArray[i];
         serr2 += fSumw2.fArray[i];
      }
      Stat_t scontsum = scont/ssum;
      Stat_t seprim2  = TMath::Abs(serr2/ssum - scontsum*scontsum);
      eprim           = 2*TMath::Sqrt(seprim2);
      sum = ssum;
   }
   if (fErrorMode == kERRORMEAN) return eprim/TMath::Sqrt(sum);
   else if (fErrorMode == kERRORSPREAD) return eprim;
   else if (fErrorMode == kERRORSPREADI) {
      if (eprim != 0) return eprim/TMath::Sqrt(sum);
      return 1/TMath::Sqrt(12*sum);
   }
   else if (fErrorMode == kERRORSPREADG) {
      return eprim/TMath::Sqrt(sum);
   }
   else return eprim;
}

//______________________________________________________________________________
Option_t *TProfile::GetErrorOption() const
{
//*-*-*-*-*-*-*-*-*-*Return option to compute profile errors*-*-*-*-*-*-*-*-*
//*-*                =======================================

   if (fErrorMode == kERRORSPREAD)  return "s";
   if (fErrorMode == kERRORSPREADI) return "i";
   if (fErrorMode == kERRORSPREADG) return "g";
   return "";
}

//______________________________________________________________________________
void TProfile::GetStats(Stat_t *stats) const
{
   // fill the array stats from the contents of this profile
   // The array stats must be correctly dimensionned in the calling program.
   // stats[0] = sumw
   // stats[1] = sumw2
   // stats[2] = sumwx
   // stats[3] = sumwx2
   // stats[4] = sumwy
   // stats[5] = sumwy2
   //
   // The function recomputes the statistics quantities
   // from the bin contents in the current axis range.

   // Loop on bins
   Int_t bin, binx;
   Stat_t w;
   Axis_t x;
   for (bin=0;bin<6;bin++) stats[bin] = 0;
   for (binx=fXaxis.GetFirst();binx<=fXaxis.GetLast();binx++) {
      w         = fBinEntries.fArray[binx];
      x         = fXaxis.GetBinCenter(binx);
      stats[0] += w;
      stats[1] += w*w;
      stats[2] += w*x;
      stats[3] += w*x*x;
      stats[4] += fArray[binx];
      stats[5] += fSumw2.fArray[binx];
   }
}

//___________________________________________________________________________
void TProfile::LabelsDeflate(Option_t *)
{
// Reduce the number of bins for this axis to the number of bins having a label.

   if (!fXaxis.GetLabels()) return;
   TIter next(fXaxis.GetLabels());
   TObject *obj;
   Int_t nbins = 0;
   while ((obj = next())) {
      if (obj->GetUniqueID()) nbins++;
   }
   if (nbins < 2) nbins = 2;
   TProfile *hold = (TProfile*)Clone();
   hold->SetDirectory(0);

   Double_t xmin = fXaxis.GetXmin();
   Double_t xmax = fXaxis.GetBinUpEdge(nbins);
   fXaxis.SetRange(0,0);
   fXaxis.Set(nbins,xmin,xmax);
   Int_t ncells = nbins+2;
   SetBinsLength(ncells);
   fBinEntries.Set(ncells);
   fSumw2.Set(ncells);

   //now loop on all bins and refill
   Int_t bin;
   for (bin=1;bin<=nbins;bin++) {
      fArray[bin] = hold->fArray[bin];
      fBinEntries.fArray[bin] = hold->fBinEntries.fArray[bin];
      fSumw2.fArray[bin] = hold->fSumw2.fArray[bin];
   }
   delete hold;
}

//___________________________________________________________________________
void TProfile::LabelsInflate(Option_t *)
{
// Double the number of bins for axis.
// Refill histogram
// This function is called by TAxis::FindBin(const char *label)

   TProfile *hold = (TProfile*)Clone();
   hold->SetDirectory(0);

   Int_t  nbold  = fXaxis.GetNbins();
   Int_t nbins   = nbold;
   Double_t xmin = fXaxis.GetXmin();
   Double_t xmax = fXaxis.GetXmax();
   xmax = xmin + 2*(xmax-xmin);
   fXaxis.SetRange(0,0);
   fXaxis.Set(2*nbins,xmin,xmax);
   nbins *= 2;
   Int_t ncells = nbins+2;
   SetBinsLength(ncells);
   fBinEntries.Set(ncells);
   fSumw2.Set(ncells);

   //now loop on all bins and refill
   Int_t bin;
   for (bin=1;bin<=nbins;bin++) {
      if (bin <= nbold) {
         fArray[bin] = hold->fArray[bin];
         fBinEntries.fArray[bin] = hold->fBinEntries.fArray[bin];
         fSumw2.fArray[bin] = hold->fSumw2.fArray[bin];
      } else {
         fArray[bin] = 0;
         fBinEntries.fArray[bin] = 0;
         fSumw2.fArray[bin] = 0;
      }
   }
   delete hold;
}

//___________________________________________________________________________
void TProfile::LabelsOption(Option_t *option, Option_t *ax)
{
//  Set option(s) to draw axis with labels
//  option = "a" sort by alphabetic order
//         = ">" sort by decreasing values
//         = "<" sort by increasing values
//         = "h" draw labels horizonthal
//         = "v" draw labels vertical
//         = "u" draw labels up (end of label right adjusted)
//         = "d" draw labels down (start of label left adjusted)

   THashList *labels = fXaxis.GetLabels();
   if (!labels) {
      Warning("LabelsOption","Cannot sort. No labels");
      return;
   }
   TString opt = option;
   opt.ToLower();
   if (opt.Contains("h")) {
      fXaxis.SetBit(TAxis::kLabelsHori);
      fXaxis.ResetBit(TAxis::kLabelsVert);
      fXaxis.ResetBit(TAxis::kLabelsDown);
      fXaxis.ResetBit(TAxis::kLabelsUp);
   }
    if (opt.Contains("v")) {
      fXaxis.SetBit(TAxis::kLabelsVert);
      fXaxis.ResetBit(TAxis::kLabelsHori);
      fXaxis.ResetBit(TAxis::kLabelsDown);
      fXaxis.ResetBit(TAxis::kLabelsUp);
   }
   if (opt.Contains("u")) {
      fXaxis.SetBit(TAxis::kLabelsUp);
      fXaxis.ResetBit(TAxis::kLabelsVert);
      fXaxis.ResetBit(TAxis::kLabelsDown);
      fXaxis.ResetBit(TAxis::kLabelsHori);
   }
   if (opt.Contains("d")) {
      fXaxis.SetBit(TAxis::kLabelsDown);
      fXaxis.ResetBit(TAxis::kLabelsVert);
      fXaxis.ResetBit(TAxis::kLabelsHori);
      fXaxis.ResetBit(TAxis::kLabelsUp);
   }
   Int_t sort = -1;
   if (opt.Contains("a")) sort = 0;
   if (opt.Contains(">")) sort = 1;
   if (opt.Contains("<")) sort = 2;
   if (sort < 0) return;

   Int_t n = TMath::Min(fXaxis.GetNbins(), labels->GetSize());
   Int_t *a = new Int_t[n+2];
   Int_t i,j;
   Double_t *cont   = new Double_t[n+2];
   Double_t *sumw   = new Double_t[n+2];
   Double_t *errors = new Double_t[n+2];
   Double_t *ent    = new Double_t[n+2];
   THashList *labold = new THashList(labels->GetSize(),1);
   TIter nextold(labels);
   TObject *obj;
   while ((obj=nextold())) {
      labold->Add(obj);
   }
   labels->Clear();
   if (sort > 0) {
      //---sort by values of bins
      for (i=1;i<=n;i++) {
         sumw[i-1]   = fArray[i];
         errors[i-1] = fSumw2.fArray[i];
         ent[i-1]    = fBinEntries.fArray[i];
         if (fBinEntries.fArray[i] == 0) cont[i-1] = 0;
         else cont[i-1] = fArray[i]/fBinEntries.fArray[i];
      }
      if (sort ==1) TMath::Sort(n,cont,a,kTRUE);  //sort by decreasing values
      else          TMath::Sort(n,cont,a,kFALSE); //sort by increasing values
      for (i=1;i<=n;i++) {
         fArray[i] = sumw[a[i-1]];
         fSumw2.fArray[i] = errors[a[i-1]];
         fBinEntries.fArray[i] = ent[a[i-1]];
      }
      for (i=1;i<=n;i++) {
         obj = labold->At(a[i-1]);
         labels->Add(obj);
         obj->SetUniqueID(i);
      }
   } else {
      //---alphabetic sort
      const UInt_t kUsed = 1<<18;
      TObject *objk=0;
      a[0] = 0;
      a[n+1] = n+1;
      for (i=1;i<=n;i++) {
         const char *label = "zzzzzzzzzzzz";
         for (j=1;j<=n;j++) {
            obj = labold->At(j-1);
            if (!obj) continue;
            if (obj->TestBit(kUsed)) continue;
            //use strcasecmp for case non-sensitive sort (may be an option)
            if (strcmp(label,obj->GetName()) < 0) continue;
            objk = obj;
            a[i] = j;
            label = obj->GetName();
         }
         if (objk) {
            objk->SetUniqueID(i);
            labels->Add(objk);
            objk->SetBit(kUsed);
         }
      }
      for (i=1;i<=n;i++) {
         obj = labels->At(i-1);
         if (!obj) continue;
         obj->ResetBit(kUsed);
      }

      for (i=1;i<=n;i++) {
         sumw[i]   = fArray[a[i]];
         errors[i] = fSumw2.fArray[a[i]];
         ent[i]    = fBinEntries.fArray[a[i]];
      }
      for (i=1;i<=n;i++) {
         fArray[i] = sumw[a[i]];
         fSumw2.fArray[i] = errors[a[i]];
         fBinEntries.fArray[i] = ent[a[i]];
      }
   }
   delete labold;
   if (a)      delete [] a;
   if (sumw)   delete [] sumw;
   if (cont)   delete [] cont;
   if (errors) delete [] errors;
   if (ent)    delete [] ent;
}

//______________________________________________________________________________
Int_t TProfile::Merge(TCollection *list)
{
   //Merge all histograms in the collection in this histogram.
   //This function computes the min/max for the x axis,
   //compute a new number of bins, if necessary,
   //add bin contents, errors and statistics.
   //The function returns the merged number of entries if the merge is
   //successfull, -1 otherwise.
   //
   //IMPORTANT remark. The axis x may have different number
   //of bins and different limits, BUT the largest bin width must be
   //a multiple of the smallest bin width.

   if (!list) return 0;
   TIter next(list);
   Double_t umin,umax;
   Int_t nx;
   Double_t xmin  = fXaxis.GetXmin();
   Double_t xmax  = fXaxis.GetXmax();
   Double_t bwix  = fXaxis.GetBinWidth(1);
   Int_t    nbix  = fXaxis.GetNbins();

   TProfile *h;
   Int_t nentries=0;
   Bool_t same = kTRUE;
   while ((h=(TProfile*)next())) {
      if (!h->InheritsFrom(TProfile::Class())) {
         Error("Add","Attempt to add object of class: %s to a %s",h->ClassName(),this->ClassName());
         return -1;
      }
      //import statistics
      nentries += (Int_t)h->GetEntries();

      // find min/max of the axes
      umin = h->GetXaxis()->GetXmin();
      umax = h->GetXaxis()->GetXmax();
      nx   = h->GetXaxis()->GetNbins();
      if (nx != nbix || umin != xmin || umax != xmax) {
         same = kFALSE;
         if (umin < xmin) xmin = umin;
         if (umax > xmax) xmax = umax;
         if (h->GetXaxis()->GetBinWidth(1) > bwix) bwix = h->GetXaxis()->GetBinWidth(1);
      }
   }

   //  if different binning compute best binning
   if (!same) {
      nbix = (Int_t) ((xmax-xmin)/bwix +0.1); while(nbix > 100) nbix /= 2;
      SetBins(nbix,xmin,xmax);
   }

   //merge bin contents and errors
   next.Reset();
   Int_t ibin, bin;
   while ((h=(TProfile*)next())) {
      nx   = h->GetXaxis()->GetNbins();
      for (bin=0;bin<=nx+1;bin++) {
         ibin = fXaxis.FindBin(h->GetBinCenter(bin));
         fArray[ibin]             += h->GetW()[bin];
         fSumw2.fArray[ibin]      += h->GetW2()[bin];
         fBinEntries.fArray[ibin] += h->GetB()[bin];
      }
      fEntries += h->GetEntries();
      fTsumw   += h->fTsumw;
      fTsumw2  += h->fTsumw2;
      fTsumwx  += h->fTsumwx;
      fTsumwx2 += h->fTsumwx2;
   }

   return nentries;
}


//______________________________________________________________________________
void TProfile::Multiply(TF1 *, Double_t )
{
   // Performs the operation: this = this*c1*f1

   Error("Multiply","Function not implemented for TProfile");
   return;
}

//______________________________________________________________________________
void TProfile::Multiply(const TH1 *)
{
//*-*-*-*-*-*-*-*-*-*-*Multiply this profile by h1*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                  =============================
//
//   this = this*h1
//
   Error("Multiply","Multiplication of profile histograms not implemented");
}


//______________________________________________________________________________
void TProfile::Multiply(const TH1 *, const TH1 *, Double_t, Double_t, Option_t *)
{
//*-*-*-*-*Replace contents of this profile by multiplication of h1 by h2*-*
//*-*      ================================================================
//
//   this = (c1*h1)*(c2*h2)
//

   Error("Multiply","Multiplication of profile histograms not implemented");
}

//______________________________________________________________________________
TH1D *TProfile::ProjectionX(const char *name, Option_t *option) const
{
//*-*-*-*-*Project this profile into a 1-D histogram along X*-*-*-*-*-*-*
//*-*      =================================================
//
//   The projection is always of the type TH1D.
//
//   if option "E" is specified, the errors are computed. (default)
//
//

  TString opt = option;
  opt.ToLower();
  Int_t nx = fXaxis.GetNbins();

// Create the projection histogram
  char *pname = (char*)name;
  if (strcmp(name,"_px") == 0) {
     Int_t nch = strlen(GetName()) + 4;
     pname = new char[nch];
     sprintf(pname,"%s%s",GetName(),name);
  }
  TH1D *h1;
  const TArrayD *bins = fXaxis.GetXbins();
  if (bins->fN == 0) {
     h1 = new TH1D(pname,GetTitle(),nx,fXaxis.GetXmin(),fXaxis.GetXmax());
  } else {
     h1 = new TH1D(pname,GetTitle(),nx,bins->fArray);
  }
  Bool_t computeErrors = kFALSE;
  if (opt.Contains("e")) {h1->Sumw2(); computeErrors = kTRUE;}
  if (pname != name)  delete [] pname;

// Fill the projected histogram
  Double_t cont,err;
  for (Int_t binx =0;binx<=nx+1;binx++) {
     cont  = GetBinContent(binx);
     err   = GetBinError(binx);
     if (cont)          h1->Fill(fXaxis.GetBinCenter(binx), cont);
     if (computeErrors) h1->SetBinError(binx,err);
  }
  h1->SetEntries(fEntries);
  return h1;
}

//______________________________________________________________________________
TH1 *TProfile::Rebin(Int_t ngroup, const char*newname)
{
//*-*-*-*-*Rebin this profile grouping ngroup bins together*-*-*-*-*-*-*-*-*
//*-*      ================================================
//   if newname is not blank a new temporary profile hnew is created.
//   else the current profile is modified (default)
//   The parameter ngroup indicates how many bins of this have to me merged
//   into one bin of hnew
//   If the original profile has errors stored (via Sumw2), the resulting
//   profile has new errors correctly calculated.
//
//   examples: if hp is an existing TProfile histogram with 100 bins
//     hp->Rebin();  //merges two bins in one in hp: previous contents of hp are lost
//     hp->Rebin(5); //merges five bins in one in hp
//     TProfile *hnew = hp->Rebin(5,"hnew"); // creates a new profile hnew
//                                       //merging 5 bins of hp in one bin
//
//   NOTE1: If ngroup is not an exact divider of the number of bins,
//          the top limit of the rebinned profile is changed
//          to the upper edge of the bin=newbins*ngroup and the corresponding
//          bins are added to the overflow bin.
//          Statistics will be recomputed from the new bin contents.

   Int_t nbins  = fXaxis.GetNbins();
   Axis_t xmin  = fXaxis.GetXmin();
   Axis_t xmax  = fXaxis.GetXmax();
   if ((ngroup <= 0) || (ngroup > nbins)) {
      Error("Rebin", "Illegal value of ngroup=%d",ngroup);
      return 0;
   }
   Int_t newbins = nbins/ngroup;

   // Save old bin contents into a new array
   Double_t *oldBins   = new Double_t[nbins];
   Double_t *oldCount  = new Double_t[nbins];
   Double_t *oldErrors = new Double_t[nbins];
   Int_t bin, i;
   Double_t *cu1 = GetW();
   Double_t *er1 = GetW2();
   Double_t *en1 = GetB();
   for (bin=0;bin<nbins;bin++) {
      oldBins[bin]   = cu1[bin+1];
      oldCount[bin]  = en1[bin+1];
      oldErrors[bin] = er1[bin+1];
   }

   // create a clone of the old histogram if newname is specified
   TProfile *hnew = this;
   if (strlen(newname) > 0) {
      hnew = (TProfile*)Clone();
      hnew->SetName(newname);
   }

   // change axis specs and rebuild bin contents array
   if(newbins*ngroup != nbins) {
      xmax = fXaxis.GetBinUpEdge(newbins*ngroup);
      hnew->fTsumw = 0; //stats must be reset because top bins will be moved to overflow bin
   }
   hnew->SetBins(newbins,xmin,xmax); //this also changes errors array (if any)

   // copy merged bin contents (ignore under/overflows)
   Double_t *cu2 = hnew->GetW();
   Double_t *er2 = hnew->GetW2();
   Double_t *en2 = hnew->GetB();
   Int_t oldbin = 0;
   Double_t binContent, binCount, binError;
   for (bin = 0;bin<=newbins;bin++) {
      binContent = 0;
      binCount   = 0;
      binError   = 0;
      for (i=0;i<ngroup;i++) {
         if (oldbin+i >= nbins) break;
         binContent += oldBins[oldbin+i];
         binCount   += oldCount[oldbin+i];
         binError   += oldErrors[oldbin+i];
      }
      cu2[bin+1] = binContent;
      er2[bin+1] = binError;
      en2[bin+1] = binCount;
      oldbin += ngroup;
   }

   delete [] oldBins;
   delete [] oldCount;
   delete [] oldErrors;
   return hnew;
}

//______________________________________________________________________________
void TProfile::Reset(Option_t *option)
{
//*-*-*-*-*-*-*-*-*-*Reset contents of a Profile histogram*-*-*-*-*-*-*-*-*
//*-*                =====================================
  TH1D::Reset(option);
  fBinEntries.Reset();
}

//______________________________________________________________________________
void TProfile::SavePrimitive(ofstream &out, Option_t *option)
{
    // Save primitive as a C++ statement(s) on output stream out

   //Note the following restrictions in the code generated:
   // - variable bin size not implemented
   // - SetErrorOption not implemented

   char quote = '"';
   out<<"   "<<endl;
   out<<"   "<<ClassName()<<" *";

   out<<GetName()<<" = new "<<ClassName()<<"("<<quote<<GetName()<<quote<<","<<quote<<GetTitle()<<quote
                 <<","<<GetXaxis()->GetNbins()
                 <<","<<GetXaxis()->GetXmin()
                 <<","<<GetXaxis()->GetXmax()
                 <<","<<quote<<GetErrorOption()<<quote<<");"<<endl;

   if (fMinimum != -1111) {
      out<<"   "<<GetName()<<"->SetMinimum("<<fMinimum<<");"<<endl;
   }
   if (fMaximum != -1111) {
      out<<"   "<<GetName()<<"->SetMaximum("<<fMaximum<<");"<<endl;
   }
   if (fNormFactor != 0) {
      out<<"   "<<GetName()<<"->SetNormFactor("<<fNormFactor<<");"<<endl;
   }
   if (fEntries != 0) {
      out<<"   "<<GetName()<<"->SetEntries("<<fEntries<<");"<<endl;
   }
   Int_t bin;
   for (bin=0;bin<fNcells;bin++) {
      Double_t bi = GetBinEntries(bin);
      if (bi) {
         out<<"   "<<GetName()<<"->SetBinEntries("<<bin<<","<<bi<<");"<<endl;
      }
   }
   for (bin=0;bin<fNcells;bin++) {
      Double_t bc = fArray[bin];
      if (bc) {
         out<<"   "<<GetName()<<"->SetBinContent("<<bin<<","<<bc<<");"<<endl;
      }
   }
   if (fSumw2.fN) {
      for (bin=0;bin<fNcells;bin++) {
         Double_t be = TMath::Sqrt(fSumw2.fArray[bin]);
         if (be) {
            out<<"   "<<GetName()<<"->SetBinError("<<bin<<","<<be<<");"<<endl;
         }
      }
   }

   // save list of functions
   TIter next(fFunctions);
   TObject *obj;
   while ((obj=next())) {
      obj->SavePrimitive(out,"nodraw");
      out<<"   "<<GetName()<<"->GetListOfFunctions()->Add("<<obj->GetName()<<");"<<endl;
   }

   SaveFillAttributes(out,GetName(),0,1001);
   SaveLineAttributes(out,GetName(),1,1,1);
   SaveMarkerAttributes(out,GetName(),1,1,1);
   fXaxis.SaveAttributes(out,GetName(),"->GetXaxis()");
   fYaxis.SaveAttributes(out,GetName(),"->GetYaxis()");
   fZaxis.SaveAttributes(out,GetName(),"->GetZaxis()");
   out<<"   "<<GetName()<<"->Draw("
      <<quote<<option<<quote<<");"<<endl;
}

//______________________________________________________________________________
void TProfile::Scale(Double_t c1)
{
//*-*-*-*-*Multiply this profile by a constant c1*-*-*-*-*-*-*-*-*
//*-*      ======================================
//
//   this = c1*this
//
// This function uses the services of TProfile::Add
//

   Double_t ent = fEntries;
   fScaling = kTRUE;
   Add(this,this,c1,0);
   fScaling = kFALSE;
   fEntries = ent;
}

//______________________________________________________________________________
void TProfile::SetBinEntries(Int_t bin, Stat_t w)
{
//*-*-*-*-*-*-*-*-*Set the number of entries in bin*-*-*-*-*-*-*-*-*-*-*-*
//*-*              ================================

   if (bin < 0 || bin >= fNcells) return;
   fBinEntries.fArray[bin] = w;
}

//______________________________________________________________________________
void TProfile::SetBins(Int_t nx, Double_t xmin, Double_t xmax)
{
//*-*-*-*-*-*-*-*-*Redefine  x axis parameters*-*-*-*-*-*-*-*-*-*-*-*
//*-*              ===========================

   fXaxis.Set(nx,xmin,xmax);
   fNcells = nx+2;
   SetBinsLength(fNcells);
   fBinEntries.Set(fNcells);
   fSumw2.Set(fNcells);
}


//______________________________________________________________________________
void TProfile::SetBuffer(Int_t buffersize, Option_t *option)
{
// set the buffer size in units of 8 bytes (double)

   if (fBuffer) {
      BufferEmpty();
      delete [] fBuffer;
      fBuffer = 0;
   }
   if (buffersize <= 0) {
      fBufferSize = 0;
      return;
   }
   if (buffersize < 100) buffersize = 100;
   fBufferSize = 1 + 3*buffersize;
   fBuffer = new Double_t[fBufferSize];
   memset(fBuffer,0,8*fBufferSize);
}

//______________________________________________________________________________
void TProfile::SetErrorOption(Option_t *option)
{
//*-*-*-*-*-*-*-*-*-*Set option to compute profile errors*-*-*-*-*-*-*-*-*
//*-*                =====================================
//
//    The computation of errors is based on the parameter option:
//    option:
//     ' '  (Default) Errors are Spread/SQRT(N) for Spread.ne.0. ,
//                      "     "  SQRT(Y)/SQRT(N) for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//     's'            Errors are Spread  for Spread.ne.0. ,
//                      "     "  SQRT(Y)  for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//     'i'            Errors are Spread/SQRT(N) for Spread.ne.0. ,
//                      "     "  1./SQRT(12.*N) for Spread.eq.0,N.gt.0 ,
//                      "     "  0.  for N.eq.0
//   See TProfile::BuildOptions for explanation of all options

   TString opt = option;
   opt.ToLower();
   fErrorMode = kERRORMEAN;
   if (opt.Contains("s")) fErrorMode = kERRORSPREAD;
   if (opt.Contains("i")) fErrorMode = kERRORSPREADI;
   if (opt.Contains("g")) fErrorMode = kERRORSPREADG;
}

//______________________________________________________________________________
void TProfile::Streamer(TBuffer &R__b)
{
   // Stream an object of class TProfile.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TProfile::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      TH1D::Streamer(R__b);
      fBinEntries.Streamer(R__b);
      R__b >> (Int_t&)fErrorMode;
      if (R__v < 2) {
         Float_t ymin,ymax;
         R__b >> ymin; fYmin = ymin;
         R__b >> ymax; fYmax = ymax;
      } else {
         R__b >> fYmin;
         R__b >> fYmax;
      }
      R__b.CheckByteCount(R__s, R__c, TProfile::IsA());
      //====end of old versions

   } else {
      TProfile::Class()->WriteBuffer(R__b,this);
   }
}
