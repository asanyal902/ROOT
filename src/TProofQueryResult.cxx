// @(#)root/proof:$Name:  $:$Id: TProofQueryResult.cxx,v 1.3 2007/02/12 13:05:32 rdm Exp $
// Author: G Ganis Sep 2005

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProofQueryResult                                                    //
//                                                                      //
// TQueryResult version adapted to PROOF neeeds.                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TDSet.h"
#include "TError.h"
#include "TList.h"
#include "TProofQueryResult.h"
#include "TSystem.h"


ClassImp(TProofQueryResult)

//______________________________________________________________________________
TProofQueryResult::TProofQueryResult(Int_t sn, const char *opt, TList *inlist,
                                     Long64_t ent, Long64_t fst, TDSet *dset,
                                     const char *sel, TObject *elist)
                  : TQueryResult(sn, opt, inlist, ent, fst, sel)
{
   // Main constructor.

   fStartLog = -1;

   // Add data sets and event lists to the input list
   if (dset)
      fInputList->Add(dset);
   if (elist)
      fInputList->Add(elist);
}

//______________________________________________________________________________
void TProofQueryResult::SetRunning(Int_t startlog, const char *par)
{
   // Call when running starts.

   fStatus = kRunning;
   fStart.Set();
   fEnd.Set(fStart.Convert()-1);
   fParList = (par && (strlen(par) > 0)) ? par : "-";
   fStartLog = startlog;

   // Add header to log file
   fLogFile->AddLine("+++");
   fLogFile->AddLine(Form("+++ Start processing query # %d (log file offset: %d)",
                     fSeqNum, startlog));
   fLogFile->AddLine("+++");
}
