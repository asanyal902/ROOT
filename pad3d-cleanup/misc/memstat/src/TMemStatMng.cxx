// @(#)root/memstat:$Id$
// Author: Anar Manafov (A.Manafov@gsi.de) 2008-03-02

/*************************************************************************
* Copyright (C) 1995-2010, Rene Brun and Fons Rademakers.               *
* All rights reserved.                                                  *
*                                                                       *
* For the licensing terms see $ROOTSYS/LICENSE.                         *
* For the list of contributors see $ROOTSYS/README/CREDITS.             *
*************************************************************************/
// STD
#include <cstdlib>
// ROOT
#include "TSystem.h"
#include "TEnv.h"
#include "TError.h"
#include "Riostream.h"
#include "TObject.h"
#include "TFile.h"
#include "TTree.h"
#include "TArrayL64.h"
#include "TH1.h"
#include "TMD5.h"
// Memstat
#include "TMemStatBacktrace.h"
#include "TMemStatMng.h"

using namespace memstat;

ClassImp(TMemStatMng)

TMemStatMng* TMemStatMng::fgInstance = NULL;

//****************************************************************************//
//
//****************************************************************************//

TMemStatMng::TMemStatMng():
   TObject(),
#if !defined(__APPLE__)
   fPreviousMallocHook(TMemStatHook::GetMallocHook()),
   fPreviousFreeHook(TMemStatHook::GetFreeHook()),
#endif
   fDumpTree(NULL),
   fUseGNUBuiltinBacktrace(kFALSE),
   fBeginTime(0),
   fPos(0),
   fTimems(0),
   fNBytes(0),
   fN(0),
   fBtID(0),
   fBTCount(0),
   fSysInfo(NULL)
{
   // Default constructor
}

//______________________________________________________________________________
void TMemStatMng::Init()
{
   //Initialize MemStat manager - used only by instance method

   fBeginTime = fTimeStamp.AsDouble();

   fDumpFile = new TFile(Form("memstat_%d.root", gSystem->GetPid()), "recreate");
   Int_t opt = 200000;
   if(!fDumpTree) {
      fDumpTree = new TTree("T", "Memory Statistics");
      fDumpTree->Branch("pos",   &fPos,   "pos/l", opt);
      fDumpTree->Branch("time",  &fTimems, "time/I", opt);
      fDumpTree->Branch("nbytes", &fNBytes, "nbytes/I", opt);
      fDumpTree->Branch("btid",  &fBtID,  "btid/I", opt);
   }

   fBTCount = 0;

   fBTIDCount = 0;

   fFAddrsList = new TObjArray();
   fFAddrsList->SetOwner(kTRUE);

   fHbtids  = new TH1I("btids", "table of btids", 10000, 0, 1);   //where fHbtids is a member of the manager class
   fHbtids->SetDirectory(0);
   // save the histogram to a tree header
   fDumpTree->GetUserInfo()->Add(fHbtids);
   // save the system info to a tree header
   string sSysInfo(gSystem->GetBuildNode());
   sSysInfo += " | ";
   sSysInfo += gSystem->GetBuildCompilerVersion();
   sSysInfo += " | ";
   sSysInfo += gSystem->GetFlagsDebug();
   sSysInfo += " ";
   sSysInfo += gSystem->GetFlagsOpt();
   fSysInfo = new TNamed("SysInfo", sSysInfo.c_str());

   fDumpTree->GetUserInfo()->Add(fSysInfo);
}

//______________________________________________________________________________
TMemStatMng* TMemStatMng::GetInstance()
{
   // GetInstance - a static function
   // Initialize a singleton of MemStat manager

   if(!fgInstance) {
      fgInstance = new TMemStatMng;
      fgInstance->Init();
   }
   return fgInstance;
}

//______________________________________________________________________________
void TMemStatMng::Close()
{
   // Close - a static function
   // This method stops the manager,
   // flashes all the buffered data and closes the output tree.

   // TODO: This is a temporary solution until we find a properalgorithm for SaveData
   fgInstance->fDumpFile->WriteObject(fgInstance->fFAddrsList, "FAddrsList");

   // to be documented
   fgInstance->fDumpTree->AutoSave();

   delete fgInstance->fFAddrsList;
   delete fgInstance->fSysInfo;

   delete fgInstance;
   fgInstance = NULL;
}

//______________________________________________________________________________
TMemStatMng::~TMemStatMng()
{
   // if an instance is destructed - the hooks are reseted to old hooks

   if(this != TMemStatMng::GetInstance())
      return;

   Info("~TMemStatMng", ">>> All free/malloc calls count: %d", fBTIDCount);
   Info("~TMemStatMng", ">>> Unique BTIDs count: %zu", fBTChecksums.size());

   Disable();
}

//______________________________________________________________________________
void TMemStatMng::Enable()
{
   // Enable memory hooks

   if(this != GetInstance())
      return;
#if defined(__APPLE__)
   TMemStatHook::trackZoneMalloc(MacAllocHook, MacFreeHook);
#else
   // set hook to our functions
   TMemStatHook::SetMallocHook(AllocHook);
   TMemStatHook::SetFreeHook(FreeHook);
#endif
}

//______________________________________________________________________________
void TMemStatMng::Disable()
{
   // Disble memory hooks

   if(this != GetInstance())
      return;
#if defined(__APPLE__)
   TMemStatHook::untrackZoneMalloc();
#else
   // set hook to our functions
   TMemStatHook::SetMallocHook(fPreviousMallocHook);
   TMemStatHook::SetFreeHook(fPreviousFreeHook);
#endif
}

//______________________________________________________________________________
void TMemStatMng::MacAllocHook(void *ptr, size_t size)
{
   // AllocHook - a static function
   // a special memory hook for Mac OS X memory zones.
   // Triggered when memory is allocated.

   TMemStatMng* instance = TMemStatMng::GetInstance();
   // Restore all old hooks
   instance->Disable();

   // Call our routine
   instance->AddPointer(ptr, Int_t(size));

   // Restore our own hooks
   instance->Enable();
}

//______________________________________________________________________________
void TMemStatMng::MacFreeHook(void *ptr)
{
   // AllocHook - a static function
   // a special memory hook for Mac OS X memory zones.
   // Triggered when memory is deallocated.

   TMemStatMng* instance = TMemStatMng::GetInstance();
   // Restore all old hooks
   instance->Disable();

   // Call our routine
   instance->AddPointer(ptr, -1);

   // Restore our own hooks
   instance->Enable();
}

//______________________________________________________________________________
void *TMemStatMng::AllocHook(size_t size, const void* /*caller*/)
{
   // AllocHook - a static function
   // A glibc memory allocation hook.

   TMemStatMng* instance = TMemStatMng::GetInstance();
   // Restore all old hooks
   instance->Disable();

   // Call recursively
   void *result = malloc(size);
   // Call our routine
   instance->AddPointer(result, Int_t(size));
   //  TTimer::SingleShot(0, "TYamsMemMng", instance, "SaveData()");

   // Restore our own hooks
   instance->Enable();

   return result;
}

//______________________________________________________________________________
void TMemStatMng::FreeHook(void* ptr, const void* /*caller*/)
{
   // FreeHook - a static function
   // A glibc memory deallocation hook.

   TMemStatMng* instance = TMemStatMng::GetInstance();
   // Restore all old hooks
   instance->Disable();

   // Call recursively
   free(ptr);

   // Call our routine
   instance->AddPointer(ptr, -1);

   // Restore our own hooks
   instance->Enable();
}

//______________________________________________________________________________
void TMemStatMng::AddPointer(void *ptr, Int_t size)
{
   // Add pointer to table.
   // This method is called every time when any of the hooks are triggered.
   // The memory de-/allocation information will is recorded.

   void *stptr[g_BTStackLevel + 1];
   const int stackentries = getBacktrace(stptr, g_BTStackLevel, fUseGNUBuiltinBacktrace);

   // save only unique BTs
   TMD5 md5;
   md5.Update(reinterpret_cast<UChar_t*>(stptr), sizeof(void*) * stackentries);
   md5.Final();
   string crc_digest(md5.AsString());

   // for Debug. A counter of all (de)allacations.
   ++fBTIDCount;

   CRCSet_t::const_iterator found = fBTChecksums.find(crc_digest);
   // TODO: define a proper default value
   Int_t btid = -1;
   if(fBTChecksums.end() == found) {
      // check the size of the BT array container
      const int nbins = fHbtids->GetNbinsX();
      //check that the current allocation in fHbtids is enough, otherwise expend it with
      if(fBTCount + stackentries + 1 >= nbins) {
         fHbtids->SetBins(nbins * 2, 0, 1);
      }

      int *btids = fHbtids->GetArray();
      // A first value is a number of entries in a given stack
      btids[fBTCount++] = stackentries;
      btid = fBTCount;
      if(stackentries <= 0) {
         Warning("AddPointer",
                 "A number of stack entries is equal or less than zero. For btid %d", btid);
      }

      // add new BT's CRC value
      pair<CRCSet_t::iterator, bool> res = fBTChecksums.insert(CRCSet_t::value_type(crc_digest, btid));
      if(!res.second)
         Error("AddPointer", "Can't added a new BTID to the container.");

      // save all symbols of this BT
      for(int i = 0; i < stackentries; ++i) {
         ULong_t func_addr = (ULong_t)(stptr[i]);
         Int_t idx = fFAddrs.find(func_addr);
         // check, whether it's a new symbol
         if(idx < 0) {
            TString strFuncAddr;
            strFuncAddr += func_addr;
            TString strSymbolInfo;
            getSymbolFullInfo(stptr[i], &strSymbolInfo);

            TNamed *nm = new TNamed(strFuncAddr, strSymbolInfo);
            fFAddrsList->Add(nm);
            idx = fFAddrsList->GetEntriesFast() - 1;
            // TODO: more detailed error message...
            if(!fFAddrs.add(func_addr, idx))
               Error("AddPointer", "Can't add a function return address to the container");
         }

         // even if we have -1 as an index we add it to the container
         btids[fBTCount++] = idx;
      }

   } else {
      // reuse an existing BT
      btid = found->second;
   }

   if(btid <= 0)
      Error("AddPointer", "bad BT id");

   fTimeStamp.Set();
   Double_t CurTime = fTimeStamp.AsDouble();
   fTimems = Int_t(10000.*(CurTime - fBeginTime));
   ULong_t ul = (ULong_t)(ptr);
   fPos    = (ULong64_t)(ul);
   fNBytes = size;
   fN      = 0;
   fBtID   = btid;
   fDumpTree->Fill();
}

