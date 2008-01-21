// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEvePointSet
#define ROOT_TEvePointSet

#include "TEveVSDStructs.h"
#include "TEveElement.h"
#include "TEveProjectionBases.h"
#include "TEveTreeTools.h"

#include "TArrayI.h"
#include "TPointSet3D.h"
#include "TQObject.h"

class TTree;
class TF3;
class TGListTreeItem;

/******************************************************************************/
// TEvePointSet
/******************************************************************************/

class TEvePointSet : public TEveElement,
                     public TPointSet3D,
                     public TEvePointSelectorConsumer,
                     public TEveProjectable,
                     public TQObject
{
   friend class TEvePointSetArray;

protected:
   TString  fTitle;           // Title/tooltip of the TEvePointSet.
   TArrayI *fIntIds;          // Optional array of integer ideices.
   Int_t    fIntIdsPerPoint;  // Number of integer indices assigned to each point.

   void AssertIntIdsSize();

public:
   TEvePointSet(Int_t n_points=0, ETreeVarType_e tv_type=kTVT_XYZ);
   TEvePointSet(const Text_t* name, Int_t n_points=0, ETreeVarType_e tv_type=kTVT_XYZ);
   virtual ~TEvePointSet();

   virtual void ComputeBBox();

   void  Reset(Int_t n_points=0, Int_t n_int_ids=0);
   Int_t GrowFor(Int_t n_points);

   virtual const Text_t* GetTitle() const          { return fTitle; }
   virtual void          SetTitle(const Text_t* t) { fTitle = t; }

   Int_t  GetIntIdsPerPoint() const { return fIntIdsPerPoint; }
   Int_t* GetPointIntIds(Int_t p) const;
   Int_t  GetPointIntId(Int_t p, Int_t i) const;

   void   SetPointIntIds(Int_t* ids);
   void   SetPointIntIds(Int_t n, Int_t* ids);

   virtual void SetRnrElNameTitle(const Text_t* name, const Text_t* title);

   virtual void SetMarkerColor(Color_t col)
   { SetMainColor(col); }

   virtual void Paint(Option_t* option="");

   virtual void InitFill(Int_t subIdNum);
   virtual void TakeAction(TEvePointSelector*);

   virtual const TGPicture* GetListTreeIcon();

   virtual TClass* ProjectedClass() const;

   ClassDef(TEvePointSet, 1); // Set of 3D points with same marker attributes; optionally each point can be assigned an external TRef or a number of integer indices.
};


/******************************************************************************/
// TEvePointSetArray
/******************************************************************************/

class TEvePointSetArray : public TEveElement,
                          public TNamed,
                          public TAttMarker,
                          public TEvePointSelectorConsumer
{
   friend class TEvePointSetArrayEditor;

   TEvePointSetArray(const TEvePointSetArray&);            // Not implemented
   TEvePointSetArray& operator=(const TEvePointSetArray&); // Not implemented

protected:
   TEvePointSet**   fBins;                 //  Pointers to subjugated TEvePointSet's.
   Int_t        fDefPointSetCapacity;  //  Default capacity of subjugated TEvePointSet's.
   Int_t        fNBins;                //  Number of subjugated TEvePointSet's.
   Int_t        fLastBin;              //! Index of the last filled TEvePointSet.
   Double_t     fMin, fCurMin;         //  Overall and current minimum value of the separating quantity.
   Double_t     fMax, fCurMax;         //  Overall and current maximum value of the separating quantity.
   Double_t     fBinWidth;             //  Separating quantity bin-width.
   TString      fQuantName;            //  Name of the separating quantity.

public:
   TEvePointSetArray(const Text_t* name="TEvePointSetArray", const Text_t* title="");
   virtual ~TEvePointSetArray();

   virtual void RemoveElementLocal(TEveElement* el);
   virtual void RemoveElementsLocal();

   virtual void Paint(Option_t* option="");

   virtual void SetMarkerColor(Color_t tcolor=1);
   virtual void SetMarkerStyle(Style_t mstyle=1);
   virtual void SetMarkerSize(Size_t msize=1);

   virtual void TakeAction(TEvePointSelector*);


   void InitBins(const Text_t* quant_name, Int_t nbins, Double_t min, Double_t max,
                 Bool_t addRe=kTRUE);
   void Fill(Double_t x, Double_t y, Double_t z, Double_t quant);
   void SetPointId(TObject* id);
   void CloseBins();

   void SetOwnIds(Bool_t o);

   Int_t GetDefPointSetCapacity() const  { return fDefPointSetCapacity; }
   void  SetDefPointSetCapacity(Int_t c) { fDefPointSetCapacity = c; }

   Int_t     GetNBins()        const { return fNBins; }
   TEvePointSet* GetBin(Int_t bin) const { return fBins[bin]; }

   Double_t GetMin()    const { return fMin; }
   Double_t GetCurMin() const { return fCurMin; }
   Double_t GetMax()    const { return fMax; }
   Double_t GetCurMax() const { return fCurMax; }

   void SetRange(Double_t min, Double_t max);

   ClassDef(TEvePointSetArray, 1); // Array of TEvePointSet's filled via a common point-source; range of displayed TEvePointSet's can be controlled, based on a separating quantity provided on fill-time by a user.
};


/******************************************************************************/
// TEvePointSetProjected
/******************************************************************************/

class TEvePointSetProjected : public TEvePointSet,
                              public TEveProjected
{
private:
   TEvePointSetProjected(const TEvePointSetProjected&);            // Not implemented
   TEvePointSetProjected& operator=(const TEvePointSetProjected&); // Not implemented

public:
   TEvePointSetProjected();
   virtual ~TEvePointSetProjected() {}

   virtual void SetProjection(TEveProjectionManager* proj, TEveProjectable* model);

   virtual void UpdateProjection();

   ClassDef(TEvePointSetProjected, 1); // Projected copy of a TEvePointSet.
};

#endif
