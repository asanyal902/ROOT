// @(#)root/treeplayer:$Name:  $:$Id: TSelectorCint.cxx,v 1.2 2000/07/17 10:26:41 brun Exp $
// Author: Rene Brun   05/02/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// This class is a special version of TSelector for user interpreted    //
// classes.                                                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "TTree.h"
#include "TSelectorCint.h"
#include "Api.h"

ClassImp(TSelectorCint)

//______________________________________________________________________________
TSelectorCint::TSelectorCint(): TSelector()
{
   // Default constructor for a Selector.

   fFuncBegin   = 0;
   fFuncNotif   = 0;
   fFuncTerm    = 0;
   fFuncCut     = 0;
   fFuncFill    = 0;
   fFuncOption  = 0;
   fIntSelector = 0;
   
}

//______________________________________________________________________________
TSelectorCint::~TSelectorCint()
{
   // destructor for a Selector.

   delete fFuncBegin;
   delete fFuncNotif;
   delete fFuncTerm;
   delete fFuncCut;
   delete fFuncFill;
   delete fFuncOption;
}

//______________________________________________________________________________
void TSelectorCint::Build(TSelector *iselector, G__ClassInfo *cl)
{
   // Initialize the CallFunc objects when selector is interpreted

   fIntSelector = iselector;
   fFuncBegin   = new G__CallFunc();
   fFuncNotif   = new G__CallFunc();
   fFuncTerm    = new G__CallFunc();
   fFuncCut     = new G__CallFunc();
   fFuncFill    = new G__CallFunc();
   fFuncOption  = new G__CallFunc();
   Long_t offset = 0;
   fFuncBegin->SetFuncProto(cl,"Begin","",&offset);
   fFuncNotif->SetFuncProto(cl,"Notify","",&offset);
   fFuncTerm->SetFuncProto (cl,"Terminate","",&offset);
   fFuncCut->SetFuncProto  (cl,"ProcessCut","int",&offset);
   fFuncFill->SetFuncProto (cl,"ProcessFill","int",&offset);
   fFuncOption->SetFuncProto (cl,"SetOption","const char*",&offset);
}


//______________________________________________________________________________
void TSelectorCint::Begin(TTree *tree)
{
   // Invoke the Begin function via the interpreter

   fFuncBegin->SetArg((Long_t)tree);
   fFuncBegin->ExecInt(fIntSelector);
}


//______________________________________________________________________________
Bool_t TSelectorCint::Notify()
{
   // Invoke the Notify function via the interpreter

   Int_t sel = fFuncNotif->ExecInt(fIntSelector);
   return (Bool_t)sel;
}


//______________________________________________________________________________
Bool_t TSelectorCint::ProcessCut(Int_t entry)
{
   // Invoke the ProcessCut function via the interpreter

   fFuncCut->SetArgArray((Long_t*)&entry);
   Int_t sel = fFuncCut->ExecInt(fIntSelector);
   return (Bool_t)sel;
}


//______________________________________________________________________________
void TSelectorCint::ProcessFill(Int_t entry)
{
   // Invoke the ProcessFill function via the interpreter

   fFuncFill->SetArgArray((Long_t*)&entry);
   fFuncFill->Exec(fIntSelector);
}

//______________________________________________________________________________
void TSelectorCint::SetOption(const char *option)
{
   // Set the selector option

   fFuncOption->SetArg((Long_t)option);
   fFuncOption->Exec(fIntSelector);
}


//______________________________________________________________________________
void TSelectorCint::Terminate()
{
   // Invoke the Terminate function via the interpreter

   fFuncTerm->Exec(fIntSelector);
}
