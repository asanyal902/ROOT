// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveDigitSet.h"

#include "TEveManager.h"

#include "TColor.h"

#include "TBuffer3D.h"
#include "TBuffer3DTypes.h"
#include "TVirtualPad.h"
#include "TVirtualViewer3D.h"

//______________________________________________________________________________
// TEveDigitSet
//
// Base-class for storage of digit collections; provides
// transformation matrix (TEveTrans), signal to color mapping
// (TEveRGBAPalette) and visual grouping (TEveFrameBox).
//
// Base-class for displaying a digit collection.
// Provdies common services for:
// - specifying signal / color per digit
// - specifying object reference per digit
// - controlling palette and thresholds (external object TEveRGBAPalette)
// - showing a frame around the digits (external object TEveFrameBox)
// - specifying transformation matrix for the whole collection
//   by data-member of class TEveTrans.
//
// See also:
//   TEveQuadSet: rectangle, hexagon or line per digit
//   TEveBoxSet   a 3D box per digit

ClassImp(TEveDigitSet)

//______________________________________________________________________________
TEveDigitSet::TEveDigitSet(const Text_t* n, const Text_t* t) :
   TEveElement     (),
   TNamed          (n, t),

   fDefaultValue   (kMinInt),
   fValueIsColor   (kFALSE),
   fOwnIds         (kFALSE),
   fPlex           (),
   fLastDigit      (0),

   fFrame          (0),
   fPalette        (0),
   fRenderMode     (RM_Fill),
   fDisableLigting (kTRUE),
   fEmitSignals    (kFALSE),
   fHistoButtons   (kTRUE),
   fHMTrans        ()
{
   // Constructor.
}

//______________________________________________________________________________
TEveDigitSet::~TEveDigitSet()
{
   // Destructor.
   // Unreference frame and palette. Destroy referenced objects if they
   // are owned by the TEveDigitSet.

   SetFrame(0);
   SetPalette(0);
   if (fOwnIds)
      ReleaseIds();
}

/******************************************************************************/

//______________________________________________________________________________
TEveDigitSet::DigitBase* TEveDigitSet::NewDigit()
{
   // Protected method called whenever a new digit is added.

   fLastDigit = new (fPlex.NewAtom()) DigitBase(fDefaultValue);
   return fLastDigit;
}

//______________________________________________________________________________
void TEveDigitSet::ReleaseIds()
{
   // Protected method. Release and delete the referenced objects, the
   // ownership is *NOT* checked.

   TEveChunkManager::iterator qi(fPlex);
   while (qi.next()) {
      DigitBase& q = * (DigitBase*) qi();
      if (q.fId.GetObject()) {
         delete q.fId.GetObject();
         q.fId = 0;
      }
   }
}

/******************************************************************************/
/******************************************************************************/

//______________________________________________________________________________
void TEveDigitSet::SetMainColor(Color_t color)
{
   // Override from TEveElement, forward to Frame.

   if (fFrame) {
      fFrame->SetFrameColor(color);
      fFrame->UpdateBackPtrItems();
   }
   gEve->Redraw3D();
}

/******************************************************************************/
/******************************************************************************/

//______________________________________________________________________________
void TEveDigitSet::RefitPlex()
{
   // Instruct underlying memory allocator to regroup itself into a
   // contiguous memory chunk.

   fPlex.Refit();
}

/******************************************************************************/

//______________________________________________________________________________
void TEveDigitSet::ScanMinMaxValues(Int_t& min, Int_t& max)
{
   // Iterate over the digits and detmine min and max signal values.

   if (fValueIsColor || fPlex.Size() == 0) return;
   min = kMaxInt;
   max = kMinInt;
   for (Int_t c=0; c<fPlex.VecSize(); ++c)
   {
      Char_t* a = fPlex.Chunk(c);
      Int_t   n = fPlex.NAtoms(c);
      while (n--)
      {
         Int_t v = ((DigitBase*)a)->fValue;
         if (v < min) min = v;
         if (v > max) max = v;
         a += fPlex.S();
      }
   }
   if (min == max)
      --min;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveDigitSet::DigitValue(Int_t value)
{
   // Set signal value for the last digit added.

   fLastDigit->fValue = value;
}

//______________________________________________________________________________
void TEveDigitSet::DigitColor(Color_t ci)
{
   // Set color for the last digit added.

   TEveUtil::ColorFromIdx(ci, (UChar_t*) & fLastDigit->fValue, kTRUE);
}

//______________________________________________________________________________
void TEveDigitSet::DigitColor(UChar_t r, UChar_t g, UChar_t b, UChar_t a)
{
   // Set color for the last digit added.

   UChar_t* x = (UChar_t*) & fLastDigit->fValue;
   x[0] = r; x[1] = g; x[2] = b; x[3] = a;
}

//______________________________________________________________________________
void TEveDigitSet::DigitColor(UChar_t* rgba)
{
   // Set color for the last digit added.

   UChar_t* x = (UChar_t*) & fLastDigit->fValue;
   x[0] = rgba[0]; x[1] = rgba[1]; x[2] = rgba[2]; x[3] = rgba[3];
}

//______________________________________________________________________________
void TEveDigitSet::DigitId(TObject* id)
{
   // Set external object reference for the last digit added.

   fLastDigit->fId = id;
}

/******************************************************************************/
/******************************************************************************/

//______________________________________________________________________________
void TEveDigitSet::Paint(Option_t* /*option*/)
{
   // Paint this object. Only direct rendering is supported.

   static const TEveException eH("TEveDigitSet::Paint ");

   TBuffer3D buff(TBuffer3DTypes::kGeneric);

   // Section kCore
   buff.fID           = this;
   buff.fColor        = fFrame ? fFrame->GetFrameColor() : 1;
   buff.fTransparency = 0;
   fHMTrans.SetBuffer3D(buff);
   buff.SetSectionsValid(TBuffer3D::kCore);

   Int_t reqSections = gPad->GetViewer3D()->AddObject(buff);
   if (reqSections != TBuffer3D::kNone)
      Error(eH, "only direct GL rendering supported.");
}

//______________________________________________________________________________
void TEveDigitSet::DigitSelected(Int_t idx)
{
   // Called from renderer when a digit with index idx is selected.

   if (fEmitSignals) {
      CtrlClicked(this, idx);
   } else {
      DigitBase* qb = GetDigit(idx);
      TObject* obj = qb->fId.GetObject();
      printf("TEveDigitSet::DigitSelected idx=%d, value=%d, obj=0x%lx\n",
             idx, qb->fValue, (ULong_t)obj);
      if (obj)
         obj->Print();
   }
}

//______________________________________________________________________________
void TEveDigitSet::CtrlClicked(TEveDigitSet* qs, Int_t idx)
{
   // Emit a CtrlClicked signal.

   Long_t args[2];
   args[0] = (Long_t) qs;
   args[1] = (Long_t) idx;

   Emit("CtrlClicked(TEveDigitSet*, Int_t)", args);
}

/******************************************************************************/
// Getters / Setters for Frame, TEveRGBAPalette, TEveTrans
/******************************************************************************/

//______________________________________________________________________________
void TEveDigitSet::SetFrame(TEveFrameBox* b)
{
   // Set TEveFrameBox pointer.

   if (fFrame == b) return;
   if (fFrame) fFrame->DecRefCount(this);
   fFrame = b;
   if (fFrame) {
      fFrame->IncRefCount(this);
      SetMainColorPtr(fFrame->PtrFrameColor());
   } else {
      SetMainColorPtr(0);
   }
}

//______________________________________________________________________________
void TEveDigitSet::SetPalette(TEveRGBAPalette* p)
{
   // Set TEveRGBAPalette pointer.

   if (fPalette == p) return;
   if (fPalette) fPalette->DecRefCount();
   fPalette = p;
   if (fPalette) fPalette->IncRefCount();
}

//______________________________________________________________________________
TEveRGBAPalette* TEveDigitSet::AssertPalette()
{
   // Make sure the TEveRGBAPalette pointer is not null.
   // If it is not set, a new one is instantiated and the range is set
   // to current min/max signal values.

   if (fPalette == 0) {
      fPalette = new TEveRGBAPalette;
      if (!fValueIsColor) {
         Int_t min, max;
         ScanMinMaxValues(min, max);
         fPalette->SetLimits(min, max);
         fPalette->SetMinMax(min, max);
      }
   }
   return fPalette;
}
