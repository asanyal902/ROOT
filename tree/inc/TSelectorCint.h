// @(#)root/treeplayer:$Name:  $:$Id: TSelectorCint.h,v 1.2 2000/07/17 10:26:41 brun Exp $
// Author: Rene Brun   05/02/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TSelectorCint
#define ROOT_TSelectorCint


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TSelectorCint                                                        //
//                                                                      //
// A utility class for Trees selections.  (via interpreter)             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifndef ROOT_TSelector
#include "TSelector.h"
#endif

class G__CallFunc;
class G__ClassInfo;

class TSelectorCint : public TSelector {

protected:
   G__CallFunc   *fFuncBegin;    //!
   G__CallFunc   *fFuncNotif;    //!
   G__CallFunc   *fFuncTerm;     //!
   G__CallFunc   *fFuncCut;      //!
   G__CallFunc   *fFuncFill;     //!
   G__CallFunc   *fFuncOption;   //!
   TSelector     *fIntSelector;  //Pointer to interpreted selector (if interpreted)
   
public:
   TSelectorCint();
   virtual            ~TSelectorCint();
   virtual void        Build(TSelector *iselector, G__ClassInfo *cl);
   virtual void        Begin(TTree *tree);
   virtual Bool_t      Notify();
   virtual Bool_t      ProcessCut(Int_t entry);
   virtual void        ProcessFill(Int_t entry);
   virtual void        SetOption(const char *option);
   virtual void        Terminate();

   ClassDef(TSelectorCint,0)  //A utility class for Trees selections. (interpreted)
};

#endif

