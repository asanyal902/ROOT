// @(#)root/proof:$Name:  $:$Id: TProof.cxx,v 1.46 2003/06/12 05:34:05 rdm Exp $
// Author: Fons Rademakers   13/02/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProof                                                               //
//                                                                      //
// This class controls a Parallel ROOT Facility, PROOF, cluster.        //
// It fires the slave servers, it keeps track of how many slaves are    //
// running, it keeps track of the slaves running status, it broadcasts  //
// messages to all slaves, it collects results, etc.                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#   include <io.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#else
#   include <unistd.h>
#endif

#include "TProof.h"
#include "TAuthenticate.h"
#include "TSortedList.h"
#include "TSlave.h"
#include "TSocket.h"
#include "TMonitor.h"
#include "TMessage.h"
#include "TSystem.h"
#include "TError.h"
#include "TUrl.h"
#include "TFTP.h"
#include "TROOT.h"
#include "TH1.h"
#include "TProofPlayer.h"
#include "TDSet.h"
#include "TEnv.h"
#include "TPluginManager.h"


//----- PROOF Interrupt signal handler -----------------------------------------------
//______________________________________________________________________________
class TProofInterruptHandler : public TSignalHandler {
private:
   TProof *fProof;
public:
   TProofInterruptHandler(TProof *p)
      : TSignalHandler(kSigInterrupt, kFALSE), fProof(p) { }
   Bool_t Notify();
};

//______________________________________________________________________________
Bool_t TProofInterruptHandler::Notify()
{
   // TProof interrupt handler.

   fProof->Interrupt(TProof::kHardInterrupt);
   return kTRUE;
}

//----- Input handler for messages from TProofServ -----------------------------
//______________________________________________________________________________
class TProofInputHandler : public TFileHandler {
private:
   TSocket *fSocket;
   TProof  *fProof;
public:
   TProofInputHandler(TProof *p, TSocket *s)
      : TFileHandler(s->GetDescriptor(), 1) { fProof = p; fSocket = s; }
   Bool_t Notify();
   Bool_t ReadNotify() { return Notify(); }
};

//______________________________________________________________________________
Bool_t TProofInputHandler::Notify()
{
   fProof->HandleAsyncInput(fSocket);
   return kTRUE;
}


ClassImp(TProof)

//______________________________________________________________________________
TProof::TProof(const char *masterurl, const char *conffile,
               const char *confdir, Int_t loglevel)
{
   // Create a PROOF environment. Starting PROOF involves either connecting
   // to a master server, which in turn will start a set of slave servers, or
   // directly starting as master server (if master = ""). Masterurl is of
   // the form: proof://host[:port] or proofs://host[:port]. Conffile is
   // the name of the config file describing the remote PROOF cluster
   // (this argument alows you to describe different cluster configurations).
   // The default proof.conf. Confdir is the directory where the config
   // file and other PROOF related files are (like motd and noproof files).
   // Loglevel is the log level (default = 1).

   if (!conffile)
      conffile = kPROOF_ConfFile;
   if (!confdir)
      confdir = kPROOF_ConfDir;

   // Can have only one PROOF session open at a time.
   if (gProof) {
      Warning("TProof", "closing currently open PROOF session");
      gProof->Close();
   }

   if (Init(masterurl, conffile, confdir, loglevel) == 0) {
      // on Init failure make sure IsValid() returns kFALSE
      SafeDelete(fActiveSlaves);
   }

   gProof = this;
}

//______________________________________________________________________________
TProof::~TProof()
{
   // Clean up PROOF environment.

   Close();

   SafeDelete(fIntHandler);
   SafeDelete(fSlaves);
   SafeDelete(fActiveSlaves);
   SafeDelete(fUniqueSlaves);
   SafeDelete(fBadSlaves);
   SafeDelete(fAllMonitor);
   SafeDelete(fActiveMonitor);
   SafeDelete(fUniqueMonitor);

   gProof = 0;
}

//______________________________________________________________________________
Int_t TProof::Init(const char *masterurl, const char *conffile,
                   const char *confdir, Int_t loglevel)
{
   // Start the PROOF environment. Starting PROOF involves either connecting
   // to a master server, which in turn will start a set of slave servers, or
   // directly starting as master server (if master = ""). For a description
   // of the arguments see the TProof ctor.

   Assert(gSystem);

   TUrl *u;
   if (!masterurl || !*masterurl)
      u = new TUrl("proof://__master__");
   else if (strstr(masterurl, "://"))
      u = new TUrl(masterurl);
   else
      u = new TUrl(Form("proof://%s", masterurl));

   fMaster         = u->GetHost();
   fPort           = u->GetPort();
   fConfDir        = confdir;
   fConfFile       = conffile;
   fWorkDir        = gSystem->WorkingDirectory();
   fLogLevel       = loglevel;
   fProtocol       = kPROOF_Protocol;
   fMasterServ     = fMaster == "__master__" ? kTRUE : kFALSE;
   fSendGroupView  = kTRUE;
   fImage          = fMasterServ ? "" : "<local>";
   fIntHandler     = 0;
   fProgressDialog = 0;
   fStatus         = 0;
   fParallel       = 0;
   fPlayer         = 0;
   fSecurity       = gEnv->GetValue("Proofd.Authentication", TAuthenticate::kClear);
   if (!strcmp(u->GetProtocol(), "proofs"))
      fSecurity = TAuthenticate::kSRP;
   if (!strcmp(u->GetProtocol(), "proofk"))
      fSecurity = TAuthenticate::kKrb5;

   delete u;

   // global logging
   gProofDebugLevel = fLogLevel;
   gProofDebugMask  = TProofDebug::kAll;

   // sort slaves by descending performance index
   fSlaves        = new TSortedList(kSortDescending);
   fActiveSlaves  = new TList;
   fUniqueSlaves  = new TList;
   fBadSlaves     = new TList;
   fAllMonitor    = new TMonitor;
   fActiveMonitor = new TMonitor;
   fUniqueMonitor = new TMonitor;

   // If this is a master server, find the config file and start slave
   // servers as specified in the config file
   if (IsMaster()) {

      // set in TProofServ
      fUser   = TAuthenticate::GetGlobalUser();
      fPasswd = TAuthenticate::GetGlobalPasswd();

      char fconf[256];
      sprintf(fconf, "%s/.%s", gSystem->Getenv("HOME"), conffile);
      if (gSystem->AccessPathName(fconf, kReadPermission)) {
          sprintf(fconf, "%s/proof/etc/%s", confdir, conffile);
         if (gSystem->AccessPathName(fconf, kReadPermission)) {
            Error("Init", "no PROOF config file found");
            return 0;
         }
      }

      PDB(kGlobal,1) Info("Init", "using PROOF config file: %s", fconf);

      FILE *pconf;
      if ((pconf = fopen(fconf, "r"))) {

         fConfFile = fconf;

         // read the config file
         char line[256];
         TString host = gSystem->GetHostByName(gSystem->HostName()).GetHostName();
         int  ord = 0;

         while (fgets(line, sizeof(line), pconf)) {
            char word[7][64];
            if (line[0] == '#') continue;   // skip comment lines
            int nword = sscanf(line, "%s %s %s %s %s %s %s", word[0], word[1],
                word[2], word[3], word[4], word[5], word[6]);

            // see if master may run on this node
            if (nword >= 2 && !strcmp(word[0], "node") && !fImage.Length()) {
               TInetAddress a = gSystem->GetHostByName(word[1]);
               if (!host.CompareTo(a.GetHostName()) ||
                   !strcmp(word[1], "localhost")) {
                  char *image = word[1];
                  if (nword > 2 && !strncmp(word[2], "image=", 6))
                     image = word[2]+6;
                  fImage = image;
               }
            }
            // find all slave servers
            if (nword >= 2 && !strcmp(word[0], "slave")) {
               int perfidx  = 100;
               int sport    = fPort;
               int security = gEnv->GetValue("Proofd.Authentication",
                                             TAuthenticate::kClear);
               const char *image = word[1];
               for (int i = 2; i < nword; i++) {
                  if (!strncmp(word[i], "perf=", 5))
                     perfidx = atoi(word[i]+5);
                  if (!strncmp(word[i], "image=", 6))
                     image = word[i]+6;
                  if (!strncmp(word[i], "port=", 5))
                     sport = atoi(word[i]+5);
                  if (!strncmp(word[i], "srp", 3))
                     security = TAuthenticate::kSRP;
                  if (!strncmp(word[i], "krb5", 3))
                     security = TAuthenticate::kKrb5;
               }
               // create slave server
               TSlave *slave = new TSlave(word[1], sport, ord++, perfidx,
                                          image, security, this);
               fSlaves->Add(slave);
               if (slave->IsValid()) {
                  fAllMonitor->Add(slave->GetSocket());
                  slave->SetInputHandler(new TProofInputHandler(this,
                                         slave->GetSocket()));
               } else
                  fBadSlaves->Add(slave);
            }
         }
      }
      fclose(pconf);

      if (fImage.Length() == 0) {
         Error("Init", "no appropriate node line found in %s", fconf);
         return 0;
      }
   } else {
      // create master server
      TSlave *slave = new TSlave(fMaster, fPort, 0, 100, "master",
                                 fSecurity, this);
      if (slave->IsValid()) {
         // check protocol compatability
         // protocol 1 is not supported anymore
         if (fProtocol == 1) {
            Error("Init", "client and remote protocols not compatible (%d and %d)",
                  fProtocol, kPROOF_Protocol);
            delete slave;
            return 0;
         }

         fSlaves->Add(slave);
         fAllMonitor->Add(slave->GetSocket());
         Collect(slave);
         if (fStatus == -99) {
            Error("Init", "not allowed to connect to PROOF master server");
            return 0;
         }

         slave->SetInputHandler(new TProofInputHandler(this, slave->GetSocket()));

         fIntHandler = new TProofInterruptHandler(this);
         fIntHandler->Add();

         if (!gROOT->IsBatch()) {
            if ((fProgressDialog = gROOT->GetPluginManager()->FindHandler("TProofProgressDialog")))
               if (fProgressDialog->LoadPlugin() == -1)
                  fProgressDialog = 0;
         }
      } else {
         delete slave;
         Error("Init", "failed to connect to a PROOF master server");
         return 0;
      }
   }

   // De-activate monitor (will be activated in Collect)
   fAllMonitor->DeActivateAll();

   // By default go into parallel mode
   GoParallel(9999);

   // Send relevant initial state to slaves
   SendInitialState();

   if (IsValid())
      gROOT->GetListOfSockets()->Add(this);

   return fActiveSlaves->GetSize();
}

//______________________________________________________________________________
void TProof::Close(Option_t *)
{
   // Close all open slave servers.

   if (fSlaves) {
      if (fIntHandler)
         fIntHandler->Remove();

      // tell master and slaves to stop
      if (!IsMaster())
         Interrupt(kShutdownInterrupt, kAll);

      fSlaves->Delete();
      if (fActiveSlaves) fActiveSlaves->Clear(); // is 0 if Init() returned 0
      fUniqueSlaves->Clear();
      fBadSlaves->Clear();
   }
}

//______________________________________________________________________________
TSlave *TProof::FindSlave(TSocket *s) const
{
   // Find slave that has TSocket s. Returns 0 in case slave is not found.

   TSlave *sl;
   TIter   next(fSlaves);

   while ((sl = (TSlave *)next())) {
      if (sl->IsValid() && sl->GetSocket() == s)
         return sl;
   }
   return 0;
}

//______________________________________________________________________________
void TProof::FindUniqueSlaves()
{
   // Add to the fUniqueSlave list the active slaves that have a unique
   // (user) file system image. This information is used to transfer files
   // only once to nodes that share a file system (an image).

   fUniqueSlaves->Clear();
   fUniqueMonitor->RemoveAll();

   TIter next(fActiveSlaves);

   TSlave *sl;
   while ((sl = (TSlave *)next())) {
      if (fImage == sl->fImage) continue;
      TIter next2(fUniqueSlaves);
      TSlave *sl2;
      Int_t   add = fUniqueSlaves->IsEmpty() ? 1 : 0;
      while ((sl2 = (TSlave *)next2())) {
         if (sl->fImage == sl2->fImage) {
            add = 0;
            break;
         }
         add++;
      }
      if (add) {
         fUniqueSlaves->Add(sl);
         fUniqueMonitor->Add(sl->GetSocket());
      }
   }

   // will be actiavted in Collect()
   fUniqueMonitor->DeActivateAll();
}

//______________________________________________________________________________
Int_t TProof::GetNumberOfSlaves() const
{
   // Return number of slaves as described in the config file.

   if (!fSlaves) return 0;
   return fSlaves->GetSize();
}

//______________________________________________________________________________
Int_t TProof::GetNumberOfActiveSlaves() const
{
   // Return number of active slaves, i.e. slaves that are valid and in
   // the current computing group.

   if (!fActiveSlaves) return 0;
   return fActiveSlaves->GetSize();
}

//______________________________________________________________________________
Int_t TProof::GetNumberOfUniqueSlaves() const
{
   // Return number of unique slaves, i.e. active slaves that have each a
   // unique different user files system.

   if (!fUniqueSlaves) return 0;
   return fUniqueSlaves->GetSize();
}

//______________________________________________________________________________
Int_t TProof::GetNumberOfBadSlaves() const
{
   // Return number of bad slaves. This are slaves that we in the config
   // file, but refused to startup or that died during the PROOF session.

   if (!fBadSlaves) return 0;
   return fBadSlaves->GetSize();
}

//______________________________________________________________________________
void TProof::AskStatus()
{
   // Ask the status of the slaves.

   if (!IsValid()) return;

   Broadcast(kPROOF_STATUS, kAll);
   Collect(kAll);
}

//______________________________________________________________________________
void TProof::Interrupt(EUrgent type, ESlaves list)
{
   // Send interrupt OOB byte to master or slave servers.

   if (!IsValid()) return;

   char oobc = (char) type;

   TList *slaves = 0;
   if (list == kAll)    slaves = fSlaves;
   if (list == kActive) slaves = fActiveSlaves;
   if (list == kUnique) slaves = fUniqueSlaves;

   if (slaves->GetSize() == 0) return;

   const int kBufSize = 1024;
   char waste[kBufSize];

   TSlave *sl;
   TIter   next(slaves);

   while ((sl = (TSlave *)next())) {
      if (sl->IsValid()) {
         TSocket *s = sl->GetSocket();

         // Send one byte out-of-band message to server
         if (s->SendRaw(&oobc, 1, kOob) <= 0) {
            Error("Interrupt", "error sending oobc to slave %d", sl->GetOrdinal());
            continue;
         }

         if (type == kHardInterrupt) {
            char  oob_byte;
            int   n, nch, nbytes = 0, nloop = 0;

            // Receive the OOB byte
            while ((n = s->RecvRaw(&oob_byte, 1, kOob)) < 0) {
               if (n == -2) {   // EWOULDBLOCK
                  //
                  // The OOB data has not yet arrived: flush the input stream
                  //
                  // In some systems (Solaris) regular recv() does not return upon
                  // receipt of the oob byte, which makes the below call to recv()
                  // block indefinitely if there are no other data in the queue.
                  // FIONREAD ioctl can be used to check if there are actually any
                  // data to be flushed.  If not, wait for a while for the oob byte
                  // to arrive and try to read it again.
                  //
                  s->GetOption(kBytesToRead, nch);
                  if (nch == 0) {
                     gSystem->Sleep(1000);
                     continue;
                  }

                  if (nch > kBufSize) nch = kBufSize;
                  n = s->RecvRaw(waste, nch);
                  if (n <= 0) {
                     Error("Interrupt", "error receiving waste from slave %d",
                           sl->GetOrdinal());
                     break;
                  }
                  nbytes += n;
               } else if (n == -3) {   // EINVAL
                  //
                  // The OOB data has not arrived yet
                  //
                  gSystem->Sleep(100);
                  if (++nloop > 100) {  // 10 seconds time-out
                     Error("Interrupt", "server %d does not respond", sl->GetOrdinal());
                     break;
                  }
               } else {
                  Error("Interrupt", "error receiving OOB from server %d",
                        sl->GetOrdinal());
                  break;
               }
            }

            //
            // Continue flushing the input socket stream until the OOB
            // mark is reached
            //
            while (1) {
               int atmark;

               s->GetOption(kAtMark, atmark);

               if (atmark)
                  break;

               // find out number of bytes to read before atmark
               s->GetOption(kBytesToRead, nch);
               if (nch == 0) {
                  gSystem->Sleep(1000);
                  continue;
               }

               if (nch > kBufSize) nch = kBufSize;
               n = s->RecvRaw(waste, nch);
               if (n <= 0) {
                  Error("Interrupt", "error receiving waste (2) from slave %d",
                        sl->GetOrdinal());
                  break;
               }
               nbytes += n;
            }
            if (nbytes > 0) {
               if (IsMaster())
                  Printf("*** Slave %s:%d synchronized: %d bytes discarded",
                         sl->GetName(), sl->GetOrdinal(), nbytes);
               else
                  Printf("*** PROOF synchronized: %d bytes discarded", nbytes);
            }

            // Get log file from master or slave after a hard interrupt
            Collect(sl);

         } else if (type == kSoftInterrupt) {

            // Get log file from master or slave after a soft interrupt
            Collect(sl);

         } else if (type == kShutdownInterrupt) {

            ; // nothing expected to be returned

         } else {

            // Unexpected message, just receive log file
            Collect(sl);

         }
      }
   }
}

//______________________________________________________________________________
Int_t TProof::GetParallel() const
{
   // Returns number of slaves active in parallel mode. Returns 0 in case
   // there are no active slaves.

   if (!IsValid()) return 0;

   if (IsMaster())
      return GetNumberOfActiveSlaves();

   return fParallel;
}

//______________________________________________________________________________
Int_t TProof::Broadcast(const TMessage &mess, TList *slaves)
{
   // Broadcast a message to all slaves in the specified list. Returns
   // the number of slaves the message was successfully sent to.

   if (!IsValid()) return 0;

   if (slaves->GetSize() == 0) return 0;

   int   nsent = 0;
   TIter next(slaves);

   TSlave *sl;
   while ((sl = (TSlave *)next())) {
      if (sl->IsValid()) {
         if (sl->GetSocket()->Send(mess) == -1)
            MarkBad(sl);
         else
            nsent++;
      }
   }

   return nsent;
}

//______________________________________________________________________________
Int_t TProof::Broadcast(const TMessage &mess, ESlaves list)
{
   // Broadcast a message to all slaves in the specified list (either
   // all slaves or only the active slaves). Returns the number of slaves
   // the message was successfully sent to.

   if (!IsValid()) return 0;

   TList *slaves = 0;
   if (list == kAll)    slaves = fSlaves;
   if (list == kActive) slaves = fActiveSlaves;
   if (list == kUnique) slaves = fUniqueSlaves;

   return Broadcast(mess, slaves);
}

//______________________________________________________________________________
Int_t TProof::Broadcast(const char *str, Int_t kind, TList *slaves)
{
   // Broadcast a character string buffer to all slaves in the specified
   // list. Use kind to set the TMessage what field. Returns the number of
   // slaves the message was sent to.

   TMessage mess(kind);
   if (str) mess.WriteString(str);
   return Broadcast(mess, slaves);
}

//______________________________________________________________________________
Int_t TProof::Broadcast(const char *str, Int_t kind, ESlaves list)
{
   // Broadcast a character string buffer to all slaves in the specified
   // list (either all slaves or only the active slaves). Use kind to
   // set the TMessage what field. Returns the number of slaves the message
   // was sent to.

   TMessage mess(kind);
   if (str) mess.WriteString(str);
   return Broadcast(mess, list);
}

//______________________________________________________________________________
Int_t TProof::BroadcastObject(const TObject *obj, Int_t kind, TList *slaves)
{
   // Broadcast an object to all slaves in the specified list. Use kind to
   // set the TMEssage what field. Returns the number of slaves the message
   // was sent to.

   TMessage mess(kind);
   mess.WriteObject(obj);
   return Broadcast(mess, slaves);
}

//______________________________________________________________________________
Int_t TProof::BroadcastObject(const TObject *obj, Int_t kind, ESlaves list)
{
   // Broadcast an object to all slaves in the specified list. Use kind to
   // set the TMEssage what field. Returns the number of slaves the message
   // was sent to.

   TMessage mess(kind);
   mess.WriteObject(obj);
   return Broadcast(mess, list);
}

//______________________________________________________________________________
Int_t TProof::BroadcastRaw(const void *buffer, Int_t length, TList *slaves)
{
   // Broadcast a raw buffer of specified length to all slaves in the
   // specified list. Returns the number of slaves the buffer was sent to.

   if (!IsValid()) return 0;

   if (slaves->GetSize() == 0) return 0;

   int   nsent = 0;
   TIter next(slaves);

   TSlave *sl;
   while ((sl = (TSlave *)next())) {
      if (sl->IsValid()) {
         if (sl->GetSocket()->SendRaw(buffer, length) == -1)
            MarkBad(sl);
         else
            nsent++;
      }
   }

   return nsent;
}

//______________________________________________________________________________
Int_t TProof::BroadcastRaw(const void *buffer, Int_t length, ESlaves list)
{
   // Broadcast a raw buffer of specified length to all slaves in the
   // specified list. Returns the number of slaves the buffer was sent to.

   if (!IsValid()) return 0;

   TList *slaves = 0;
   if (list == kAll)    slaves = fSlaves;
   if (list == kActive) slaves = fActiveSlaves;
   if (list == kUnique) slaves = fUniqueSlaves;

   return BroadcastRaw(buffer, length, slaves);
}

//______________________________________________________________________________
Int_t TProof::Collect(const TSlave *sl)
{
   // Collect responses from slave sl. Returns the number of slaves that
   // responded (=1).

   if (!sl->IsValid()) return 0;

   TMonitor *mon = fAllMonitor;

   mon->DeActivateAll();
   mon->Activate(sl->GetSocket());

   return Collect(mon);
}

//______________________________________________________________________________
Int_t TProof::Collect(TList *slaves)
{
   // Collect responses from the slave servers. Returns the number of slaves
   // that responded.

   TMonitor *mon = fAllMonitor;
   mon->DeActivateAll();

   TIter next(slaves);
   TSlave *sl;
   while ((sl = (TSlave*) next())) {
      if (sl->IsValid())
         mon->Activate(sl->GetSocket());
   }

   return Collect(mon);
}

//______________________________________________________________________________
Int_t TProof::Collect(ESlaves list)
{
   // Collect responses from the slave servers. Returns the number of slaves
   // that responded.

   TMonitor *mon = 0;
   if (list == kAll)    mon = fAllMonitor;
   if (list == kActive) mon = fActiveMonitor;
   if (list == kUnique) mon = fUniqueMonitor;

   mon->ActivateAll();

   return Collect(mon);
}

//______________________________________________________________________________
Int_t TProof::Collect(TMonitor *mon)
{
   // Collect responses from the slave servers. Returns the number of messages
   // received. Can be 0 if there are no active slaves.

   fStatus = 0;
   if (!mon->GetActive()) return 0;

   DeActivateAsyncInput();

   int cnt = 0, loop = 1;

   fBytesRead = 0;
   fRealTime  = 0.0;
   fCpuTime   = 0.0;

   while (loop) {
      char      str[512];
      TMessage *mess;
      TSocket  *s;
      TSlave   *sl;
      TObject  *obj;
      Int_t     what;

      s = mon->Select();

      if (s->Recv(mess) < 0) {
         MarkBad(s);
         continue;
      }

      what = mess->What();

      switch (what) {

         case kMESS_OBJECT:
            obj = mess->ReadObject(mess->GetClass());
            if (obj->InheritsFrom(TH1::Class())) {
               TH1 *h = (TH1*)obj;
               h->SetDirectory(0);
               TH1 *horg = (TH1*)gDirectory->GetList()->FindObject(h->GetName());
               if (horg)
                  horg->Add(h);
               else
                  h->SetDirectory(gDirectory);
            }
            break;

         case kPROOF_FATAL:
            MarkBad(s);
            if (!mon->GetActive()) loop = 0;
            break;

         case kPROOF_GETOBJECT:
            mess->ReadString(str, sizeof(str));
            obj = gDirectory->Get(str);
            if (obj)
               s->SendObject(obj);
            else
               s->Send(kMESS_NOTOK);
            break;

         case kPROOF_GETPACKET:
            {
               TDSetElement *elem = 0;
               if (fPlayer) {
                  sl = FindSlave(s);
                  elem = fPlayer->GetNextPacket(sl, mess);
               }

               TMessage answ(kPROOF_GETPACKET);

               if (elem != 0) {
                  answ << kTRUE
                       << TString(elem->GetFileName())
                       << TString(elem->GetDirectory())
                       << TString(elem->GetObjName())
                       << elem->GetFirst()
                       << elem->GetNum();
               } else {
                  answ << kFALSE;
               }

               s->Send(answ);
            }
            break;

         case kPROOF_LOGFILE:
            {
               Int_t size;
               (*mess) >> size;
               RecvLogFile(s, size);
            }
            break;

         case kPROOF_LOGDONE:
            (*mess) >> fStatus >> fParallel;
            mon->DeActivate(s);
            if (!mon->GetActive()) loop = 0;
            break;

         case kPROOF_STATUS:
            if (IsMaster()) {
               sl = FindSlave(s);
               (*mess) >> sl->fBytesRead >> sl->fRealTime >> sl->fCpuTime
                       >> sl->fWorkDir;
               fBytesRead += sl->fBytesRead;
               fRealTime  += sl->fRealTime;
               fCpuTime   += sl->fCpuTime;
            } else {
               (*mess) >> fParallel;
            }
            mon->DeActivate(s);
            if (!mon->GetActive()) loop = 0;
            break;

         case kPROOF_OUTPUTLIST:
            {
               PDB(kGlobal,2) Info("Collect","Got kPROOF_OUTPUTLIST");
               TList *out = (TList *) mess->ReadObject(TList::Class());
               out->SetOwner();
               fPlayer->StoreOutput(out); // Adopts the list
            }
            break;

         case kPROOF_FEEDBACK:
            {
               PDB(kGlobal,2) Info("Collect","Got kPROOF_FEEDBACK");
               TList *out = (TList *) mess->ReadObject(TList::Class());
               out->SetOwner();
               sl = FindSlave(s);
               fPlayer->StoreFeedback(sl, out); // Adopts the list
            }
            break;

         case kPROOF_AUTOBIN:
            {
               TString name;
               Double_t xmin, xmax, ymin, ymax, zmin, zmax;

               (*mess) >> name >> xmin >> xmax >> ymin >> ymax >> zmin >> zmax;

               if (fPlayer != 0 ) {
                  fPlayer->UpdateAutoBin(name,xmin,xmax,ymin,ymax,zmin,zmax);
               }

               TMessage answ(kPROOF_AUTOBIN);

               answ << name << xmin << xmax << ymin << ymax << zmin << zmax;

               s->Send(answ);
            }
            break;

         case kPROOF_PROGRESS:
            {
               PDB(kGlobal,2) Info("Collect","Got kPROOF_PROGRESS");
               Long64_t total, processed;

               (*mess) >> total >> processed;

               fPlayer->Progress(total, processed);
            }
            break;

         default:
            Error("Collect", "unknown command received from slave (%d)", what);
            break;
      }

      cnt++;
      delete mess;
   }

   // make sure group view is up to date
   SendGroupView();

   ActivateAsyncInput();

   return cnt;
}

//______________________________________________________________________________
void TProof::ActivateAsyncInput()
{
   // Activate the a-sync input handler.

   TIter next(fSlaves);
   TSlave *sl;

   while ((sl = (TSlave*) next()))
      if (sl->GetInputHandler())
         sl->GetInputHandler()->Add();
}

//______________________________________________________________________________
void TProof::DeActivateAsyncInput()
{
   // De-actiate a-sync input handler.

   TIter next(fSlaves);
   TSlave *sl;

   while ((sl = (TSlave*) next()))
      if (sl->GetInputHandler())
         sl->GetInputHandler()->Remove();
}

//______________________________________________________________________________
void TProof::HandleAsyncInput(TSocket *sl)
{
   // Handle input coming from the master server (when this is a client)
   // or from a slave server (when this is a master server). This is mainly
   // for a-synchronous communication. Normally when PROOF issues a command
   // the (slave) server messages are directly handle by Collect().

   TMessage *mess;
   Int_t     what;

   if (sl->Recv(mess) <= 0)
      return;                // do something more intelligent here

   what = mess->What();

   switch (what) {

      case kPROOF_PING:
         // do nothing (ping is already acknowledged)
         break;

      default:
         Error("HandleAsyncInput", "unknown command %d", what);
         break;
   }

   delete mess;
}

//______________________________________________________________________________
void TProof::MarkBad(TSlave *sl)
{
   // Add a bad slave server to the bad slave list and remove it from
   // the active list and from the two monitor objects.

   fActiveSlaves->Remove(sl);
   FindUniqueSlaves();
   fBadSlaves->Add(sl);

   fAllMonitor->Remove(sl->GetSocket());
   fActiveMonitor->Remove(sl->GetSocket());

   sl->Close();

   fSendGroupView = kTRUE;
}

//______________________________________________________________________________
void TProof::MarkBad(TSocket *s)
{
   // Add slave with socket s to the bad slave list and remove if from
   // the active list and from the two monitor objects.

   TSlave *sl = FindSlave(s);
   MarkBad(sl);
}

//______________________________________________________________________________
Int_t TProof::Ping()
{
   // Ping PROOF. Returns 1 if master server responded.

   return Ping(kActive);
}

//______________________________________________________________________________
Int_t TProof::Ping(ESlaves list)
{
   // Ping PROOF slaves. Returns the number of slaves that responded.

   TMessage mess(kPROOF_PING | kMESS_ACK);
   return Broadcast(mess, list);
}

//______________________________________________________________________________
void TProof::Print(Option_t *option) const
{
   // Print status of PROOF cluster.

   if (!IsMaster()) {
      Printf("Connected to:             %s (%s)", GetMaster(),
                                          IsValid() ? "valid" : "invalid");
      Printf("Port number:              %d", GetPort());
      Printf("User:                     %s", GetUser());
      Printf("Client protocol version:  %d", GetClientProtocol());
      Printf("Remote protocol version:  %d", GetRemoteProtocol());
      Printf("Log level:                %d", GetLogLevel());
      if (IsValid())
         ((TProof*)this)->SendPrint();

   } else {
      ((TProof*)this)->AskStatus();
      if (IsParallel())
         Printf("*** Master server (parallel mode, %d slaves):",
                GetNumberOfActiveSlaves());
      else
         Printf("*** Master server (sequential mode):");

      Printf("Master host name:         %s", gSystem->HostName());
      Printf("Port number:              %d", GetPort());
      Printf("User:                     %s", GetUser());
      Printf("Protocol version:         %d", GetClientProtocol());
      Printf("Image name:               %s", GetImage());
      Printf("Working directory:        %s", gSystem->WorkingDirectory());
      Printf("Config directory:         %s", GetConfDir());
      Printf("Config file:              %s", GetConfFile());
      Printf("Log level:                %d", GetLogLevel());
      Printf("Number of slaves:         %d", GetNumberOfSlaves());
      Printf("Number of active slaves:  %d", GetNumberOfActiveSlaves());
      Printf("Number of unique slaves:  %d", GetNumberOfUniqueSlaves());
      Printf("Number of bad slaves:     %d", GetNumberOfBadSlaves());
      Printf("Total MB's processed:     %.2f", float(GetBytesRead())/(1024*1024));
      Printf("Total real time used (s): %.3f", GetRealTime());
      Printf("Total CPU time used (s):  %.3f", GetCpuTime());
      if (GetNumberOfSlaves()) {
         Printf("List of slaves:");
         fSlaves->ForEach(TSlave,Print)(option);
      }
   }
}

//______________________________________________________________________________
Int_t TProof::Process(TDSet *set, const char *selector, Option_t *option,
                      Long64_t nentries, Long64_t first, TEventList *evl)
{
   // Process a data set (TDSet) using the specified selector (.C) file.
   // Returns -1 in case of error, 0 otherwise.

   if (!fPlayer)
      fPlayer = new TProofPlayerRemote(this);

   if (fProgressDialog)
      fProgressDialog->ExecPlugin(5, this, selector, set->GetListOfElements()->GetSize(),
                                  first, nentries);

   return fPlayer->Process(set, selector, option, nentries, first, evl);
}

//______________________________________________________________________________
Int_t TProof::DrawSelect(TDSet *set, const char *varexp, const char *selection, Option_t *option,
                         Long64_t nentries, Long64_t first)
{
   // Process a data set (TDSet) using the specified selector (.C) file.
   // Returns -1 in case of error, 0 otherwise.

   if (!fPlayer)
      fPlayer = new TProofPlayerRemote(this);

   return fPlayer->DrawSelect(set, varexp, selection, option, nentries, first);
}

//______________________________________________________________________________
void TProof::StopProcess(Bool_t abort)
{
   if (fPlayer != 0) fPlayer->StopProcess(abort);
}

//______________________________________________________________________________
void TProof::AddInput(TObject *obj)
{
   // Add objects that might be needed during the processing of
   // the selector (see Process()).

   if (!fPlayer)
      fPlayer = new TProofPlayerRemote(this);

   fPlayer->AddInput(obj);
}

//______________________________________________________________________________
void TProof::ClearInput()
{
   // Clear input object list.

   if (fPlayer)
      fPlayer->ClearInput();
}

//______________________________________________________________________________
TObject *TProof::GetOutput(const char *name)
{
   // Get specified object that has been produced during the processing
   // (see Process()).

   if (fPlayer)
      return fPlayer->GetOutput(name);
   return 0;
}

//______________________________________________________________________________
TList *TProof::GetOutputList()
{
   // Get list with all object created during processing (see Process()).

   if (fPlayer)
      return fPlayer->GetOutputList();
   return 0;
}

//______________________________________________________________________________
void TProof::RecvLogFile(TSocket *s, Int_t size)
{
   // Receive the log file of the slave with socket s.

   const Int_t kMAXBUF = 16384;  //32768  //16384  //65536;
   char buf[kMAXBUF];

   Int_t  left, r;
   Long_t filesize = 0;

   while (filesize < size) {
      left = Int_t(size - filesize);
      if (left > kMAXBUF)
         left = kMAXBUF;
      r = s->RecvRaw(&buf, left);
      if (r > 0) {
         char *p = buf;

         filesize += r;
         while (r) {
            Int_t w;

            w = write(fileno(stdout), p, r);

            if (w < 0) {
               SysError("RecvLogFile", "error writing to stdout");
               break;
            }
            r -= w;
            p += w;
         }
      } else if (r < 0) {
         Error("RecvLogFile", "error during receiving log file");
         break;
      }
   }
}

//______________________________________________________________________________
Int_t TProof::SendGroupView()
{
   // Send to all active slaves servers the current slave group size
   // and their unique id. Returns number of active slaves.

   if (!IsValid() || !IsMaster()) return 0;
   if (!fSendGroupView) return 0;
   fSendGroupView = kFALSE;

   TIter   next(fActiveSlaves);
   TSlave *sl;

   int  bad = 0, cnt = 0, size = GetNumberOfActiveSlaves();
   char str[32];

   while ((sl = (TSlave *)next())) {
      sprintf(str, "%d %d", cnt, size);
      if (sl->GetSocket()->Send(str, kPROOF_GROUPVIEW) == -1) {
         MarkBad(sl);
         bad++;
      } else
         cnt++;
   }

   // Send the group view again in case there was a change in the
   // group size due to a bad slave

   if (bad) SendGroupView();

   return GetNumberOfActiveSlaves();
}

//______________________________________________________________________________
Int_t TProof::Exec(const char *cmd)
{
   // Send command to be executed on the PROOF master and/or slaves.
   // Command can be any legal command line command. Commands like
   // ".x file.C" or ".L file.C" will cause the file file.C to be send
   // to the PROOF cluster. Returns -1 in case of error, >=0 in case of
   // succes.

   return Exec(cmd, kActive);
}

//______________________________________________________________________________
Int_t TProof::Exec(const char *cmd, ESlaves list)
{
   // Send command to be executed on the PROOF master and/or slaves.
   // Command can be any legal command line command. Commands like
   // ".x file.C" or ".L file.C" will cause the file file.C to be send
   // to the PROOF cluster. Returns -1 in case of error, >=0 in case of
   // succes.

   if (!IsValid()) return 0;

   TString s = cmd;
   s = s.Strip(TString::kBoth);

   if (!s.Length()) return 0;

   // check for macro file and make sure the file is available on all slaves
   if (s.BeginsWith(".L") || s.BeginsWith(".x") || s.BeginsWith(".X")) {
      TString file = s(2, s.Length());
      file = file.Strip(TString::kLeading);
      file = file.Strip(TString::kTrailing, '+');
      char *fn = gSystem->Which(TROOT::GetMacroPath(), file, kReadPermission);
      if (fn) {
         if (GetNumberOfUniqueSlaves() > 0) {
            if (SendFile(fn, kFALSE) < 0) {
               Error("Exec", "file %s could not be transfered", fn);
               delete [] fn;
               return -1;
            }
         } else {
            TString scmd = s(0,3) + fn;
            Int_t n = SendCommand(scmd, list);
            delete [] fn;
            return n;
         }
      } else {
         Error("Exec", "macro %s not found", file.Data());
         return -1;
      }
      delete [] fn;
   }

   return SendCommand(cmd, list);
}

//______________________________________________________________________________
Int_t TProof::SendCommand(const char *cmd, ESlaves list)
{
   // Send command to be executed on the PROOF master and/or slaves.
   // Command can be any legal command line command, however commands
   // like ".x file.C" or ".L file.C" will not cause the file.C to be
   // transfered to the PROOF cluster. In that case use TProof::Exec().
   // Returns the status send by the remote server as part of the
   // kPROOF_LOGDONE message. Typically this is the return code of the
   // command on the remote side.

   if (!IsValid()) return 0;

   Broadcast(cmd, kMESS_CINT, list);
   Collect(list);
   return fStatus;
}

//______________________________________________________________________________
Int_t TProof::SendCurrentState(ESlaves list)
{
   // Transfer the current state of the master to the active slave servers.
   // The current state includes: the current working directory, etc.

   if (!IsValid()) return 0;

   // Go to the new directory, reset the interpreter environment and
   // tell slave to delete all objects from its new current directory.
   Broadcast(gDirectory->GetPath(), kPROOF_RESET, list);

   return GetNumberOfActiveSlaves();
}

//______________________________________________________________________________
Int_t TProof::SendInitialState()
{
   // Transfer the initial (i.e. current) state of the master to all
   // slave servers. Currently the initial state includes: log level.

   if (!IsValid()) return 0;

   SetLogLevel(fLogLevel, gProofDebugMask);

   return GetNumberOfActiveSlaves();
}

//______________________________________________________________________________
Long_t TProof::CheckFile(const char *file, TSlave *slave)
{
   // Check if a file needs to be send to the slave. Use the following
   // algorithm:
   //   - check if file appears in file map
   //     - if yes, get file's modtime and check against time in map,
   //       if modtime not same get md5 and compare against md5 in map,
   //       if not same return size
   //     - if no, get file's md5 and modtime and store in file map, ask
   //       slave if file exists with specific md5, if yes return 0,
   //       if no return file's size
   // Returns size of file in case file needs to be send, returns 0 in case
   // file is already on remote and -1 in case of error.

   Long_t id, size, flags, modtime;
   if (gSystem->GetPathInfo(file, &id, &size, &flags, &modtime) == 1) {
      Error("CheckFile", "cannot stat file %s", file);
      return -1;
   }
   if (size == 0) {
      Error("CheckFile", "empty file %s", file);
      return -1;
   }

   Bool_t sendto = kFALSE;

   // create slave based filename
   TString sn = slave->GetName();
   sn += ":";
   sn += slave->GetOrdinal();
   sn += ":";
   sn += gSystem->BaseName(file);

   // check if file is in map
   FileMap_t::const_iterator it;
   if ((it = fFileMap.find(sn)) != fFileMap.end()) {
      // file in map
      MD5Mod_t md = (*it).second;
      if (md.fModtime != modtime) {
         TMD5 *md5 = TMD5::FileChecksum(file);
         if ((*md5) != md.fMD5) {
            sendto       = kTRUE;
            md.fMD5      = *md5;
            md.fModtime  = modtime;
            fFileMap[sn] = md;
            // When on the master, the master and/or slaves may share
            // their file systems and cache. Therefore always make a
            // check for the file. If the file already exists with the
            // expected md5 the kPROOF_CHECKFILE command will cause the
            // file to be copied from cache to slave sandbox.
            if (IsMaster()) {
               sendto = kFALSE;
               TMessage mess(kPROOF_CHECKFILE);
               mess << TString(gSystem->BaseName(file)) << md.fMD5;
               slave->GetSocket()->Send(mess);

               TMessage *reply;
               slave->GetSocket()->Recv(reply);
               if (reply->What() != kPROOF_CHECKFILE)
                  sendto = kTRUE;
               delete reply;
            }
         }
         delete md5;
      }
   } else {
      // file not in map
      TMD5 *md5 = TMD5::FileChecksum(file);
      MD5Mod_t md;
      md.fMD5      = *md5;
      md.fModtime  = modtime;
      fFileMap[sn] = md;
      delete md5;
      TMessage mess(kPROOF_CHECKFILE);
      mess << TString(gSystem->BaseName(file)) << md.fMD5;
      slave->GetSocket()->Send(mess);

      TMessage *reply;
      slave->GetSocket()->Recv(reply);
      if (reply->What() != kPROOF_CHECKFILE)
         sendto = kTRUE;
      delete reply;
   }

   if (sendto)
      return size;

   return 0;
}

//______________________________________________________________________________
Int_t TProof::SendFile(const char *file, Bool_t bin)
{
   // Send a file to master or slave servers. Returns number of slaves
   // the file was sent to, maybe 0 in case master and slaves have the same
   // file system image, -1 in case of error. If bin is true binary
   // file transfer is used, otherwise ASCII mode.

   TList *slaves = fActiveSlaves;

   if (slaves->GetSize() == 0) return 0;

#ifndef R__WIN32
   Int_t fd = open(file, O_RDONLY);
#else
   Int_t fd = open(file, O_RDONLY | O_BINARY);
#endif
   if (fd < 0) {
      SysError("SendFile", "cannot open file %s", file);
      return -1;
   }

   const Int_t kMAXBUF = 32768;  //16384  //65536;
   char buf[kMAXBUF];
   Int_t nsl = 0;

   TIter next(slaves);
   TSlave *sl;
   while ((sl = (TSlave *)next())) {
      if (!sl->IsValid())
         continue;

      Long_t size = CheckFile(file, sl);
      // if on client and size==0 broadcast anyway the kPROOF_SENDFILE command
      // to the master so that the master can propagate the file to possibly
      // newly added slaves
      if (IsMaster() && size == 0)
         continue;

      PDB(kPackage,2)
         if (size > 0) {
            if (!nsl)
               Info("SendFile", "sending file %s to:", file);
            printf("   slave = %s:%d\n", sl->GetName(), sl->GetOrdinal());
         }

      sprintf(buf, "%s %d %ld", gSystem->BaseName(file), bin, size);
      if (sl->GetSocket()->Send(buf, kPROOF_SENDFILE) == -1) {
         MarkBad(sl);
         continue;
      }

      if (size == 0)
         continue;

      lseek(fd, 0, SEEK_SET);

      Int_t len;
      do {
         while ((len = read(fd, buf, kMAXBUF)) < 0 && TSystem::GetErrno() == EINTR)
            TSystem::ResetErrno();

         if (len < 0) {
            SysError("SendFile", "error reading from file %s", file);
            Interrupt(kSoftInterrupt, kActive);
            close(fd);
            return -1;
         }

         if (sl->GetSocket()->SendRaw(buf, len) == -1) {
            SysError("SendFile", "error writing to slave %s:%d (now offline)",
                     sl->GetName(), sl->GetOrdinal());
            MarkBad(sl);
            break;
         }

      } while (len > 0);

      nsl++;
   }

   close(fd);

   return nsl;
}

//______________________________________________________________________________
Int_t TProof::SendObject(const TObject *obj, ESlaves list)
{
   // Send object to master or slave servers. Returns number slaves object
   // was sent to, 0 in case of error.

   if (!IsValid() || !obj) return 0;

   TMessage mess(kMESS_OBJECT);

   mess.WriteObject(obj);
   return Broadcast(mess, list);
}

//______________________________________________________________________________
Int_t TProof::SendPrint()
{
   // Send print command to master server.

   if (!IsValid()) return 0;

   Broadcast(kPROOF_PRINT, kActive);
   return Collect(kActive);
}

//______________________________________________________________________________
void TProof::SetLogLevel(Int_t level, UInt_t mask)
{
   // Set server logging level.

   char str[32];
   fLogLevel        = level;
   gProofDebugLevel = level;
   gProofDebugMask  = (TProofDebug::EProofDebugMask) mask;
   sprintf(str, "%d %u", level, mask);
   Broadcast(str, kPROOF_LOGLEVEL, kAll);
}

//______________________________________________________________________________
Int_t TProof::SetParallel(Int_t nodes)
{
   // Tell RPOOF how many slaves to use in parallel. Returns the number of
   // parallel slaves.

   if (!IsValid()) return 0;

   if (IsMaster()) {
      GoParallel(nodes);
      return SendCurrentState();
   } else {
      TMessage mess(kPROOF_PARALLEL);
      mess << nodes;
      Broadcast(mess);
      Collect();
      return fParallel;
   }
}

//______________________________________________________________________________
Int_t TProof::GoParallel(Int_t nodes)
{
   // Go in parallel mode with at most "nodes" slaves. Since the fSlaves
   // list is sorted by slave performace the active list will contain first
   // the most performant nodes.

   if (nodes <= 0) nodes = 1;

   fActiveSlaves->Clear();
   fActiveMonitor->RemoveAll();

   TIter next(fSlaves);

   int cnt = 0;
   TSlave *sl;
   while (cnt < nodes && (sl = (TSlave *)next())) {
      if (sl->IsValid()) {
         fActiveSlaves->Add(sl);
         fActiveMonitor->Add(sl->GetSocket());
         cnt++;
      }
   }

   // Will be activated in Collect
   fActiveMonitor->DeActivateAll();

   // Get slave status (will set the slaves fWorkDir correctly)
   AskStatus();

   // Find active slaves with unique image
   FindUniqueSlaves();

   // Send new group-view to slaves
   SendGroupView();

   Int_t n = GetNumberOfActiveSlaves();
   if (IsMaster()) {
      if (n > 1)
         printf("PROOF set to parallel mode (%d slaves)\n", n);
      else
         printf("PROOF set to sequential mode\n");
   }

   return n;
}

//______________________________________________________________________________
void TProof::ShowCache(Bool_t all)
{
   // List contents of file cache. If all is true show all caches also on
   // slaves. If everything is ok all caches are to be the same.

   if (!IsValid()) return;

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(1) << all;
   Broadcast(mess, kUnique);
   Collect(kUnique);
}

//______________________________________________________________________________
void TProof::ClearCache()
{
    // Remove files from all file caches.

   if (!IsValid()) return;

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(2);
   Broadcast(mess, kUnique);
   Collect(kUnique);
   // clear file map so files get send again to remote nodes
   fFileMap.clear();
}

//______________________________________________________________________________
void TProof::ShowPackages(Bool_t all)
{
   // List contents of package directory. If all is true show all package
   // directries also on slaves. If everything is ok all package directories
   // should be the same.

   if (!IsValid()) return;

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(3) << all;
   Broadcast(mess, kUnique);
   Collect(kUnique);
}

//______________________________________________________________________________
void TProof::ShowEnabledPackages(Bool_t all)
{
   // List which packages are enabled. If all is true show enabled packages
   // for all active slaves. If everything is ok all active slaves should
   // have the same packages enabled.

   if (!IsValid()) return;

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(8) << all;
   Broadcast(mess);
   Collect();
}

//______________________________________________________________________________
void TProof::ClearPackages()
{
   // Remove all packages.

   if (!IsValid()) return;

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(4);
   Broadcast(mess, kUnique);
   Collect(kUnique);
}

//______________________________________________________________________________
void TProof::ClearPackage(const char *package)
{
   // Remove a specific package.

   if (!IsValid()) return;

   if (!package || !strlen(package)) {
      Error("ClearPackage", "need to specify a package name");
      return;
   }

   // if name, erroneously, is a par pathname strip off .par and path
   TString pac = package;
   if (pac.EndsWith(".par"))
      pac.Remove(pac.Length()-4);
   pac = gSystem->BaseName(pac);

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(5) << pac;
   Broadcast(mess, kUnique);
   Collect(kUnique);
}

//______________________________________________________________________________
Int_t TProof::BuildPackage(const char *package)
{
   // Build specified package. Executes the PROOF-INF/BUILD.sh
   // script if it exists on all unique nodes.
   // Returns 0 in case of success and -1 in case of error.

   if (!IsValid()) return -1;

   if (!package || !strlen(package)) {
      Error("BuildPackage", "need to specify a package name");
      return -1;
   }

   // if name, erroneously, is a par pathname strip off .par and path
   TString pac = package;
   if (pac.EndsWith(".par"))
      pac.Remove(pac.Length()-4);
   pac = gSystem->BaseName(pac);

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(6) << pac;
   Broadcast(mess, kUnique);
   Collect(kUnique);

   return fStatus;
}

//______________________________________________________________________________
Int_t TProof::LoadPackage(const char *package)
{
   // Load specified package. Executes the PROOF-INF/SETUP.C script
   // on all active nodes.
   // Returns 0 in case of success and -1 in case of error.

   if (!IsValid()) return -1;

   if (!package || !strlen(package)) {
      Error("LoadPackage", "need to specify a package name");
      return -1;
   }

   // if name, erroneously, is a par pathname strip off .par and path
   TString pac = package;
   if (pac.EndsWith(".par"))
      pac.Remove(pac.Length()-4);
   pac = gSystem->BaseName(pac);

   TMessage mess(kPROOF_CACHE);
   mess << Int_t(7) << pac;
   Broadcast(mess);
   Collect();

   return fStatus;
}

//______________________________________________________________________________
Int_t TProof::EnablePackage(const char *package)
{
   // Enable specified package. Executes the PROOF-INF/BUILD.sh
   // script if it exists followed by the PROOF-INF/SETUP.C script.
   // Returns 0 in case of success and -1 in case of error.

   if (!IsValid()) return -1;

   if (!package || !strlen(package)) {
      Error("EnablePackage", "need to specify a package name");
      return -1;
   }

   // if name, erroneously, is a par pathname strip off .par and path
   TString pac = package;
   if (pac.EndsWith(".par"))
      pac.Remove(pac.Length()-4);
   pac = gSystem->BaseName(pac);

   if (BuildPackage(pac) == -1)
      return -1;

   if (LoadPackage(pac) == -1)
      return -1;

   return 0;
}

//______________________________________________________________________________
Int_t TProof::UploadPackage(const char *tpar, Int_t parallel)
{
   // Upload a PROOF archive (PAR file). A PAR file is a compressed
   // tar file with one special additional directory, PROOF-INF
   // (blatantly copied from Java's jar format). It must have the extension
   // .par. A PAR file can be directly a binary or a source with a build
   // procedure. In the PROOF-INF directory there can be a build script:
   // BUILD.sh to be called to build the package, in case of a binary PAR
   // file don't specify a build script or make it a no-op. Then there is
   // SETUP.C which sets the right environment variables to use the package,
   // like LD_LIBRARY_PATH, etc. Parallel is the number of parallel streams
   // that can be used to upload the package to the master server.
   // Returns 0 in case of success and -1 in case of error.

   if (!IsValid()) return -1;

   TString par = tpar;
   if (!par.EndsWith(".par")) {
      Error("UploadPackage", "package %s must have extension .par", tpar);
      return -1;
   }

   gSystem->ExpandPathName(par);

   if (gSystem->AccessPathName(par, kReadPermission)) {
      Error("UploadPackage", "package %s does not exist", par.Data());
      return -1;
   }

   // Strategy: get md5 of package and check if it is different from the
   // one stored on the remote node. If it is different lock the remote
   // package directory and use TFTP to ftp the package to the remote node,
   // unlock the directory.

   TMD5 *md5 = TMD5::FileChecksum(par);
   TMessage mess(kPROOF_CHECKFILE);
   mess << TString("+")+TString(gSystem->BaseName(par)) << (*md5);
   TMessage mess2(kPROOF_CHECKFILE);
   mess2 << TString("-")+TString(gSystem->BaseName(par)) << (*md5);
   delete md5;

   // loop over all unique nodes
   TIter next(fUniqueSlaves);
   TSlave *sl;
   while ((sl = (TSlave *) next())) {
      if (!sl->IsValid())
         continue;

      sl->GetSocket()->Send(mess);

      TMessage *reply;
      sl->GetSocket()->Recv(reply);
      if (reply->What() != kPROOF_CHECKFILE) {
         // remote directory is locked, upload file via TFTP
         if (IsMaster())
            parallel = 1;  // assume LAN
         {
            TFTP ftp(TString("root://")+sl->GetName(), parallel);
            if (!ftp.IsZombie()) {
               ftp.cd(Form("%s/%s", kPROOF_WorkDir, kPROOF_PackDir));
               ftp.put(par, gSystem->BaseName(par));
            }
         }
         // install package and unlock dir
         sl->GetSocket()->Send(mess2);
         delete reply;
         sl->GetSocket()->Recv(reply);
         if (reply->What() != kPROOF_CHECKFILE) {
            Error("UploadPackage", "unpacking of package %s failed", par.Data());
            delete reply;
            return -1;
         }
      }
      delete reply;
   }

   return 0;
}


//______________________________________________________________________________
void TProof::Progress(Long64_t total, Long64_t processed)
{
   // Get query progress information. Connect a slot to this signal
   // to track progress.

   PDB(kGlobal,1)
      Info("Progress","%2f (%lld/%lld)", 100.*processed/total, processed, total);

   Long_t parm[2];
   parm[0] = (Long_t) (&total);
   parm[1] = (Long_t) (&processed);
   Emit("Progress(Long64_t,Long64_t)", parm);
}

//______________________________________________________________________________
void TProof::Feedback(TList *objs)
{
   // Get list of feedback objects. Connect a slot to this signal
   // to monitor the feedback object.

   PDB(kGlobal,1) Info("Feedback","%d Objects", objs->GetSize());
   PDB(kFeedback,1) {
     Info("Feedback","%d Objects", objs->GetSize());
      objs->ls();
   }

   Emit("Feedback(TList *objs)", (Long_t) objs);
}
