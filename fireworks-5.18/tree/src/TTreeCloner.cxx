// @(#)root/tree:$Id$
// Author: Philippe Canal 07/11/2005

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TTreeCloner                                                          //
//                                                                      //
// Class implementing or helping  the various TTree cloning method      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TBasket.h"
#include "TBranch.h"
#include "TBranchClones.h"
#include "TBranchElement.h"
#include "TStreamerInfo.h"
#include "TBranchRef.h"
#include "TError.h"
#include "TProcessID.h"
#include "TMath.h"
#include "TTree.h"
#include "TTreeCloner.h"
#include "TFile.h"
#include "TLeafB.h"
#include "TLeafI.h"
#include "TLeafL.h"

TTreeCloner::TTreeCloner(TTree *from, TTree *to, Option_t *method) :
   fIsValid(kTRUE),
   fFromTree(from),
   fToTree(to),
   fMethod(method),
   fFromBranches( from ? from->GetListOfLeaves()->GetEntries()+1 : 0),
   fToBranches( to ? from->GetListOfLeaves()->GetEntries()+1 : 0),
   fMaxBaskets(CollectBranches()),
   fBasketBranchNum(new UInt_t[fMaxBaskets]),
   fBasketNum(new UInt_t[fMaxBaskets]),
   fBasketSeek(new Long64_t[fMaxBaskets]),
   fBasketEntry(new Long64_t[fMaxBaskets]),
   fBasketIndex(new Int_t[fMaxBaskets]),
   fCloneMethod(TTreeCloner::kDefault),
   fToStartEntries(0)
{
   // Constructor.  This object would transfer the data from
   // 'from' to 'to' using the method indicated in method.
   //
   // The value of the parameter 'method' determines in which 
   // order the branches' baskets are written to the output file.
   //
   // When a TTree is filled the data is stored in the individual
   // branches' basket.  Each basket is written individually to 
   // the disk as soon as it is full.  In consequence the baskets 
   // of branches that contain 'large' data chunk are written to
   // the disk more often.
   //
   // There is currently 3 supported sorting order:
   //    SortBasketsByOffset (the default)
   //    SortBasketsByBranch
   //    SortBasketsByEntry
   //
   // When using SortBasketsByOffset the baskets are written in
   // the output file in the same order as in the original file
   // (i.e. the basket are sorted on their offset in the original
   // file; Usually this also means that the baskets are sorted
   // on the index/number of the _last_ entry they contain)
   // 
   // When using SortBasketsByBranch all the baskets of each 
   // individual branches are stored contiguously.  This tends to 
   // optimize reading speed when reading a small number (1->5) of 
   // branches, since all their baskets will be clustered together 
   // instead of being spread across the file.  However it might
   // decrease the performance when reading more branches (or the full 
   // entry).
   // 
   // When using SortBasketsByEntry the baskets with the lowest
   // starting entry are written first.  (i.e. the baskets are 
   // sorted on the index/number of the first entry they contain). 
   // This means that on the file the baskets will be in the order
   // in which they will be needed when reading the whole tree
   // sequentially.
   //

   TString opt(method);
   opt.ToLower();
   if (opt.Contains("sortbasketsbybranch")) {
      //::Info("TTreeCloner::TTreeCloner","use: kSortBasketsByBranch");
      fCloneMethod = TTreeCloner::kSortBasketsByBranch;
   } else if (opt.Contains("sortbasketsbyentry")) {
      //::Info("TTreeCloner::TTreeCloner","use: kSortBasketsByEntry");
      fCloneMethod = TTreeCloner::kSortBasketsByEntry;
   } else {
      //::Info("TTreeCloner::TTreeCloner","use: kSortBasketsByOffset");
      fCloneMethod = TTreeCloner::kSortBasketsByOffset;
   }
   if (fToTree) fToStartEntries = fToTree->GetEntries();
}

Bool_t TTreeCloner::Exec()
{
   // Execute the cloning.

   CopyStreamerInfos();
   CopyProcessIds();
   CloseOutWriteBaskets();
   CollectBaskets();
   SortBaskets();
   WriteBaskets();
   CopyMemoryBaskets();

   return kTRUE;
}

TTreeCloner::~TTreeCloner()
{
   // TTreeCloner destructor

   delete [] fBasketBranchNum;
   delete [] fBasketNum;
   delete [] fBasketSeek;
   delete [] fBasketEntry;
   delete [] fBasketIndex;
}

void TTreeCloner::CloseOutWriteBaskets()
{
   // Before we can start adding new basket, we need to flush to
   // disk the partially filled baskets (the WriteBasket)

   for(Int_t i=0; i<fToBranches.GetEntries(); ++i) {
      TBranch *to = (TBranch*)fToBranches.UncheckedAt(i);

      TObjArray *array = to->GetListOfBaskets();
      if (array->GetEntries()) {
         TBasket *basket = to->GetBasket(to->GetWriteBasket());
         if (basket) {
            if (basket->GetNevBuf()) {
               // If the basket already contains entry we need to close it
               // out. (This is because we can only transfer full compressed
               // buffer)

               if (basket->GetBufferRef()->IsReading()) {
                  basket->SetWriteMode();
               }
               to->WriteBasket(basket);
               basket = to->GetBasket(to->GetWriteBasket()); // WriteBasket create an empty basket
            }
            if (basket) {
               basket->DropBuffers();
               to->GetListOfBaskets()->RemoveAt(to->GetWriteBasket());
               delete basket;
            }
         }
      }
   }
}

UInt_t TTreeCloner::CollectBranches(TBranch *from, TBranch *to)
{
   // Fill the array of branches, adding the branch 'from' and 'to',
   // and matching the sub-branches of the 'from' and 'to' branches.
   // Returns the total number of baskets in all the from branch and
   // it sub-branches.

   // Since this is called from the constructor, this can not be a virtual function

   UInt_t numBaskets = 0;
   if (from->InheritsFrom(TBranchClones::Class())) {
      TBranchClones *fromclones = (TBranchClones*)from;
      TBranchClones *toclones = (TBranchClones*)to;
      numBaskets += CollectBranches(fromclones->fBranchCount,toclones->fBranchCount);
   
   } else if (from->InheritsFrom(TBranchElement::Class())) {
      TBranchElement *fromelem = (TBranchElement*)from;
      TBranchElement *toelem   = (TBranchElement*)to;
      if (fromelem->fMaximum > toelem->fMaximum) toelem->fMaximum = fromelem->fMaximum;
   } else {

      Int_t nb = from->GetListOfLeaves()->GetEntries();
      Int_t fnb= to->GetListOfLeaves()->GetEntries();
      if (nb!=fnb) {
         Error("TTreeCloner::CollectBranches",
            "The export branch and the import branch do not have the same number of leaves (%d vs %d)",
            fnb,nb);
         fIsValid = kFALSE;
         return 0;
      }
      for (Int_t i=0;i<nb;i++)  {
         TLeaf *fromleaf_gen = (TLeaf*)from->GetListOfLeaves()->At(i);
         if (fromleaf_gen->IsA()==TLeafI::Class()) {
            TLeafI *fromleaf = (TLeafI*)from->GetListOfLeaves()->At(i);
            TLeafI *toleaf   = (TLeafI*)to->GetListOfLeaves()->At(i);
            if (fromleaf->GetMaximum() > toleaf->GetMaximum()) 
               toleaf->SetMaximum( fromleaf->GetMaximum() );
            if (fromleaf->GetMinimum() < toleaf->GetMinimum()) 
               toleaf->SetMinimum( fromleaf->GetMinimum() );
         } else if (fromleaf_gen->IsA()==TLeafL::Class()) {
            TLeafL *fromleaf = (TLeafL*)from->GetListOfLeaves()->At(i);
            TLeafL *toleaf   = (TLeafL*)to->GetListOfLeaves()->At(i);
            if (fromleaf->GetMaximum() > toleaf->GetMaximum()) 
               toleaf->SetMaximum( fromleaf->GetMaximum() );
            if (fromleaf->GetMinimum() < toleaf->GetMinimum()) 
               toleaf->SetMinimum( fromleaf->GetMinimum() );
         } else if (fromleaf_gen->IsA()==TLeafB::Class()) {
            TLeafB *fromleaf = (TLeafB*)from->GetListOfLeaves()->At(i);
            TLeafB *toleaf   = (TLeafB*)to->GetListOfLeaves()->At(i);
            if (fromleaf->GetMaximum() > toleaf->GetMaximum()) 
               toleaf->SetMaximum( fromleaf->GetMaximum() );
            if (fromleaf->GetMinimum() < toleaf->GetMinimum()) 
               toleaf->SetMinimum( fromleaf->GetMinimum() );
         }
      }

   }

   fFromBranches.AddLast(from);
   fToBranches.AddLast(to);

   numBaskets += from->GetWriteBasket();
   numBaskets += CollectBranches(from->GetListOfBranches(),to->GetListOfBranches());

   return numBaskets;
}

UInt_t TTreeCloner::CollectBranches(TObjArray *from, TObjArray *to)
{
   // Fill the array of branches, matching the branches of the 'from' and 'to' arrays.
   // Returns the total number of baskets in all the branches.

   // Since this is called from the constructor, this can not be a virtual function

   Int_t fnb = from->GetEntries();
   Int_t tnb = to->GetEntries();
   if (!fnb || !tnb) {
      return 0;
   }

   UInt_t numBasket = 0;
   Int_t fi = 0;
   Int_t ti = 0;
   while (ti < tnb) {
      TBranch* fb = (TBranch*) from->UncheckedAt(fi);
      TBranch* tb = (TBranch*) to->UncheckedAt(ti);
      if (strcmp(fb->GetName(), tb->GetName())) {
         ++fi;
         if (fi >= fnb) {
            break;
         }
         continue;
      }
      numBasket += CollectBranches(fb, tb);
      ++fi;
      if (fi >= fnb) {
         break;
      }
      ++ti;
   }
   return numBasket;
}


UInt_t TTreeCloner::CollectBranches()
{
   // Fill the array of branches, matching the branches of the 'from' and 'to' TTrees
   // Returns the total number of baskets in all the branches.

   // Since this is called from the constructor, this can not be a virtual function

   UInt_t numBasket = CollectBranches(fFromTree->GetListOfBranches(),
                                      fToTree->GetListOfBranches());

   if (fFromTree->GetBranchRef()) {
      fToTree->BranchRef();
      numBasket += CollectBranches(fFromTree->GetBranchRef(),fToTree->GetBranchRef());
   }
   return numBasket;
}

void TTreeCloner::CollectBaskets()
{
   // Collect the information about the on-file basket that need
   // to be copied.

   UInt_t len = fFromBranches.GetEntries();

   for(UInt_t i=0,bi=0; i<len; ++i) {
      TBranch *from = (TBranch*)fFromBranches.UncheckedAt(i);
      for(Int_t b=0; b<from->GetWriteBasket(); ++b,++bi) {
         fBasketBranchNum[bi] = i;
         fBasketNum[bi] = b;
         fBasketSeek[bi] = from->GetBasketSeek(b);
         fBasketEntry[bi] = from->GetBasketEntry()[b];
         fBasketIndex[bi] = bi;
      }
   }
}

void TTreeCloner::CopyStreamerInfos()
{
   // Make sure that all the needed TStreamerInfo are
   // present in the output file

   TFile *fromFile = fFromTree->GetDirectory()->GetFile();
   TFile *toFile = fToTree->GetDirectory()->GetFile();
   TList *l = fromFile->GetStreamerInfoList();
   TIter next(l);
   TStreamerInfo *oldInfo;
   while ( (oldInfo = (TStreamerInfo*)next()) ) {
      TStreamerInfo *curInfo = 0;
      TClass *cl = TClass::GetClass(oldInfo->GetName());

      if ((cl->IsLoaded() && (cl->GetNew()!=0 || cl->HasDefaultConstructor()))
          || !cl->IsLoaded())  {
         // Insure that the TStreamerInfo is loaded
         curInfo = (TStreamerInfo*)cl->GetStreamerInfo(oldInfo->GetClassVersion());
         if (oldInfo->GetClassVersion()==1) {
            // We may have a Foreign class let's look using the
            // checksum:
            curInfo = (TStreamerInfo*)cl->FindStreamerInfo(oldInfo->GetCheckSum());
         }
         curInfo->TagFile(toFile);
      } else {
         // If there is no default constructor the GetStreamerInfo
         // will not work. It also means (hopefully) that an
         // inheriting class has a streamerInfo in the list (which
         // will induces the setting of this streamerInfo)

         oldInfo->TagFile(toFile);
      }
   }
   delete l;
}

void TTreeCloner::CopyMemoryBaskets()
{
   // Transfer the basket from the input file to the output file

   TBasket *basket = 0;
   for(Int_t i=0; i<fToBranches.GetEntries(); ++i) {
      TBranch *from = (TBranch*)fFromBranches.UncheckedAt( i );
      TBranch *to   = (TBranch*)fToBranches.UncheckedAt( i );

      basket = from->GetListOfBaskets()->GetEntries() ? from->GetBasket(from->GetWriteBasket()) : 0;
      if (basket) {
         basket = (TBasket*)basket->Clone();
         basket->SetBranch(to);
         to->AddBasket(*basket, kFALSE, fToStartEntries+from->GetBasketEntry()[from->GetWriteBasket()]);
      }
      // If the branch is a TBranchElement non-terminal 'object' branch, it's basket will contain 0
      // events.
      if (from->GetEntries()!=0 && from->GetWriteBasket()==0 && basket->GetNevBuf()==0) {
         to->SetEntries(to->GetEntries()+from->GetEntries());
      }
   }
}

void TTreeCloner::CopyProcessIds()
{
   // Make sure that all the needed TStreamerInfo are
   // present in the output file

   // NOTE: We actually need to merge the ProcessId somehow :(

   TFile *fromfile = fFromTree->GetDirectory()->GetFile();
   TFile *tofile = fToTree->GetDirectory()->GetFile();

   fPidOffset = tofile->GetNProcessIDs();

   TIter next(fromfile->GetListOfKeys());
   TKey *key;
   TDirectory::TContext cur(gDirectory,fromfile);
   while ((key = (TKey*)next())) {
      if (!strcmp(key->GetClassName(),"TProcessID")) {
         TProcessID *pid = (TProcessID*)key->ReadObjectAny(0);

         //UShort_t out = TProcessID::WriteProcessID(id,tofile);
         UShort_t out = 0;
         TObjArray *pids = tofile->GetListOfProcessIDs();
         Int_t npids = tofile->GetNProcessIDs();
         Bool_t wasIn = kFALSE;
         for (Int_t i=0;i<npids;i++) {
            if (pids->At(i) == pid) {out = (UShort_t)i; wasIn = kTRUE; break;}
         }

         if (!wasIn) {
            TDirectory *dirsav = gDirectory;
            tofile->cd();
            tofile->SetBit(TFile::kHasReferences);
            pids->AddAtAndExpand(pid,npids);
            pid->IncrementCount();
            char name[32];
            sprintf(name,"ProcessID%d",npids);
            pid->Write(name);
            tofile->IncrementProcessIDs();
            if (gDebug > 0) {
               printf("WriteProcessID, name=%s, file=%s\n",name,tofile->GetName());
            }
            if (dirsav) dirsav->cd();
            out = (UShort_t)npids;
         }
         if (out<fPidOffset) {
            Error("CopyProcessIDs","Copied %s from %s might already exist!\n",
                  pid->GetName(),fromfile->GetName());
         }
      }
   }
}

void TTreeCloner::SortBaskets()
{
   // Sort the basket according to the user request.

   // Currently this sort __has to__ preserve the order
   // of basket for each individual branch.

   switch (fCloneMethod) {
      case kSortBasketsByBranch:
         // nothing to do, it is already sorted.
         break;
      case kSortBasketsByEntry:
         TMath::Sort( fMaxBaskets, fBasketEntry, fBasketIndex, kFALSE );
         break;
      case kSortBasketsByOffset:
      default:
         TMath::Sort( fMaxBaskets, fBasketSeek, fBasketIndex, kFALSE );
         break;
   }
}

void TTreeCloner::WriteBaskets()
{
   // Transfer the basket from the input file to the output file

   TBasket *basket = new TBasket();
   for(UInt_t j=0; j<fMaxBaskets; ++j) {
      TBranch *from = (TBranch*)fFromBranches.UncheckedAt( fBasketBranchNum[ fBasketIndex[j] ] );
      TBranch *to   = (TBranch*)fToBranches.UncheckedAt( fBasketBranchNum[ fBasketIndex[j] ] );

      TFile *tofile = to->GetFile(0);
      TFile *fromfile = from->GetFile(0);

      Int_t index = fBasketNum[ fBasketIndex[j] ];

      Long64_t pos = from->GetBasketSeek(index);
      if (from->GetBasketBytes()[index] == 0) {
         from->GetBasketBytes()[index] = basket->ReadBasketBytes(pos, fromfile);
      }
      Int_t len = from->GetBasketBytes()[index];
      Long64_t entryInBasket;
      if (index == from->GetWriteBasket()) {
         entryInBasket = from->GetEntries() - from->GetBasketEntry()[index];
      } else {
         entryInBasket = from->GetBasketEntry()[index+1] - from->GetBasketEntry()[index];
      }

      basket->LoadBasketBuffers(pos,len,fromfile);
      basket->IncrementPidOffset(fPidOffset);
      basket->CopyTo(tofile);
      to->AddBasket(*basket,kTRUE,fToStartEntries + from->GetBasketEntry()[index]);

   }
   delete basket;
}
