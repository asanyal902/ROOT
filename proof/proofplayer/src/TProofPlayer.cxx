// @(#)root/proofplayer:$Id$
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
// This internal class and its subclasses steer the processing in PROOF.//
// Instances of the TProofPlayer class are created on the worker nodes  //
// per session and do the processing.                                   //
// Instances of its subclass - TProofPlayerRemote are created per each  //
// query on the master(s) and on the client. On the master(s),          //
// TProofPlayerRemote coordinate processing, check the dataset, create  //
// the packetizer and take care of merging the results of the workers.  //
// The instance on the client collects information on the input         //
// (dataset and selector), it invokes the Begin() method and finalizes  //
// the query by calling Terminate().                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TProofDraw.h"
#include "TProofPlayer.h"
#include "THashList.h"
#include "TEnv.h"
#include "TEventIter.h"
#include "TVirtualPacketizer.h"
#include "TSelector.h"
#include "TSocket.h"
#include "TProofServ.h"
#include "TProof.h"
#include "TProofFile.h"
#include "TProofSuperMaster.h"
#include "TSlave.h"
#include "TClass.h"
#include "TROOT.h"
#include "TError.h"
#include "TException.h"
#include "MessageTypes.h"
#include "TMessage.h"
#include "TDSetProxy.h"
#include "TString.h"
#include "TSystem.h"
#include "TFile.h"
#include "TFileInfo.h"
#include "TFileMerger.h"
#include "TProofDebug.h"
#include "TTimer.h"
#include "TMap.h"
#include "TPerfStats.h"
#include "TStatus.h"
#include "TEventList.h"
#include "TProofLimitsFinder.h"
#include "TSortedList.h"
#include "TTree.h"
#include "TEntryList.h"
#include "TDSet.h"
#include "TDrawFeedback.h"
#include "TNamed.h"
#include "TObjString.h"
#include "TQueryResult.h"
#include "TMD5.h"
#include "TMethodCall.h"
#include "TObjArray.h"
#include "TMutex.h"
#ifndef R__TH1MERGEFIXED
#include "TH1.h"
#endif
#include "TVirtualMonitoring.h"
#include "TParameter.h"

// Timeout exception
#define kPEX_STOPPED  1001
#define kPEX_ABORTED  1002

// To flag an abort condition: use a local static variable to avoid
// warnings about problems with longjumps
static Bool_t gAbort = kFALSE;

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

//
// Special timer to dispatch pending events while processing
//______________________________________________________________________________
class TDispatchTimer : public TTimer {
private:
   TProofPlayer    *fPlayer;

public:
   TDispatchTimer(TProofPlayer *p) : TTimer(1000, kFALSE), fPlayer(p) { }

   Bool_t Notify();
};
//______________________________________________________________________________
Bool_t TDispatchTimer::Notify()
{
   // Handle expiration of the timer associated with dispatching pending
   // events while processing. We must act as fast as possible here, so
   // we just set a flag submitting a request for dispatching pending events

   if (gDebug > 0)
      Info ("Notify","called!");

   fPlayer->SetBit(TProofPlayer::kDispatchOneEvent);

   // Needed for the next shot
   Reset();
   return kTRUE;
}

//
// Special timer to handle stop/abort request via exception raising
//______________________________________________________________________________
class TStopTimer : public TTimer {
private:
   Bool_t           fAbort;
   TProofPlayer    *fPlayer;

public:
   TStopTimer(TProofPlayer *p, Bool_t abort, Int_t to);

   Bool_t Notify();
};

//______________________________________________________________________________
TStopTimer::TStopTimer(TProofPlayer *p, Bool_t abort, Int_t to)
           : TTimer(((to <= 0 || to > 864000) ? 10 : to * 1000), kFALSE)
{
   // Constructor for the timer to stop/abort processing.
   // The 'timeout' is in seconds.
   // Make sure that 'to' make sense, i.e. not larger than 10 days;
   // the minimum value is 10 ms (0 does not seem to start the timer ...).

   if (gDebug > 0)
      Info ("TStopTimer","enter: %d, timeout: %d", abort, to);

   fPlayer = p;
   fAbort = abort;

   if (gDebug > 1)
      Info ("TStopTimer","timeout set to %s ms", fTime.AsString());
}

//______________________________________________________________________________
Bool_t TStopTimer::Notify()
{
   // Handle the signal coming from the expiration of the timer
   // associated with an abort or stop request.
   // We raise an exception which will be processed in the
   // event loop.

   if (gDebug > 0)
      Info ("Notify","called!");

   if (fAbort)
      Throw(kPEX_ABORTED);
   else
      Throw(kPEX_STOPPED);

   return kTRUE;
}

//------------------------------------------------------------------------------

ClassImp(TProofPlayer)

//______________________________________________________________________________
TProofPlayer::TProofPlayer(TProof *)
   : fAutoBins(0), fOutput(0), fSelector(0), fSelectorClass(0),
     fFeedbackTimer(0), fFeedbackPeriod(2000),
     fEvIter(0), fSelStatus(0), fEventsProcessed(0),
     fTotalEvents(0), fQueryResults(0), fQuery(0), fDrawQueries(0),
     fMaxDrawQueries(1), fStopTimer(0), fStopTimerMtx(0), fDispatchTimer(0)
{
   // Default ctor.

   fInput         = new TList;
   fExitStatus    = kFinished;
   SetProcessing(kFALSE);

   static Bool_t initLimitsFinder = kFALSE;
   if (!initLimitsFinder && gProofServ && !gProofServ->IsMaster()) {
      THLimitsFinder::SetLimitsFinder(new TProofLimitsFinder);
      initLimitsFinder = kTRUE;
   }
}

//______________________________________________________________________________
TProofPlayer::~TProofPlayer()
{
   // Destructor.

   fInput->Clear("nodelete");
   SafeDelete(fInput);
   SafeDelete(fSelector);
   SafeDelete(fFeedbackTimer);
   SafeDelete(fEvIter);
   SafeDelete(fQueryResults);
   SafeDelete(fDispatchTimer);
   SafeDelete(fStopTimer);
}

//______________________________________________________________________________
void TProofPlayer::SetProcessing(Bool_t on)
{
   // Set processing bit according to 'on'

   if (on)
      SetBit(TProofPlayer::kIsProcessing);
   else
      ResetBit(TProofPlayer::kIsProcessing);
}

//______________________________________________________________________________
void TProofPlayer::StopProcess(Bool_t abort, Int_t timeout)
{
   // Stop the process after this event. If timeout is positive, start
   // a timer firing after timeout seconds to hard-stop time-expensive
   // events.

   if (gDebug > 0)
      Info ("StopProcess","abort: %d, timeout: %d", abort, timeout);

   if (fEvIter != 0)
      fEvIter->StopProcess(abort);
   Long_t to = 1;
   if (abort == kTRUE) {
      fExitStatus = kAborted;
   } else {
      fExitStatus = kStopped;
      to = timeout;
   }
   // Start countdown, if needed
   if (to > 0)
      SetStopTimer(kTRUE, abort, to);
}

//______________________________________________________________________________
void TProofPlayer::SetDispatchTimer(Bool_t on)
{
   // Enable/disable the timer to dispatch pening events while processing.

   SafeDelete(fDispatchTimer);
   ResetBit(TProofPlayer::kDispatchOneEvent);
   if (on) {
      fDispatchTimer = new TDispatchTimer(this);
      fDispatchTimer->Start();
   }
}

//______________________________________________________________________________
void TProofPlayer::SetStopTimer(Bool_t on, Bool_t abort, Int_t timeout)
{
   // Enable/disable the timer to stop/abort processing.
   // The 'timeout' is in seconds.

   fStopTimerMtx = (fStopTimerMtx) ? fStopTimerMtx : new TMutex(kTRUE);
   R__LOCKGUARD(fStopTimerMtx);

   // Clean-up the timer
   SafeDelete(fStopTimer);
   if (on) {
      // create timer
      fStopTimer = new TStopTimer(this, abort, timeout);
      // Start the countdown
      fStopTimer->Start();
      if (gDebug > 0)
         Info ("SetStopTimer", "%s timer STARTED (timeout: %d)",
                               (abort ? "ABORT" : "STOP"), timeout);
   } else {
      if (gDebug > 0)
         Info ("SetStopTimer", "timer STOPPED");
   }
}

//______________________________________________________________________________
void TProofPlayer::AddQueryResult(TQueryResult *q)
{
   // Add query result to the list, making sure that there are no
   // duplicates.

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
   // the list of results.

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
   // the list of results.

   if (fQueryResults) {
      if (ref && strlen(ref) > 0) {
         TIter nxq(fQueryResults);
         TQueryResult *qr = 0;
         while ((qr = (TQueryResult *) nxq())) {
            if (qr->Matches(ref))
               return qr;
         }
      } else {
         // Get last
         return (TQueryResult *) fQueryResults->Last();
      }
   }

   // Nothing found
   return (TQueryResult *)0;
}

//______________________________________________________________________________
void TProofPlayer::SetCurrentQuery(TQueryResult *q)
{
   // Set current query and save previous value.

   fPreviousQuery = fQuery;
   fQuery = q;
}

//______________________________________________________________________________
void TProofPlayer::AddInput(TObject *inp)
{
   // Add object to input list.

   fInput->Add(inp);
}

//______________________________________________________________________________
void TProofPlayer::ClearInput()
{
   // Clear input list.

   fInput->Clear();
}

//______________________________________________________________________________
TObject *TProofPlayer::GetOutput(const char *name) const
{
   // Get output object by name.

   if (fOutput)
      return fOutput->FindObject(name);
   return 0;
}

//______________________________________________________________________________
TList *TProofPlayer::GetOutputList() const
{
   // Get output list.

   TList *ol = fOutput;
   if (!ol && fQuery)
      ol = fQuery->GetOutputList();
   return ol;
}

//______________________________________________________________________________
Int_t TProofPlayer::ReinitSelector(TQueryResult *qr)
{
   // Reinitialize fSelector using the selector files in the query result.
   // Needed when Finalize is called after a Process execution for the same
   // selector name.

   Int_t rc = 0;

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

   // Find out if this is a precompiled selector: in such a case we do not
   // have the code in TMacros, so we must rely on local libraries
   Bool_t compselec = (selec.Contains(".") || stdselec) ? kFALSE : kTRUE;

   // If not, find out if it needs to be expanded
   TString ipathold;
   if (!stdselec && !compselec) {
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
   Int_t iglevelsave = gErrorIgnoreLevel;
   if (compselec)
      // Silent error printout on first attempt
      gErrorIgnoreLevel = kBreak;

   if ((fSelector = TSelector::GetSelector(selec))) {
      if (compselec)
         gErrorIgnoreLevel = iglevelsave; // restore ignore level
      fSelectorClass = fSelector->IsA();
      fSelector->SetOption(qr->GetOptions());

   } else {
      if (compselec) {
         gErrorIgnoreLevel = iglevelsave; // restore ignore level
         // Retry by loading first the libraries listed in TQueryResult, if any
         if (strlen(qr->GetLibList()) > 0) {
            TString sl(qr->GetLibList());
            TObjArray *oa = sl.Tokenize(" ");
            if (oa) {
               Bool_t retry = kFALSE;
               TIter nxl(oa);
               TObjString *os = 0;
               while ((os = (TObjString *) nxl())) {
                  TString lib = gSystem->BaseName(os->GetName());
                  if (lib != "lib") {
                     lib.ReplaceAll("-l", "lib");
                     if (gSystem->Load(lib) == 0)
                        retry = kTRUE;
                  }
               }
               // Retry now, if the case
               if (retry)
                  fSelector = TSelector::GetSelector(selec);
            }
         }
      }
      if (!fSelector) {
         if (compselec)
            Info("ReinitSelector", "compiled selector re-init failed:"
                                   " automatic reload unsuccessful:"
                                   " please load manually the correct library");
         rc = -1;
      }
   }
   if (fSelector) {
      // Draw needs to reinit temp histos
      fSelector->SetInputList(qr->GetInputList());
      if (stdselec) {
         ((TProofDraw *)fSelector)->DefVar();
      } else {
         // variables may have been initialized in Begin()
         fSelector->Begin(0);
      }
   }

   // Restore original include path, if needed
   if (ipathold.Length() > 0)
      gSystem->SetIncludePath(ipathold.Data());

   return rc;
}

//______________________________________________________________________________
Int_t TProofPlayer::AddOutputObject(TObject *)
{
   // Incorporate output object (may not be used in this class).

   MayNotUse("AddOutputObject");
   return -1;
}

//______________________________________________________________________________
void TProofPlayer::AddOutput(TList *)
{
   // Incorporate output list (may not be used in this class).

   MayNotUse("AddOutput");
}

//______________________________________________________________________________
void TProofPlayer::StoreOutput(TList *)
{
   // Store output list (may not be used in this class).

   MayNotUse("StoreOutput");
}

//______________________________________________________________________________
void TProofPlayer::StoreFeedback(TObject *, TList *)
{
   // Store feedback list (may not be used in this class).

   MayNotUse("StoreFeedback");
}

//______________________________________________________________________________
void TProofPlayer::Progress(Long64_t /*total*/, Long64_t /*processed*/)
{
   // Report progress (may not be used in this class).

   MayNotUse("Progress");
}

//______________________________________________________________________________
void TProofPlayer::Progress(Long64_t /*total*/, Long64_t /*processed*/,
                            Long64_t /*bytesread*/,
                            Float_t /*evtRate*/, Float_t /*mbRate*/,
                            Float_t /*evtrti*/, Float_t /*mbrti*/)
{
   // Report progress (may not be used in this class).

   MayNotUse("Progress");
}

//______________________________________________________________________________
void TProofPlayer::Feedback(TList *)
{
   // Set feedback list (may not be used in this class).

   MayNotUse("Feedback");
}

//______________________________________________________________________________
TDrawFeedback *TProofPlayer::CreateDrawFeedback(TProof *p)
{
   // Draw feedback creation proxy. When accessed via TProof avoids
   // link dependency on libProofPlayer.

   return new TDrawFeedback(p);
}

//______________________________________________________________________________
void TProofPlayer::SetDrawFeedbackOption(TDrawFeedback *f, Option_t *opt)
{
   // Set draw feedback option.

   if (f)
      f->SetOption(opt);
}

//______________________________________________________________________________
void TProofPlayer::DeleteDrawFeedback(TDrawFeedback *f)
{
   // Delete draw feedback object.

   delete f;
}

//______________________________________________________________________________
Long64_t TProofPlayer::Process(TDSet *dset, const char *selector_file,
                               Option_t *option, Long64_t nentries,
                               Long64_t first)
{
   // Process specified TDSet on PROOF worker.
   // The return value is -1 in case of error and TSelector::GetStatus()
   // in case of success.

   PDB(kGlobal,1) Info("Process","Enter");

   fExitStatus = kFinished;
   fOutput = 0;

   SafeDelete(fSelector);
   fSelectorClass = 0;
   Int_t version = -1;
   TRY {
      if (!(fSelector = TSelector::GetSelector(selector_file))) {
         Error("Process", "cannot load: %s", selector_file );
         return -1;
      }

      // Save binaries to cache, if any
      gProofServ->CopyToCache(selector_file, 1);

      fSelectorClass = fSelector->IsA();
      version = fSelector->Version();

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

   } CATCH(excode) {
      SetProcessing(kFALSE);
      Error("Process","exception %d caught", excode);
      return -1;
   } ENDTRY;

   if (gMonitoringWriter)
      gMonitoringWriter->SendProcessingStatus("STARTED",kTRUE);

   PDB(kLoop,1)
      Info("Process","Looping over Process()");

   // get the byte read counter at the beginning of processing
   Long64_t readbytesatstart = 0;
   readbytesatstart = TFile::GetFileBytesRead();
   // force the first monitoring info
   if (gMonitoringWriter)
      gMonitoringWriter->SendProcessingProgress(0,0,kTRUE);

   // Start asynchronous timer to dispatch pending events
   SetDispatchTimer(kTRUE);

   // Loop over range
   gAbort = kFALSE;
   Long64_t entry;
   fEventsProcessed = 0;

   // Signal the master that we start processing
   gProofServ->GetSocket()->Send(kPROOF_ENDINIT);

   TRY {

      while ((entry = fEvIter->GetNextEvent()) >= 0 && fSelStatus->IsOk()) {

         // This is needed by the inflate infrastructure to calculate
         // sleeping times
         SetProcessing(kTRUE);

         if (version == 0) {
            PDB(kLoop,3)
               Info("Process","Call ProcessCut(%lld)", entry);
            if (fSelector->ProcessCut(entry)) {
               PDB(kLoop,3)
                  Info("Process","Call ProcessFill(%lld)", entry);
               fSelector->ProcessFill(entry);
            }
         } else {
            PDB(kLoop,3)
               Info("Process","Call Process(%lld)", entry);
            fSelector->Process(entry);
            if (fSelector->GetAbort() == TSelector::kAbortProcess) break;
         }

         if (fSelStatus->IsOk()) {
            fEventsProcessed++;
            if (gMonitoringWriter)
               gMonitoringWriter->SendProcessingProgress(fEventsProcessed,
                       TFile::GetFileBytesRead()-readbytesatstart, kFALSE);
         }

         if (TestBit(TProofPlayer::kDispatchOneEvent)) {
            gSystem->DispatchOneEvent(kTRUE);
            ResetBit(TProofPlayer::kDispatchOneEvent);
         }
         if (!fSelStatus->IsOk() || gROOT->IsInterrupted()) break;
         SetProcessing(kFALSE);
      }

   } CATCH(excode) {
      if (excode == kPEX_STOPPED) {
         Info("Process","received stop-process signal");
         fExitStatus = kStopped;
      } else if (excode == kPEX_ABORTED) {
         gAbort = kTRUE;
         Info("Process","received abort-process signal");
         fExitStatus = kAborted;
      } else {
         Error("Process","exception %d caught", excode);
         // Perhaps we need a dedicated status code here ...
         gAbort = kTRUE;
         fExitStatus = kAborted;
      }
      SetProcessing(kFALSE);
   } ENDTRY;

   PDB(kGlobal,2)
      Info("Process","%lld events processed",fEventsProcessed);

   if (gMonitoringWriter) {
      gMonitoringWriter->SendProcessingProgress(fEventsProcessed,TFile::GetFileBytesRead()-readbytesatstart, kFALSE);
      gMonitoringWriter->SendProcessingStatus("DONE");
   }

   // Stop active timers
   SetDispatchTimer(kFALSE);
   if (fStopTimer != 0)
      SetStopTimer(kFALSE, gAbort);
   if (fFeedbackTimer != 0)
      HandleTimer(0);

   StopFeedback();

   SafeDelete(fEvIter);

   // Finalize

   if (fExitStatus != kAborted) {

      TIter nxo(GetOutputList());
      TObject *o = 0;
      while ((o = nxo())) {
         // Special treatment for files
         if (o->IsA() == TProofFile::Class()) {
            ((TProofFile *)o)->SetWorkerOrdinal(gProofServ->GetOrdinal());
            if (!strcmp(((TProofFile *)o)->GetDir(),""))
               ((TProofFile *)o)->SetDir(gProofServ->GetSessionDir());
         }
      }

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
         while (TObject *o = next())
            fOutput->Add(o);
      }
   }

   TPerfStats::Stop();

   return 0;
}

//______________________________________________________________________________
Long64_t TProofPlayer::Finalize(Bool_t, Bool_t)
{
   // Finalize query (may not be used in this class).

   MayNotUse("Finalize");
   return -1;
}

//______________________________________________________________________________
Long64_t TProofPlayer::Finalize(TQueryResult *)
{
   // Finalize query (may not be used in this class).

   MayNotUse("Finalize");
   return -1;
}

//______________________________________________________________________________
void TProofPlayer::UpdateAutoBin(const char *name,
                                 Double_t& xmin, Double_t& xmax,
                                 Double_t& ymin, Double_t& ymax,
                                 Double_t& zmin, Double_t& zmax)
{
   // Update automatic binning parameters for given object "name".

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
   // Get next packet (may not be used in this class).

   MayNotUse("GetNextPacket");
   return 0;
}

//______________________________________________________________________________
void TProofPlayer::SetupFeedback()
{
   // Set up feedback (may not be used in this class).

   MayNotUse("SetupFeedback");
}

//______________________________________________________________________________
void TProofPlayer::StopFeedback()
{
   // Stop feedback (may not be used in this class).

   MayNotUse("StopFeedback");
}

//______________________________________________________________________________
Long64_t TProofPlayer::DrawSelect(TDSet * /*set*/, const char * /*varexp*/,
                                  const char * /*selection*/, Option_t * /*option*/,
                                  Long64_t /*nentries*/, Long64_t /*firstentry*/)
{
   // Draw (may not be used in this class).

   MayNotUse("DrawSelect");
   return -1;
}

//______________________________________________________________________________
void TProofPlayer::HandleGetTreeHeader(TMessage *)
{
   // Handle tree header request.

   MayNotUse("HandleGetTreeHeader|");
}

//______________________________________________________________________________
void TProofPlayer::HandleRecvHisto(TMessage *mess)
{
   // Receive histo from slave.

   TObject *obj = mess->ReadObject(mess->GetClass());
   if (obj->InheritsFrom(TH1::Class())) {
      TH1 *h = (TH1*)obj;
      h->SetDirectory(0);
      TH1 *horg = (TH1*)gDirectory->GetList()->FindObject(h->GetName());
      if (horg)
         horg->Add(h);
      else
         h->SetDirectory(gDirectory);
   }
}

//______________________________________________________________________________
Int_t TProofPlayer::DrawCanvas(TObject *obj)
{
   // Draw the object if it is a canvas.
   // Return 0 in case of success, 1 if it is not a canvas or libProofDraw
   // is not available.

   static Int_t (*gDrawCanvasHook)(TObject *) = 0;

   // Load the library the first time
   if (!gDrawCanvasHook) {
      // Load library needed for graphics ...
      TString drawlib = "libProofDraw";
      char *p = 0;
      if ((p = gSystem->DynamicPathName(drawlib, kTRUE))) {
         delete[] p;
         if (gSystem->Load(drawlib) != -1) {
            // Locate DrawCanvas
            Func_t f = 0;
            if ((f = gSystem->DynFindSymbol(drawlib,"DrawCanvas")))
               gDrawCanvasHook = (Int_t (*)(TObject *))(f);
            else
               Warning("DrawCanvas", "can't find DrawCanvas");
         } else
            Warning("DrawCanvas", "can't load %s", drawlib.Data());
      } else
         Warning("DrawCanvas", "can't locate %s", drawlib.Data());
   }
   if (gDrawCanvasHook && obj)
      return (*gDrawCanvasHook)(obj);
   // No drawing hook or object undefined
   return 1;
}

//______________________________________________________________________________
Int_t TProofPlayer::GetDrawArgs(const char *var, const char *sel, Option_t *opt,
                                TString &selector, TString &objname)
{
   // Parse the arguments from var, sel and opt and fill the selector and
   // object name accordingly.
   // Return 0 in case of success, 1 if libProofDraw is not available.

   static Int_t (*gGetDrawArgsHook)(const char *, const char *, Option_t *,
                                    TString &, TString &) = 0;

   // Load the library the first time
   if (!gGetDrawArgsHook) {
      // Load library needed for graphics ...
      TString drawlib = "libProofDraw";
      char *p = 0;
      if ((p = gSystem->DynamicPathName(drawlib, kTRUE))) {
         delete[] p;
         if (gSystem->Load(drawlib) != -1) {
            // Locate GetDrawArgs
            Func_t f = 0;
            if ((f = gSystem->DynFindSymbol(drawlib,"GetDrawArgs")))
               gGetDrawArgsHook = (Int_t (*)(const char *, const char *, Option_t *,
                                             TString &, TString &))(f);
            else
               Warning("GetDrawArgs", "can't find GetDrawArgs");
         } else
            Warning("GetDrawArgs", "can't load %s", drawlib.Data());
      } else
         Warning("GetDrawArgs", "can't locate %s", drawlib.Data());
   }
   if (gGetDrawArgsHook)
      return (*gGetDrawArgsHook)(var, sel, opt, selector, objname);
   // No parser hook or object undefined
   return 1;
}

//------------------------------------------------------------------------------

ClassImp(TProofPlayerLocal)


//------------------------------------------------------------------------------

ClassImp(TProofPlayerRemote)


//______________________________________________________________________________
TProofPlayerRemote::~TProofPlayerRemote()
{
   // Destructor.

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
                                     Long64_t first)
{
   // Process specified TDSet on PROOF.
   // This method is called on client and on the PROOF master.
   // The return value is -1 in case of an error and TSelector::GetStatus() in
   // in case of success.

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
   Bool_t noData = kFALSE;

   TDSet *set = dset;
   if (fProof->IsMaster()) {

      PDB(kPacketizer,1) Info("Process","Create Proxy TDSet");
      set = new TDSetProxy( dset->GetType(), dset->GetObjName(),
                            dset->GetDirectory() );
      TString packetizer;
      TMethodCall callEnv;
      TClass *cl;
      noData = dset->TestBit(TDSet::kEmpty) ? kTRUE : kFALSE;
      TList *listOfMissingFiles = 0;

      if (noData) {

         set->SetBit(TDSet::kEmpty);
         if (TProof::GetParameter(fInput, "PROOF_Packetizer", packetizer) != 0)
            packetizer = "TPacketizerUnit";
         else
            Info("Process", "using alternate packetizer: %s", packetizer.Data());

         // Get linked to the related class
         cl = TClass::GetClass(packetizer);
         if (cl == 0) {
            Error("Process", "class '%s' not found", packetizer.Data());
            fExitStatus = kAborted;
            return -1;
         }

         // Init the constructor
         callEnv.InitWithPrototype(cl, cl->GetName(),"TList*,Long64_t,TList*");
         if (!callEnv.IsValid()) {
            Error("Process", "cannot find correct constructor for '%s'", cl->GetName());
            fExitStatus = kAborted;
            return -1;
         }
         callEnv.ResetParam();
         callEnv.SetParam((Long_t) fProof->GetListOfActiveSlaves());
         callEnv.SetParam((Long64_t) nentries);
         callEnv.SetParam((Long_t) fInput);

      } else {

         // Lookup - resolve the end-point urls to optmize the distribution.
         // The lookup was previously called in the packetizer's constructor.
         // A list for the missing files may already have been added to the
         // output list; otherwise, if needed it will be created inside
         if ((listOfMissingFiles = (TList *)fInput->FindObject("MissingFiles"))) {
            // Move it to the output list
            fInput->Remove(listOfMissingFiles);
         } else {
            listOfMissingFiles = new TList;
         }
         dset->Lookup(kTRUE, &listOfMissingFiles);
         if (fProof->GetRunStatus() != TProof::kRunning) {
            // We have been asked to stop
            Error("Process", "received stop/abort request");
            fExitStatus = kAborted;
            return -1;
         }
         if (!(dset->GetListOfElements()) ||
             !(dset->GetListOfElements()->GetSize())) {
            gProofServ->SendAsynMessage("Process: No files from the data set were found - Aborting");
            Error("Process", "No files from the data set were found - Aborting");
            fExitStatus = kAborted;
            if (listOfMissingFiles) {
               listOfMissingFiles->SetOwner();
               fOutput->Remove(listOfMissingFiles);
               SafeDelete(listOfMissingFiles);
            }
            return -1;
         }

         if (TProof::GetParameter(fInput, "PROOF_Packetizer", packetizer) != 0)
            // Using standard packetizer TAdaptivePacketizer
            packetizer = "TPacketizerAdaptive";
         else
            Info("Process", "using alternate packetizer: %s", packetizer.Data());

         // Get linked to the related class
         cl = TClass::GetClass(packetizer);
         if (cl == 0) {
            Error("Process", "class '%s' not found", packetizer.Data());
            fExitStatus = kAborted;
            return -1;
         }

         // Init the constructor
         callEnv.InitWithPrototype(cl, cl->GetName(),"TDSet*,TList*,Long64_t,Long64_t,TList*");
         if (!callEnv.IsValid()) {
            Error("Process", "cannot find correct constructor for '%s'", cl->GetName());
            fExitStatus = kAborted;
            return -1;
         }
         callEnv.ResetParam();
         callEnv.SetParam((Long_t) dset);
         callEnv.SetParam((Long_t) fProof->GetListOfActiveSlaves());
         callEnv.SetParam((Long64_t) first);
         callEnv.SetParam((Long64_t) nentries);
         callEnv.SetParam((Long_t) fInput);

         // We are going to test validity during the packetizer initialization
         dset->SetBit(TDSet::kValidityChecked);
         dset->ResetBit(TDSet::kSomeInvalid);
      }

      // Get an instance of the packetizer
      Long_t ret = 0;
      callEnv.Execute(ret);
      if ((fPacketizer = (TVirtualPacketizer *)ret) == 0) {
         Error("Process", "cannot construct '%s'", cl->GetName());
         fExitStatus = kAborted;
         return -1;
      }

      if (!fPacketizer->IsValid()) {
         Error("Process", "instantiated packetizer object '%s' is invalid", cl->GetName());
         fExitStatus = kAborted;
         return -1;
      }
      // Add invalid elemnets to the list of missing elements
      TDSetElement *elem = 0;
      if (!noData && dset->TestBit(TDSet::kSomeInvalid)) {
         TIter nxe(dset->GetListOfElements());
         while ((elem = (TDSetElement *)nxe())) {
            if (!elem->GetValid()) {
               if (!listOfMissingFiles)
                  listOfMissingFiles = new TList;
               listOfMissingFiles->Add(elem->GetFileInfo(dset->GetType()));
               dset->Remove(elem, kFALSE);
            }
         }
         // The invalid elements have been removed
         dset->ResetBit(TDSet::kSomeInvalid);
      }

      // Record the list of missing or invalid elements in the output list
      if (listOfMissingFiles && listOfMissingFiles->GetSize() > 0) {
         TIter missingFiles(listOfMissingFiles);
         TFileInfo *fi = 0;
         while ((fi = (TFileInfo *) missingFiles.Next())) {
            TString msg;
            if (fi->GetCurrentUrl()) {
               msg = Form("File not found: %s - skipping!",
                                             fi->GetCurrentUrl()->GetUrl());
            } else {
               msg = Form("File not found: %s - skipping!", fi->GetName());
            }
            gProofServ->SendAsynMessage(msg.Data());
         }
         // Make sure it will be sent back
         if (!GetOutput("MissingFiles")) {
            listOfMissingFiles->SetName("MissingFiles");
            AddOutputObject(listOfMissingFiles);
         }
         TStatus *tmpStatus = (TStatus *)GetOutput("PROOF_Status");
         if (!tmpStatus) {
            tmpStatus = new TStatus();
            AddOutputObject(tmpStatus);
         }
         tmpStatus->Add("Some files were missing; check the 'MissingFiles'"
                        " list in the output list");
      } else {
         // Cleanup
         SafeDelete(listOfMissingFiles);
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

   // Old servers need a dedicated streamer
   if (fProof->fProtocol < 13)
      dset->SetWriteV3(kTRUE);

   // Workers will get the entry ranges from the packetizer
   Long64_t num = (gProofServ && gProofServ->IsMaster() && gProofServ->IsParallel()) ? -1 : nentries;
   Long64_t fst = (gProofServ && gProofServ->IsMaster() && gProofServ->IsParallel()) ? -1 : first;

   // Entry- or Event- list ?
   TEntryList *enl = (!fProof->IsMaster()) ? dynamic_cast<TEntryList *>(set->GetEntryList())
                                           : (TEntryList *)0;
   TEventList *evl = (!fProof->IsMaster() && !enl) ? dynamic_cast<TEventList *>(set->GetEntryList())
                                           : (TEventList *)0;
   if (fProof->fProtocol > 14) {
      mesg << set << fn << fInput << opt << num << fst << evl << sync << enl;
   } else {
      mesg << set << fn << fInput << opt << num << fst << evl << sync;
      if (enl)
         // Not supported remotely
         Warning("Process","entry lists not supported by the server");
   }

   PDB(kGlobal,1) Info("Process","Calling Broadcast");
   fProof->Broadcast(mesg);

   // Reset streamer choice
   if (fProof->fProtocol < 13)
      dset->SetWriteV3(kFALSE);

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

      if (!IsClient()) {
         HandleTimer(0); // force an update of final result
         // Store process info
         if (fPacketizer && fQuery)
            fQuery->SetProcessInfo(0, 0., fPacketizer->GetBytesRead(),
                                          fPacketizer->GetInitTime(),
                                          fPacketizer->GetProcTime());
      }
      StopFeedback();

      if (!IsClient() || GetExitStatus() != TProofPlayer::kAborted)
         return Finalize(kFALSE,sync);
      else
         return -1;
   }
}

//______________________________________________________________________________
Bool_t  TProofPlayerRemote::MergeOutputFiles()
{
   // Merge output in files

   if (fMergeFiles) {
      TFileMerger *filemerger = TProofFile::GetFileMerger();
      if (!filemerger) {
         Error("MergeOutputFiles", "file merger is null in gProofServ! Protocol error?");
         return kFALSE;
      }

      Bool_t result = filemerger->Merge();
      if (!result) {
         Error("MergeOutputFiles", "cannot merge the output files");
         return kFALSE;
      }

      TList *fileList = filemerger->GetMergeList();
      if (fileList) {
         TIter next(fileList);
         TObjString *url = 0;
         while((url = (TObjString*)next())) {
            gSystem->Unlink(url->GetString());
         }
      }
      filemerger->Reset();
   }
   // Done
   return kTRUE;
}


//______________________________________________________________________________
Long64_t TProofPlayerRemote::Finalize(Bool_t force, Bool_t sync)
{
   // Finalize a query.
   // Returns -1 in case of an error, 0 otherwise.

   if (IsClient()) {
      if (fOutputLists == 0) {
         if (force)
            if (fQuery)
               return fProof->Finalize(Form("%s:%s", fQuery->GetTitle(),
                                                     fQuery->GetName()), force);
      } else {
         if (fProof->fProtocol < 11) {
            PDB(kGlobal,1) Info("Finalize","Calling Merge Output");
            MergeOutput();
         }
      }
   }

   Long64_t rv = 0;
   if (fProof->IsMaster()) {
      TPerfStats::Stop();

      // Merge the output files created on workers, if any
      MergeOutputFiles();

      fOutput->SetOwner();
      SafeDelete(fSelector);
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

         if (fPacketizer)
            if (TList *failedPackets = fPacketizer->GetFailedPackets()) {
               fPacketizer->SetFailedPackets(0);
               failedPackets->SetName("FailedPackets");
               AddOutputObject(failedPackets);

               TStatus *status = (TStatus *)GetOutput("PROOF_Status");
               if (!status) {
                  status = new TStatus();
                  AddOutputObject(status);
               }
               status->Add("Some packets were not processed! Check the the"
                           " 'FailedPackets' list in the output list");
            }

         // Some input parameters may be needed in Terminate
         fSelector->SetInputList(fInput);

         TIter next(fOutput);
         TList *output = fSelector->GetOutputList();
         while(TObject* obj = next()) {
            if (fProof->IsParallel() || DrawCanvas(obj) == 1)
               // Either parallel or not a canvas or not able to display it:
               // just add to the list
               output->Add(obj);
         }

         PDB(kLoop,1) Info("Finalize","Call Terminate()");
         fOutput->Clear("nodelete");
         fSelector->Terminate();

         rv = fSelector->GetStatus();

         // copy the output list back and clean the selector's list
         TIter it(output);
         while(TObject* o = it()) {
            fOutput->Add(o);
         }

         // Save the output list in the current query, if any
         if (fQuery) {
            fQuery->SetOutputList(fOutput);
            // Set in finalized state (cannot be done twice)
            fQuery->SetFinalized();
         } else {
            Warning("Finalize","current TQueryResult object is undefined!");
         }

         // We have transferred copy of the output objects in TQueryResult,
         // so now we can cleanup the selector, making sure that we do not
         // touch the output objects
         output->SetOwner(kFALSE);
         SafeDelete(fSelector);

         // Delete fOutput (not needed anymore, cannot be finalized twice),
         // making sure that the objects saved in TQueryResult are not deleted
         fOutput->SetOwner(kFALSE);
         SafeDelete(fOutput);
      }
   }
   PDB(kGlobal,1) Info("Process","exit");
   return rv;
}

//______________________________________________________________________________
Long64_t TProofPlayerRemote::Finalize(TQueryResult *qr)
{
   // Finalize the results of a query already processed.

   PDB(kGlobal,1) Info("Finalize(TQueryResult *)","Enter");

   if (!IsClient()) {
      Info("Finalize(TQueryResult *)",
           "method to be executed only on the clients");
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
   TList *out = fOutput;
   if (fProof->fProtocol < 11)
      out = new TList;
   TIter nxo(tmp);
   TObject *o = 0;
   while ((o = nxo()))
      out->Add(o->Clone());

   // Adopts the list
   if (fProof->fProtocol < 11) {
      out->SetOwner();
      StoreOutput(out);
   }
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
   // Send the selector file(s) to master or worker nodes.

   // Check input
   if (!selector_file) {
      Info("SendSelector", "Invalid input: selector (file) name undefined");
      return kFALSE;
   }

   if (!strchr(gSystem->BaseName(selector_file), '.')) {
      if (gDebug > 1)
         Info("SendSelector", "selector name '%s' does not contain a '.':"
              " nothing to send, it will be loaded from a library", selector_file);
      return kTRUE;
   }

   // Supported extensions for the implementation file
   const char *cext[3] = { ".C", ".cxx", ".cc" };
   Int_t e = 0;
   for ( ; e < 3; e++)
      if (strstr(selector_file, cext[e]))
         break;
   if (e >= 3) {
      Info("SendSelector",
           "Invalid extension: %s (supportd extensions: .C, .cxx, .cc", selector_file);
      return kFALSE;
   }
   Int_t l = strlen(cext[e]);

   // Extract the fine name first
   TString selec = selector_file;
   TString aclicMode;
   TString arguments;
   TString io;
   selec = gSystem->SplitAclicMode(selec, aclicMode, arguments, io);

   // Expand possible envs or '~'
   gSystem->ExpandPathName(selec);

   // Header file
   TString header = selec;
   header.Replace(header.Length()-l, l,".h");
   if (gSystem->AccessPathName(header, kReadPermission)) {
      TString h = header;
      header = selec;
      header.Replace(header.Length()-l, l,".hh");
      if (gSystem->AccessPathName(header, kReadPermission)) {
         Info("SendSelector",
              "header file not found: tried: %s %s", h.Data(), header.Data());
         return kFALSE;
      }
   }

   // Send files now
   if (fProof->SendFile(selec) == -1) {
      Info("SendSelector", "problems sending implementation file %s", selec.Data());
      return kFALSE;
   }
   if (fProof->SendFile(header) == -1) {
      Info("SendSelector", "problems sending header file %s", header.Data());
      return kFALSE;
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
   // Progress signal.

   if (IsClient()) {
      fProof->Progress(total, processed);
   } else {
      // Send to the previous tier
      TMessage m(kPROOF_PROGRESS);
      m << total << processed;
      gProofServ->GetSocket()->Send(m);
   }
}

//______________________________________________________________________________
void TProofPlayerRemote::Progress(Long64_t total, Long64_t processed,
                                  Long64_t bytesread,
                                  Float_t initTime, Float_t procTime,
                                  Float_t evtrti, Float_t mbrti)
{
   // Progress signal.

   PDB(kGlobal,1)
      Info("Progress","%lld %lld %lld %f %f %f %f", total, processed, bytesread,
                                             initTime, procTime, evtrti, mbrti);

   if (IsClient()) {
      fProof->Progress(total, processed, bytesread, initTime, procTime, evtrti, mbrti);
   } else {
      // Send to the previous tier
      TMessage m(kPROOF_PROGRESS);
      m << total << processed << bytesread << initTime << procTime << evtrti << mbrti;
      gProofServ->GetSocket()->Send(m);
   }
}

//______________________________________________________________________________
void TProofPlayerRemote::Feedback(TList *objs)
{
   // Feedback signal.

   fProof->Feedback(objs);
}

//______________________________________________________________________________
void TProofPlayerRemote::StopProcess(Bool_t abort, Int_t)
{
   // Stop process after this event.

   if (fPacketizer != 0)
      fPacketizer->StopProcess(abort);
   if (abort == kTRUE)
      fExitStatus = kAborted;
   else
      fExitStatus = kStopped;
}

//______________________________________________________________________________
Int_t TProofPlayerRemote::AddOutputObject(TObject *obj)
{
   // Incorporate the received object 'obj' into the output list fOutput.
   // The latter is created if not existing.
   // This method short cuts 'StoreOutput + MergeOutput' optimizing the memory
   // consumption.
   // Returns -1 in case of error, 1 if the object has been merged into another
   // one (so that its ownership has not been taken and can be deleted), and 0
   // otherwise.

   PDB(kOutput,1) Info("AddOutputObject","Enter");

   // We must something to process
   if (!obj) {
      PDB(kOutput,1) Info("AddOutputObject","Invalid input (obj == 0x0)");
      return -1;
   }

   // Create the output list, if not yet done
   if (!fOutput)
      fOutput = new TList;

   // Flag about merging
   Bool_t merged = kTRUE;

   // Process event lists first
   TList *elists = dynamic_cast<TList *> (obj);
   if (elists && !strcmp(elists->GetName(), "PROOF_EventListsList")) {

      // Create a global event list, result of merging the event lists
      // coresponding to the various data set elements
      TEventList *evlist = new TEventList("PROOF_EventList");

      // Iterate the list of event list segments
      TIter nxevl(elists);
      TEventList *evl = 0;
      while ((evl = dynamic_cast<TEventList *> (nxevl()))) {

         // Find the file offset (fDSet is the current TDSet instance)
         // locating the element by name
         TIter nxelem(fDSet->GetListOfElements());
         TDSetElement *elem = 0;
         while ((elem = dynamic_cast<TDSetElement *> (nxelem()))) {
            if (!strcmp(elem->GetFileName(), evl->GetName()))
               break;
         }
         if (!elem) {
            Error("AddOutputObject", "Found an event list for %s, but no object with"
                                     " the same name in the TDSet", evl->GetName());
            continue;
         }
         Long64_t offset = elem->GetTDSetOffset();

         // Shift the list by the number of first event in that file
         Long64_t *arr = evl->GetList();
         Int_t num = evl->GetN();
         if (arr && offset > 0)
            for (Int_t i = 0; i < num; i++)
               arr[i] += offset;

         // Add to the global event list
         evlist->Add(evl);
      }

      // Incorporate the resulting global list in fOutput
      Incorporate(evlist, fOutput, merged);

      // Delete the global list if merged
      if (merged)
         SafeDelete(evlist);

      // The original object has been transformed in something else; we do
      // not have ownership on it
      return 1;
   }

   // Check if we need to merge files
   TProofFile *pf = dynamic_cast<TProofFile*>(obj);
   if (pf) {
      if (!strcmp(pf->GetMode(),"CENTRAL"))
         fMergeFiles = kTRUE;
      if (!IsClient()) {
         // Fill the output file name, if not done by the client
         if (strlen(pf->GetOutputFileName()) <= 0) {
            TString of(Form("root://%s", gSystem->HostName()));
            if (gSystem->Getenv("XRDPORT")) {
               TString sp(gSystem->Getenv("XRDPORT"));
               if (sp.IsDigit())
                  of += Form(":%s", sp.Data());
            }
            TString sessionPath(gProofServ->GetSessionDir());
            // Take into account a prefix, if any
            TString pfx  = gEnv->GetValue("Path.Localroot","");
            if (!pfx.IsNull())
               sessionPath.Remove(0, pfx.Length());
            of += Form("/%s/%s", sessionPath.Data(), pf->GetFileName());
            pf->SetOutputFileName(of);
         }
         // Notify, if required
         if (gDebug > 0)
            pf->Print();
      } else {
         // On clients notify the output path
         Printf("Output file: %s", pf->GetOutputFileName());
      }
   }

   // For other objects we just run the incorporation procedure
   Incorporate(obj, fOutput, merged);

   // We are done
   return (merged ? 1 : 0);
}

//______________________________________________________________________________
void TProofPlayerRemote::AddOutput(TList *out)
{
   // Incorporate the content of the received output list 'out' into the final
   // output list fOutput. The latter is created if not existing.
   // This method short cuts 'StoreOutput + MergeOutput' limiting the memory
   // consumption.

   PDB(kOutput,1) Info("AddOutput","Enter");

   // We must something to process
   if (!out) {
      PDB(kOutput,1) Info("AddOutput","Invalid input (out == 0x0)");
      return;
   }

   // Create the output list, if not yet done
   if (!fOutput)
      fOutput = new TList;

   // Process event lists first
   Bool_t merged = kTRUE;
   TList *elists = dynamic_cast<TList *> (out->FindObject("PROOF_EventListsList"));
   if (elists) {

      // Create a global event list, result of merging the event lists
      // coresponding to the various data set elements
      TEventList *evlist = new TEventList("PROOF_EventList");

      // Iterate the list of event list segments
      TIter nxevl(elists);
      TEventList *evl = 0;
      while ((evl = dynamic_cast<TEventList *> (nxevl()))) {

         // Find the file offset (fDSet is the current TDSet instance)
         // locating the element by name
         TIter nxelem(fDSet->GetListOfElements());
         TDSetElement *elem = 0;
         while ((elem = dynamic_cast<TDSetElement *> (nxelem()))) {
            if (!strcmp(elem->GetFileName(), evl->GetName()))
               break;
         }
         if (!elem) {
            Error("AddOutput", "Found an event list for %s, but no object with"
                               " the same name in the TDSet", evl->GetName());
            continue;
         }
         Long64_t offset = elem->GetTDSetOffset();

         // Shift the list by the number of first event in that file
         Long64_t *arr = evl->GetList();
         Int_t num = evl->GetN();
         if (arr && offset > 0)
            for (Int_t i = 0; i < num; i++)
               arr[i] += offset;

         // Add to the global event list
         evlist->Add(evl);
      }

      // Remove and delete the events lists object to avoid spoiling iteration
      // during next steps
      out->Remove(elists);
      delete elists;

      // Incorporate the resulting global list in fOutput
      Incorporate(evlist, fOutput, merged);
   }

   // Iterate on the remaining objects in the received list
   TIter nxo(out);
   TObject *obj = 0;
   while ((obj = nxo())) {
      Incorporate(obj, fOutput, merged);
      // If not merged, drop from the temporary list, as the ownership
      // passes to fOutput
      if (!merged)
         out->Remove(obj);
   }

   // Done
   return;
}

//______________________________________________________________________________
Int_t TProofPlayerRemote::Incorporate(TObject *newobj, TList *outlist, Bool_t &merged)
{
   // Incorporate object 'newobj' in the list 'outlist'.
   // The object is merged with an object of the same name already existing in
   // the list, or just added.
   // The boolean merged is set to kFALSE when the object is just added to 'outlist';
   // this happens if the Merge() method does not exist or if a object named as 'obj'
   // is not already in the list. If the obj is not 'merged' than it should not be
   // deleted, unless outlist is not owner of its objects.
   // Return 0 on success, -1 on error.

   merged = kTRUE;

   // The object and list must exist
   if (!newobj || !outlist) {
      Error("Incorporate","Invalid inputs: obj: %p, list: %p", newobj, outlist);
      return -1;
   }

   // Check if an object with the same name exists already
   TObject *obj = outlist->FindObject(newobj->GetName());

   // If no, add the new object and return
   if (!obj) {
      outlist->Add(newobj);
      merged = kFALSE;
      // Done
      return 0;
   }

   // Locate the Merge(TCollection *) method
   TMethodCall callEnv;
   if (obj->IsA())
      callEnv.InitWithPrototype(obj->IsA(), "Merge", "TCollection*");
   if (callEnv.IsValid()) {
      // Found: put the object in a one-element list
      static TList *xlist = new TList;
      xlist->Add(newobj);
      // Call the method
      callEnv.SetParam((Long_t) xlist);
      callEnv.Execute(obj);
      // Ready for next call
      xlist->Clear();
   } else {
      // Not found: return individual objects
      outlist->Add(newobj);
      merged = kFALSE;
   }

   // Done
   return 0;
}

//______________________________________________________________________________
void TProofPlayerRemote::StoreOutput(TList *out)
{
   // Store received output list.

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
         Long64_t offset = elem->GetTDSetOffset();

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

   PDB(kFeedback,1)
      Info("MergeFeedback","Enter");

   if ( fFeedbackLists == 0 ) {
      PDB(kFeedback,1)
         Info("MergeFeedback","Leave (no output)");
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

   PDB(kFeedback,1)
      Info("MergeFeedback","Leave (%d object(s))", fb->GetSize());

   return fb;
}

//______________________________________________________________________________
void TProofPlayerRemote::StoreFeedback(TObject *slave, TList *out)
{
   // Store feedback results from the specified slave.

   PDB(kFeedback,1)
      Info("StoreFeedback","Enter");

   if ( out == 0 ) {
      PDB(kFeedback,1)
         Info("StoreFeedback","Leave (empty)");
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
      PDB(kFeedback,2)
         Info("StoreFeedback","Find '%s'", obj->GetName() );

      TMap *map = (TMap*) fFeedbackLists->FindObject(obj->GetName());
      if ( map == 0 ) {
         PDB(kFeedback,2)
            Info("StoreFeedback","Map not Found (creating)", obj->GetName() );
         // map must not be owner (ownership is with regards to the keys (only))
         map = new TMap;
         map->SetName(obj->GetName());
         fFeedbackLists->Add(map);
      } else {
         PDB(kFeedback,2)
            Info("StoreFeedback","removing previous value");
         if (map->GetValue(slave))
            delete map->GetValue(slave);
         map->Remove(slave);
      }
      map->Add(slave, obj);
   }

   delete out;
   PDB(kFeedback,1)
      Info("StoreFeedback","Leave");
}

//______________________________________________________________________________
void TProofPlayerRemote::SetupFeedback()
{
   // Setup reporting of feedback objects.

   if (IsClient()) return; // Client does not need timer

   fFeedback = (TList*) fInput->FindObject("FeedbackList");

   PDB(kFeedback,1) Info("SetupFeedback","\"FeedbackList\" %sfound",
      fFeedback == 0 ? "NOT ":"");

   if (fFeedback == 0 || fFeedback->GetSize() == 0) return;

   // OK, feedback was requested, setup the timer
   SafeDelete(fFeedbackTimer);
   fFeedbackPeriod = 2000;
   TProof::GetParameter(fInput, "PROOF_FeedbackPeriod", fFeedbackPeriod);
   fFeedbackTimer = new TTimer;
   fFeedbackTimer->SetObject(this);
   fFeedbackTimer->Start(fFeedbackPeriod, kTRUE);
}

//______________________________________________________________________________
void TProofPlayerRemote::StopFeedback()
{
   // Stop reporting of feedback objects.

   if (fFeedbackTimer == 0) return;

   PDB(kFeedback,1) Info("StopFeedback","Stop Timer");

   SafeDelete(fFeedbackTimer);
}

//______________________________________________________________________________
Bool_t TProofPlayerRemote::HandleTimer(TTimer *)
{
   // Send feedback objects to client.

   PDB(kFeedback,2) Info("HandleTimer","Entry");

   R__ASSERT(!IsClient());

   if (fFeedbackTimer == 0) return kFALSE; // timer already switched off


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

   if (fFeedbackLists == 0) {
      fFeedbackTimer->Start(fFeedbackPeriod, kTRUE);   // maybe next time
      return kFALSE;
   }

   fb = MergeFeedback();

   PDB(kFeedback,2) Info("HandleTimer","Sending %d objects", fb->GetSize());

   TMessage m(kPROOF_FEEDBACK);
   m << fb;

   // send message to client;
   gProofServ->GetSocket()->Send(m);

   delete fb;

   fFeedbackTimer->Start(fFeedbackPeriod, kTRUE);

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
   // Draw (support for TChain::Draw()).
   // Returns -1 in case of error or number of selected events in case of success.

   TString selector, objname;
   if (GetDrawArgs(varexp, selection, option, selector, objname) != 0) {
      Error("DrawSelect", "parsing arguments");
      return -1;
   }

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

   if (objname == "")
      objname = "htemp";
   fProof->AddFeedback(objname);
   Long64_t r = Process(set, selector, option, nentries, firstentry);
   fProof->RemoveFeedback(objname);

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

//______________________________________________________________________________
void TProofPlayerRemote::SetInitTime()
{
   // Set init time

   if (fPacketizer)
      fPacketizer->SetInitTime();
}

//------------------------------------------------------------------------------


ClassImp(TProofPlayerSlave)

//______________________________________________________________________________
void TProofPlayerSlave::SetupFeedback()
{
   // Setup feedback.

   TList *fb = (TList*) fInput->FindObject("FeedbackList");

   PDB(kFeedback,1) Info("SetupFeedback","\"FeedbackList\" %sfound",
      fb == 0 ? "NOT ":"");

   if (fb == 0 || fb->GetSize() == 0) return;

   // OK, feedback was requested, setup the timer

   SafeDelete(fFeedbackTimer);
   fFeedbackPeriod = 2000;
   TProof::GetParameter(fInput, "PROOF_FeedbackPeriod", fFeedbackPeriod);
   fFeedbackTimer = new TTimer;
   fFeedbackTimer->SetObject(this);
   fFeedbackTimer->Start(fFeedbackPeriod, kTRUE);

   fFeedback = fb;
}

//______________________________________________________________________________
void TProofPlayerSlave::StopFeedback()
{
   // Stop feedback.

   if (fFeedbackTimer == 0) return;

   PDB(kFeedback,1) Info("StopFeedback","Stop Timer");

   SafeDelete(fFeedbackTimer);
}

//______________________________________________________________________________
Bool_t TProofPlayerSlave::HandleTimer(TTimer *)
{
   // Handle timer event.

   PDB(kFeedback,2) Info("HandleTimer","Entry");

   // If in sequential (0-slave-PROOF) mode we do not have a packetizer
   // so we also send the info to update the progress bar.
   if (gProofServ && gProofServ->IsMaster() && !gProofServ->IsParallel()) {
      TMessage m(kPROOF_PROGRESS);
      m << fTotalEvents << fEventsProcessed;
      gProofServ->GetSocket()->Send(m);
   }

   if (fFeedback == 0) return kFALSE;

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

   fFeedbackTimer->Start(fFeedbackPeriod, kTRUE);

   return kFALSE; // ignored?
}

//______________________________________________________________________________
void TProofPlayerSlave::HandleGetTreeHeader(TMessage *mess)
{
   // Handle tree header request.

   TMessage answ(kMESS_OBJECT);

   TDSet *dset;
   (*mess) >> dset;
   dset->Reset();
   TDSetElement *e = dset->Next();
   Long64_t entries = 0;
   TFile *f = 0;
   TTree *t = 0;
   if (!e) {
      PDB(kGlobal, 1) Info("HandleGetTreeHeader", "empty TDSet");
   } else {
      f = TFile::Open(e->GetFileName());
      t = 0;
      if (f) {
         t = (TTree*) f->Get(e->GetObjName());
         if (t) {
            t->SetMaxVirtualSize(0);
            t->DropBaskets();
            entries = t->GetEntries();

            // compute #entries in all the files
            while ((e = dset->Next()) != 0) {
               TFile *f1 = TFile::Open(e->GetFileName());
               if (f1) {
                  TTree *t1 = (TTree*) f1->Get(e->GetObjName());
                  if (t1) {
                     entries += t1->GetEntries();
                     delete t1;
                  }
                  delete f1;
               }
            }
            t->SetMaxEntryLoop(entries);   // this field will hold the total number of entries ;)
         }
      }
   }
   if (t)
      answ << TString("Success") << t;
   else
      answ << TString("Failed") << t;

   fSocket->Send(answ);

   SafeDelete(t);
   SafeDelete(f);
}


//------------------------------------------------------------------------------

ClassImp(TProofPlayerSuperMaster)

//______________________________________________________________________________
Long64_t TProofPlayerSuperMaster::Process(TDSet *dset, const char *selector_file,
                                          Option_t *option, Long64_t nentries,
                                          Long64_t first)
{
   // Process specified TDSet on PROOF. Runs on super master.
   // The return value is -1 in case of error and TSelector::GetStatus() in
   // in case of success.

   fEventsProcessed = 0;
   PDB(kGlobal,1) Info("Process","Enter");

   TProofSuperMaster *proof = dynamic_cast<TProofSuperMaster*>(GetProof());
   if (!proof) return -1;

   delete fOutput;
   fOutput = new TList;

   TPerfStats::Start(fInput, fOutput);

   if (!SendSelector(selector_file)) {
      Error("Process", "sending selector %s", selector_file);
      return -1;
   }

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
               fSlaveProgress[fSlaveProgress.GetSize()-1] = 0;
               fSlaveTotals.Set(fSlaveTotals.GetSize()+1);
               fSlaveTotals[fSlaveTotals.GetSize()-1] = nentries;
               fSlaveBytesRead.Set(fSlaveBytesRead.GetSize()+1);
               fSlaveBytesRead[fSlaveBytesRead.GetSize()-1] = 0;
               fSlaveInitTime.Set(fSlaveInitTime.GetSize()+1);
               fSlaveInitTime[fSlaveInitTime.GetSize()-1] = -1.;
               fSlaveProcTime.Set(fSlaveProcTime.GetSize()+1);
               fSlaveProcTime[fSlaveProcTime.GetSize()-1] = -1.;
               fSlaveEvtRti.Set(fSlaveEvtRti.GetSize()+1);
               fSlaveEvtRti[fSlaveEvtRti.GetSize()-1] = -1.;
               fSlaveMBRti.Set(fSlaveMBRti.GetSize()+1);
               fSlaveMBRti[fSlaveMBRti.GetSize()-1] = -1.;
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
void TProofPlayerSuperMaster::Progress(TSlave *sl, Long64_t total,
                                       Long64_t processed, Long64_t bytesread,
                                       Float_t initTime, Float_t procTime,
                                       Float_t evtrti, Float_t mbrti)
{
   // Report progress.

   Info("Progress","%s: %lld %lld %f %f %f %f", sl->GetName(),
                   processed, bytesread, initTime, procTime, evtrti, mbrti);

   Int_t idx = fSlaves.IndexOf(sl);
   if (fSlaveTotals[idx] != total)
      Warning("Progress", "total events has changed for slave %s", sl->GetName());
   fSlaveTotals[idx] = total;
   fSlaveProgress[idx] = processed;
   fSlaveBytesRead[idx] = bytesread;
   fSlaveInitTime[idx] = (initTime > -1.) ? initTime : fSlaveInitTime[idx];
   fSlaveProcTime[idx] = (procTime > -1.) ? procTime : fSlaveProcTime[idx];
   fSlaveEvtRti[idx] = (evtrti > -1.) ? evtrti : fSlaveEvtRti[idx];
   fSlaveMBRti[idx] = (mbrti > -1.) ? mbrti : fSlaveMBRti[idx];

   Int_t i;
   Long64_t tot = 0;
   Long64_t proc = 0;
   Long64_t bytes = 0;
   Float_t init = -1.;
   Float_t ptime = -1.;
   Float_t erti = 0.;
   Float_t srti = 0.;
   Int_t nerti = 0;
   Int_t nsrti = 0;
   for (i = 0; i < fSlaveTotals.GetSize(); i++) {
      tot += fSlaveTotals[i];
      if (i < fSlaveProgress.GetSize())
         proc += fSlaveProgress[i];
      if (i < fSlaveBytesRead.GetSize())
         bytes += fSlaveBytesRead[i];
      if (i < fSlaveInitTime.GetSize())
         if (fSlaveInitTime[i] > -1. && (init < 0. || fSlaveInitTime[i] < init))
            init = fSlaveInitTime[i];
      if (i < fSlaveProcTime.GetSize())
         if (fSlaveProcTime[i] > -1. && (ptime < 0. || fSlaveProcTime[i] > ptime))
            ptime = fSlaveProcTime[i];
      if (i < fSlaveEvtRti.GetSize())
         if (fSlaveEvtRti[i] > -1.) {
            erti += fSlaveEvtRti[i];
            nerti++;
         }
      if (i < fSlaveMBRti.GetSize())
         if (fSlaveMBRti[i] > -1.) {
            srti += fSlaveMBRti[i];
            nsrti++;
         }
   }
   erti = (nerti > 0) ? erti / nerti : 0.;
   srti = (nsrti > 0) ? srti / nerti : 0.;

   Progress(tot, proc, bytes, init, ptime, erti, srti);
}

//______________________________________________________________________________
Bool_t TProofPlayerSuperMaster::HandleTimer(TTimer *)
{
   // Send progress and feedback to client.

   if (fFeedbackTimer == 0) return kFALSE; // timer stopped already

   Int_t i;
   Long64_t tot = 0;
   Long64_t proc = 0;
   Long64_t bytes = 0;
   Float_t init = -1.;
   Float_t ptime = -1.;
   Float_t erti = 0.;
   Float_t srti = 0.;
   Int_t nerti = 0;
   Int_t nsrti = 0;
   for (i = 0; i < fSlaveTotals.GetSize(); i++) {
      tot += fSlaveTotals[i];
      if (i < fSlaveProgress.GetSize())
         proc += fSlaveProgress[i];
      if (i < fSlaveBytesRead.GetSize())
         bytes += fSlaveBytesRead[i];
      if (i < fSlaveInitTime.GetSize())
         if (fSlaveInitTime[i] > -1. && (init < 0. || fSlaveInitTime[i] < init))
            init = fSlaveInitTime[i];
      if (i < fSlaveProcTime.GetSize())
         if (fSlaveProcTime[i] > -1. && (ptime < 0. || fSlaveProcTime[i] > ptime))
            ptime = fSlaveProcTime[i];
      if (i < fSlaveEvtRti.GetSize())
         if (fSlaveEvtRti[i] > -1.) {
            erti += fSlaveEvtRti[i];
            nerti++;
         }
      if (i < fSlaveMBRti.GetSize())
         if (fSlaveMBRti[i] > -1.) {
            srti += fSlaveMBRti[i];
            nsrti++;
         }
   }
   erti = (nerti > 0) ? erti / nerti : 0.;
   srti = (nsrti > 0) ? srti / nerti : 0.;

   TMessage m(kPROOF_PROGRESS);

   m << tot << proc << bytes << init << ptime << erti << srti;

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
   SafeDelete(fFeedbackTimer);
   fFeedbackPeriod = 2000;
   TProof::GetParameter(fInput, "PROOF_FeedbackPeriod", fFeedbackPeriod);
   fFeedbackTimer = new TTimer;
   fFeedbackTimer->SetObject(this);
   fFeedbackTimer->Start(fFeedbackPeriod, kTRUE);
}
