// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveRGBAPalette.h"

#include "TColor.h"
#include "TStyle.h"
#include "TMath.h"
//______________________________________________________________________________
// TEveRGBAPalette
//

ClassImp(TEveRGBAPalette)

//______________________________________________________________________________
TEveRGBAPalette::TEveRGBAPalette() :
   TObject(),
   TEveRefCnt(),

   fLowLimit(0), fHighLimit(0), fMinVal(0), fMaxVal(0), fNBins(0),

   fInterpolate     (kFALSE),
   fShowDefValue    (kTRUE),
   fUnderflowAction (LA_Cut),
   fOverflowAction  (LA_Clip),

   fDefaultColor(0),
   fUnderColor  (1),
   fOverColor   (2),
   fColorArray  (0)
{
   SetLimits(0, 1024);
   SetMinMax(0,  512);
}

//______________________________________________________________________________
TEveRGBAPalette::TEveRGBAPalette(Int_t min, Int_t max, Bool_t interp, Bool_t showdef) :
   TObject(),
   TEveRefCnt(),

   fLowLimit(0), fHighLimit(0), fMinVal(0), fMaxVal(0), fNBins(0),

   fInterpolate     (interp),
   fShowDefValue    (showdef),
   fUnderflowAction (LA_Cut),
   fOverflowAction  (LA_Clip),

   fDefaultColor(0),
   fUnderColor  (1),
   fOverColor   (2),
   fColorArray  (0)
{
   SetLimits(min, max);
   SetMinMax(min, max);
}

//______________________________________________________________________________
TEveRGBAPalette::~TEveRGBAPalette()
{
   delete [] fColorArray;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveRGBAPalette::SetupColor(Int_t val, UChar_t* pixel) const
{
   using namespace TMath;
   Float_t div  = Max(1, fMaxVal - fMinVal);
   Int_t   nCol = gStyle->GetNumberOfColors();

   Float_t f;
   if      (val >= fMaxVal) f = nCol - 1;
   else if (val <= fMinVal) f = 0;
   else                     f = (val - fMinVal)/div*(nCol - 1);

   if (fInterpolate) {
      Int_t  bin = (Int_t) f;
      Float_t f1 = f - bin, f2 = 1.0f - f1;
      TEveUtil::ColorFromIdx(f1, gStyle->GetColorPalette(bin),
                             f2, gStyle->GetColorPalette(Min(bin + 1, nCol - 1)),
                             pixel);
   } else {
      TEveUtil::ColorFromIdx(gStyle->GetColorPalette((Int_t) Nint(f)), pixel);
   }
}

//______________________________________________________________________________
void TEveRGBAPalette::SetupColorArray() const
{
   if(fColorArray) // !!!! should reinit anyway, maybe palette in gstyle changed
      return;

   // !!!! probably should store original palette for editing ...

   fColorArray = new UChar_t [4 * fNBins];
   UChar_t* p = fColorArray;
   for(Int_t v=fMinVal; v<=fMaxVal; ++v, p+=4)
      SetupColor(v, p);
}

//______________________________________________________________________________
void TEveRGBAPalette::ClearColorArray()
{
   if(fColorArray) {
      delete [] fColorArray;
      fColorArray = 0;
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveRGBAPalette::SetLimits(Int_t low, Int_t high)
{
   fLowLimit  = low;
   fHighLimit = high;
   Bool_t changed = kFALSE;
   if (fMaxVal < fLowLimit)  { SetMax(fLowLimit);  changed = kTRUE; }
   if (fMinVal < fLowLimit)  { SetMin(fLowLimit);  changed = kTRUE; }
   if (fMinVal > fHighLimit) { SetMin(fHighLimit); changed = kTRUE; }
   if (fMaxVal > fHighLimit) { SetMax(fHighLimit); changed = kTRUE; }
   if (changed)
      ClearColorArray();
}

//______________________________________________________________________________
void TEveRGBAPalette::SetLimitsScaleMinMax(Int_t low, Int_t high)
{
   Float_t rng_old = fHighLimit - fLowLimit;
   Float_t rng_new = high - low;

   fMinVal = TMath::Nint(low + (fMinVal - fLowLimit)*rng_new/rng_old);
   fMaxVal = TMath::Nint(low + (fMaxVal - fLowLimit)*rng_new/rng_old);
   fLowLimit  = low;
   fHighLimit = high;

   fNBins  = fMaxVal - fMinVal + 1;
   ClearColorArray();
}

//______________________________________________________________________________
void TEveRGBAPalette::SetMin(Int_t min)
{
   fMinVal = TMath::Min(min, fMaxVal);
   fNBins  = fMaxVal - fMinVal + 1;
   ClearColorArray();
}

//______________________________________________________________________________
void TEveRGBAPalette::SetMax(Int_t max)
{
   fMaxVal = TMath::Max(max, fMinVal);
   fNBins  = fMaxVal - fMinVal + 1;
   ClearColorArray();
}

//______________________________________________________________________________
void TEveRGBAPalette::SetMinMax(Int_t min, Int_t max)
{
   fMinVal = min;
   fMaxVal = max;
   fNBins  = fMaxVal - fMinVal + 1;
   ClearColorArray();
}

/******************************************************************************/

//______________________________________________________________________________
void TEveRGBAPalette::SetInterpolate(Bool_t b)
{
   fInterpolate = b;
   ClearColorArray();
}

/******************************************************************************/

//______________________________________________________________________________
void TEveRGBAPalette::SetDefaultColor(Color_t ci)
{
   fDefaultColor = ci;
   TEveUtil::ColorFromIdx(ci, fDefaultRGBA, kTRUE);
}

//______________________________________________________________________________
void TEveRGBAPalette::SetDefaultColor(Pixel_t pix)
{
   SetDefaultColor(Color_t(TColor::GetColor(pix)));
}

//______________________________________________________________________________
void TEveRGBAPalette::SetDefaultColor(UChar_t r, UChar_t g, UChar_t b, UChar_t a)
{
   fDefaultColor = Color_t(TColor::GetColor(r, g, b));
   fDefaultRGBA[0] = r;
   fDefaultRGBA[1] = g;
   fDefaultRGBA[2] = b;
   fDefaultRGBA[3] = a;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveRGBAPalette::SetUnderColor(Color_t ci)
{
   fUnderColor = ci;
   TEveUtil::ColorFromIdx(ci, fUnderRGBA, kTRUE);
}

//______________________________________________________________________________
void TEveRGBAPalette::SetUnderColor(Pixel_t pix)
{
   SetUnderColor(Color_t(TColor::GetColor(pix)));
}

//______________________________________________________________________________
void TEveRGBAPalette::SetUnderColor(UChar_t r, UChar_t g, UChar_t b, UChar_t a)
{
   fUnderColor = Color_t(TColor::GetColor(r, g, b));
   fUnderRGBA[0] = r;
   fUnderRGBA[1] = g;
   fUnderRGBA[2] = b;
   fUnderRGBA[3] = a;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveRGBAPalette::SetOverColor(Color_t ci)
{
   fOverColor = ci;
   TEveUtil::ColorFromIdx(ci, fOverRGBA, kTRUE);
}

//______________________________________________________________________________
void TEveRGBAPalette::SetOverColor(Pixel_t pix)
{
   SetOverColor(Color_t(TColor::GetColor(pix)));
}

//______________________________________________________________________________
void TEveRGBAPalette::SetOverColor(UChar_t r, UChar_t g, UChar_t b, UChar_t a)
{
   fOverColor = Color_t(TColor::GetColor(r, g, b));
   fOverRGBA[0] = r;
   fOverRGBA[1] = g;
   fOverRGBA[2] = b;
   fOverRGBA[3] = a;
}
