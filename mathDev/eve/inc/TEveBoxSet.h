// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveBoxSet
#define ROOT_TEveBoxSet

#include "TEveDigitSet.h"

class TGeoMatrix;
class TRandom;

class TEveBoxSet: public TEveDigitSet
{
   friend class TEveBoxSetGL;

   TEveBoxSet(const TEveBoxSet&);            // Not implemented
   TEveBoxSet& operator=(const TEveBoxSet&); // Not implemented

public:
   enum EBoxType_e
      {
         kBT_Undef,           // unknown-ignored
         kBT_FreeBox,         // arbitrary box: specify 8*(x,y,z) box corners
         kBT_AABox,           // axis-aligned box: specify (x,y,z) and (w, h, d)
         kBT_AABoxFixedDim    // axis-aligned box w/ fixed dimensions: specify (x,y,z)
      };

protected:

   struct BFreeBox_t       : public DigitBase_t { Float_t fVertices[24]; };

   struct BOrigin_t        : public DigitBase_t { Float_t fA, fB, fC; };

   struct BAABox_t         : public BOrigin_t   { Float_t fW, fH, fD; };

   struct BAABoxFixedDim_t : public BOrigin_t {};

protected:
   EBoxType_e        fBoxType;      // Type of rendered box.

   Float_t           fDefWidth;     // Breadth assigned to first coordinate  (A).
   Float_t           fDefHeight;    // Breadth assigned to second coordinate (B).
   Float_t           fDefDepth;     // Breadth assigned to third coordinate  (C).

   static Int_t SizeofAtom(EBoxType_e bt);

public:
   TEveBoxSet(const Text_t* n="TEveBoxSet", const Text_t* t="");
   virtual ~TEveBoxSet() {}

   void Reset(EBoxType_e boxType, Bool_t valIsCol, Int_t chunkSize);
   void Reset();

   void AddBox(const Float_t* verts);
   void AddBox(Float_t a, Float_t b, Float_t c, Float_t w, Float_t h, Float_t d);
   void AddBox(Float_t a, Float_t b, Float_t c);

   virtual void ComputeBBox();
   // virtual void Paint(Option_t* option = "");

   void Test(Int_t nboxes);

   Float_t GetDefWidth()  const { return fDefWidth;  }
   Float_t GetDefHeight() const { return fDefHeight; }
   Float_t GetDefDepth()  const { return fDefDepth;  }

   void SetDefWidth(Float_t v)  { fDefWidth  = v ; }
   void SetDefHeight(Float_t v) { fDefHeight = v ; }
   void SetDefDepth(Float_t v)  { fDefDepth  = v ; }

   ClassDef(TEveBoxSet, 1); // Collection of 3D primitives (fixed-size boxes, boxes of different sizes, or arbitrary sexto-epipeds); each primitive can be assigned a signal value and a TRef.
};

#endif
