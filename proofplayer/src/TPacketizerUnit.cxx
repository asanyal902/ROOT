// @(#)root/proofplayer:$Id$
// Author: Long Tran-Thanh    22/07/07

/*************************************************************************
 * Copyright (C) 1995-2002, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TPacketizerUnit                                                      //
//                                                                      //
// This packetizer generates packets of generic units, representing the //
// number of times an operation cycle has to be repeated by the worker  //
// node, e.g. the number of Monte carlo events to be generated.         //
// Packets sizes are generated taking into account the performance of   //
// worker nodes, based on the time needed to process previous packets.  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include "TPacketizerUnit.h"

#include "Riostream.h"
#include "TDSet.h"
#include "TError.h"
#include "TEventList.h"
#include "TMap.h"
#include "TMessage.h"
#include "TMonitor.h"
#include "TNtupleD.h"
#include "TObject.h"
#include "TParameter.h"
#include "TPerfStats.h"
#include "TProofDebug.h"
#include "TProof.h"
#include "TProofPlayer.h"
#include "TProofServ.h"
#include "TSlave.h"
#include "TSocket.h"
#include "TStopwatch.h"
#include "TTimer.h"
#include "TUrl.h"
#include "TClass.h"
#include "TMath.h"
#include "TObjString.h"


using namespace TMath;
//
// The following utility class manage the state of the
// work to be performed and the slaves involved in the process.
//
// The list of TSlaveStat(s) keep track of the work (being) done
// by each slave
//

//------------------------------------------------------------------------------

class TPacketizerUnit::TSlaveStat : public TObject {

friend class TPacketizerUnit;

private:
   TSlave   *fSlave;         // corresponding TSlave record
   Long64_t  fProcessed;     // number of entries processed
   Long64_t  fLastProcessed; // number of processed entries of the last packet
   Double_t  fSpeed;         // estimated current average speed of the processing slave
   Double_t  fTimeInstant;   // stores the time instant when the current packet started
   TNtupleD *fCircNtp;       // Keeps circular info for speed calculations
   Long_t    fCircLvl;       // Circularity level


public:
   TSlaveStat(TSlave *sl, TList *input);
   ~TSlaveStat();

   void        GetCurrentTime();

   const char *GetName() const { return fSlave->GetName(); }
   Long64_t    GetEntriesProcessed() const { return fProcessed; }

   void        UpdatePerformance(Double_t time);
};

//______________________________________________________________________________
TPacketizerUnit::TSlaveStat::TSlaveStat(TSlave *slave, TList *input)
                            : fSlave(slave), fProcessed(0), fLastProcessed(0),
                              fSpeed(0), fTimeInstant(0), fCircLvl(5)
{
   // Main constructor

   // Initialize the circularity ntple for speed calculations
   fCircNtp = new TNtupleD("Speed Circ Ntp", "Circular process info","tm:ev");
   TProof::GetParameter(input, "PROOF_TPacketizerUnitCircularity", fCircLvl);
   fCircLvl = (fCircLvl > 0) ? fCircLvl : 5;
   fCircNtp->SetCircular(fCircLvl);
}

//______________________________________________________________________________
TPacketizerUnit::TSlaveStat::~TSlaveStat()
{
   // Destructor

   SafeDelete(fCircNtp);
}

//______________________________________________________________________________
void TPacketizerUnit::TSlaveStat::UpdatePerformance(Double_t time)
{
   // Update the circular ntple

   Double_t ttot = time;
   Double_t *ar = fCircNtp->GetArgs();
   Int_t ne = fCircNtp->GetEntries();
   if (ne <= 0) {
      // First call: just fill one ref entry and return
      fCircNtp->Fill(0., 0);
      fSpeed = 0.;
      return;
   }
   // Fill the entry
   fCircNtp->GetEntry(ne-1);
   ttot = ar[0] + time;
   fCircNtp->Fill(ttot, fProcessed);

   // Calculate the speed
   fCircNtp->GetEntry(0);
   Double_t dtime = (ttot > ar[0]) ? ttot - ar[0] : ne+1 ;
   Long64_t nevts = fProcessed - (Long64_t)ar[1];
   fSpeed = nevts / dtime;
   PDB(kPacketizer,2)
      Info("UpdatePerformance", "time:%f, dtime:%f, nevts:%lld, speed: %f",
                                time, dtime, nevts, fSpeed);

}

//------------------------------------------------------------------------------

ClassImp(TPacketizerUnit)

//______________________________________________________________________________
TPacketizerUnit::TPacketizerUnit(TList *slaves, Long64_t num, TList *input,
                                 TProofProgressStatus *st)
                : TVirtualPacketizer(input, st)
{
   // Constructor

   PDB(kPacketizer,1) Info("TPacketizerUnit", "enter (num %lld)", num);

   // Init pointer members
   fSlaveStats = 0;
   fPackets = 0;

   fTimeLimit = 1;
   TProof::GetParameter(input, "PROOF_PacketizerTimeLimit", fTimeLimit);
   PDB(kPacketizer,1)
      Info("TPacketizerUnit", "time limit is %lf", fTimeLimit);
   fProcessing = 0;

   fStopwatch = new TStopwatch();

   fPackets = new TList;
   fPackets->SetOwner();

   fSlaveStats = new TMap;
   fSlaveStats->SetOwner(kFALSE);

   TSlave *slave;
   TIter si(slaves);
   while ((slave = (TSlave*) si.Next()))
      fSlaveStats->Add(slave, new TSlaveStat(slave, input));

   fTotalEntries = num;

   fStopwatch->Start();
   PDB(kPacketizer,1) Info("TPacketizerUnit", "return");
}

//______________________________________________________________________________
TPacketizerUnit::~TPacketizerUnit()
{
   // Destructor.

   if (fSlaveStats)
      fSlaveStats->DeleteValues();
   SafeDelete(fSlaveStats);
   SafeDelete(fPackets);
   SafeDelete(fStopwatch);
}

//______________________________________________________________________________
Double_t TPacketizerUnit::GetCurrentTime()
{
   // Get current time

   Double_t retValue = fStopwatch->RealTime();
   fStopwatch->Continue();
   return retValue;
}

//______________________________________________________________________________
TDSetElement *TPacketizerUnit::GetNextPacket(TSlave *sl, TMessage *r)
{
   // Get next packet

   if (!fValid)
      return 0;

   // Find slave
   TSlaveStat *slstat = (TSlaveStat*) fSlaveStats->GetValue(sl);
   R__ASSERT(slstat != 0);

   // Update stats & free old element
   Double_t latency, proctime, proccpu;
   Long64_t bytesRead = -1;
   Long64_t totalEntries = -1;
   Long64_t totev = 0;
   Long64_t numev = -1;

   if (!gProofServ || gProofServ->GetProtocol() > 18) {
      TProofProgressStatus *status = 0;
      (*r) >> latency;
      (*r) >> status;

      proctime = status ? status->GetProcTime() : -1.;
      proccpu  = status ? status->GetCPUTime() : -1.;
      totev  = status ? status->GetEntries() : 0;
      bytesRead  = status ? status->GetBytesRead() : 0;

      delete status;
   } else {

      (*r) >> latency >> proctime >> proccpu;

      // only read new info if available
      if (r->BufferSize() > r->Length()) (*r) >> bytesRead;
      if (r->BufferSize() > r->Length()) (*r) >> totalEntries;
      if (r->BufferSize() > r->Length()) (*r) >> totev;
   }
   numev = totev - slstat->fProcessed;

   PDB(kPacketizer,2)
      Info("GetNextPacket","worker-%s: fProcessed %lld\t", sl->GetOrdinal(), GetEntriesProcessed());
   fProcessing = 0;

   PDB(kPacketizer,2)
      Info("GetNextPacket","worker-%s (%s): %lld %7.3lf %7.3lf %7.3lf %lld",
                           sl->GetOrdinal(), sl->GetName(),
                           numev, latency, proctime, proccpu, bytesRead);

   if (gPerfStats != 0) {
      gPerfStats->PacketEvent(sl->GetOrdinal(), sl->GetName(), "", numev,
                              latency, proctime, proccpu, bytesRead);
   }

   if (GetEntriesProcessed() == fTotalEntries) {
      // Send last timer message
      HandleTimer(0);
      SafeDelete(fProgress);
   }

   if (fStop) {
      // Send last timer message
      HandleTimer(0);
      return 0;
   }


   Long64_t num;

   // Get the current time
   Double_t cTime = GetCurrentTime();

   if (slstat->fCircNtp->GetEntries() <= 0) {
      // The calibration phase
      PDB(kPacketizer,2) Info("GetNextPacket"," calibration: "
                              "total entries %lld, workers %d",
                              fTotalEntries, fSlaveStats->GetSize());
      Long64_t avg = fTotalEntries/(fSlaveStats->GetSize());
      num = (avg > 5) ? 5 : avg;

      // Create a reference entry
      slstat->UpdatePerformance(0.);

   } else {
      // Schedule tasks for workers based on the currently estimated processing speeds

      // Update worker stats and performances
      slstat->fProcessed += slstat->fLastProcessed;
      slstat->UpdatePerformance(proctime);

      TMapIter *iter = (TMapIter *)fSlaveStats->MakeIterator();
      TSlave * tmpSlave;
      TSlaveStat * tmpStat;

      Double_t sumSpeed = 0;
      Double_t sumBusy = 0;

      // The basic idea is to estimate the optimal finishing time for the process, assuming
      // that the currently measured processing speeds of the slaves remain the same in the future.
      // The optTime can be calculated as follows.
      // Let s_i be the speed of worker i. We assume that worker i will be busy in the
      // next b_i time instant. Therefore we have the following equation:
      //                 SUM((optTime-b_i)*s_i) = (remaining_entries),
      // Hence optTime can be calculated easily:
      //                 optTime = ((remaining_entries) + SUM(b_i*s_i))/SUM(s_i)

      while ((tmpSlave = (TSlave *)iter->Next())) {
         tmpStat = (TSlaveStat *)fSlaveStats->GetValue(tmpSlave);
         // If the slave doesn't response for a long time, its service rate will be considered as 0
         if ((cTime - tmpStat->fTimeInstant) > 4*fTimeLimit)
            tmpStat->fSpeed = 0;
         PDB(kPacketizer,2)
            Info("GetNextPacket",
                 "worker-%s: speed %lf", tmpSlave->GetOrdinal(), tmpStat->fSpeed);
         if (tmpStat->fSpeed > 0) {
            // Calculating the SUM(s_i)
            sumSpeed += tmpStat->fSpeed;
            // There is nothing to do if slave_i is not calibrated or slave i is the current slave
            if ((tmpStat->fTimeInstant) && (cTime - tmpStat->fTimeInstant > 0)) {
               // Calculating the SUM(b_i*s_i)
               //      s_i = tmpStat->fSpeed
               //      b_i = tmpStat->fTimeInstant + tmpStat->fLastProcessed/s_i - cTime)
               //  b_i*s_i = (tmpStat->fTimeInstant - cTime)*s_i + tmpStat->fLastProcessed
               Double_t busyspeed =
                  ((tmpStat->fTimeInstant - cTime)*tmpStat->fSpeed + tmpStat->fLastProcessed);
               if (busyspeed > 0)
                  sumBusy += busyspeed;
            }
         }
      }
      PDB(kPacketizer,2)
         Info("GetNextPacket", "worker-%s: sum speed: %lf, sum busy: %f",
                               sl->GetOrdinal(), sumSpeed, sumBusy);
      // firstly the slave will try to get all of the remaining entries
      if (slstat->fSpeed > 0 &&
         (fTotalEntries - GetEntriesProcessed())/(slstat->fSpeed) < fTimeLimit) {
         num = (fTotalEntries - GetEntriesProcessed());
      } else {
         if (slstat->fSpeed > 0) {
            // calculating the optTime
            Double_t optTime = (fTotalEntries - GetEntriesProcessed() + sumBusy)/sumSpeed;
            // if optTime is greater than the official time limit, then the slave gets a number
            // of entries that still fit into the time limit, otherwise it uses the optTime as
            // a time limit
            num = (optTime > fTimeLimit) ? Nint(fTimeLimit*(slstat->fSpeed))
                                         : Nint(optTime*(slstat->fSpeed));
           PDB(kPacketizer,2)
              Info("GetNextPacket", "opTime %lf num %lld speed %lf", optTime, num, slstat->fSpeed);
         } else {
            Long64_t avg = fTotalEntries/(fSlaveStats->GetSize());
            num = (avg > 5) ? 5 : avg;
         }
      }
   }
   num = (num > 1) ? num : 1;
   fProcessing = (num < (fTotalEntries - GetEntriesProcessed())) ? num : (fTotalEntries - GetEntriesProcessed());

   // Set the informations of the current slave
   slstat->fLastProcessed = fProcessing;
   // Set the start time of the current packet
   slstat->fTimeInstant = cTime;

   PDB(kPacketizer,2)
      Info("GetNextPacket", "worker-%s: num %lld, processing %lld, remaining %lld",sl->GetOrdinal(),
                            num, fProcessing, (fTotalEntries - GetEntriesProcessed() - fProcessing));
   TDSetElement *elem = new TDSetElement("", "", "", 0, fProcessing);
   elem->SetBit(TDSetElement::kEmpty);

   // Update the total counter
   fProgressStatus->IncEntries(slstat->fLastProcessed);

   return elem;
}
