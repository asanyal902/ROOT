// @(#)root/treeplayer:$Name:  $:$Id: TSelector.h,v 1.6 2001/07/10 06:21:35 brun Exp $
// Author: Rene Brun   05/02/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TSelector
#define ROOT_TSelector


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TSelector                                                            //
//                                                                      //
// A utility class for Trees selections.                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_TString
#include "TString.h"
#endif

class TTree;

class TSelector : public TObject {
protected:
   TString   fOption;  //option given to TTree::Process
   TObject  *fObject;  // Current oject if processing object (vs. Tree)
   TList    *fInput;
   TList    *fOutput;

public:
   TSelector();
   virtual            ~TSelector();
   virtual void        Begin(TTree *) {;}
   virtual void        Begin() {;}
   virtual Bool_t      Notify() {return kTRUE;}
   virtual const char *GetOption() const {return fOption.Data();}
   static  TSelector  *GetSelector(const char *filename);
   virtual Bool_t      ProcessCut(Int_t entry) {return kTRUE;}
   virtual void        ProcessFill(Int_t entry) {;}
   virtual Bool_t      Process() {return kFALSE;}
   virtual void        SetOption(const char *option) {fOption=option;}
   virtual void        SetObject(TObject *obj) {fObject=obj;}
   virtual void        SetInputList(TList *input) {fInput = input;}
   virtual TList      *GetOutputList() {return fOutput;}
   virtual void        Terminate() {;}

   ClassDef(TSelector,0)  //A utility class for Trees selections.
};

#endif

