// @(#)root/treeplayer:$Id$
// Author: Axel Naumann, 2011-09-21

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers and al.        *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TTreeReader.h"
#include "TBranchProxyDirector.h"
#include "TTreeProxyGenerator.h"
#include "TTreeReaderValuePtr.h"

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// TTreeReader                                                                //
//                                                                            //
// Connects TTreeReaderValuePtr to a TTree.                                   //
//                                                                            //
//                                                                            //
//                                                                            //
//                                                                            //
//                                                                            //
//                                                                            //
//                                                                            //
//                                                                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

ClassImp(TTreeReader)

//______________________________________________________________________________
TTreeReader::TTreeReader(TTree* tree):
   fTree(tree),
   fDirectory(0),
   fLocalEntryNumber(0),
//fEntryStatus(kEntryNotLoaded),
   fProxyGenerator(0),
   fDirector(0)
{
   // Access data from tree.
   InitializeProxyGenerator();
}

//______________________________________________________________________________
TTreeReader::TTreeReader(const char* keyname, TDirectory* dir /*= NULL*/)
   fTree(0),
   fDirectory(0),
   fLocalEntryNumber(0),
//fEntryStatus(kEntryNotLoaded),
   fProxyGenerator(0),
   fDirector(0)
{
   // Access data from the tree called keyname in the directory dir (or
   // current directory if dir is NULL). If keyname cannot be found, or if
   // it is not a TTree, IsZombie() will return true.
   if (!fDirectory) fDirectory = gDirectory;
   fDirectory->GetObject(keyname, fTree);
   InitializeProxyGenerator();
}

//______________________________________________________________________________
void TTreeReader::InitializeProxyGenerator()
{
   // Initialization of the TTreeProxyGenerator
   if (!fTree) {
      SetBit(kIsZombie);
      //fEntryStatus = kEntryNoTree;
   } else {
      fProxyGenerator = new ROOT::TTreeProxyGenerator(tree);
      fDirector = new ROOT::TBranchProxyDirector(tree, -1);
   }
}

//______________________________________________________________________________
void TTreeReader::GetEntry(Long64_t entry)
{
   // Load an entry into the tree, return the status of the read.
   if (!fTree) return fEntryStatus;
   fDirector->SetReadEntry(entry);
}

