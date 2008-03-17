// @(#)root/proofx:$Id$
// Author: Gerardo Ganis  12/12/2005

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TXProofServ                                                          //
//                                                                      //
// TXProofServ is the XRD version of the PROOF server. It differs from  //
// TXProofServ only for the underlying connection technology            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "RConfigure.h"
#include "RConfig.h"
#include "Riostream.h"

#ifdef WIN32
   #include <io.h>
   typedef long off_t;
#endif
#include <sys/types.h>
#include <netinet/in.h>

#include "TXProofServ.h"
#include "TObjString.h"
#include "TEnv.h"
#include "TError.h"
#include "TException.h"
#include "THashList.h"
#include "TInterpreter.h"
#include "TProofDebug.h"
#include "TProof.h"
#include "TProofPlayer.h"
#include "TProofQueryResult.h"
#include "TRegexp.h"
#include "TClass.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TPluginManager.h"
#include "TXSocketHandler.h"
#include "TXUnixSocket.h"
#include "compiledata.h"
#include "TProofResourcesStatic.h"
#include "TProofNodeInfo.h"
#include "XProofProtocol.h"

#include <XrdClient/XrdClientConst.hh>
#include <XrdClient/XrdClientEnv.hh>


// debug hook
static volatile Int_t gProofServDebug = 1;

//----- Interrupt signal handler -----------------------------------------------
//______________________________________________________________________________
class TXProofServInterruptHandler : public TSignalHandler {
   TXProofServ  *fServ;
public:
   TXProofServInterruptHandler(TXProofServ *s)
      : TSignalHandler(kSigUrgent, kFALSE) { fServ = s; }
   Bool_t  Notify();
};

//______________________________________________________________________________
Bool_t TXProofServInterruptHandler::Notify()
{
   fServ->HandleUrgentData();
   if (TROOT::Initialized()) {
      Throw(GetSignal());
   }
   return kTRUE;
}

//----- SigPipe signal handler -------------------------------------------------
//______________________________________________________________________________
class TXProofServSigPipeHandler : public TSignalHandler {
   TXProofServ  *fServ;
public:
   TXProofServSigPipeHandler(TXProofServ *s) : TSignalHandler(kSigPipe, kFALSE)
      { fServ = s; }
   Bool_t  Notify();
};

//______________________________________________________________________________
Bool_t TXProofServSigPipeHandler::Notify()
{
   fServ->HandleSigPipe();
   return kTRUE;
}

//----- Termination signal handler ---------------------------------------------
//______________________________________________________________________________
class TXProofServTerminationHandler : public TSignalHandler {
   TXProofServ  *fServ;
public:
   TXProofServTerminationHandler(TXProofServ *s)
      : TSignalHandler(kSigTermination, kFALSE) { fServ = s; }
   Bool_t  Notify();
};

//______________________________________________________________________________
Bool_t TXProofServTerminationHandler::Notify()
{
   Printf("TXProofServTerminationHandler::Notify: wake up!");

   fServ->HandleTermination();
   return kTRUE;
}

//----- Seg violation signal handler ---------------------------------------------
//______________________________________________________________________________
class TXProofServSegViolationHandler : public TSignalHandler {
   TXProofServ  *fServ;
public:
   TXProofServSegViolationHandler(TXProofServ *s)
      : TSignalHandler(kSigSegmentationViolation, kFALSE) { fServ = s; }
   Bool_t  Notify();
};

//______________________________________________________________________________
Bool_t TXProofServSegViolationHandler::Notify()
{
   Printf("**** ");
   Printf("**** Segmentation violation: terminating ****");
   Printf("**** ");
   fServ->HandleTermination();
   return kTRUE;
}

//----- Input handler for messages from parent or master -----------------------
//______________________________________________________________________________
class TXProofServInputHandler : public TFileHandler {
   TXProofServ  *fServ;
public:
   TXProofServInputHandler(TXProofServ *s, Int_t fd) : TFileHandler(fd, 1)
      { fServ = s; }
   Bool_t Notify();
   Bool_t ReadNotify() { return Notify(); }
};

//______________________________________________________________________________
Bool_t TXProofServInputHandler::Notify()
{
   fServ->HandleSocketInput();
   // This request has been completed: remove the client ID from the pipe
   ((TXUnixSocket *) fServ->GetSocket())->RemoveClientID();
   return kTRUE;
}

ClassImp(TXProofServ)

// Hook to the constructor. This is needed to avoid using the plugin manager
// which may create problems in multi-threaded environments.
extern "C" {
   TApplication *GetTXProofServ(Int_t *argc, char **argv, FILE *flog)
   { return new TXProofServ(argc, argv, flog); }
}

//______________________________________________________________________________
TXProofServ::TXProofServ(Int_t *argc, char **argv, FILE *flog)
            : TProofServ(argc, argv, flog)
{
   // Main constructor

   fInterruptHandler = 0;
   fInputHandler = 0;
   fTerminated = kFALSE;
   fShutdownTimerMtx = new TMutex(kTRUE);
}

//______________________________________________________________________________
Int_t TXProofServ::CreateServer()
{
   // Finalize the server setup. If master, create the TProof instance to talk
   // the worker or submaster nodes.
   // Return 0 on success, -1 on error

   Bool_t xtest = (Argc() > 3 && !strcmp(Argv(3), "test")) ? kTRUE : kFALSE;

   if (gProofDebugLevel > 0)
      Info("CreateServer", "starting%s server creation", (xtest ? " test" : ""));

   // Get file descriptor for log file
   if (fLogFile) {
      // Use the file already open by pmain
      if ((fLogFileDes = fileno(fLogFile)) < 0) {
         Error("CreateServer", "resolving the log file description number");
         return -1;
      }
   }

   // Global location string in TXSocket
   TXSocket::fgLoc = (IsMaster()) ? "master" : "slave" ;

   // Set debug level in XrdClient
   EnvPutInt(NAME_DEBUG, gEnv->GetValue("XNet.Debug", 0));

   // Get socket to be used to call back our xpd
   if (xtest) {
      // test session, just send the protocol version on the open pipe
      // and exit
      if (!(fSockPath = gSystem->Getenv("ROOTOPENSOCK"))) {
         Error("CreateServer", "Socket setup by xpd undefined");
         return -1;
      }
      Int_t fpw = (Int_t) strtol(fSockPath.Data(), 0, 10);
      int proto = htonl(kPROOF_Protocol);
      fSockPath = "";
      if (write(fpw, &proto, sizeof(proto)) != sizeof(proto)) {
         Error("CreateServer", "test: sending protocol number");
         return -1;
      }
      exit(0);
   } else {
      fSockPath = gEnv->GetValue("ProofServ.OpenSock", "");
      if (fSockPath.Length() <= 0) {
         Error("CreateServer", "Socket setup by xpd undefined");
         return -1;
      }
      TString entity = gEnv->GetValue("ProofServ.Entity", "");
      if (entity.Length() > 0)
         fSockPath.Insert(0,Form("%s/", entity.Data()));
   }

   // Get the sessions ID
   Int_t psid = gEnv->GetValue("ProofServ.SessionID", -1);
   if (psid < 0) {
     Error("CreateServer", "Session ID undefined");
     return -1;
   }

   // Call back the server
   fSocket = new TXUnixSocket(fSockPath, psid, -1, this);
   if (!fSocket || !(fSocket->IsValid())) {
      Error("CreateServer", "Failed to open connection to XrdProofd coordinator");
      return -1;
   }

   // Set the this as reference of this socket
   ((TXSocket *)fSocket)->fReference = this;

   // Get socket descriptor
   Int_t sock = fSocket->GetDescriptor();

   // Install interrupt and message input handlers
   fInterruptHandler = new TXProofServInterruptHandler(this);
   gSystem->AddSignalHandler(fInterruptHandler);
   fInputHandler =
      TXSocketHandler::GetSocketHandler(new TXProofServInputHandler(this, sock), fSocket);
   gSystem->AddFileHandler(fInputHandler);

   // Get the client ID
   Int_t cid = gEnv->GetValue("ProofServ.ClientID", -1);
   if (cid < 0) {
     Error("CreateServer", "Client ID undefined");
     SendLogFile();
     return -1;
   }
   ((TXSocket *)fSocket)->SetClientID(cid);

   // debug hooks
   if (IsMaster()) {
      // wait (loop) in master to allow debugger to connect
      if (gEnv->GetValue("Proof.GdbHook",0) == 1) {
         while (gProofServDebug)
            ;
      }
   } else {
      // wait (loop) in slave to allow debugger to connect
      if (gEnv->GetValue("Proof.GdbHook",0) == 2) {
         while (gProofServDebug)
            ;
      }
   }

   if (gProofDebugLevel > 0)
      Info("CreateServer", "Service: %s, ConfDir: %s, IsMaster: %d",
           fService.Data(), fConfDir.Data(), (Int_t)fMasterServ);

   if (Setup() == -1) {
      // Setup failure
      Terminate(0);
      SendLogFile();
      return -1;
   }

   if (!fLogFile) {
      RedirectOutput();
      // If for some reason we failed setting a redirection file for the logs
      // we cannot continue
      if (!fLogFile || (fLogFileDes = fileno(fLogFile)) < 0) {
         Terminate(0);
         SendLogFile(-98);
         return -1;
      }
   }

   // Send message of the day to the client
   if (IsMaster()) {
      if (CatMotd() == -1) {
         Terminate(0);
         SendLogFile(-99);
         return -1;
      }
   }

   // Everybody expects iostream to be available, so load it...
   ProcessLine("#include <iostream>", kTRUE);
   ProcessLine("#include <_string>",kTRUE); // for std::string iostream.

   // Allow the usage of ClassDef and ClassImp in interpreted macros
   ProcessLine("#include <RtypesCint.h>", kTRUE);

   // Disallow the interpretation of Rtypes.h, TError.h and TGenericClassInfo.h
   ProcessLine("#define ROOT_Rtypes 0", kTRUE);
   ProcessLine("#define ROOT_TError 0", kTRUE);
   ProcessLine("#define ROOT_TGenericClassInfo 0", kTRUE);

   // Load user functions
   const char *logon;
   logon = gEnv->GetValue("Proof.Load", (char *)0);
   if (logon) {
      char *mac = gSystem->Which(TROOT::GetMacroPath(), logon, kReadPermission);
      if (mac)
         ProcessLine(Form(".L %s", logon), kTRUE);
      delete [] mac;
   }

   // Execute logon macro
   logon = gEnv->GetValue("Proof.Logon", (char *)0);
   if (logon && !NoLogOpt()) {
      char *mac = gSystem->Which(TROOT::GetMacroPath(), logon, kReadPermission);
      if (mac)
         ProcessFile(logon);
      delete [] mac;
   }

   // Save current interpreter context
   gInterpreter->SaveContext();
   gInterpreter->SaveGlobalsContext();

   // if master, start slave servers
   if (IsMaster()) {
      TString master = Form("proof://%s@__master__", fUser.Data());

      // Add port, if defined
      Int_t port = gEnv->GetValue("ProofServ.XpdPort", -1);
      if (port > -1) {
         master += ":";
         master += port;
      }

      // Make sure that parallel startup via threads is not active
      // (it is broken for xpd because of the locks on gCINTMutex)
      gEnv->SetValue("Proof.ParallelStartup", 0);

      // Get plugin manager to load appropriate TProof from
      TPluginManager *pm = gROOT->GetPluginManager();
      if (!pm) {
         Error("CreateServer", "no plugin manager found");
         SendLogFile(-99);
         Terminate(0);
         return -1;
      }

      // Find the appropriate handler
      TPluginHandler *h = pm->FindHandler("TProof", fConfFile);
      if (!h) {
         Error("CreateServer", "no plugin found for TProof with a"
                             " config file of '%s'", fConfFile.Data());
         SendLogFile(-99);
         Terminate(0);
         return -1;
      }

      // load the plugin
      if (h->LoadPlugin() == -1) {
         Error("CreateServer", "plugin for TProof could not be loaded");
         SendLogFile(-99);
         Terminate(0);
         return -1;
      }

      // make instance of TProof
      fProof = reinterpret_cast<TProof*>(h->ExecPlugin(5, master.Data(),
                                                          fConfFile.Data(),
                                                          fConfDir.Data(),
                                                          fLogLevel,
                                                          fSessionTag.Data()));
      if (!fProof || !fProof->IsValid()) {
         Error("CreateServer", "plugin for TProof could not be executed");
         delete fProof;
         fProof = 0;
         SendLogFile(-99);
         Terminate(0);
         return -1;
      }
      // Find out if we are a master in direct contact only with workers
      fEndMaster = fProof->IsEndMaster();

      // Save worker info
      fProof->SaveWorkerInfo();

      SendLogFile();
   }

   // Done
   return 0;
}

//______________________________________________________________________________
TXProofServ::~TXProofServ()
{
   // Cleanup. Not really necessary since after this dtor there is no
   // live anyway.

   delete fSocket;
}

//______________________________________________________________________________
void TXProofServ::HandleUrgentData()
{
   // Handle high priority data sent by the master or client.

   // Real-time notification of messages
   TProofServLogHandlerGuard hg(fLogFile, fSocket, "", fRealTimeLog);

   // Get interrupt
   Int_t iLev = ((TXSocket *)fSocket)->GetInterrupt();
   if (iLev < 0) {
      Error("HandleUrgentData", "error receiving interrupt");
      return;
   }

   PDB(kGlobal, 5)
      Info("HandleUrgentData", "got interrupt: %d\n", iLev);

   if (fProof)
      fProof->SetActive();

   switch (iLev) {

      case TProof::kPing:
         PDB(kGlobal, 5)
            Info("HandleUrgentData", "*** Ping");

         // If master server, propagate interrupt to slaves
         if (IsMaster()) {
            Int_t nbad = fProof->fActiveSlaves->GetSize()-fProof->Ping();
            if (nbad > 0) {
               Info("HandleUrgentData","%d slaves did not reply to ping",nbad);
            }
         }

         // Reply to ping
         ((TXSocket *)fSocket)->Ping();

         // Send log with result of ping
         if (IsMaster())
            SendLogFile();

         break;

      case TProof::kHardInterrupt:
         Info("HandleUrgentData", "*** Hard Interrupt");

         // If master server, propagate interrupt to slaves
         if (IsMaster())
            fProof->Interrupt(TProof::kHardInterrupt);

         // Flush input socket
         ((TXSocket *)fSocket)->Flush();

         if (IsMaster())
            SendLogFile();

         break;

      case TProof::kSoftInterrupt:
         Info("HandleUrgentData", "Soft Interrupt");

         // If master server, propagate interrupt to slaves
         if (IsMaster())
            fProof->Interrupt(TProof::kSoftInterrupt);

         Interrupt();

         if (IsMaster())
            SendLogFile();

         break;


      case TProof::kShutdownInterrupt:
         Info("HandleUrgentData", "Shutdown Interrupt");

         // When retuning for here connection are closed
         HandleTermination();

         break;

      default:
         Error("HandleUrgentData", "unexpected type");
         break;
   }


   if (fProof) fProof->SetActive(kFALSE);
}

//______________________________________________________________________________
void TXProofServ::HandleSigPipe()
{
   // Called when the client is not alive anymore; terminate the session.

   // Real-time notification of messages
   TProofServLogHandlerGuard hg(fLogFile, fSocket, "", fRealTimeLog);

   // If master server, propagate interrupt to slaves
   // (shutdown interrupt send internally).
   if (IsMaster())
      fProof->Close("S");

   Terminate(0);  // will not return from here....
}

//______________________________________________________________________________
void TXProofServ::HandleTermination()
{
   // Called when the client is not alive anymore; terminate the session.

   // If master server, propagate interrupt to slaves
   // (shutdown interrupt send internally).
   if (IsMaster()) {

      // If not idle, try first to stop processing
      if (!fIdle) {
         // Remove pending requests
         fWaitingQueries->Delete();
         // Interrupt the current monitor
         fProof->InterruptCurrentMonitor();
         // Do not wait for ever, but al least 20 seconds
         Long_t timeout = gEnv->GetValue("Proof.ShutdownTimeout", 60);
         timeout = (timeout > 20) ? timeout : 20;
         // Processing will be aborted
         fProof->StopProcess(kTRUE, (Long_t) (timeout / 2));
         // Receive end-of-processing messages, but do not wait for ever
         fProof->Collect(TProof::kActive, timeout);
         // Still not idle
         if (!fIdle)
            Warning("HandleTermination","processing could not be stopped");
      }
      // Close the session
      if (fProof)
         fProof->Close("S");
   }

   Terminate(0);  // will not return from here....
}

//______________________________________________________________________________
Int_t TXProofServ::Setup()
{
   // Print the ProofServ logo on standard output.
   // Return 0 on success, -1 on error

   char str[512];

   if (IsMaster()) {
      sprintf(str, "**** Welcome to the PROOF server @ %s ****", gSystem->HostName());
   } else {
      sprintf(str, "**** PROOF worker server @ %s started ****", gSystem->HostName());
   }

   if (fSocket->Send(str) != 1+static_cast<Int_t>(strlen(str))) {
      Error("Setup", "failed to send proof server startup message");
      return -1;
   }

   // Get client protocol
   if ((fProtocol = gEnv->GetValue("ProofServ.ClientVersion", -1)) < 0) {
      Error("Setup", "remote proof protocol missing");
      return -1;
   }

   // The local user
   fUser = gEnv->GetValue("ProofServ.Entity", "");
   if (fUser.Length() >= 0) {
      if (fUser.Contains(":"))
         fUser.Remove(fUser.Index(":"));
      if (fUser.Contains("@"))
         fUser.Remove(fUser.Index("@"));
   } else {
      UserGroup_t *pw = gSystem->GetUserInfo();
      if (pw) {
         fUser = pw->fUser;
         delete pw;
      }
   }

   // Work dir and ...
   if (IsMaster()) {
      TString cf = gEnv->GetValue("ProofServ.ProofConfFile", "");
      if (cf.Length() > 0)
         fConfFile = cf;
   }
   fWorkDir = gEnv->GetValue("ProofServ.Sandbox", kPROOF_WorkDir);

   // Get Session tag
   if ((fSessionTag = gEnv->GetValue("ProofServ.SessionTag", "-1")) == "-1") {
      Error("Setup", "Session tag missing");
      return -1;
   }
   if (gProofDebugLevel > 0)
      Info("Setup", "session tag is %s", fSessionTag.Data());

   // Get Session dir (sandbox)
   if ((fSessionDir = gEnv->GetValue("ProofServ.SessionDir", "-1")) == "-1") {
      Error("Setup", "Session dir missing");
      return -1;
   }

   // Goto to the main PROOF working directory
   char *workdir = gSystem->ExpandPathName(fWorkDir.Data());
   fWorkDir = workdir;
   delete [] workdir;
   if (gProofDebugLevel > 0)
      Info("Setup", "working directory set to %s", fWorkDir.Data());

   // Common setup
   if (SetupCommon() != 0) {
      Error("Setup", "common setup failed");
      return -1;
   }

   // Install SigPipe handler to handle kKeepAlive failure
   gSystem->AddSignalHandler(new TXProofServSigPipeHandler(this));

   // Install Termination handler
   gSystem->AddSignalHandler(new TXProofServTerminationHandler(this));

   // Install seg violation handler
   gSystem->AddSignalHandler(new TXProofServSegViolationHandler(this));

   // Done
   return 0;
}

//______________________________________________________________________________
void TXProofServ::SendLogFile(Int_t status, Int_t start, Int_t end)
{
   // Send log file to master.
   // If start > -1 send only bytes in the range from start to end,
   // if end <= start send everything from start.

   // Determine the number of bytes left to be read from the log file.
   fflush(stdout);

   off_t ltot, lnow;
   Int_t left;

   ltot = lseek(fileno(stdout),   (off_t) 0, SEEK_END);
   lnow = lseek(fLogFileDes, (off_t) 0, SEEK_CUR);

   Bool_t adhoc = kFALSE;
   if (start > -1) {
      lseek(fLogFileDes, (off_t) start, SEEK_SET);
      if (end <= start || end > ltot)
         end = ltot;
      left = (Int_t)(end - start);
      if (end < ltot)
         left++;
      adhoc = kTRUE;
   } else {
      left = (Int_t)(ltot - lnow);
   }

   if (left > 0) {
      fSocket->Send(left, kPROOF_LOGFILE);

      const Int_t kMAXBUF = 32768;  //16384  //65536;
      char buf[kMAXBUF];
      Int_t wanted = (left > kMAXBUF) ? kMAXBUF : left;
      Int_t len;
      do {
         while ((len = read(fLogFileDes, buf, wanted)) < 0 &&
                TSystem::GetErrno() == EINTR)
            TSystem::ResetErrno();

         if (len < 0) {
            SysError("SendLogFile", "error reading log file");
            break;
         }

         if (end == ltot && len == wanted)
            buf[len-1] = '\n';

         if (fSocket->SendRaw(buf, len, kDontBlock) < 0) {
            SysError("SendLogFile", "error sending log file");
            break;
         }

         // Update counters
         left -= len;
         wanted = (left > kMAXBUF) ? kMAXBUF : left;

      } while (len > 0 && left > 0);
   }

   // Restore initial position if partial send
   if (adhoc)
      lseek(fLogFileDes, lnow, SEEK_SET);

   TMessage mess(kPROOF_LOGDONE);
   if (IsMaster())
      mess << status << (fProof ? fProof->GetParallel() : 0);
   else
      mess << status << (Int_t) 1;

   fSocket->Send(mess);
}

//______________________________________________________________________________
TProofServ::EQueryAction TXProofServ::GetWorkers(TList *workers,
                                                 Int_t & /* prioritychange */)
{
   // Get list of workers to be used from now on.
   // The list must be provide by the caller.

   // Needs a list where to store the info
   if (!workers) {
      Error("GetWorkers", "output list undefined");
      return kQueryStop;
   }

   // If user config files are enabled, check them first
   if (gEnv->GetValue("ProofServ.UseUserCfg", 0) != 0) {
      Int_t pc = 1;
      TProofServ::EQueryAction rc = TProofServ::GetWorkers(workers, pc);
      if (rc == kQueryOK)
         return rc;
   }

   // Send request to the coordinator
   TObjString *os = ((TXSocket *)fSocket)->SendCoordinator(TXSocket::kGetWorkers);

   // The reply contains some information about the master (image, workdir)
   // followed by the information about the workers; the tokens for each node
   // are separated by '&'
   if (os) {
      TString fl(os->GetName());
      TString tok;
      Ssiz_t from = 0;
      if (fl.Tokenize(tok, from, "&")) {
         if (!tok.IsNull()) {
            TProofNodeInfo *master = new TProofNodeInfo(tok);
            if (!master) {
               Error("GetWorkers", "no appropriate master line got from coordinator");
               return kQueryStop;
            } else {
               // Set image if not yet done and available
               if (fImage.IsNull() && strlen(master->GetImage()) > 0)
                  fImage = master->GetImage();
               SafeDelete(master);
            }
            // Now the workers
            while (fl.Tokenize(tok, from, "&")) {
               if (!tok.IsNull())
                  workers->Add(new TProofNodeInfo(tok));
            }
         }
      }
   }

   // We are done
   return kQueryOK;
}

//_____________________________________________________________________________
Bool_t TXProofServ::HandleError(const void *)
{
   // Handle error on the input socket

   Printf("TXProofServ::HandleError: %p: got called ...", this);

   // If master server, propagate interrupt to slaves
   // (shutdown interrupt send internally).
   if (IsMaster())
      fProof->Close("S");

   // Avoid communicating back anything to the coordinator (it is gone)
   ((TXSocket *)fSocket)->SetSessionID(-1);

   Terminate(0);

   Printf("TXProofServ::HandleError: %p: DONE ... ", this);

   // We are done
   return kTRUE;
}

//_____________________________________________________________________________
Bool_t TXProofServ::HandleInput(const void *in)
{
   // Handle asynchronous input on the input socket

   if (gDebug > 2)
      Printf("TXProofServ::HandleInput %p, in: %p", this, in);

   XHandleIn_t *hin = (XHandleIn_t *) in;
   Int_t acod = (hin) ? hin->fInt1 : kXPD_msg;

   // Act accordingly
   if (acod == kXPD_ping || acod == kXPD_interrupt) {
      // Interrupt or Ping
      HandleUrgentData();

   } else if (acod == kXPD_timer) {
      // Shutdown option
      fShutdownWhenIdle = (hin->fInt2 == 2) ? kFALSE : kTRUE;
      if (hin->fInt2 > 0)
         // Setup Shutdown timer
         SetShutdownTimer(kTRUE, hin->fInt3);
      else
         // Stop Shutdown timer, if any
         SetShutdownTimer(kFALSE);

   } else if (acod == kXPD_flush) {
      // Flush stdout, so that we can access the full log file
      Info("HandleInput","kXPD_flush: flushing log file (stdout)");
      fflush(stdout);

   } else if (acod == kXPD_urgent) {
      // Get type
      Int_t type = hin->fInt2;
      switch (type) {
      case TXSocket::kStopProcess:
         {
            // Abort or Stop ?
            Bool_t abort = (hin->fInt3 != 0) ? kTRUE : kFALSE;
            // Timeout
            Int_t timeout = hin->fInt4;
            // Act now
            if (fProof)
               fProof->StopProcess(abort, timeout);
            else
               if (fPlayer)
                  fPlayer->StopProcess(abort, timeout);
         }
         break;
      default:
         Info("HandleInput","kXPD_urgent: unknown type: %d", type);
      }

   } else if (acod == kXPD_inflate) {

      // Set inflate factor
      fInflateFactor = (hin->fInt2 >= 1000) ? hin->fInt2 : fInflateFactor;
      // Notify
      Info("HandleInput", "kXPD_inflate: inflate factor set to %f",
           (Float_t) fInflateFactor / 1000.);

   } else if (acod == kXPD_priority) {

      // The factor is the priority to be propagated
      fGroupPriority = hin->fInt2;
      if (fProof)
         fProof->BroadcastGroupPriority(fGroup, fGroupPriority);
      // Notify
      Info("HandleInput", "kXPD_priority: group %s priority set to %f",
           fGroup.Data(), (Float_t) fGroupPriority / 100.);

   } else {
      // Standard socket input
      HandleSocketInput();
      // This request has been completed: remove the client ID from the pipe
      ((TXSocket *)fSocket)->RemoveClientID();
   }

   // We are done
   return kTRUE;
}

//______________________________________________________________________________
void TXProofServ::DisableTimeout()
{
   // Disable read timeout on the underlying socket

   if (fSocket)
     ((TXSocket *)fSocket)->DisableTimeout();
}

//______________________________________________________________________________
void TXProofServ::EnableTimeout()
{
   // Enable read timeout on the underlying socket

   if (fSocket)
     ((TXSocket *)fSocket)->EnableTimeout();
}

//______________________________________________________________________________
void TXProofServ::Terminate(Int_t status)
{
   // Terminate the proof server.
   if (fTerminated)
      // Avoid doubling the exit operations
      exit(1);
   fTerminated = kTRUE;

   // Notify
   Info("Terminate", "starting session termination operations ...");

   // Deactivate current monitor, if any
   if (fProof)
      fProof->SetMonitor(0, kFALSE);

   // Cleanup session directory
   if (status == 0) {
      // make sure we remain in a "connected" directory
      gSystem->ChangeDirectory("/");
      // needed in case fSessionDir is on NFS ?!
      gSystem->MakeDirectory(fSessionDir+"/.delete");
      gSystem->Exec(Form("%s %s", kRM, fSessionDir.Data()));
   }

   // Cleanup queries directory if empty
   if (IsMaster()) {
      if (!(fQueries->GetSize())) {
         // make sure we remain in a "connected" directory
         gSystem->ChangeDirectory("/");
         // needed in case fQueryDir is on NFS ?!
         gSystem->MakeDirectory(fQueryDir+"/.delete");
         gSystem->Exec(Form("%s %s", kRM, fQueryDir.Data()));
         // Remove lock file
         if (fQueryLock)
            gSystem->Unlink(fQueryLock->GetName());
       }

      // Unlock the query dir owned by this session
      if (fQueryLock)
         fQueryLock->Unlock();
   }

   // Remove input and signal handlers to avoid spurious "signals"
   // for closing activities executed upon exit()
   gSystem->RemoveFileHandler(fInputHandler);
   gSystem->RemoveSignalHandler(fInterruptHandler);

   // Stop processing events (set a flag to exit the event loop)
   gSystem->ExitLoop();

   // We post the pipe once to wake up the main thread which is waiting for
   // activity on this socket; this fake activity will make it return and
   // eventually exit the loop.
   TXSocket::PostPipe((TXSocket *)fSocket);

   // Avoid communicating back anything to the coordinator (it is gone)
   ((TXSocket *)fSocket)->SetSessionID(-1);

   // Notify
   Printf("Terminate: termination operations ended: quitting!");
}

//______________________________________________________________________________
Int_t TXProofServ::LockSession(const char *sessiontag, TProofLockPath **lck)
{
   // Try locking query area of session tagged sessiontag.
   // The id of the locking file is returned in fid and must be
   // unlocked via UnlockQueryFile(fid).

   // We do not need to lock our own session
   if (strstr(sessiontag, fSessionTag))
      return 0;

   if (!lck) {
      Info("LockSession","locker space undefined");
      return -1;
   }
   *lck = 0;

   // Check the format
   TString stag = sessiontag;
   TRegexp re("session-.*-.*-.*");
   Int_t i1 = stag.Index(re);
   if (i1 == kNPOS) {
      Info("LockSession","bad format: %s", sessiontag);
      return -1;
   }
   stag.ReplaceAll("session-","");

   // Drop query number, if any
   Int_t i2 = stag.Index(":q");
   if (i2 != kNPOS)
      stag.Remove(i2);

   // Make sure that parent process does not exist anylonger
   TString parlog = fSessionDir;
   parlog = parlog.Remove(parlog.Index("master-")+strlen("master-"));
   parlog += stag;
   if (!gSystem->AccessPathName(parlog)) {
      Info("LockSession","parent still running: do nothing");
      return -1;
   }

   // Lock the query lock file
   TString qlock = fQueryLock->GetName();
   qlock.ReplaceAll(fSessionTag, stag);

   if (!gSystem->AccessPathName(qlock)) {
      *lck = new TProofLockPath(qlock);
      if (((*lck)->Lock()) < 0) {
         Info("LockSession","problems locking query lock file");
         SafeDelete(*lck);
         return -1;
      }
   }

   // We are done
   return 0;
}

//______________________________________________________________________________
void TXProofServ::SetShutdownTimer(Bool_t on, Int_t delay)
{
   // Enable/disable the timer for delayed shutdown; the delay will be 'delay'
   // seconds; depending on fShutdownWhenIdle, the countdown will start
   // immediately or when the session is idle.

   R__LOCKGUARD(fShutdownTimerMtx);

   if (delay < 0 && !fShutdownTimer)
      // No shutdown request, nothing to do
      return;

   // Make sure that 'delay' make sense, i.e. not larger than 10 days
   if (delay > 864000) {
      Warning("SetShutdownTimer",
              "abnormous delay value (%d): corruption? setting to 0", delay);
      delay = 1;
   }
   // Set a minimum value (0 does not seem to start the timer ...)
   Int_t del = (delay <= 0) ? 10 : delay * 1000;

   if (on) {
      if (!fShutdownTimer) {
         // First setup call: create timer
         fShutdownTimer = new TShutdownTimer(this, del);
         // Start the countdown if requested
         if (!fShutdownWhenIdle || fIdle)
            fShutdownTimer->Start(-1, kTRUE);
      } else {
         // Start the countdown
         fShutdownTimer->Start(-1, kTRUE);
      }
      // Notify
      Info("SetShutdownTimer",
              "session will be shutdown in %d seconds (%d millisec)", delay, del);
   } else {
      if (fShutdownTimer) {
         // Stop and Clean-up the timer
         SafeDelete(fShutdownTimer);
         // Notify
         Info("SetShutdownTimer", "shutdown countdown timer stopped: resuming session");
      } else {
         // Notify
         Info("SetShutdownTimer", "shutdown countdown timer never started - do nothing");
      }
   }

   // To avoid having the client notified about this at reconnection
   FlushLogFile();
}

