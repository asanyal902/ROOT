// @(#)root/proof:$Name:  $:$Id: TProofPlayer.cxx,v 1.71 2005/11/02 15:35:23 rdm Exp $
// Author: Maarten Ballintijn   07/01/02

/*************************************************************************
 * Copyright (C) 1995-2001, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProofPlayer                                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TProofDraw.h"
#include "TProofPlayer.h"
#include "THashList.h"
#include "TEventIter.h"
#include "TVirtualPacketizer.h"
#include "TPacketizer.h"
#include "TPacketizer2.h"
#include "TPacketizerProgressive.h"
#include "TSelector.h"
#include "TSocket.h"
#include "TProofServ.h"
#include "TProof.h"
#include "TProofSuperMaster.h"
#include "TSlave.h"
#include "TROOT.h"
#include "TError.h"
#include "MessageTypes.h"
#include "TMessage.h"
#include "TDSetProxy.h"
#include "TString.h"
#include "TSystem.h"
#include "TFile.h"
#include "TProofDebug.h"
#include "TTimer.h"
#include "TMap.h"
#include "TPerfStats.h"
#include "TStatus.h"
#include "TEventList.h"
#include "TProofLimitsFinder.h"
#include "TSortedList.h"
#include "TTreeDrawArgsParser.h"
#include "TCanvas.h"
#include "TNamed.h"
#include "TObjString.h"
#include "TQueryResult.h"
#include "TMD5.h"
#include "TMethodCall.h"
#ifndef R__TH1MERGEFIXED
#include "TH1.h"
#endif

class TAutoBinVal : public TNamed {
private:
   Double_t fXmin, fXmax, fYmin, fYmax, fZmin, fZmax;

public:
   TAutoBinVal(const char *name, Double_t xmin, Double_t xmax, Double_t ymin,
               Double_t ymax, Double_t zmin, Double_t zmax) : TNamed(name,"")
   {
      fXmin = xmin; fXmax = xmax;
      fYmin = ymin; fYmax = ymax;
      fZmin = zmin; fZmax = zmax;
   }
   void GetAll(Double_t& xmin, Double_t& xmax, Double_t& ymin,
               Double_t& ymax, Double_t& zmin, Double_t& zmax)
   {
      xmin = fXmin; xmax = fXmax;
      ymin = fYmin; ymax = fYmax;
      zmin = fZmin; zmax = fZmax;
   }

};


//------------------------------------------------------------------------------


ClassImp(TProofPlayer)

//______________________________________________________________________________
TProofPlayer::TProofPlayer()
   : fAutoBins(0), fOutput(0), fSelector(0), fSelectorClass(0),
     fFeedbackTimer(0), fEvIter(0), fSelStatus(0), fEventsProcessed(0),
     fTotalEvents(0), fQueryResults(0), fQuery(0), fDrawQueries(0),
     fMaxDrawQueries(1)
{
   // Default ctor.

   fInput         = new TList;
   fExitStatus    = kFinished;
}

//______________________________________________________________________________
TProofPlayer::~TProofPlayer()
{
   SafeDelete(fInput);
   SafeDelete(fSelector);
   SafeDelete(fFeedbackTimer);
   SafeDelete(fEvIter);
   SafeDelete(fQueryResults);
}

//______________________________________________________________________________
void TProofPlayer::StopProcess(Bool_t abort)
{
   if (fEvIter != 0) fEvIter->StopProcess(abort);
   if (abort == kTRUE)
      fExitStatus = kAborted;
   else
      fExitStatus = kStopped;
}

//______________________________________________________________________________
void TProofPlayer::AddQueryResult(TQueryResult *q)
{
   // Add query result to the list, making sure that there are no
   // duplicates

   if (!q) {
      Warning("AddQueryResult","query undefined - do nothing");
      return;
   }

   // Treat differently normal and draw queries
   if (!(q->IsDraw())) {
      if (!fQueryResults) {
         fQueryResults = new TList;
         fQueryResults->Add(q);
      } else {
         TIter nxr(fQueryResults);
         TQueryResult *qr = 0;
         TQueryResult *qp = 0;
         while ((qr = (TQueryResult *) nxr())) {
            // If same query, remove old version and break
            if (*qr == *q) {
               fQueryResults->Remove(qr);
               delete qr;
               break;
            }
            // Record position according to end time
            if (qr->GetEndTime().Convert() < q->GetEndTime().Convert())
               qp = qr;
         }

         if (!qp) {
            fQueryResults->AddFirst(q);
         } else {
            fQueryResults->AddAfter(qp, q);
         }
      }
   } else if(IsClient()) {
      // If max reached, eliminate first the oldest one
      if (fDrawQueries == fMaxDrawQueries && fMaxDrawQueries > 0) {
         TIter nxr(fQueryResults);
         TQueryResult *qr = 0;
         while ((qr = (TQueryResult *) nxr())) {
            // If same query, remove old version and break
            if (qr->IsDraw()) {
               fDrawQueries--;
               fQueryResults->Remove(qr);
               delete qr;
               break;
            }
         }
      }
      // Add new draw query
      if (fDrawQueries >= 0 && fDrawQueries < fMaxDrawQueries) {
         fDrawQueries++;
         if (!fQueryResults)
            fQueryResults = new TList;
         fQueryResults->Add(q);
      }
   }
}

//______________________________________________________________________________
void TProofPlayer::RemoveQueryResult(const char *ref)
{
   // Remove all query result instances referenced 'ref' from
   // the list of results

   if (fQueryResults) {
      TIter nxq(fQueryResults);
      TQueryResult *qr = 0;
      while ((qr = (TQueryResult *) nxq())) {
         if (qr->Matches(ref)) {
            fQueryResults->Remove(qr);
            delete qr;
         }
      }
   }
}

//______________________________________________________________________________
TQueryResult *TProofPlayer::GetQueryResult(const char *ref)
{
   // Get query result instances referenced 'ref' from
   // the list of results

   if (fQueryResults) {
      TIter nxq(fQueryResults);
      TQueryResult *qr = 0;
      while ((qr = (TQueryResult *) nxq())) {
         if (qr->Matches(ref))
            return qr;
      }
   }

   // Nothing found
   return (TQueryResult *)0;
}

//______________________________________________________________________________
void TProofPlayer::SetCurrentQuery(TQueryResult *q)
{
   // Set current query and save previous value

   fPreviousQuery = fQuery;
   fQuery = q;
}

//______________________________________________________________________________
void TProofPlayer::AddInput(TObject *inp)
{
   fInput->Add(inp);
}

//______________________________________________________________________________
void TProofPlayer::ClearInput()
{
   fInput->Clear();
}

//______________________________________________________________________________
TObject *TProofPlayer::GetOutput(const char *name) const
{
   if (fOutput != 0) {
      return fOutput->FindObject(name);
   } else {
      return 0;
   }
}

//______________________________________________________________________________
TList *TProofPlayer::GetOutputList() const
{
   return fOutput;
}

//______________________________________________________________________________
Int_t TProofPlayer::ReinitSelector(TQueryResult *qr)
{
   // Reinitialize fSelector using the selector files in the query result.
   // Needed when Finalize is called after a Process execution for the same
   // selector name

   // Make sure we have a query
   if (!qr) {
      Info("ReinitSelector", "query undefined - do nothing");
      return -1;
   }

   // Selector name
   TString selec = qr->GetSelecImp()->GetName();
   if (selec.Length() <= 0) {
      Info("ReinitSelector", "selector name undefined - do nothing");
      return -1;
   }

   // Find out if this is a standard selection used for Draw actions
   Bool_t stdselec = TSelector::IsStandardDraw(selec);

   // If not, find out if it needs to be expanded
   TString ipathold;
   if (!stdselec) {
      // Check checksums for the versions of the selector files
      Bool_t expandselec = kTRUE;
      TString dir, ipath;
      char *selc = gSystem->Which(TROOT::GetMacroPath(), selec, kReadPermission);
      if (selc) {
         // Check checksums
         TMD5 *md5icur = 0, *md5iold = 0, *md5hcur = 0, *md5hold = 0;
         // Implementation files
         md5icur = TMD5::FileChecksum(selc);
         md5iold = qr->GetSelecImp()->Checksum();
         // Header files
         char *selh = StrDup(selc);
         char *p = (char *) strrchr(selh,'.');
         if (p) strcpy(p+1,"h");
         if (!gSystem->AccessPathName(selh, kReadPermission))
            md5hcur = TMD5::FileChecksum(selh);
         md5hold = qr->GetSelecHdr()->Checksum();

         // If nothing has changed nothing to do
         if (*md5hcur == *md5hold && *md5icur == *md5iold)
            expandselec = kFALSE;

         SafeDelete(md5icur);
         SafeDelete(md5hcur);
         SafeDelete(md5iold);
         SafeDelete(md5hold);
         if (selc) delete [] selc;
         if (selh) delete [] selh;
      }

      Bool_t ok = kTRUE;
      // Expand selector files, if needed
      if (expandselec) {

         ok = kFALSE;
         // Expand files in a temporary directory
         TUUID u;
         dir = Form("%s/%s",gSystem->TempDirectory(),u.AsString());
         if (!(gSystem->MakeDirectory(dir))) {

            // Export implementation file
            selec = Form("%s/%s",dir.Data(),selec.Data());
            qr->GetSelecImp()->SaveSource(selec);

            // Export header file
            TString seleh = Form("%s/%s",dir.Data(),qr->GetSelecHdr()->GetName());
            qr->GetSelecHdr()->SaveSource(seleh);

            // Adjust include path
            ipathold = gSystem->GetIncludePath();
            ipath = Form("-I%s %s", dir.Data(), gSystem->GetIncludePath());
            gSystem->SetIncludePath(ipath.Data());

            ok = kTRUE;
         }
      } else {
         // Add Aclic mode, if any
         TString opts(qr->GetOptions());
         Int_t idx = opts.Index("#");
         if (idx != kNPOS) {
            opts.Remove(0,idx+1);
            selec += opts;
         }
      }

      if (!ok) {
         Info("ReinitSelector", "problems locating or exporting selector files");
         return -1;
      }
   }

   // Cleanup previous stuff
   SafeDelete(fSelector);
   fSelectorClass = 0;

   // Init the selector now
   if ((fSelector = TSelector::GetSelector(selec))) {
      fSelectorClass = fSelector->IsA();
      fSelector->SetOption(qr->GetOptions());

      // Draw needs to reinit temp histos
      if (stdselec) {
         fSelector->SetInputList(qr->GetInputList());
         ((TProofDraw *)fSelector)->DefVar();
      }
   }

   // Restore original include path, if needed
   if (ipathold.Length() > 0)
      gSystem->SetIncludePath(ipathold.Data());

   return 0;
}

//______________________________________________________________________________
void TProofPlayer::StoreOutput(TList *)
{
   MayNotUse("StoreOutput");
}

//______________________________________________________________________________
void TProofPlayer::StoreFeedback(TObject *, TList *)
{
   MayNotUse("StoreFeedback");
}

//______________________________________________________________________________
void TProofPlayer::Progress(Long64_t /*total*/, Long64_t /*processed*/)
{
   MayNotUse("Progress");
}

//______________________________________________________________________________
void TProofPlayer::Feedback(TList *)
{
   MayNotUse("Feedback");
}

//______________________________________________________________________________
Long64_t TProofPlayer::Process(TDSet *dset, const char *selector_file,
                               Option_t *option, Long64_t nentries,
                               Long64_t first, TEventList * /*evl*/)
{
   PDB(kGlobal,1) Info("Process","Enter");

   fExitStatus = kFinished;
   fOutput = 0;

   SafeDelete(fSelector);
   fSelectorClass = 0;
   if (!(fSelector = TSelector::GetSelector(selector_file))) {
      Error("Process", "cannot load: %s", selector_file );
      return -1;
   }

   fSelectorClass = fSelector->IsA();
   Int_t version = fSelector->Version();

   fOutput = fSelector->GetOutputList();

   TPerfStats::Start(fInput, fOutput);

   fSelStatus = new TStatus;
   fOutput->Add(fSelStatus);

   TCleanup clean(this);
   SetupFeedback();

   fSelector->SetOption(option);
   fSelector->SetInputList(fInput);

   // If in sequential (0-PROOF) mode validate the data set to get
   // the number of entries
   fTotalEvents = nentries;
   if (fTotalEvents < 0 && gProofServ &&
       gProofServ->IsMaster() && !gProofServ->IsParallel()) {
      dset->Validate();
      dset->Reset();
      TDSetElement *e = 0;
      while ((e = dset->Next())) {
         fTotalEvents += e->GetNum();
      }
   }

   dset->Reset();

   fEvIter = TEventIter::Create(dset, fSelector, first, nentries);

   if (version == 0) {
      PDB(kLoop,1) Info("Process","Call Begin(0)");
      fSelector->Begin(0);
   } else {
      if (IsClient()) {
         // on client (for local run)
         PDB(kLoop,1) Info("Process","Call Begin(0)");
         fSelector->Begin(0);
      }
      if (fSelStatus->IsOk()) {
         PDB(kLoop,1) Info("Process","Call SlaveBegin(0)");
         fSelector->SlaveBegin(0);  // Init is called explicitly
                                    // from GetNextEvent()
      }
   }

   PDB(kLoop,1) Info("Process","Looping over Process()");

   // Loop over range
   Long64_t entry;
   fEventsProcessed = 0;
   while (fSelStatus->IsOk() &&
         (entry = fEvIter->GetNextEvent()) >= 0 && fSelStatus->IsOk()) {

      if (version == 0) {
         PDB(kLoop,3)Info("Process","Call ProcessCut(%lld)", entry);
         if (fSelector->ProcessCut(entry)) {
            PDB(kLoop,3)Info("Process","Call ProcessFill(%lld)", entry);
            fSelector->ProcessFill(entry);
         }
      } else {
         PDB(kLoop,3)Info("Process","Call Process(%lld)", entry);
         fSelector->Process(entry);
      }
      fEventsProcessed++;

      gSystem->DispatchOneEvent(kTRUE);
      if (gROOT->IsInterrupted()) break;
   }
   PDB(kGlobal,2) Info("Process","%lld events processed",fEventsProcessed);

   if (fFeedbackTimer != 0) HandleTimer(0);

   StopFeedback();

   delete fEvIter; fEvIter = 0;

   // Finalize

   if (fExitStatus != kAborted) {
      if (fSelStatus->IsOk()) {
         if (version == 0) {
            PDB(kLoop,1) Info("Process","Call Terminate()");
            fSelector->Terminate();
         } else {
            PDB(kLoop,1) Info("Process","Call SlaveTerminate()");
            fSelector->SlaveTerminate();
            if (IsClient() && fSelStatus->IsOk()) {
               PDB(kLoop,1) Info("Process","Call Terminate()");
               fSelector->Terminate();
            }
         }
      }
      if (gProofServ && !gProofServ->IsParallel()) {  // put all the canvases onto the output list
         TIter next(gROOT->GetListOfCanvases());
         while (TCanvas* c = dynamic_cast<TCanvas*> (next()))
            fOutput->Add(c);
      }
   }

   TPerfStats::Stop();

   return 0;
}

//______________________________________________________________________________
Long64_t TProofPlayer::Finalize(Bool_t, Bool_t)
{
   MayNotUse("Finalize");
   return -1;
}

//______________________________________________________________________________
Long64_t TProofPlayer::Finalize(TQueryResult *)
{
   MayNotUse("Finalize");
   return -1;
}

//______________________________________________________________________________
void TProofPlayer::UpdateAutoBin(const char *name,
                                 Double_t& xmin, Double_t& xmax,
                                 Double_t& ymin, Double_t& ymax,
                                 Double_t& zmin, Double_t& zmax)
{
   if ( fAutoBins == 0 ) {
      fAutoBins = new THashList;
   }

   TAutoBinVal *val = (TAutoBinVal*) fAutoBins->FindObject(name);

   if ( val == 0 ) {
      //look for info in higher master
      if (gProofServ && !gProofServ->IsTopMaster()) {
         TString key = name;
         TProofLimitsFinder::AutoBinFunc(key,xmin,xmax,ymin,ymax,zmin,zmax);
      }

      val = new TAutoBinVal(name,xmin,xmax,ymin,ymax,zmin,zmax);
      fAutoBins->Add(val);
   } else {
      val->GetAll(xmin,xmax,ymin,ymax,zmin,zmax);
   }
}

//______________________________________________________________________________
TDSetElement *TProofPlayer::GetNextPacket(TSlave *, TMessage *)
{
   MayNotUse("GetNextPacket");
   return 0;
}

//______________________________________________________________________________
void TProofPlayer::SetupFeedback()
{
   MayNotUse("SetupFeedback");
}

//______________________________________________________________________________
void TProofPlayer::StopFeedback()
{
   MayNotUse("StopFeedback");
}

//______________________________________________________________________________
Long64_t TProofPlayer::DrawSelect(TDSet * /*set*/, const char * /*varexp*/,
                               const char * /*selection*/, Option_t * /*option*/,
                               Long64_t /*nentries*/, Long64_t /*firstentry*/)
{
   MayNotUse("DrawSelect");

   return 0;
}



//------------------------------------------------------------------------------

ClassImp(TProofPlayerLocal)


//------------------------------------------------------------------------------

ClassImp(TProofPlayerRemote)


//______________________________________________________________________________
TProofPlayerRemote::~TProofPlayerRemote()
{
   SafeDelete(fOutput);      // owns the output list
   SafeDelete(fOutputLists);

   if (fFeedbackLists != 0) {
      TIter next(fFeedbackLists);
      while (TMap *m = (TMap*) next()) {
         m->DeleteValues();
      }
   }
   SafeDelete(fFeedbackLists);
   SafeDelete(fPacketizer);
}

//______________________________________________________________________________
Long64_t TProofPlayerRemote::Process(TDSet *dset, const char *selector_file,
                                     Option_t *option, Long64_t nentries,
                                     Long64_t first, TEventList * /*evl*/)
{
   // Process specified TDSet on PROOF.
   // Returns -1 in case error, 0 otherwise.

   PDB(kGlobal,1) Info("Process","Enter");
   fDSet = dset;
   fExitStatus = kFinished;
   fEventsProcessed = 0;

   //   delete fOutput;
   if (!fOutput)
      fOutput = new TList;
   else
      fOutput->Clear();

   if (fProof->IsMaster()){
      TPerfStats::Start(fInput, fOutput);
   } else {
      TPerfStats::Setup(fInput);
   }

   if(!SendSelector(selector_file)) return -1;

   TMessage mesg(kPROOF_PROCESS);
   TString fn(gSystem->BaseName(selector_file));

   // Parse option
   Bool_t sync = (fProof->GetQueryMode(option) == TProof::kSync);

   TDSet *set = dset;
   if (fProof->IsMaster()) {

      PDB(kPacketizer,1) Info("Process","Create Proxy TDSet");
      set = new TDSetProxy( dset->GetType(), dset->GetObjName(),
                            dset->GetDirectory() );

      delete fPacketizer;
      if (fInput->FindObject("PROOF_NewPacketizer") != 0) {
         Info("Process","!!! Using TPacketizer2 !!!");
         fPacketizer = new TPacketizer2(dset, fProof->GetListOfActiveSlaves(),
                                        first, nentries, fInput);
      } else if (fInput->FindObject("PROOF_ProgressivePacketizer") != 0) {
         Info("Process","!!! Using TPacketizerProgressive !!!");
         fPacketizer = new TPacketizerProgressive(dset, fProof->GetListOfActiveSlaves(),
                                                  first, nentries, fInput);
      } else {
         PDB(kGlobal,1) Info("Process","Using Standard TPacketizer");
         fPacketizer = new TPacketizer(dset, fProof->GetListOfActiveSlaves(),
                                       first, nentries, fInput);
      }

      if ( !fPacketizer->IsValid() ) {
         return -1;
      }

      // reset start, this is now managed by the packetizer
      first = 0;

   } else {

      // For a new query clients should make sure that the temporary
      // output list is empty
      if (fOutputLists) {
         fOutputLists->Delete();
         delete fOutputLists;
         fOutputLists = 0;
      }

      if (!sync) {
         gSystem->RedirectOutput(fProof->fLogFileName);
         Printf(" ");
         Info("Process","starting new query");
      }

      SafeDelete(fSelector);
      fSelectorClass = 0;
      if (!(fSelector = TSelector::GetSelector(selector_file))) {
         if (!sync)
            gSystem->RedirectOutput(0);
         return -1;
      }
      fSelectorClass = fSelector->IsA();
      fSelector->SetInputList(fInput);
      fSelector->SetOption(option);

      PDB(kLoop,1) Info("Process","Call Begin(0)");
      fSelector->Begin(0);

      if (!sync)
         gSystem->RedirectOutput(0);
   }

   TCleanup clean(this);
   SetupFeedback();

   TString opt = option;
   TEventList* elist = 0;
   if (!fProof->IsMaster() && set->GetEventList()) {
      elist = set->GetEventList();
   }

   if (gProofServ && gProofServ->IsMaster() && gProofServ->IsParallel()) {
      // Slaves will get the entry ranges from the packetizer
      mesg << set << fn << fInput << opt << (Long64_t)-1 << (Long64_t)0 << elist << sync;
   } else {
      mesg << set << fn << fInput << opt << nentries << first << elist << sync;
   }

   PDB(kGlobal,1) Info("Process","Calling Broadcast");
   fProof->Broadcast(mesg);

   // Redirect logs from master to special log frame
   if (IsClient())
      fProof->fRedirLog = kTRUE;

   if (!sync) {
      if (IsClient()) {
         // Asynchronous query: just make sure that asynchronous input
         // is enabled and return the prompt
         PDB(kGlobal,1) Info("Process","Asynchronous processing:"
                                       " activating CollectInputFrom");
         fProof->Activate();

         // Receive the acknowledgement and query sequential number
         fProof->Collect();

         return fProof->fSeqNum;

      } else {
         PDB(kGlobal,1) Info("Process","Calling Collect");
         fProof->Collect();

         HandleTimer(0); // force an update of final result
         StopFeedback();

         return Finalize(kFALSE,sync);
      }
   } else {

      PDB(kGlobal,1) Info("Process","Synchronous processing: calling Collect");
      fProof->Collect();

      // Restore prompt logging, for clients (Collect leaves things as they were
      // at the time it was called)
      if (IsClient())
         fProof->fRedirLog = kFALSE;

      if (!IsClient())
         HandleTimer(0); // force an update of final result
      StopFeedback();

      if (!IsClient() || GetExitStatus() != TProofPlayer::kAborted)
         return Finalize(kFALSE,sync);
      else
         return -1;
   }
}

//______________________________________________________________________________
Long64_t TProofPlayerRemote::Finalize(Bool_t force, Bool_t sync)
{
   // Finalize a query.
   // Returns -1 in case error, 0 otherwise.

   if (fOutputLists == 0) {
      if (IsClient()) {
         if (force)
            if (fQuery)
               return fProof->Finalize(Form("%s:%s", fQuery->GetTitle(),
                                                     fQuery->GetName()), force);
         Info("Finalize","Output list is empty (already finalized?)");
      } else {
         PDB(kGlobal,1)
             Info("Finalize","Output list is empty (already finalized?)");
      }
      return -1;
   }

   PDB(kGlobal,1) Info("Finalize","Calling Merge Output");
   MergeOutput();

   Long64_t rv = 0;
   if (fProof->IsMaster()) {
      TPerfStats::Stop();
   } else {
      if (fExitStatus != kAborted) {

         if (!sync) {
            // Reinit selector (with multi-sessioning we must do this until
            // TSelector::GetSelector() is optimized to i) avoid reloading of an
            // unchanged selector and ii) invalidate existing instances of
            // reloaded selector)
            if (ReinitSelector(fQuery) == -1) {
               Info("Finalize", "problems reinitializing selector \"%s\"",
                    fQuery->GetSelecImp()->GetName());
               return -1;
            }
         }

         TIter next(fOutput);
         TList *output = fSelector->GetOutputList();
         while(TObject* obj = next()) {
            if (!fProof->IsParallel()) {
               if (TCanvas* c = dynamic_cast<TCanvas *> (obj))
                  c->Draw();
               else
                  output->Add(obj);
            }
            else
               output->Add(obj);
         }

         PDB(kLoop,1) Info("Process","Call Terminate()");
         fOutput->Clear("nodelete");
         fSelector->Terminate();

         rv = fSelector->GetStatus();

         // copy the output list back and clean the selector's list
         TIter it(output);
         while(TObject* o = it()) {
            fOutput->Add(o);
         }
         // Save the output list in the current query
         // (list contents is duplicated inside)
         fQuery->SetOutputList(fOutput);

         // Set in finalized state (cannot be done twice)
         fQuery->SetFinalized();

         // FIXME
         output->SetOwner(kFALSE);
         output->Clear("nodelete");
      }
   }
   PDB(kGlobal,1) Info("Process","exit");
   return rv;
}

//______________________________________________________________________________
Long64_t TProofPlayerRemote::Finalize(TQueryResult *qr)
{
   // Finalize the results of a query already processed

   PDB(kGlobal,1) Info("Finalize(TQueryResult *)","Enter");

   if (!IsClient()) {
      Info("Finalize(TQueryResult *)","method to be executed only clients");
      return -1;
   }

   if (!qr) {
      Info("Finalize(TQueryResult *)", "query undefined");
      return -1;
   }

   if (qr->IsFinalized()) {
      Info("Finalize(TQueryResult *)", "query already finalized");
      return -1;
   }

   // Reset the list
   if (!fOutput)
      fOutput = new TList;
   else
      fOutput->Clear();

   // Make sure that the temporary output list is empty
   if (fOutputLists) {
      fOutputLists->Delete();
      delete fOutputLists;
      fOutputLists = 0;
   }

   // Re-init the selector
   gSystem->RedirectOutput(fProof->fLogFileName);

   // Import the output list
   TList *tmp = (TList *) qr->GetOutputList();
   if (!tmp) {
      gSystem->RedirectOutput(0);
      Info("Finalize(TQueryResult *)", "ouputlist is empty");
      return -1;
   }
   TList *out = new TList;
   TIter nxo(tmp);
   TObject *o = 0;
   while ((o = nxo()))
      out->Add(o->Clone());

   // Adopts the list
   out->SetOwner();
   StoreOutput(out);

   gSystem->RedirectOutput(0);

   // Finalize it
   SetCurrentQuery(qr);
   Long64_t rc = Finalize();
   RestorePreviousQuery();

   return rc;
}

//______________________________________________________________________________
Bool_t TProofPlayerRemote::SendSelector(const char* selector_file)
{
   // Send the selector file to the slave nodes

   // If the filename does not contain "." assume class is compiled in
   if ( strchr(selector_file,'.') != 0 ) {
      TString filename = selector_file;
      TString aclicMode;
      TString arguments;
      TString io;
      filename = gSystem->SplitAclicMode(filename, aclicMode, arguments, io);

      PDB(kSelector,1) Info("SendSelector", "Sendfile: %s", filename.Data() );
      if ( fProof->SendFile(filename) == -1 ) return kFALSE;

      // NOTE: should we allow more extension?
      if ( filename.EndsWith(".C") ) {
         filename.Replace(filename.Length()-1,1,"h");
         if (!gSystem->AccessPathName(filename,kReadPermission)) {
            PDB(kSelector,1) Info("SendSelector", "SendFile: %s", filename.Data() );
            if ( fProof->SendFile(filename) == -1 ) return kFALSE;
         }
      }
   }
   return kTRUE;
}

//______________________________________________________________________________
void TProofPlayerRemote::MergeOutput()
{
   // Merge objects in output the lists.

   PDB(kOutput,1) Info("MergeOutput","Enter");

   if (fOutputLists == 0) {
      PDB(kOutput,1) Info("MergeOutput","Leave (no output)");
      return;
   }

   TIter next(fOutputLists);

   TList *list;
   while ( (list = (TList *) next()) ) {

      TObject *obj = fOutput->FindObject(list->GetName());

      if (obj == 0) {
         obj = list->First();
         list->Remove(obj);
         fOutput->Add(obj);
      }

      if ( list->IsEmpty() ) continue;

      TMethodCall callEnv;
      if (obj->IsA())
         callEnv.InitWithPrototype(obj->IsA(), "Merge", "TCollection*");
      if (callEnv.IsValid()) {
         callEnv.SetParam((Long_t) list);
         callEnv.Execute(obj);
      } else {
         // No Merge interface, return individual objects
         while ( (obj = list->First()) ) {
            fOutput->Add(obj);
            list->Remove(obj);
         }
      }
   }

   SafeDelete(fOutputLists);

   PDB(kOutput,1) Info("MergeOutput","Leave (%d object(s))", fOutput->GetSize());
}

//______________________________________________________________________________
void TProofPlayerRemote::Progress(Long64_t total, Long64_t processed)
{
   PDB(kGlobal,1)
      Info("Progress","%2f (%lld/%lld)", 100.*processed/total, processed, total);

   EmitVA("Progress(Long64_t,Long64_t)", 2, total, processed);

   fProof->Progress(total, processed);
}

//______________________________________________________________________________
void TProofPlayerRemote::Feedback(TList *objs)
{
   PDB(kGlobal,1) Info("Feedback","%d Objects", objs->GetSize());
   PDB(kFeedback,1) {
      Info("Feedback","%d Objects", objs->GetSize());
      objs->ls();
   }

   Emit("Feedback(TList *objs)", (Long_t) objs);

   fProof->Feedback(objs);
}

//______________________________________________________________________________
void TProofPlayerRemote::StopProcess(Bool_t abort)
{
   if (fPacketizer != 0) fPacketizer->StopProcess(abort);
   if (abort == kTRUE)
      fExitStatus = kAborted;
   else
      fExitStatus = kStopped;
}

//______________________________________________________________________________
void TProofPlayerRemote::StoreOutput(TList *out)
{
   PDB(kOutput,1) Info("StoreOutput","Enter");

   if ( out == 0 ) {
      PDB(kOutput,1) Info("StoreOutput","Leave (empty)");
      return;
   }

   TIter next(out);
   out->SetOwner(kFALSE);  // take ownership of the contents

   if (fOutputLists == 0) {
      PDB(kOutput,2) Info("StoreOutput","Create fOutputLists");
      fOutputLists = new TList;
      fOutputLists->SetOwner();
   }
   // process eventlists first
   TList* lists = dynamic_cast<TList*> (out->FindObject("PROOF_EventListsList"));
   if (lists) {
      out->Remove(lists);
      TEventList *mainList = new TEventList("PROOF_EventList");
      out->Add(mainList);
      TIter it(lists);
      TEventList *aList;
      while ( (aList = dynamic_cast<TEventList*> (it())) ) {
         // find file offset
         TIter next(fDSet->GetListOfElements());
         TDSetElement *elem;
         while ( (elem = dynamic_cast<TDSetElement*> (next())) ) {
            if (strcmp(elem->GetFileName(), aList->GetName()) == 0)
               break;
         }
         if (!elem) {
            Error("StoreOutput",Form("Found the EventList for %s, but no object with that name "
                                 "in the TDSet", aList->GetName()));
            continue;
         }
         int offset = elem->GetTDSetOffset();

         // shift the list by the number of first event in that file
         Long64_t *arr = aList->GetList();
         Int_t num = aList->GetN();
         if (arr && offset)
            for (int i = 0; i < num; i++)
               arr[i] += offset;

         mainList->Add(aList);           // add to the main list
      }
      delete lists;
   }

   TObject *obj;
   while( (obj = next()) ) {
      PDB(kOutput,2) Info("StoreOutput","Find '%s'", obj->GetName() );

      TList *list = (TList *) fOutputLists->FindObject( obj->GetName() );
      if ( list == 0 ) {
         PDB(kOutput,2) Info("StoreOutput","List not Found (creating)", obj->GetName() );
         list = new TList;
         list->SetName( obj->GetName() );
         list->SetOwner();
         fOutputLists->Add( list );
      }
      list->Add( obj );
   }

   delete out;
   PDB(kOutput,1) Info("StoreOutput","Leave");
}

//______________________________________________________________________________
TList *TProofPlayerRemote::MergeFeedback()
{
   // Merge feedback lists.

   PDB(kFeedback,1) Info("MergeFeedback","Enter");

   if ( fFeedbackLists == 0 ) {
      PDB(kFeedback,1) Info("MergeFeedback","Leave (no output)");
      return 0;
   }

   TList *fb = new TList;   // collection of feedback objects
   fb->SetOwner();

   TIter next(fFeedbackLists);

   TMap *map;
   while ( (map = (TMap*) next()) ) {

      // turn map into list ...

      TList *list = new TList;
      TIter keys(map);

#ifndef R__TH1MERGEFIXED
      Int_t nbmx = -1;
      TObject *oref = 0;
#endif
      while ( TObject *key = keys() ) {
         list->Add(map->GetValue(key));
#ifndef R__TH1MERGEFIXED
         // Temporary fix for to cope with the problem in TH1::Merge.
         // We need to use a reference histo the one with the largest number
         // of bins so that the histos from all submasters can be correctly
         // fit in
         TObject *o = map->GetValue(key);
         if (o->InheritsFrom("TH1") && !strncmp(o->GetName(),"PROOF_",6)) {
            if (((TH1 *)o)->GetNbinsX() > nbmx) {
               nbmx=  ((TH1 *)o)->GetNbinsX();
               oref = o;
            }
         }
#endif
      }

      // clone first object, remove from list
#ifdef R__TH1MERGEFIXED
      TObject *obj = list->First();
#else
      TObject *obj = (oref) ? oref : list->First();
#endif
      list->Remove(obj);
      obj = obj->Clone();
      fb->Add(obj);

      if ( list->IsEmpty() ) {
         delete list;
         continue;
      }

      // merge list with clone
      TMethodCall callEnv;
      if (obj->IsA())
         callEnv.InitWithPrototype(obj->IsA(), "Merge", "TCollection*");
      if (callEnv.IsValid()) {
         callEnv.SetParam((Long_t) list);
         callEnv.Execute(obj);
      } else {
         // No Merge interface, return copy of individual objects
         while ( (obj = list->First()) ) {
            fb->Add(obj->Clone());
            list->Remove(obj);
         }
      }

      delete list;
   }

   PDB(kFeedback,1) Info("MergeFeedback","Leave (%d object(s))", fb->GetSize());

   return fb;
}

//______________________________________________________________________________
void TProofPlayerRemote::StoreFeedback(TObject *slave, TList *out)
{
   // Store feedback results from the specified slave.

   PDB(kFeedback,1) Info("StoreFeedback","Enter");

   if ( out == 0 ) {
      PDB(kFeedback,1) Info("StoreFeedback","Leave (empty)");
      return;
   }

   if ( IsClient() ) {
      // in client
      Feedback(out);
      delete out;
      return;
   }

   if (fFeedbackLists == 0) {
      PDB(kFeedback,2) Info("StoreFeedback","Create fFeedbackLists");
      fFeedbackLists = new TList;
      fFeedbackLists->SetOwner();
   }

   TIter next(out);
   out->SetOwner(kFALSE);  // take ownership of the contents

   TObject *obj;
   while( (obj = next()) ) {
      PDB(kFeedback,2) Info("StoreFeedback","Find '%s'", obj->GetName() );

      TMap *map = (TMap*) fFeedbackLists->FindObject(obj->GetName());
      if ( map == 0 ) {
         PDB(kFeedback,2) Info("StoreFeedback","Map not Found (creating)", obj->GetName() );
         // map must not be owner (ownership is with regards to the keys (only))
         map = new TMap;
         map->SetName(obj->GetName());
         fFeedbackLists->Add(map);
      } else {
         PDB(kFeedback,2) Info("StoreFeedback","removing previous value");
         if (map->GetValue(slave))
            delete map->GetValue(slave);
         map->Remove(slave);
      }
      map->Add(slave, obj);
   }

   delete out;
   PDB(kFeedback,1) Info("StoreFeedback","Leave");
}

//______________________________________________________________________________
void TProofPlayerRemote::SetupFeedback()
{
   // Setup reporting of feedback objects.

   if ( IsClient() ) return; // Client does not need timer

   fFeedback = (TList*) fInput->FindObject("FeedbackList");

   PDB(kFeedback,1) Info("SetupFeedback","\"FeedbackList\" %sfound",
      fFeedback == 0 ? "NOT ":"");

   if (fFeedback == 0) return;

   // OK, feedback was requested, setup the timer

   fFeedbackTimer = new TTimer;
   fFeedbackTimer->SetObject(this);
   fFeedbackTimer->Start(500,kTRUE);
}

//______________________________________________________________________________
void TProofPlayerRemote::StopFeedback()
{
   // Stop reporting of feedback objects.

   if (fFeedbackTimer == 0) return;

   PDB(kFeedback,1) Info("StopFeedback","Stop Timer");

   delete fFeedbackTimer; fFeedbackTimer = 0;
}

//______________________________________________________________________________
Bool_t TProofPlayerRemote::HandleTimer(TTimer *)
{
   // Send feedback objects to client.

   PDB(kFeedback,2) Info("HandleTimer","Entry");

   Assert( !IsClient() );

   if ( fFeedbackTimer == 0 ) return kFALSE; // timer already switched off


   // process local feedback objects

   TList *fb = new TList;
   fb->SetOwner();

   TIter next(fFeedback);
   while( TObjString *name = (TObjString*) next() ) {
      TObject *o = fOutput->FindObject(name->GetName());
      if (o != 0) fb->Add(o->Clone());
   }

   if (fb->GetSize() > 0)
      StoreFeedback(this, fb); // adopts fb
   else
      delete fb;

   if ( fFeedbackLists == 0 ) {
      fFeedbackTimer->Start(500,kTRUE);   // maybe next time
      return kFALSE;
   }

   fb = MergeFeedback();

   PDB(kFeedback,2) Info("HandleTimer","Sending %d objects", fb->GetSize());

   TMessage m(kPROOF_FEEDBACK);
   m << fb;

   // send message to client;
   gProofServ->GetSocket()->Send(m);

   delete fb;

   fFeedbackTimer->Start(500,kTRUE);

   return kFALSE; // ignored?
}

//______________________________________________________________________________
TDSetElement *TProofPlayerRemote::GetNextPacket(TSlave *slave, TMessage *r)
{
   // Get next packet for specified slave.

   TDSetElement *e = fPacketizer->GetNextPacket( slave, r );

   if (e == 0) {
      PDB(kPacketizer,2) Info("GetNextPacket","Done");
   } else if (e == (TDSetElement*) -1) {
      PDB(kPacketizer,2) Info("GetNextPacket","Waiting");
   } else {
      PDB(kPacketizer,2)
         Info("GetNextPacket","To slave-%d (%s): '%s' '%s' '%s' %lld %lld",
              slave->GetOrdinal(), slave->GetName(), e->GetFileName(),
              e->GetDirectory(), e->GetObjName(), e->GetFirst(), e->GetNum());
   }

   return e;
}

//______________________________________________________________________________
Bool_t TProofPlayerRemote::IsClient() const
{
   // Is the player running on the client?

   return !fProof->IsMaster();
}

//______________________________________________________________________________
Long64_t TProofPlayerRemote::DrawSelect(TDSet *set, const char *varexp,
                               const char *selection, Option_t *option,
                               Long64_t nentries, Long64_t firstentry)
{
   TTreeDrawArgsParser info;
   info.Parse(varexp, selection, option);
   TString selector = info.GetProofSelectorName();

   TNamed *varexpobj = new TNamed("varexp", varexp);
   TNamed *selectionobj = new TNamed("selection", selection);

   // save the feedback list
   TList *fb = (TList*) fInput->FindObject("FeedbackList");
   if (fb)
      fInput->Remove(fb);

   fInput->Clear();  // good idea? what about a feedbacklist, but old query
                     // could have left objs? clear at end? no, may want to
                     // rerun, separate player?
   if (fb)
      fInput->Add(fb);

   fInput->Add(varexpobj);
   fInput->Add(selectionobj);

   if (info.GetObjectName() == "")
      info.SetObjectName("htemp");
   fProof->AddFeedback(info.GetObjectName());
   Long64_t r = Process(set, selector, option, nentries, firstentry);
   fProof->RemoveFeedback(info.GetObjectName());

   fInput->Remove(varexpobj);
   fInput->Remove(selectionobj);
   if (TNamed *opt = dynamic_cast<TNamed*> (fInput->FindObject("PROOF_OPTIONS"))) {
      fInput->Remove(opt);
      delete opt;
   }

   delete varexpobj;
   delete selectionobj;

   return r;
}


//------------------------------------------------------------------------------


ClassImp(TProofPlayerSlave)


//______________________________________________________________________________
TProofPlayerSlave::TProofPlayerSlave()
{
  fSocket = 0;
  fFeedback = 0;
}

//______________________________________________________________________________
TProofPlayerSlave::TProofPlayerSlave(TSocket *socket)
{
      fSocket = socket;
      fFeedback = 0;
}

//______________________________________________________________________________
void TProofPlayerSlave::SetupFeedback()
{
   //
   TList *fb = (TList*) fInput->FindObject("FeedbackList");

   PDB(kFeedback,1) Info("SetupFeedback","\"FeedbackList\" %sfound",
      fb == 0 ? "NOT ":"");

   if (fb == 0) return;

   // OK, feedback was requested, setup the timer

   fFeedbackTimer = new TTimer;
   fFeedbackTimer->SetObject(this);
   fFeedbackTimer->Start(500,kFALSE);

   fFeedback = fb;

}

//______________________________________________________________________________
void TProofPlayerSlave::StopFeedback()
{
   if (fFeedbackTimer == 0) return;

   PDB(kFeedback,1) Info("StopFeedback","Stop Timer");

   fFeedbackTimer->Stop();
   delete fFeedbackTimer;
   fFeedbackTimer = 0;
}

//______________________________________________________________________________
Bool_t TProofPlayerSlave::HandleTimer(TTimer *)
{
   PDB(kFeedback,2) Info("HandleTimer","Entry");

   // If in sequential (0-slave-PROOF) mode we do not have a packetizer
   // so we also send the info to update the progress bar.
   if (gProofServ && gProofServ->IsMaster() && !gProofServ->IsParallel()) {
      TMessage m(kPROOF_PROGRESS);
      m << fTotalEvents << fEventsProcessed;
      gProofServ->GetSocket()->Send(m);
   }

   if ( fFeedback == 0 ) return kFALSE;

   TList *fb = new TList;
   fb->SetOwner(kFALSE);

   if (fOutput == 0) {
      fOutput = fSelector->GetOutputList();
   }

   if (fOutput) {
      TIter next(fFeedback);
      while( TObjString *name = (TObjString*) next() ) {
         // TODO: find object in memory ... maybe allow only in fOutput ?
         TObject *o = fOutput->FindObject(name->GetName());
         if (o != 0) fb->Add(o);
      }
   }

   PDB(kFeedback,2) Info("HandleTimer","Sending %d objects", fb->GetSize());

   TMessage m(kPROOF_FEEDBACK);
   m << fb;

   // send message to client;
   gProofServ->GetSocket()->Send(m);

   delete fb;

   fFeedbackTimer->Start(500,kTRUE);

   return kFALSE; // ignored?
}


//______________________________________________________________________________
Long64_t TProofPlayerSlave::DrawSelect(TDSet * /*set*/, const char * /*varexp*/,
                               const char * /*selection*/, Option_t * /*option*/,
                               Long64_t /*nentries*/, Long64_t /*firstentry*/)
{
   MayNotUse("DrawSelect");

   return -1;
}

//------------------------------------------------------------------------------

ClassImp(TProofPlayerSuperMaster)


//______________________________________________________________________________
Long64_t TProofPlayerSuperMaster::Process(TDSet *dset, const char *selector_file,
                                          Option_t *option, Long64_t nentries,
                                          Long64_t first, TEventList * /*evl*/)
{
   // Process specified TDSet on PROOF.
   // Returns -1 in case error, 0 otherwise.

   fEventsProcessed = 0;
   PDB(kGlobal,1) Info("Process","Enter");

   TProofSuperMaster *proof = dynamic_cast<TProofSuperMaster*>(GetProof());
   if (!proof) return -1;

   delete fOutput;
   fOutput = new TList;

   TPerfStats::Start(fInput, fOutput);

   if (!SendSelector(selector_file)) return -1;

   TCleanup clean(this);
   SetupFeedback();

   if (proof->IsMaster()) {

      // make sure the DSet is valid
      if (!dset->ElementsValid()) {
         proof->ValidateDSet(dset);
         if (!dset->ElementsValid()) {
            Error("Process", "could not validate TDSet");
            return -1;
         }
      }

      TList msds;
      msds.SetOwner(); // This will delete TPairs

      TList keyholder; // List to clean up key part of the pairs
      keyholder.SetOwner();
      TList valueholder; // List to clean up value part of the pairs
      valueholder.SetOwner();

      // Construct msd list using the slaves
      TIter nextslave(proof->GetListOfActiveSlaves());
      while (TSlave *sl = dynamic_cast<TSlave*>(nextslave())) {
         TList *submasters = 0;
         TPair *msd = dynamic_cast<TPair*>(msds.FindObject(sl->GetMsd()));
         if (!msd) {
            submasters = new TList;
            submasters->SetName(sl->GetMsd());
            keyholder.Add(submasters);
            TList *setelements = new TSortedList(kSortDescending);
            setelements->SetName(TString(sl->GetMsd())+"_Elements");
            valueholder.Add(setelements);
            msds.Add(new TPair(submasters, setelements));
         } else {
            submasters = dynamic_cast<TList*>(msd->Key());
         }
         submasters->Add(sl);
      }

      // Add TDSetElements to msd list
      Long64_t cur = 0; //start of next element
      TIter nextelement(dset->GetListOfElements());
      while (TDSetElement *elem = dynamic_cast<TDSetElement*>(nextelement())) {

         if (elem->GetNum()<1) continue; // get rid of empty elements

         if (nentries !=-1 && cur>=first+nentries) {
            // we are done
            break;
         }

         if (cur+elem->GetNum()-1<first) {
            //element is before first requested entry
            cur+=elem->GetNum();
            continue;
         }

         if (cur<first) {
            //modify element to get proper start
            elem->SetNum(elem->GetNum()-(first-cur));
            elem->SetFirst(elem->GetFirst()+first-cur);
            cur=first;
         }

         if (nentries==-1 || cur+elem->GetNum()<=first+nentries) {
            cur+=elem->GetNum();
         } else {
            //modify element to get proper end
            elem->SetNum(first+nentries-cur);
            cur=first+nentries;
         }

         TPair *msd = dynamic_cast<TPair*>(msds.FindObject(elem->GetMsd()));
         if (!msd) {
            Error("Process", "data requires mass storage domain '%s'"
                  " which is not accessible in this proof session",
                  elem->GetMsd());
            return -1;
         } else {
            TList *elements = dynamic_cast<TList*>(msd->Value());
            elements->Add(elem);
         }
      }

      TList usedmasters;
      TIter nextmsd(msds.MakeIterator());
      while (TPair *msd = dynamic_cast<TPair*>(nextmsd())) {
         TList *submasters = dynamic_cast<TList*>(msd->Key());
         TList *setelements = dynamic_cast<TList*>(msd->Value());

         // distribute elements over the masters
         Int_t nmasters = submasters->GetSize();
         Int_t nelements = setelements->GetSize();
         for (Int_t i=0; i<nmasters; i++) {

            Long64_t nentries = 0;
            TDSet set(dset->GetType(), dset->GetObjName(),
                      dset->GetDirectory());
            for (Int_t j = (i*nelements)/nmasters;
                       j < ((i+1)*nelements)/nmasters;
                       j++) {
               TDSetElement *elem =
                  dynamic_cast<TDSetElement*>(setelements->At(j));
               set.Add(elem->GetFileName(), elem->GetObjName(),
                       elem->GetDirectory(), elem->GetFirst(),
                       elem->GetNum(), elem->GetMsd());
               nentries+=elem->GetNum();
            }

            if (set.GetListOfElements()->GetSize()>0) {
               TMessage mesg(kPROOF_PROCESS);
               TString fn(gSystem->BaseName(selector_file));
               TString opt = option;
               mesg << &set << fn << fInput << opt << Long64_t(-1) << Long64_t(0);

               TSlave *sl = dynamic_cast<TSlave*>(submasters->At(i));
               PDB(kGlobal,1) Info("Process",
                                   "Sending TDSet with %d elements to submaster %s",
                                   set.GetListOfElements()->GetSize(),
                                   sl->GetOrdinal());
               sl->GetSocket()->Send(mesg);
               usedmasters.Add(sl);

               // setup progress info
               fSlaves.AddLast(sl);
               fSlaveProgress.Set(fSlaveProgress.GetSize()+1);
               fSlaveProgress[fSlaveProgress.GetSize()-1]=0;
               fSlaveTotals.Set(fSlaveTotals.GetSize()+1);
               fSlaveTotals[fSlaveTotals.GetSize()-1]=nentries;
            }
         }
      }

      if ( !IsClient() ) HandleTimer(0);
      PDB(kGlobal,1) Info("Process","Calling Collect");
      proof->Collect(&usedmasters);
      HandleTimer(0);

   }

   StopFeedback();

   PDB(kGlobal,1) Info("Process","Calling Merge Output");
   MergeOutput();

   TPerfStats::Stop();

   return 0;
}

//______________________________________________________________________________
void TProofPlayerSuperMaster::Progress(TSlave *sl, Long64_t total, Long64_t processed)
{
   // Report progress.

   Int_t idx = fSlaves.IndexOf(sl);
   fSlaveProgress[idx] = processed;
   if (fSlaveTotals[idx] != total)
      Warning("Progress", "total events has changed for slave %s", sl->GetName());
   fSlaveTotals[idx] = total;

   Long64_t tot = 0;
   Int_t i;
   for (i = 0; i < fSlaveTotals.GetSize(); i++) tot += fSlaveTotals[i];
   Long64_t proc = 0;
   for (i = 0; i < fSlaveProgress.GetSize(); i++) proc += fSlaveProgress[i];

   Progress(tot, proc);
}

//______________________________________________________________________________
Bool_t TProofPlayerSuperMaster::HandleTimer(TTimer *)
{
   // Send progress and feedback to client.

   if (fFeedbackTimer == 0) return kFALSE; // timer stopped already

   Long64_t tot = 0;
   Int_t i;
   for (i = 0; i < fSlaveTotals.GetSize(); i++) tot += fSlaveTotals[i];
   Long64_t proc = 0;
   for (i = 0; i < fSlaveProgress.GetSize(); i++) proc += fSlaveProgress[i];

   TMessage m(kPROOF_PROGRESS);

   m << tot << proc;

   // send message to client;
   gProofServ->GetSocket()->Send(m);

   if (fReturnFeedback)
      return TProofPlayerRemote::HandleTimer(0);
   else
      return kFALSE;
}

//______________________________________________________________________________
void TProofPlayerSuperMaster::SetupFeedback()
{
   // Setup reporting of feedback objects and progress messages.

   if (IsClient()) return; // Client does not need timer

   TProofPlayerRemote::SetupFeedback();

   if (fFeedbackTimer) {
      fReturnFeedback = kTRUE;
      return;
   } else {
      fReturnFeedback = kFALSE;
   }

   // setup the timer for progress message

   fFeedbackTimer = new TTimer;
   fFeedbackTimer->SetObject(this);
   fFeedbackTimer->Start(500,kFALSE);
}
