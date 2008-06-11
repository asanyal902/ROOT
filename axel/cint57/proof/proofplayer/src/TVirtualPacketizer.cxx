// @(#)root/proofplayer:$Id$
// Author: Maarten Ballintijn    9/7/2002

/*************************************************************************
 * Copyright (C) 1995-2002, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TVirtualPacketizer                                                   //
//                                                                      //
// The packetizer is a load balancing object created for each query.    //
// It generates packets to be processed on PROOF worker servers.        //
// A packet is an event range (begin entry and number of entries) or    //
// object range (first object and number of objects) in a TTree         //
// (entries) or a directory (objects) in a file.                        //
// Packets are generated taking into account the performance of the     //
// remote machine, the time it took to process a previous packet on     //
// the remote machine, the locality of the database files, etc.         //
//                                                                      //
// TVirtualPacketizer includes common parts of PROOF packetizers.       //
// Look in subclasses for details.                                      //
// The default packetizer is TPacketizerAdaptive.                       //
// To use an alternative one, for instance - the TPacketizer, call:     //
// proof->SetParameter("PROOF_Packetizer", "TPacketizer");              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include "TVirtualPacketizer.h"
#include "TEnv.h"
#include "TFile.h"
#include "TTree.h"
#include "TKey.h"
#include "TDSet.h"
#include "TError.h"
#include "TEventList.h"
#include "TEntryList.h"
#include "TMap.h"
#include "TMessage.h"
#include "TObjString.h"

#include "TProof.h"
#include "TProofDebug.h"
#include "TProofPlayer.h"
#include "TProofServ.h"
#include "TSlave.h"
#include "TSocket.h"
#include "TTimer.h"
#include "TUrl.h"
#include "TMath.h"
#include "TMonitor.h"
#include "TNtupleD.h"
#include "TPerfStats.h"

ClassImp(TVirtualPacketizer)

//______________________________________________________________________________
TVirtualPacketizer::TVirtualPacketizer(TList *input)
{
   // Constructor.

   fProcessed = 0;
   fBytesRead = 0;
   fTotalEntries = 0;
   fValid = kTRUE;
   fStop = kFALSE;
   fFailedPackets = 0;

   // Performance monitoring
   TTime tnow = gSystem->Now();
   fStartTime = Long_t(tnow);
   SetBit(TVirtualPacketizer::kIsInitializing);
   ResetBit(TVirtualPacketizer::kIsDone);
   fInitTime = 0;
   fProcTime = 0;
   fTimeUpdt = -1.;

   // Init circularity ntple for performance calculations
   fCircProg = new TNtupleD("CircNtuple","Circular progress info","tm:ev:mb");
   fCircN = 10;
   TProof::GetParameter(input, "PROOF_ProgressCircularity", fCircN);
   fCircProg->SetCircular(fCircN);

   // Init progress timer
   Long_t period = 500;
   TProof::GetParameter(input, "PROOF_ProgressPeriod", period);
   fProgress = new TTimer;
   fProgress->SetObject(this);
   fProgress->Start(period, kFALSE);

   // Whether to send estimated values for the progress info
   TString estopt;
   TProof::GetParameter(input, "PROOF_RateEstimation", estopt);
   if (estopt.IsNull()) {
      // Parse option from the env
      estopt = gEnv->GetValue("Proof.RateEstimation", "");
   }
   fUseEstOpt = kEstOff;
   if (estopt == "current")
      fUseEstOpt = kEstCurrent;
   else if (estopt == "average")
      fUseEstOpt = kEstAverage;
}

//______________________________________________________________________________
TVirtualPacketizer::~TVirtualPacketizer()
{
   // Destructor.

   SafeDelete(fCircProg);
   SafeDelete(fProgress);
   SafeDelete(fFailedPackets);
}

//______________________________________________________________________________
Long64_t TVirtualPacketizer::GetEntries(Bool_t tree, TDSetElement *e)
{
   // Get entries.

   Long64_t entries;
   TFile *file = TFile::Open(e->GetFileName());

   if ( file->IsZombie() ) {
      Error("GetEntries","Cannot open file: %s (%s)",
            e->GetFileName(), strerror(file->GetErrno()) );
      return -1;
   }

   TDirectory *dirsave = gDirectory;
   if ( ! file->cd(e->GetDirectory()) ) {
      Error("GetEntries","Cannot cd to: %s", e->GetDirectory() );
      delete file;
      return -1;
   }
   TDirectory *dir = gDirectory;
   dirsave->cd();

   if ( tree ) {
      TKey *key = dir->GetKey(e->GetObjName());
      if ( key == 0 ) {
         Error("GetEntries","Cannot find tree \"%s\" in %s",
               e->GetObjName(), e->GetFileName() );
         delete file;
         return -1;
      }
      TTree *t = (TTree *) key->ReadObj();
      if ( t == 0 ) {
         // Error always reported?
         delete file;
         return -1;
      }
      entries = (Long64_t) t->GetEntries();
      delete t;

   } else {
      TList *keys = dir->GetListOfKeys();
      entries = keys->GetSize();
   }

   delete file;

   return entries;
}

//______________________________________________________________________________
Long64_t TVirtualPacketizer::GetEntriesProcessed(TSlave *) const
{
   // Get Entries processed by the given slave.

   AbstractMethod("GetEntriesProcessed");
   return 0;
}

//______________________________________________________________________________
TDSetElement *TVirtualPacketizer::GetNextPacket(TSlave *, TMessage *)
{
   // Get next packet.

   AbstractMethod("GetNextPacket");
   return 0;
}

//______________________________________________________________________________
void TVirtualPacketizer::StopProcess(Bool_t /*abort*/)
{
   // Stop process.

   fStop = kTRUE;
}

//______________________________________________________________________________
TDSetElement* TVirtualPacketizer::CreateNewPacket(TDSetElement* base,
                                                  Long64_t first, Long64_t num)
{
   // Creates a new TDSetElement from from base packet starting from
   // the first entry with num entries.
   // The function returns a new created objects which have to be deleted.

   TDSetElement* elem = new TDSetElement(base->GetFileName(), base->GetObjName(),
                                         base->GetDirectory(), first, num);

   // create TDSetElements for all the friends of elem.
   TList *friends = base->GetListOfFriends();
   if (friends) {
      TIter nxf(friends);
      TPair *p = 0;
      while ((p = (TPair *) nxf())) {
         TDSetElement *fe = (TDSetElement *) p->Key();
         elem->AddFriend(new TDSetElement(fe->GetFileName(), fe->GetObjName(),
                                          fe->GetDirectory(), first, num),
                                         ((TObjString *)(p->Value()))->GetName());
      }
   }

   return elem;
}

//______________________________________________________________________________
Bool_t TVirtualPacketizer::HandleTimer(TTimer *)
{
   // Send progress message to client.

   if (fProgress == 0 || TestBit(TVirtualPacketizer::kIsDone))
      return kFALSE; // timer stopped already or reports completed

   // Message to be sent over
   TMessage m(kPROOF_PROGRESS);

   if (gProofServ->GetProtocol() > 11) {

      // Prepare progress info
      TTime tnow = gSystem->Now();
      Float_t now = (Float_t) (Long_t(tnow) - fStartTime) / (Double_t)1000.;
      Long64_t estent = fProcessed;
      Long64_t estmb = fBytesRead;

      // Times and counters
      Float_t evtrti = -1., mbrti = -1.;
      if (TestBit(TVirtualPacketizer::kIsInitializing)) {
         // Initialization
         fInitTime = now;
      } else {
         // Fill the reference as first
         if (fCircProg->GetEntries() <= 0) {
            fCircProg->Fill((Double_t)0., 0., 0.);
            // Best estimation of the init time
            fInitTime = (now + fInitTime) / 2.;
         }
         // Time between updates
         fTimeUpdt = now - fProcTime;
         // Update proc time
         fProcTime = now - fInitTime;
         // Estimated number of processed events
         GetEstEntriesProcessed(fProcTime, estent, estmb);
         Double_t evts = (Double_t) estent;
         Double_t mbs = (estmb > 0) ?  estmb / TMath::Power(2.,20.) : 0.; //--> MB
         // Good entry
         fCircProg->Fill((Double_t)fProcTime, evts, mbs);
         // Instantaneous rates (at least 5 reports)
         if (fCircProg->GetEntries() > 4) {
            Double_t *ar = fCircProg->GetArgs();
            fCircProg->GetEntry(0);
            Double_t dt = (Double_t)fProcTime - ar[0];
            evtrti = (dt > 0) ? (Float_t) (evts - ar[1]) / dt : -1. ;
            mbrti = (dt > 0) ? (Float_t) (mbs - ar[2]) / dt : -1. ;
            if (gPerfStats != 0)
               gPerfStats->RateEvent((Double_t)fProcTime, dt,
                                     (Long64_t) (evts - ar[1]),
                                     (Long64_t) ((mbs - ar[2])*TMath::Power(2.,20.)));
         }
      }

      // Fill the message now
      m << fTotalEntries << estent << estmb << fInitTime << fProcTime
        << evtrti << mbrti;


   } else {
      // Old format
      m << fTotalEntries << fProcessed;
   }

   // Final report only once (to correctly determine the proc time)
   if (fTotalEntries > 0 && fProcessed >= fTotalEntries)
      SetBit(TVirtualPacketizer::kIsDone);

   // send message to client;
   gProofServ->GetSocket()->Send(m);

   return kFALSE; // ignored?
}

//______________________________________________________________________________
void TVirtualPacketizer::SetInitTime()
{
   // Set the initialization time

   if (TestBit(TVirtualPacketizer::kIsInitializing)) {
      fInitTime = (Float_t) (Long_t(gSystem->Now()) - fStartTime) / (Double_t)1000.;
      ResetBit(TVirtualPacketizer::kIsInitializing);
   }
   PDB(kPacketizer,2)
      Info("SetInitTime","fInitTime: %f s", fInitTime);
}
