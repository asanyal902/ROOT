// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveDigitSet
#define ROOT_TEveDigitSet

#include "TNamed.h"
#include "TQObject.h"
#include "TAtt3D.h"
#include "TAttBBox.h"

#include "TEveUtil.h"
#include "TEveElement.h"
#include "TEveFrameBox.h"
#include "TEveRGBAPalette.h"
#include "TEveChunkManager.h"
#include "TEveTrans.h"

#include "TObject.h"

class TEveDigitSet : public TEveElement,
                     public TNamed, public TQObject,
                     public TAtt3D,
                     public TAttBBox
{
   friend class TEveDigitSetEditor;

   TEveDigitSet(const TEveDigitSet&);            // Not implemented
   TEveDigitSet& operator=(const TEveDigitSet&); // Not implemented

public:
   enum ERenderMode_e { kRM_AsIs, kRM_TEveLine, kRM_Fill };

protected:
   struct DigitBase_t
   {
      // Base-class for digit representation classes.

      Int_t fValue; // signal value of a digit (can be direct RGBA color)
      TRef  fId;    // external object reference

      DigitBase_t(Int_t v=0) : fValue(v), fId() {}
   };

   Int_t             fDefaultValue;   // Default signal value.
   Bool_t            fValueIsColor;   // Interpret signal value as RGBA color.
   Bool_t            fOwnIds;         // Flag specifying if id-objects are owned by the TEveDigitSet
   TEveChunkManager  fPlex;           // Container of digit data.
   DigitBase_t*      fLastDigit;      //! The last digit added to collection.

   TEveFrameBox*     fFrame;          // Pointer to frame structure.
   TEveRGBAPalette*  fPalette;        // Pointer to signal-color palette.
   ERenderMode_e     fRenderMode;     // Render mode: as-is / line / filled.
   Bool_t            fDisableLigting; // Disable lighting for rendering.
   Bool_t            fEmitSignals;    // Emit signals on secondary-select.
   Bool_t            fHistoButtons;   // Show histogram buttons in object editor.
   TEveTrans         fHMTrans;        // Overall transformation of whole collection.

   DigitBase_t* NewDigit();
   void       ReleaseIds();

public:
   TEveDigitSet(const Text_t* n="TEveDigitSet", const Text_t* t="");
   virtual ~TEveDigitSet();

   virtual Bool_t CanEditMainColor() { return kTRUE; }
   virtual void   SetMainColor(Color_t color);

   // Implemented in sub-classes:
   // virtual void Reset(EQuadType_e quadType, Bool_t valIsCol, Int_t chunkSize);

   void RefitPlex();
   void ScanMinMaxValues(Int_t& min, Int_t& max);

   // --------------------------------

   void DigitValue(Int_t value);
   void DigitColor(Color_t ci);
   void DigitColor(UChar_t r, UChar_t g, UChar_t b, UChar_t a=255);
   void DigitColor(UChar_t* rgba);

   void DigitId(TObject* id);

   Bool_t GetOwnIds() const     { return fOwnIds; }
   void   SetOwnIds(Bool_t o)   { fOwnIds = o; }

   DigitBase_t* GetDigit(Int_t n) { return (DigitBase_t*) fPlex.Atom(n);   }
   TObject*     GetId(Int_t n)    { return GetDigit(n)->fId.GetObject(); }

   // --------------------------------

   // Implemented in subclasses:
   // virtual void ComputeBBox();

   virtual void Paint(Option_t* option="");

   virtual void DigitSelected(Int_t idx);
   virtual void CtrlClicked(TEveDigitSet* qs, Int_t idx); // *SIGNAL*

   // --------------------------------

   TEveChunkManager* GetPlex() { return &fPlex; }

   TEveFrameBox* GetFrame() const { return fFrame; }
   void          SetFrame(TEveFrameBox* b);

   Bool_t GetValueIsColor()  const { return fValueIsColor; }

   TEveRGBAPalette* GetPalette() const { return fPalette; }
   void             SetPalette(TEveRGBAPalette* p);
   TEveRGBAPalette* AssertPalette();

   ERenderMode_e  GetRenderMode()           const { return fRenderMode; }
   void           SetRenderMode(ERenderMode_e rm) { fRenderMode = rm; }

   Bool_t GetEmitSignals() const   { return fEmitSignals; }
   void   SetEmitSignals(Bool_t f) { fEmitSignals = f; }

   Bool_t GetHistoButtons() const   { return fHistoButtons; }
   void   SetHistoButtons(Bool_t f) { fHistoButtons = f; }

   TEveTrans& RefHMTrans()                    { return fHMTrans; }
   void SetTransMatrix(Double_t* carr)        { fHMTrans.SetFrom(carr); }
   void SetTransMatrix(const TGeoMatrix& mat) { fHMTrans.SetFrom(mat);  }

   ClassDef(TEveDigitSet, 1); // Base-class for storage of digit collections; provides transformation matrix (TEveTrans), signal to color mapping (TEveRGBAPalette) and visual grouping (TEveFrameBox).
};

#endif
