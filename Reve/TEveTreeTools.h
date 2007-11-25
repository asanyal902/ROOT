// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveTreeTools
#define ROOT_TEveTreeTools

#include <TSelectorDraw.h>
#include <TEventList.h>

/******************************************************************************/
// TEveSelectorToEventList
/******************************************************************************/

class TEveSelectorToEventList : public TSelectorDraw
{
   TEveSelectorToEventList(const TEveSelectorToEventList&);            // Not implemented
   TEveSelectorToEventList& operator=(const TEveSelectorToEventList&); // Not implemented

protected:
   TEventList* fEvList;
   TList       fInput;
public:
   TEveSelectorToEventList(TEventList* evl, const Text_t* sel);

   virtual Int_t  Version() const { return 1; }
   virtual Bool_t Process(Long64_t entry);

   ClassDef(TEveSelectorToEventList, 1); // TSelector that stores entry numbers of matching TTree entries into an event-list.
};

/******************************************************************************/
// TEvePointSelectorConsumer, TEvePointSelector
/******************************************************************************/

class TEvePointSelector;

class TEvePointSelectorConsumer
{
public:
   enum TreeVarType_e { TVT_XYZ, TVT_RPhiZ };

protected:
   TreeVarType_e fSourceCS; // Coordinate-System of the source tree variables

public:
   TEvePointSelectorConsumer(TreeVarType_e cs=TVT_XYZ) :fSourceCS(cs) {}
   virtual ~TEvePointSelectorConsumer() {}

   virtual void InitFill(Int_t /*subIdNum*/) {}
   virtual void TakeAction(TEvePointSelector*) = 0;

   TreeVarType_e GetSourceCS() const  { return fSourceCS; }
   void SetSourceCS(TreeVarType_e cs) { fSourceCS = cs; }

   ClassDef(TEvePointSelectorConsumer, 1); // Virtual base for classes that can be filled from TTree data via the TEvePointSelector class.
};

class TEvePointSelector : public TSelectorDraw
{
   TEvePointSelector(const TEvePointSelector&);            // Not implemented
   TEvePointSelector& operator=(const TEvePointSelector&); // Not implemented

protected:
   TTree                  *fTree;
   TEvePointSelectorConsumer *fConsumer;

   TString                 fVarexp;
   TString                 fSelection;

   TString                 fSubIdExp;
   Int_t                   fSubIdNum;

   TList                   fInput;

public:
   TEvePointSelector(TTree* t=0, TEvePointSelectorConsumer* c=0,
                     const Text_t* vexp="", const Text_t* sel="");
   virtual ~TEvePointSelector() {}

   virtual Long64_t Select(const Text_t* selection=0);
   virtual Long64_t Select(TTree* t, const Text_t* selection=0);
   virtual void  TakeAction();


   TTree* GetTree() const   { return fTree; }
   void   SetTree(TTree* t) { fTree = t; }

   TEvePointSelectorConsumer* GetConsumer() const { return fConsumer; }
   void SetConsumer(TEvePointSelectorConsumer* c) { fConsumer = c; }

   const Text_t* GetVarexp() const { return fVarexp; }
   void SetVarexp(const Text_t* v) { fVarexp = v; }

   const Text_t* GetSelection() const { return fSelection; }
   void SetSelection(const Text_t* s) { fSelection = s; }

   const Text_t* GetSubIdExp() const { return fSubIdExp; }
   void SetSubIdExp(const Text_t* s) { fSubIdExp = s; }

   Int_t GetSubIdNum() const { return fSubIdNum; }

   ClassDef(TEvePointSelector, 1); // TSelector for direct extraction of point-like data from a Tree.
};

#endif
