// @(#)root/proofplayer:$Id: TPerfStats.cxx,v 1.14 2007/06/21 08:50:47 rdm Exp $
// Author: Kristjan Gulbrandsen   11/05/04

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TPerfStats                                                           //
//                                                                      //
// Provides the interface for the PROOF internal performance measurment //
// and event tracing.                                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include "TPerfStats.h"

#include "Riostream.h"
#include "TCollection.h"
#include "TEnv.h"
#include "TError.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProofDebug.h"
#include "TProof.h"
#include "TProofServ.h"
#include "TSlave.h"
#include "TTree.h"
#include "TSQLServer.h"
#include "TSQLResult.h"
#include "TParameter.h"
#include "TPluginManager.h"
#include "TROOT.h"
#include "TVirtualMonitoring.h"


ClassImp(TPerfEvent)
ClassImp(TPerfStats)


//------------------------------------------------------------------------------

//______________________________________________________________________________
TPerfEvent::TPerfEvent(TTimeStamp *offset)
   : fEvtNode("-3"), fType(TVirtualPerfStats::kUnDefined), fSlave(),
     fEventsProcessed(0), fBytesRead(0), fLen(0), fLatency(0.0), fProcTime(0.0), fCpuTime(0.0),
     fIsStart(kFALSE), fIsOk(kFALSE)
{
   // Constructor

   if (gProofServ != 0) {
      fEvtNode = gProofServ->GetOrdinal();
   } else {
      fEvtNode = "-2"; // not on a PROOF server
   }

   if (offset != 0) {
      fTimeStamp = TTimeStamp(fTimeStamp.GetSec() - offset->GetSec(),
                     fTimeStamp.GetNanoSec() - offset->GetNanoSec());
   }
}

//______________________________________________________________________________
Int_t TPerfEvent::Compare(const TObject *obj) const
{
   // Compare method. Must return -1 if this is smaller than obj,
   // 0 if objects are equal and 1 if this is larger than obj.

   const TPerfEvent *pe = dynamic_cast<const TPerfEvent*>(obj);

   R__ASSERT(pe != 0);

   if (fTimeStamp < pe->fTimeStamp) {
      return -1;
   } else if (fTimeStamp == pe->fTimeStamp) {
      return 0;
   } else {
      return 1;
   }
}

//______________________________________________________________________________
void TPerfEvent::Print(Option_t *) const
{
   // Dump content of this instance

   cout << "TPerfEvent: ";

   if ( fEvtNode == -2 ) {
      cout << "StandAlone ";
   } else if ( fEvtNode == -1 ) {
      cout << "Master ";
   } else {
      cout << "Slave " << fEvtNode << " ";
   }
   cout << TVirtualPerfStats::EventType(fType) << " "
        << double(fTimeStamp)
        << endl;
}


//------------------------------------------------------------------------------

//______________________________________________________________________________
TPerfStats::TPerfStats(TList *input, TList *output)
   : fTrace(0), fPerfEvent(0), fPacketsHist(0), fEventsHist(0), fLatencyHist(0),
      fProcTimeHist(0), fCpuTimeHist(0), fBytesRead(0),
      fTotCpuTime(0.), fTotBytesRead(0), fTotEvents(0), fSlaves(0), fDoHist(kFALSE),
      fDoTrace(kFALSE), fDoTraceRate(kFALSE), fDoSlaveTrace(kFALSE), fDoQuota(kFALSE),
      fMonitoringWriter(0)
{
   // Normal constructor.

   TProof *proof = gProofServ->GetProof();
   TList *l = proof ? proof->GetListOfSlaveInfos() : 0 ;
   TIter nextslaveinfo(l);
   while (TSlaveInfo *si = dynamic_cast<TSlaveInfo*>(nextslaveinfo()))
      if (si->fStatus == TSlaveInfo::kActive) fSlaves++;

   PDB(kGlobal,1) Info("TPerfStats", "Statistics for %d slave(s)", fSlaves);

   fDoHist = (input->FindObject("PROOF_StatsHist") != 0);
   fDoTrace = (input->FindObject("PROOF_StatsTrace") != 0);
   fDoTraceRate = (input->FindObject("PROOF_RateTrace") != 0);
   fDoSlaveTrace = (input->FindObject("PROOF_SlaveStatsTrace") != 0);

   if ((gProofServ->IsMaster() && (fDoTrace || fDoTraceRate)) ||
       (!gProofServ->IsMaster() && fDoSlaveTrace)) {
      // Construct tree
      fTrace = new TTree("PROOF_PerfStats", "PROOF Statistics");
      fTrace->SetDirectory(0);
      fTrace->Bronch("PerfEvents", "TPerfEvent", &fPerfEvent, 64000, 0);
      output->Add(fTrace);
   }

   if (fDoHist && gProofServ->IsMaster()) {
      // Make Histograms
      Double_t time_per_bin = 1e-3; // 10ms
      Double_t min_time = 0;
      Int_t ntime_bins = 1000;

      fPacketsHist = new TH1D("PROOF_PacketsHist", "Packets processed per Slave",
                              fSlaves, 0, fSlaves);
      fPacketsHist->SetDirectory(0);
      fPacketsHist->SetMinimum(0);
      output->Add(fPacketsHist);

      fEventsHist = new TH1D("PROOF_EventsHist", "Events processed per Slave",
                             fSlaves, 0, fSlaves);
      fEventsHist->SetDirectory(0);
      fEventsHist->SetMinimum(0);
      output->Add(fEventsHist);

      fNodeHist = new TH1D("PROOF_NodeHist", "Slaves per Fileserving Node",
                           fSlaves, 0, fSlaves);
      fNodeHist->SetDirectory(0);
      fNodeHist->SetMinimum(0);
      fNodeHist->SetBit(TH1::kCanRebin);
      output->Add(fNodeHist);

      fLatencyHist = new TH2D("PROOF_LatencyHist", "GetPacket Latency per Slave",
                              fSlaves, 0, fSlaves,
                              ntime_bins, min_time, time_per_bin);
      fLatencyHist->SetDirectory(0);
      fLatencyHist->SetMarkerStyle(4);
      fLatencyHist->SetBit(TH1::kCanRebin);
      output->Add(fLatencyHist);

      fProcTimeHist = new TH2D("PROOF_ProcTimeHist", "Packet Processing Time per Slave",
                               fSlaves, 0, fSlaves,
                               ntime_bins, min_time, time_per_bin);
      fProcTimeHist->SetDirectory(0);
      fProcTimeHist->SetMarkerStyle(4);
      fProcTimeHist->SetBit(TH1::kCanRebin);
      output->Add(fProcTimeHist);

      fCpuTimeHist = new TH2D("PROOF_CpuTimeHist", "Packet CPU Time per Slave",
                              fSlaves, 0, fSlaves,
                              ntime_bins, min_time, time_per_bin);
      fCpuTimeHist->SetDirectory(0);
      fCpuTimeHist->SetMarkerStyle(4);
      fCpuTimeHist->SetBit(TH1::kCanRebin);
      output->Add(fCpuTimeHist);

      nextslaveinfo.Reset();
      Int_t slavebin=1;
      while (TSlaveInfo *si = dynamic_cast<TSlaveInfo*>(nextslaveinfo())) {
         if (si->fStatus == TSlaveInfo::kActive) {
            fPacketsHist->GetXaxis()->SetBinLabel(slavebin, si->GetOrdinal());
            fEventsHist->GetXaxis()->SetBinLabel(slavebin, si->GetOrdinal());
            fLatencyHist->GetXaxis()->SetBinLabel(slavebin, si->GetOrdinal());
            fProcTimeHist->GetXaxis()->SetBinLabel(slavebin, si->GetOrdinal());
            fCpuTimeHist->GetXaxis()->SetBinLabel(slavebin, si->GetOrdinal());
            slavebin++;
         }
      }
   }

   if (gProofServ->IsMaster()) {
      // Monitoring for query performances using SQL DB
      TString sqlserv = gEnv->GetValue("ProofServ.QueryLogDB", "");
      if (sqlserv != "") {
         PDB(kGlobal,1) Info("TPerfStats", "store monitoring data in SQL DB: %s", sqlserv.Data());
         fDoQuota = kTRUE;
      }

      // Monitoring for query performances using monitoring system (e.g. Monalisa)
      TString mon = gEnv->GetValue("ProofServ.Monitoring", "");
      if (mon != "") {
         // Extract arguments (up to 9 'const char *')
         TString a[10];
         Int_t from = 0;
         TString tok;
         Int_t na = 0;
         while (mon.Tokenize(tok, from, " "))
            a[na++] = tok;
         na--;
         // Get monitor object from the plugin manager
         TPluginHandler *h = 0;
         if ((h = gROOT->GetPluginManager()->FindHandler("TVirtualMonitoringWriter", a[0]))) {
            if (h->LoadPlugin() != -1) {
               fMonitoringWriter =
                  (TVirtualMonitoringWriter *) h->ExecPlugin(na, a[1].Data(), a[2].Data(), a[3].Data(),
                                                                 a[4].Data(), a[5].Data(), a[6].Data(),
                                                                 a[7].Data(), a[8].Data(), a[9].Data());
               if (fMonitoringWriter && fMonitoringWriter->IsZombie()) {
                  delete fMonitoringWriter;
                  fMonitoringWriter = 0;
               }
            }
         }
      }

      if (fMonitoringWriter) {
         PDB(kGlobal,1) Info("TPerfStats", "created monitoring object: %s", mon.Data());
         fDoQuota = kTRUE;
      }
   }
}

//______________________________________________________________________________
void TPerfStats::SimpleEvent(EEventType type)
{
   // Simple event.

   if (type == kStop && fPacketsHist != 0) {
      fNodeHist->LabelsDeflate("X");
      fNodeHist->LabelsOption("auv","X");
   }

   if (type == kStop && fDoQuota)
      WriteQueryLog();

   if (fTrace == 0) return;

   TPerfEvent pe(&fTzero);
   pe.fType = type;

   fPerfEvent = &pe;
   fTrace->SetBranchAddress("PerfEvents",&fPerfEvent);
   fTrace->Fill();
   fPerfEvent = 0;
}

//______________________________________________________________________________
void TPerfStats::PacketEvent(const char *slave, const char* slavename, const char* filename,
                             Long64_t eventsprocessed, Double_t latency, Double_t proctime,
                             Double_t cputime, Long64_t bytesRead)
{
   // Packet event.

   if (fDoTrace && fTrace != 0) {
      TPerfEvent pe(&fTzero);

      pe.fType = kPacket;
      pe.fSlaveName = slavename;
      pe.fFileName = filename;
      pe.fSlave = slave;
      pe.fEventsProcessed = eventsprocessed;
      pe.fBytesRead = bytesRead;
      pe.fLatency = latency;
      pe.fProcTime = proctime;
      pe.fCpuTime = cputime;

      fPerfEvent = &pe;
      fTrace->SetBranchAddress("PerfEvents",&fPerfEvent);
      fTrace->Fill();
      fPerfEvent = 0;
   }

   if (fDoHist && fPacketsHist != 0) {
      fPacketsHist->Fill(slave, 1);
      fEventsHist->Fill(slave, eventsprocessed);
      fLatencyHist->Fill(slave, latency, 1);
      fProcTimeHist->Fill(slave, proctime, 1);
      fCpuTimeHist->Fill(slave, cputime, 1);
   }

   if (fDoQuota) {
      fTotCpuTime += cputime;
      fTotBytesRead += bytesRead;
      fTotEvents += eventsprocessed;
   }
}

//______________________________________________________________________________
void TPerfStats::FileEvent(const char *slave, const char *slavename, const char *nodename,
                            const char *filename, Bool_t isStart)
{
   // File event.

   if (fDoTrace && fTrace != 0) {
      TPerfEvent pe(&fTzero);

      pe.fType = kFile;
      pe.fSlaveName = slavename;
      pe.fNodeName = nodename;
      pe.fFileName = filename;
      pe.fSlave = slave;
      pe.fIsStart = isStart;

      fPerfEvent = &pe;
      fTrace->SetBranchAddress("PerfEvents",&fPerfEvent);
      fTrace->Fill();
      fPerfEvent = 0;
   }

   if (fDoHist && fPacketsHist != 0) {
      fNodeHist->Fill(nodename, isStart ? 1 : -1);
   }
}

//______________________________________________________________________________
void TPerfStats::FileOpenEvent(TFile *file, const char *filename, Double_t proctime)
{
   // Open file event.

   if (fDoTrace && fTrace != 0) {
      TPerfEvent pe(&fTzero);

      pe.fType = kFileOpen;
      pe.fFileName = filename;
      pe.fFileClass = file != 0 ? file->ClassName() : "none";
      pe.fProcTime = proctime;
      pe.fIsOk = (file != 0);

      fPerfEvent = &pe;
      fTrace->SetBranchAddress("PerfEvents",&fPerfEvent);
      fTrace->Fill();
      fPerfEvent = 0;
   }
}

//______________________________________________________________________________
void TPerfStats::FileReadEvent(TFile *file, Int_t len, Double_t proctime)
{
   // Read file event.

   if (fDoTrace && fTrace != 0) {
      TPerfEvent pe(&fTzero);

      pe.fType = kFileRead;
      pe.fFileName = file->GetName();
      pe.fFileClass = file->ClassName();
      pe.fLen = len;
      pe.fProcTime = proctime;

      fPerfEvent = &pe;
      fTrace->SetBranchAddress("PerfEvents",&fPerfEvent);
      fTrace->Fill();
      fPerfEvent = 0;
   }
}

//______________________________________________________________________________
void TPerfStats::RateEvent(Double_t proctime, Double_t deltatime,
                           Long64_t eventsprocessed, Long64_t bytesRead)
{
   // Rate event.

   if ((fDoTrace || fDoTraceRate) && fTrace != 0) {
      TPerfEvent pe(&fTzero);

      pe.fType = kRate;
      pe.fEventsProcessed = eventsprocessed;
      pe.fBytesRead = bytesRead;
      pe.fProcTime = proctime;
      pe.fLatency = deltatime;

      fPerfEvent = &pe;
      fTrace->SetBranchAddress("PerfEvents",&fPerfEvent);
      fTrace->Fill();
      fPerfEvent = 0;
   }
}

//______________________________________________________________________________
void TPerfStats::SetBytesRead(Long64_t num)
{
   // Set number of bytes read.

   fBytesRead = num;
}

//______________________________________________________________________________
Long64_t TPerfStats::GetBytesRead() const
{
   // Get number of bytes read.

   return fBytesRead;
}

//______________________________________________________________________________
void TPerfStats::WriteQueryLog()
{
   // Connect to SQL server and register query log used for quotas.
   // The proofquerylog table has the format:
   // CREATE TABLE proofquerylog (
   //   id            INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
   //   user          VARCHAR(32) NOT NULL,
   //   group         VARCHAR(32),
   //   begin         DATETIME,
   //   end           DATETIME,
   //   walltime      INT,
   //   cputime       FLOAT,
   //   bytesread     BIGINT,
   //   events        BIGINT,
   //   workers       INT
   //)
   // The same info is send to Monalisa (or other monitoring systems) in the
   // form of a list of name,value pairs.

   TTimeStamp stop;

   TString sqlserv = gEnv->GetValue("ProofServ.QueryLogDB","");
   TString sqluser = gEnv->GetValue("ProofServ.QueryLogUser","");
   TString sqlpass = gEnv->GetValue("ProofServ.QueryLogPasswd","");

   // write to SQL DB
   if (sqlserv != "" && sqluser != "" && sqlpass != "" && gProofServ) {
      TString sql;
      sql.Form("INSERT INTO proofquerylog VALUES (0, '%s', '%s', "
               "'%s', '%s', %d, %.2f, %lld, %lld, %d)",
               gProofServ->GetUser(), gProofServ->GetGroup(),
               fTzero.AsString("s"), stop.AsString("s"),
               stop.GetSec()-fTzero.GetSec(), fTotCpuTime,
               fTotBytesRead, fTotEvents, fSlaves);

      // open connection to SQL server
      TSQLServer *db =  TSQLServer::Connect(sqlserv, sqluser, sqlpass);

      if (!db || db->IsZombie()) {
         Error("WriteQueryLog", "failed to connect to SQL server %s as %s %s",
               sqlserv.Data(), sqluser.Data(), sqlpass.Data());
         printf("%s\n", sql.Data());
      } else {
         TSQLResult *res = db->Query(sql);

         if (!res) {
            Error("WriteQueryLog", "insert into proofquerylog failed");
            printf("%s\n", sql.Data());
         }
         delete res;
      }
      delete db;
   }

   // write to monitoring system
   if (fMonitoringWriter) {
      if (!gProofServ || !gProofServ->GetSessionTag() || !gProofServ->GetProof() ||
          !gProofServ->GetProof()->GetQueryResult()) {
         Error("WriteQueryLog", "some require object are 0 (0x%x 0x%x 0x%x 0x%x)",
               gProofServ, gProofServ->GetSessionTag(), gProofServ->GetProof(),
               gProofServ->GetProof()->GetQueryResult());
         return;
      }

      TString identifier;
      identifier.Form("%s-%d", gProofServ->GetSessionTag(),
                      gProofServ->GetProof()->GetQueryResult()->GetSeqNum());

      TList values;
      values.SetOwner();
      values.Add(new TParameter<int>("id", 0));
      values.Add(new TNamed("user", gProofServ->GetUser()));
      values.Add(new TNamed("group", gProofServ->GetGroup()));
      values.Add(new TNamed("begin", fTzero.AsString("s")));
      values.Add(new TNamed("end", stop.AsString("s")));
      values.Add(new TParameter<int>("walltime", stop.GetSec()-fTzero.GetSec()));
      values.Add(new TParameter<float>("cputime", fTotCpuTime));
      values.Add(new TParameter<Long64_t>("bytesread", fTotBytesRead));
      values.Add(new TParameter<Long64_t>("events", fTotEvents));
      if (!fMonitoringWriter->SendParameters(&values, identifier))
         Error("WriteQueryLog", "sending of monitoring info failed");
   }
}

//______________________________________________________________________________
void TPerfStats::Setup(TList *input)
{
   // Setup the PROOF input list with requested statistics and tracing options.

   const Int_t ntags=3;
   const Char_t *tags[ntags] = {"StatsHist",
                                "StatsTrace",
                                "SlaveStatsTrace"};

   for (Int_t i=0; i<ntags; i++) {
      TString envvar = "Proof.";
      envvar += tags[i];
      TString inputname = "PROOF_";
      inputname += tags[i];
      TObject* obj = input->FindObject(inputname.Data());
      if (gEnv->GetValue(envvar.Data(), 0)) {
         if (!obj)
            input->Add(new TNamed(inputname.Data(),""));
      } else {
         if (obj) {
            input->Remove(obj);
            delete obj;
         }
      }
   }
}

//______________________________________________________________________________
void TPerfStats::Start(TList *input, TList *output)
{
   // Initialize PROOF statistics run.

   if (gPerfStats != 0) {
      delete gPerfStats;
   }

   gPerfStats = new TPerfStats(input, output);

   gPerfStats->SimpleEvent(TVirtualPerfStats::kStart);
}

//______________________________________________________________________________
void TPerfStats::Stop()
{
   // Terminate the PROOF statistics run.

   if (gPerfStats == 0) return;

   gPerfStats->SimpleEvent(TVirtualPerfStats::kStop);

   delete gPerfStats;
   gPerfStats = 0;
}
