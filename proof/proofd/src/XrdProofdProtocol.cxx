// @(#)root/proofd:$Id$
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
// XrdProofdProtocol                                                    //
//                                                                      //
// Authors: G. Ganis, CERN, 2005                                        //
//                                                                      //
// XrdProtocol implementation to coordinate 'proofserv' applications.   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "XrdProofdPlatform.h"

#ifdef OLDXRDOUC
#  include "XrdOuc/XrdOucError.hh"
#  include "XrdOuc/XrdOucLogger.hh"
#  include "XrdOuc/XrdOucPlugin.hh"
#  include "XrdOuc/XrdOucTimer.hh"
#  define XPD_LOG_01 OUC_LOG_01
#else
#  include "XrdSys/XrdSysError.hh"
#  include "XrdSys/XrdSysLogger.hh"
#  include "XrdSys/XrdSysPlugin.hh"
#  include "XrdSys/XrdSysTimer.hh"
#  define XPD_LOG_01 SYS_LOG_01
#endif

#include "XrdVersion.hh"
#include "XrdClient/XrdClientMessage.hh"
#include "XrdClient/XrdClientUrlInfo.hh"
#include "XrdSys/XrdSysPriv.hh"
#include "XrdOuc/XrdOucErrInfo.hh"
#include "XrdOuc/XrdOucReqID.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdSut/XrdSutAux.hh"
#include "XrdNet/XrdNet.hh"
#include "XrdNet/XrdNetDNS.hh"
#include "XrdNet/XrdNetPeer.hh"
#include "Xrd/XrdLink.hh"
#include "Xrd/XrdPoll.hh"
#include "Xrd/XrdBuffer.hh"
#include "Xrd/XrdScheduler.hh"

#include "XrdProofConn.h"
#include "XrdProofdClient.h"
#include "XrdProofdProtocol.h"
#include "XrdProofSched.h"
#include "XrdProofWorker.h"
#include "XrdROOT.h"

#ifdef R__HAVE_CONFIG
#include "RConfigure.h"
#endif

// Tracing utils
#include "XrdProofdTrace.h"
XrdOucTrace          *XrdProofdTrace = 0;
static const char    *gTraceID = " ";

// Static variables
static XrdOucReqID   *XrdProofdReqID = 0;
XrdSysRecMutex        gSysPrivMutex;

// Loggers: we need two to avoid deadlocks
static XrdSysLogger   gMainLogger;

//
// Static area: general protocol managing section
int                   XrdProofdProtocol::fgCount    = 0;
XrdSysRecMutex        XrdProofdProtocol::fgXPDMutex;
XrdObjectQ<XrdProofdProtocol>
                      XrdProofdProtocol::fgProtStack("ProtStack",
                                                     "xproofd protocol anchor");
XrdBuffManager       *XrdProofdProtocol::fgBPool    = 0;
int                   XrdProofdProtocol::fgMaxBuffsz= 0;
XrdSecService        *XrdProofdProtocol::fgCIA      = 0;
XrdScheduler         *XrdProofdProtocol::fgSched    = 0;
XrdSysError           XrdProofdProtocol::fgEDest(0, "xpd");
XrdSysLogger          XrdProofdProtocol::fgMainLogger;

//
// Static area: protocol configuration section
XrdProofdFile         XrdProofdProtocol::fgCfgFile;
bool                  XrdProofdProtocol::fgConfigDone = 0;
std::list<XrdROOT *>  XrdProofdProtocol::fgROOT;
XrdOucString          XrdProofdProtocol::fgBareLibPath;
char                 *XrdProofdProtocol::fgTMPdir   = 0;
char                 *XrdProofdProtocol::fgSecLib   = 0;
//
char                 *XrdProofdProtocol::fgPoolURL = 0;
char                 *XrdProofdProtocol::fgNamespace = strdup("/proofpool");
//
XrdSysSemWait         XrdProofdProtocol::fgForkSem;   // To serialize fork requests
//
std::list<XrdOucString *> XrdProofdProtocol::fgMastersAllowed;
std::list<XrdProofdPriority *> XrdProofdProtocol::fgPriorities;
char                 *XrdProofdProtocol::fgSuperUsers = 0; // ':' separated list of privileged users
//
bool                  XrdProofdProtocol::fgWorkerUsrCfg = 0; // user cfg files enabled / disabled
//
int                   XrdProofdProtocol::fgReadWait = 0;
int                   XrdProofdProtocol::fgInternalWait = 5; // seconds
// Shutdown options
int                   XrdProofdProtocol::fgShutdownOpt = 1;
int                   XrdProofdProtocol::fgShutdownDelay = 0; // minimum
// Cron options
int                   XrdProofdProtocol::fgCron = 1; // Default: start cron thread
int                   XrdProofdProtocol::fgCronFrequency = 60; // Default: run checks every minute
// Access control
int                   XrdProofdProtocol::fgOperationMode = kXPD_OpModeOpen; // Operation mode
XrdOucString          XrdProofdProtocol::fgAllowedUsers; // Users allowed in controlled mode
bool                  XrdProofdProtocol::fgMultiUser = 0; // Allow/disallow multi-user mode
bool                  XrdProofdProtocol::fgChangeOwn = 0; // TRUE is ownership has to be changed
// Proofserv configuration
XrdOucString          XrdProofdProtocol::fgProofServEnvs; // Additional envs to be exported before proofserv
XrdOucString          XrdProofdProtocol::fgProofServRCs; // Additional rcs to be passed to proofserv
// Groups manager
XrdProofGroupMgr      XrdProofdProtocol::fgGroupsMgr;
// Cluster manager
XrdProofdManager      XrdProofdProtocol::fgMgr;
// PROOF scheduler
XrdProofSched        *XrdProofdProtocol::fgProofSched = 0;
//
float                 XrdProofdProtocol::fgOverallInflate = 1.; // Overall inflate factor
int                   XrdProofdProtocol::fgSchedOpt = kXPD_sched_off; // Worker Sched option
//
// Static area: client section
std::list<XrdProofdClient *> XrdProofdProtocol::fgProofdClients;  // keeps track of all users
std::list<XrdProofdPInfo *> XrdProofdProtocol::fgTerminatedProcess; // List of pids of processes terminating

// Local definitions
#define MAX_ARGS 128
#define TRACEID gTraceID

// Macros used to set conditional options
#ifndef XPDCOND
#define XPDCOND(n,ns) ((n == -1 && ns == -1) || (n > 0 && n >= ns))
#endif
#ifndef XPDSETSTRING
#define XPDSETSTRING(n,ns,c,s) \
 { if (XPDCOND(n,ns)) { \
     SafeFree(c); c = strdup(s.c_str()); ns = n; }}
#endif

#ifndef XPDADOPTSTRING
#define XPDADOPTSTRING(n,ns,c,s) \
  { char *t = 0; \
    XPDSETSTRING(n, ns, t, s); \
    if (t && strlen(t)) { \
       SafeFree(c); c = t; \
  } else \
       SafeFree(t); }
#endif

#ifndef XPDSETINT
#define XPDSETINT(n,ns,i,s) \
 { if (XPDCOND(n,ns)) { \
     i = strtol(s.c_str(),0,10); ns = n; }}
#endif

typedef struct {
   kXR_int32 ptyp;  // must be always 0 !
   kXR_int32 rlen;
   kXR_int32 pval;
   kXR_int32 styp;
} hs_response_t;

// Security handle
typedef XrdSecService *(*XrdSecServLoader_t)(XrdSysLogger *, const char *cfn);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)

typedef struct kinfo_proc kinfo_proc;

//__________________________________________________________________________
static int GetMacProcList(kinfo_proc **plist, int &nproc)
{
   // Returns a list of all processes on the system.  This routine
   // allocates the list and puts it in *plist and counts the
   // number of entries in 'nproc'. Caller is responsible for 'freeing'
   // the list.
   // On success, the function returns 0.
   // On error, the function returns an errno value.
   //
   // Adapted from: reply to Technical Q&A 1123,
   //               http://developer.apple.com/qa/qa2001/qa1123.html
   //

   int rc = 0;
   kinfo_proc *res;
   bool done = 0;
   static const int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

   TRACE(ACT, "GetMacProcList: enter ");

   // Declaring name as const requires us to cast it when passing it to
   // sysctl because the prototype doesn't include the const modifier.
   size_t len = 0;

   if (!plist || (*plist))
      return EINVAL;
   nproc = 0;

   // We start by calling sysctl with res == 0 and len == 0.
   // That will succeed, and set len to the appropriate length.
   // We then allocate a buffer of that size and call sysctl again
   // with that buffer.  If that succeeds, we're done.  If that fails
   // with ENOMEM, we have to throw away our buffer and loop.  Note
   // that the loop causes use to call sysctl with 0 again; this
   // is necessary because the ENOMEM failure case sets length to
   // the amount of data returned, not the amount of data that
   // could have been returned.

   res = 0;
   do {
      // Call sysctl with a 0 buffer.
      len = 0;
      if ((rc = sysctl((int *)name, (sizeof(name)/sizeof(*name)) - 1,
                       0, &len, 0, 0)) == -1) {
         rc = errno;
      }

      // Allocate an appropriately sized buffer based on the results
      // from the previous call.
      if (rc == 0) {
         res = (kinfo_proc *) malloc(len);
         if (!res)
            rc = ENOMEM;
      }

      // Call sysctl again with the new buffer.  If we get an ENOMEM
      // error, toss away our buffer and start again.
      if (rc == 0) {
         if ((rc = sysctl((int *)name, (sizeof(name)/sizeof(*name)) - 1,
                          res, &len, 0, 0)) == -1) {
            rc = errno;
         }
         if (rc == 0) {
            done = 1;
         } else if (rc == ENOMEM) {
            if (res)
               free(res);
            res = 0;
            rc = 0;
         }
      }
   } while (rc == 0 && !done);

   // Clean up and establish post conditions.
   if (rc != 0 && !res) {
      free(res);
      res = 0;
   }
   *plist = res;
   if (rc == 0)
      nproc = len / sizeof(kinfo_proc);

   // Done
   return rc;
}
#endif

//__________________________________________________________________________
static int CreateGroupDataSetDir(const char *, XrdProofGroup *g, void *rd)
{
   // Create dataset dir for group 'g' under the root dataset dir 'rd'

   const char *dsetroot = (const char *)rd;

   if (!dsetroot || strlen(dsetroot) <= 0)
      // Dataset root dir undefined: we cannot continue
      return 1;

   XrdOucString gdsetdir = dsetroot;
   gdsetdir += '/';
   gdsetdir += g->Name();

   XrdProofUI ui;
   XrdProofdAux::GetUserInfo(geteuid(), ui);

   if (XrdProofdAux::AssertDir(gdsetdir.c_str(), ui, 1) != 0) {
      MERROR(MHEAD, "CreateGroupDataSetDir: could not assert " << gdsetdir);
   }

   // Process next
   return 0;
}

//--------------------------------------------------------------------------
//
// XrdProofdCron
//
// Function run in separate thread to run periodic checks, ... at a tunable
// frequency
//
//--------------------------------------------------------------------------
void *XrdProofdCron(void *p)
{
   // This is an endless loop to periodically check the system

   int freq = *((int *)p);

   while(1) {
      // Wait a while
      XrdSysTimer::Wait(freq*1000);
      // Do something here
      TRACE(REQ, "XrdProofdCron: running periodical checks");
      // Trim the list of processes asked for termination
      XrdProofdProtocol::TrimTerminatedProcesses();
      // Check if there was any change in the configuration
      XrdProofdProtocol::Reconfig();
   }

   // Should never come here
   return (void *)0;
}

//_____________________________________________________________________________
XrdProofSched *XrdProofdProtocol::LoadScheduler(const char *cfn, XrdSysError *edest)
{
   // Load PROOF scheduler

   XrdProofSched *sched = 0;
   XrdOucString name, lib;

   // Locate first the relevant directives in the config file
   if (cfn && strlen(cfn) > 0) {
      XrdOucStream cfg(edest, getenv("XRDINSTANCE"));
      // Open and attach the config file
      int cfgFD;
      if ((cfgFD = open(cfn, O_RDONLY, 0)) >= 0) {
         cfg.Attach(cfgFD);
         // Process items
         char *val = 0, *var = 0;
         while ((var = cfg.GetMyFirstWord())) {
            if (!(strcmp("xpd.sched", var))) {
               // Get the name
               val = cfg.GetToken();
               if (val && val[0]) {
                  name = val;
                  // Get the lib
                  val = cfg.GetToken();
                  if (val && val[0])
                     lib = val;
                  // We are done
                  break;
               }
            }
         }
      } else {
         XrdOucString m("failure opening config file (errno:");
         m += errno;
         m += "): ";
         TRACE(XERR, "LoadScheduler: "<< m);
      }
   }

   // If undefined or static init a static instance
   if (name == "default" || !(name.length() > 0 && lib.length() > 0)) {
      if ((name.length() <= 0 && lib.length() > 0) ||
          (name.length() > 0 && lib.length() <= 0)) {
         XrdOucString m("LoadScheduler: missing or incomplete info (name:");
         m += name;
         m += ", lib:";
         m += lib;
         m += ")";
         TRACE(DBG, m.c_str());
      }
      TRACE(DBG,"LoadScheduler: instantiating default scheduler");
      sched = new XrdProofSched("default", &fgMgr, &fgGroupsMgr, cfn, edest);
   } else {
      // Load the required plugin
      if (lib.beginswith("~") || lib.beginswith("$"))
         XrdProofdAux::Expand(lib);
      XrdSysPlugin *h = new XrdSysPlugin(edest, lib.c_str());
      if (!h)
         return (XrdProofSched *)0;
      // Get the scheduler object creator
      XrdProofSchedLoader_t ep = (XrdProofSchedLoader_t) h->getPlugin("XrdgetProofSched", 1);
      if (!ep) {
         delete h;
         return (XrdProofSched *)0;
      }
      // Get the scheduler object
      if (!(sched = (*ep)(cfn, &fgMgr, &fgGroupsMgr, edest))) {
         TRACE(XERR, "LoadScheduler: unable to create scheduler object from " << lib);
         return (XrdProofSched *)0;
      }
   }
   // Check result
   if (!(sched->IsValid())) {
      TRACE(XERR, "LoadScheduler:"
                  " unable to instantiate the "<<sched->Name()<<" scheduler using "<< cfn);
      delete sched;
      return (XrdProofSched *)0;
   }
   // Notify
   XPDPRT("LoadScheduler: scheduler loaded: type: " << sched->Name());

   // All done
   return sched;
}

//_____________________________________________________________________________
XrdSecService *XrdProofdProtocol::LoadSecurity(const char *seclib, const char *cfn)
{
   // Load security framework and plugins, if not already done

   TRACE(ACT, "LoadSecurity: enter");

   // Make sure the input config file is defined
   if (!cfn) {
      fgEDest.Emsg("LoadSecurity","config file not specified");
      return 0;
   }

   // Open the security library
   void *lh = 0;
   if (!(lh = dlopen(seclib, RTLD_NOW))) {
      fgEDest.Emsg("LoadSecurity",dlerror(),"opening shared library",seclib);
      return 0;
   }

   // Get the server object creator
   XrdSecServLoader_t ep = 0;
   if (!(ep = (XrdSecServLoader_t)dlsym(lh, "XrdSecgetService"))) {
      fgEDest.Emsg("LoadSecurity", dlerror(),
                  "finding XrdSecgetService() in", seclib);
      return 0;
   }

   // Extract in a temporary file the directives prefixed "xpd.sec..." (filtering
   // out the prefix), "sec.protocol" and "sec.protparm"
   int nd = 0;
   char *rcfn = FilterSecConfig(cfn, nd);
   if (!rcfn) {
      if (nd == 0) {
         // No directives to be processed
         fgEDest.Emsg("LoadSecurity",
                     "no security directives: strong authentication disabled");
         return 0;
      }
      // Failure
      fgEDest.Emsg("LoadSecurity", "creating temporary config file");
      return 0;
   }

   // Get the server object
   XrdSecService *cia = 0;
   if (!(cia = (*ep)(fgEDest.logger(), rcfn))) {
      fgEDest.Emsg("LoadSecurity",
                  "Unable to create security service object via", seclib);
      return 0;
   }
   // Notify
   fgEDest.Emsg("LoadSecurity", "strong authentication enabled");

   // Unlink the temporary file and cleanup its path
//   unlink(rcfn);
   delete[] rcfn;

   // All done
   return cia;
}

extern "C" {
//_________________________________________________________________________________
XrdProtocol *XrdgetProtocol(const char *, char *parms, XrdProtocol_Config *pi)
{
   // This protocol is meant to live in a shared library. The interface below is
   // used by the server to obtain a copy of the protocol object that can be used
   // to decide whether or not a link is talking a particular protocol.

   // Return the protocol object to be used if static init succeeds
   if (XrdProofdProtocol::Configure(parms, pi)) {

      // Issue herald
      char msg[256];
      sprintf(msg,"xproofd: protocol V %s successfully loaded", XPROOFD_VERSION);
      pi->eDest->Say(0, msg);

      return (XrdProtocol *) new XrdProofdProtocol();
   }
   return (XrdProtocol *)0;
}

//_________________________________________________________________________________
int XrdgetProtocolPort(const char * /*pname*/, char * /*parms*/, XrdProtocol_Config *pi)
{
      // This function is called early on to determine the port we need to use. The
      // The default is ostensibly 1093 but can be overidden; which we allow.

      // Default 1093
      int port = (pi && pi->Port > 0) ? pi->Port : 1093;
      return port;
}}

//__________________________________________________________________________________
XrdProofdProtocol::XrdProofdProtocol()
   : XrdProtocol("xproofd protocol handler"), fProtLink(this)
{
   // Protocol constructor
   fLink = 0;
   fArgp = 0;
   fClientID = 0;
   fGroupID = 0;
   fPClient = 0;
   fClient = 0;
   fAuthProt = 0;
   fBuff = 0;

   // Instantiate a Proofd protocol object
   Reset();
}

//______________________________________________________________________________
int XrdProofdProtocol::ResolveKeywords(XrdOucString &s, XrdProofdClient *pcl)
{
   // Resolve special keywords in 's' for client 'pcl'. Recognized keywords
   //     <workdir>          root for working dirs
   //     <host>             local host name
   //     <user>             user name
   // Return the number of keywords resolved.

   int nk = 0;

   TRACE(HDBG,"ResolveKeywords: enter: "<<s<<" - fgMgr.WorkDir(): "<<fgMgr.WorkDir());

   // Parse <workdir>
   if (s.replace("<workdir>",(const char *)fgMgr.WorkDir()))
      nk++;

   TRACE(HDBG,"ResolveKeywords: after <workdir>: "<<s);

   // Parse <host>
   if (s.replace("<host>",(const char *)fgMgr.Host()))
      nk++;

   TRACE(HDBG,"ResolveKeywords: after <host>: "<<s);

   // Parse <user>
   if (pcl)
      if (s.replace("<user>", pcl->ID()))
         nk++;

   TRACE(HDBG,"ResolveKeywords: exit: "<<s);

   // We are done
   return nk;
}

//______________________________________________________________________________
XrdProtocol *XrdProofdProtocol::Match(XrdLink *lp)
{
   // Check whether the request matches this protocol

   struct ClientInitHandShake hsdata;
   char  *hsbuff = (char *)&hsdata;

   static hs_response_t hsresp = {0, 0, htonl(XPROOFD_VERSBIN), 0};

   XrdProofdProtocol *xp;
   int dlen;

   // Peek at the first 20 bytes of data
   if ((dlen = lp->Peek(hsbuff,sizeof(hsdata),fgReadWait)) != sizeof(hsdata)) {
      if (dlen <= 0) lp->setEtext("Match: handshake not received");
      return (XrdProtocol *)0;
   }

   // Verify that this is our protocol
   hsdata.third  = ntohl(hsdata.third);
   if (dlen != sizeof(hsdata) ||  hsdata.first || hsdata.second
       || !(hsdata.third == 1) || hsdata.fourth || hsdata.fifth) return 0;

   // Respond to this request with the handshake response
   if (!lp->Send((char *)&hsresp, sizeof(hsresp))) {
      lp->setEtext("Match: handshake failed");
      return (XrdProtocol *)0;
   }

   // We can now read all 20 bytes and discard them (no need to wait for it)
   int len = sizeof(hsdata);
   if (lp->Recv(hsbuff, len) != len) {
      lp->setEtext("Match: reread failed");
      return (XrdProtocol *)0;
   }

   // Get a protocol object off the stack (if none, allocate a new one)
   if (!(xp = fgProtStack.Pop()))
      xp = new XrdProofdProtocol();

   // Bind the protocol to the link and return the protocol
   xp->fLink = lp;
   strcpy(xp->fEntity.prot, "host");
   xp->fEntity.host = strdup((char *)lp->Host());

   // Dummy data used by 'proofd'
   kXR_int32 dum[2];
   if (xp->GetData("dummy",(char *)&dum[0],sizeof(dum)) != 0) {
      xp->Recycle(0,0,0);
      return (XrdProtocol *)0;
   }

   // We are done
   return (XrdProtocol *)xp;
}

//_____________________________________________________________________________
int XrdProofdProtocol::Stats(char *buff, int blen, int)
{
   // Return statistics info about the protocol.
   // Not really implemented yet: this is a reduced XrdXrootd version.

   static char statfmt[] = "<stats id=\"xproofd\"><num>%ld</num></stats>";

   // If caller wants only size, give it to him
   if (!buff)
      return sizeof(statfmt)+16;

   // We have only one statistic -- number of successful matches
   return snprintf(buff, blen, statfmt, fgCount);
}

//______________________________________________________________________________
void XrdProofdProtocol::Reset()
{
   // Reset static and local vars

   // Init local vars
   fLink      = 0;
   fArgp      = 0;
   fStatus    = 0;
   SafeDelArray(fClientID);
   SafeDelArray(fGroupID);
   fUI.Reset();
   fCapVer    = 0;
   fSrvType   = kXPD_TopMaster;
   fTopClient = 0;
   fSuperUser = 0;
   fPClient   = 0;
   fCID       = -1;
   fClient    = 0;
   SafeDelete(fClient);
   if (fAuthProt) {
      fAuthProt->Delete();
      fAuthProt = 0;
   }
   memset(&fEntity, 0, sizeof(fEntity));
   fTopClient = 0;
   fSuperUser = 0;
   fBuff      = 0;
   fBlen      = 0;
   fBlast     = 0;
   // Magic numbers cut & pasted from Xrootd
   fhcPrev    = 13;
   fhcMax     = 28657;
   fhcNext    = 21;
   fhcNow     = 13;
   fhalfBSize = 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::Configure(char *parms, XrdProtocol_Config *pi)
{
   // Protocol configuration tool
   // Function: Establish configuration at load time.
   // Output: 1 upon success or 0 otherwise.

   XrdOucString mp;

   // Only once
   if (fgConfigDone)
      return 1;
   fgConfigDone = 1;

   // Copy out the special info we want to use at top level
   fgEDest.logger(&fgMainLogger);
   XrdProofdTrace = new XrdOucTrace(&fgEDest);
   fgSched        = pi->Sched;
   fgBPool        = pi->BPool;
   fgReadWait     = pi->readWait;

   // Debug flag
   TRACESET(XERR, 1);
   if (pi->DebugON)
      XrdProofdTrace->What |= (TRACE_REQ | TRACE_LOGIN | TRACE_FORK);

   // Multi-user option (default ON if superuser)
   fgMultiUser = (!getuid()) ? 1 : 0;

   // Process the config file for directives meaningful to us
   if (pi->ConfigFN) {
      // Save path for re-configuration checks
      fgCfgFile.fName = pi->ConfigFN;
      XrdProofdAux::Expand(fgCfgFile.fName);
      // Configure tracing
      if (TraceConfig(fgCfgFile.fName.c_str()))
         return 0;
      // Configure the manager
      if (fgMgr.Config(fgCfgFile.fName.c_str(), &fgEDest))
         return 0;
      // Configure the protocol
      if (Config(fgCfgFile.fName.c_str()))
         return 0;
   }

   // Notify port
   mp = "Proofd : Configure: listening on port ";
   mp += fgMgr.Port();
   fgEDest.Say(0, mp.c_str());

   // Default pool entry point is this host
   int pulen = strlen("root://") + strlen(fgMgr.Host());
   fgPoolURL = (char *) malloc(pulen + 1);
   if (!fgPoolURL)
      return 0;
   sprintf(fgPoolURL,"root://%s", fgMgr.Host());
   fgPoolURL[pulen] = 0;

   // Pre-initialize some i/o values
   fgMaxBuffsz = fgBPool->MaxSize();

   // Now process and configuration parameters: if we are not run as
   // default protocol those specified on the xrd.protocol line have
   // priority
   char *pe = parms;

   // Find out timeout on internal communications
   char *pto = pe ? (char *)strstr(pe+1, "intwait:") : 0;
   if (pto) {
      pe = (char *)strstr(pto, " ");
      if (pe) *pe = 0;
      fgInternalWait = strtol(pto+8, 0, 10);
      fgEDest.Say(0, "Proofd : Configure: setting internal timeout to (secs): ", pto+8);
   }

   // Find out if a specific temporary directory is required
   char *tmp = parms ? (char *)strstr(parms, "tmp:") : 0;
   if (tmp)
      tmp += 5;
   fgTMPdir = tmp ? strdup(tmp) : strdup("/tmp");
   fgEDest.Say(0, "Proofd : Configure: using temp dir: ", fgTMPdir);

   // Initialize the security system if this is wanted
   if (!fgSecLib)
      fgEDest.Say(0, "XRD seclib not specified; strong authentication disabled");
   else {
      if (!(fgCIA = XrdProofdProtocol::LoadSecurity(fgSecLib, pi->ConfigFN))) {
         fgEDest.Emsg(0, "Proofd : Configure: unable to load security system.");
         return 0;
      }
      fgEDest.Emsg(0, "Proofd : Configure: security library loaded");
   }

   // Notify role
   const char *roles[] = { "any", "worker", "submaster", "master" };
   fgEDest.Say(0, "Proofd : Configure: role set to: ", roles[fgMgr.SrvType()+1]);

   // Notify allow rules
   if (fgMgr.SrvType() == kXPD_WorkerServer || fgMgr.SrvType() == kXPD_MasterServer) {
      if (fgMastersAllowed.size() > 0) {
         std::list<XrdOucString *>::iterator i;
         for (i = fgMastersAllowed.begin(); i != fgMastersAllowed.end(); ++i)
            fgEDest.Say(0, "Proofd : Configure: masters allowed to connect: ", (*i)->c_str());
      } else {
            fgEDest.Say(0, "Proofd : Configure: masters allowed to connect: any");
      }
   }

   // Notify change priority rules
   if (fgPriorities.size() > 0) {
      std::list<XrdProofdPriority *>::iterator i;
      for (i = fgPriorities.begin(); i != fgPriorities.end(); ++i) {
         XrdOucString msg("priority will be changed by ");
         msg += (*i)->fDeltaPriority;
         msg += " for user(s): ";
         msg += (*i)->fUser;
         fgEDest.Say(0, "Proofd : Configure: ", msg.c_str());
      }
   } else {
      fgEDest.Say(0, "Proofd : Configure: no priority changes requested");
   }

   // Pool and namespace
   fgEDest.Say(0, "Proofd : Configure: PROOF pool: ", fgPoolURL);
   fgEDest.Say(0, "Proofd : Configure: PROOF pool namespace: ", fgNamespace);

   // Initialize resource broker (if not worker)
   if (fgMgr.SrvType() != kXPD_WorkerServer) {

      // Scheduler instance
      if (!(fgProofSched = LoadScheduler(fgCfgFile.fName.c_str(), &fgEDest))) {
         fgEDest.Say(0, "Proofd : Configure: scheduler initialization failed");
         return 0;
      }

      if (!fgMgr.PROOFcfg() || strlen(fgMgr.PROOFcfg()) <= 0)
         // Enable user config files
         fgWorkerUsrCfg = 1;
      const char *st[] = { "disabled", "enabled" };
      fgEDest.Say(0, "Proofd : Configure: user config files are ", st[fgWorkerUsrCfg]);
   }

   // Shutdown options
   mp = "Proofd : Configure: client sessions shutdown after disconnection";
   if (fgShutdownOpt > 0) {
      if (fgShutdownOpt == 1)
         mp = "Proofd : Configure: client sessions kept idle for ";
      else if (fgShutdownOpt == 2)
         mp = "Proofd : Configure: client sessions kept for ";
      mp += fgShutdownDelay;
      mp += " secs after disconnection";
   }
   fgEDest.Say(0, mp.c_str());

   // Superusers: add default
   if (fgSuperUsers) {
      int l = strlen(fgSuperUsers);
      char *su = (char *) malloc(l + strlen(fgMgr.EffectiveUser()) + 2);
      if (su) {
         sprintf(su, "%s,%s", fgMgr.EffectiveUser(), fgSuperUsers);
         free(fgSuperUsers);
         fgSuperUsers = su;
      } else {
         // Failure: stop
         fgEDest.Say(0, "Proofd : Configure: no memory for superuser list - stop");
         return 0;
      }
   } else {
      fgSuperUsers = strdup(fgMgr.EffectiveUser());
   }
   mp = "Proofd : Configure: list of superusers: ";
   mp += fgSuperUsers;
   fgEDest.Say(0, mp.c_str());

   // Notify controlled mode, if such
   if (fgOperationMode == kXPD_OpModeControlled) {
      fgAllowedUsers += ',';
      fgAllowedUsers += fgSuperUsers;
      mp = "Proofd : Configure: running in controlled access mode: users allowed: ";
      mp += fgAllowedUsers;
      fgEDest.Say(0, mp.c_str());
   }

   // Bare lib path
   if (getenv(XPD_LIBPATH)) {
      // Try to remove existing ROOT dirs in the path
      XrdOucString paths = getenv(XPD_LIBPATH);
      XrdOucString ldir;
      int from = 0;
      while ((from = paths.tokenize(ldir, from, ':')) != STR_NPOS) {
         bool isROOT = 0;
         if (ldir.length() > 0) {
            // Check this dir
            DIR *dir = opendir(ldir.c_str());
            if (dir) {
               // Scan the directory
               struct dirent *ent = 0;
               while ((ent = (struct dirent *)readdir(dir))) {
                  if (!strncmp(ent->d_name, "libCore", 7)) {
                     isROOT = 1;
                     break;
                  }
               }
               // Close the directory
               closedir(dir);
            }
            if (!isROOT) {
               if (fgBareLibPath.length() > 0)
                  fgBareLibPath += ":";
               fgBareLibPath += ldir;
            }
         }
      }
      fgEDest.Say(0, "Proofd : Configure: bare lib path for proofserv: ",
                     fgBareLibPath.c_str());
   }

   // Validate the ROOT dirs
   if (fgROOT.size() <= 0) {
      // None defined: use ROOTSYS as default, if any; otherwise we fail
      if (getenv("ROOTSYS")) {
         fgROOT.push_back(new XrdROOT(getenv("ROOTSYS"), ""));
      } else {
         fgEDest.Say(0, "Proofd : Configure: no ROOT dir defined;"
                        " ROOTSYS location missing - unloading");
         return 0;
      }
   }
   XrdOucString tags;
   std::list<XrdROOT *>::iterator xri;
   for (xri = fgROOT.begin(); xri != fgROOT.end();) {
      mp = "Proofd : Configure: ROOT dist: \"";
      // Tag for checking duplications and the record
      XrdOucString tag("|"); tag += (*xri)->Tag(); tag += "|";
      bool ok = 0;
      if (tags.find(tag) == STR_NPOS) {
         mp += (*xri)->Export();
         // Validate this version
         if ((*xri)->Validate()) {
            mp += "\" validated";
            ++xri;
            tags += tag;
            ok = 1;
         } else
            mp += "\" could not be validated: removing from the list";
      } else {
         // Version tags must be unique
         mp = "Proofd : Configure: version tag '";
         mp += (*xri)->Tag();
         mp += "' already existing: removing from the list";
      }
      if (!ok) {
         delete (*xri);
         xri = fgROOT.erase(xri);
      }
      fgEDest.Say(0, mp.c_str());
   }
   if (fgROOT.size() <= 0) {
      // None defined: we need at least one, so we fail
      fgEDest.Say(0, "Proofd : Configure: no valid ROOT dir found: unloading");
      return 0;
   }

   // Groups
   fgGroupsMgr.Print(0);

   // Scheduling option
   if (fgGroupsMgr.Num() > 1 && fgSchedOpt != kXPD_sched_off) {
      mp = "Configure: worker sched based on: ";
      mp += (fgSchedOpt == kXPD_sched_fraction) ? "fractions" : "priorities";
      fgEDest.Say(0, mp.c_str());
   }

   // Dataset dir
   if (fgMgr.DataSetDir()) {
      // Make sure that the group dataset dirs exists
      fgGroupsMgr.Apply(CreateGroupDataSetDir, (void *)fgMgr.DataSetDir());
   }

   // Schedule protocol object cleanup; the maximum number of objects
   // and the max age are taken from XrdXrootdProtocol: this may need
   // some optimization in the future.
   fgProtStack.Set(pi->Sched, XrdProofdTrace, TRACE_MEM);
   fgProtStack.Set((pi->ConnMax/3 ? pi->ConnMax/3 : 30), 60*60);

   // Initialize the request ID generation object
   XrdProofdReqID = new XrdOucReqID((int)fgMgr.Port(), pi->myName,
                                    XrdNetDNS::IPAddr(pi->myAddr));

   // Start cron thread, if required
   if (fgCron == 1) {
      pthread_t tid;
      if (XrdSysThread::Run(&tid, XrdProofdCron, (void *)&fgCronFrequency, 0,
                                    "Proof cron thread") != 0) {
         fgEDest.Say(0, "Proofd : Configure: could not start cron thread");
         return 0;
      }
      fgEDest.Say(0, "Proofd : Configure: cron thread started");
   }

   // Indicate we configured successfully
   fgEDest.Say(0, "XProofd protocol version " XPROOFD_VERSION
               " build " XrdVERSION " successfully loaded.");

   // Return success
   return 1;
}

//______________________________________________________________________________
bool XrdProofdProtocol::CheckMaster(const char *m)
{
   // Check if master 'm' is allowed to connect to this host
   bool rc = 1;

   if (fgMastersAllowed.size() > 0) {
      rc = 0;
      XrdOucString wm(m);
      std::list<XrdOucString *>::iterator i;
      for (i = fgMastersAllowed.begin(); i != fgMastersAllowed.end(); ++i) {
         if (wm.matches((*i)->c_str())) {
            rc = 1;
            break;
         }
      }
   }

   // We are done
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::TraceConfig(const char *cfn)
{
   // Scan the config file for tracing settings

   TRACE(ACT, "TraceConfig: enter: file: " <<cfn);

   // Get the modification time
   struct stat st;
   if (stat(cfn, &st) != 0)
      return -1;
   TRACE(DBG, "TraceConfig: time of last modification: " << st.st_mtime);
   fgCfgFile.fMtime = st.st_mtime;

   XrdOucStream cfg(&fgEDest, getenv("XRDINSTANCE"));

   // Open and attach the config file
   int cfgFD;
   if ((cfgFD = open(cfn, O_RDONLY, 0)) < 0)
      return fgEDest.Emsg("Config", errno, "open config file", cfn);
   cfg.Attach(cfgFD);

   // Process items
   char *val = 0, *var = 0;
   while ((var = cfg.GetMyFirstWord())) {
      if (!(strncmp("xpd.trace", var, 9))) {
         // Get the value
         val = cfg.GetToken();
         if (val && val[0]) {
            // Specifies tracing options. Valid keywords are:
            //   req            trace protocol requests             [on]*
            //   login          trace details about login requests  [on]*
            //   act            trace internal actions              [off]
            //   rsp            trace server replies                [off]
            //   fork           trace proofserv forks               [on]*
            //   dbg            trace details about actions         [off]
            //   hdbg           trace more details about actions    [off]
            //   err            trace errors                        [on]
            //   inflt          trace details about inflate factors [off]
            //   all            trace everything
            //
            // Defaults are shown in brackets; '*' shows the default when the '-d'
            // option is passed on the command line. Each option may be
            // optionally prefixed by a minus sign to turn off the setting.
            // Order matters: 'all' in last position enables everything; in first
            // position is corrected by subsequent settings
            //
            while (val && val[0]) {
               bool on = 1;
               if (val[0] == '-') {
                  on = 0;
                  val++;
               }
               if (!strcmp(val,"req")) {
                  TRACESET(REQ, on);
               } else if (!strcmp(val,"login")) {
                  TRACESET(LOGIN, on);
               } else if (!strcmp(val,"act")) {
                  TRACESET(ACT, on);
               } else if (!strcmp(val,"rsp")) {
                  TRACESET(RSP, on);
               } else if (!strcmp(val,"fork")) {
                  TRACESET(FORK, on);
               } else if (!strcmp(val,"dbg")) {
                  TRACESET(DBG, on);
               } else if (!strcmp(val,"hdbg")) {
                  TRACESET(HDBG, on);
               } else if (!strcmp(val,"err")) {
                  TRACESET(XERR, on);
               } else if (!strcmp(val,"inflt")) {
                  TRACESET(INFLT, on);
               } else if (!strcmp(val,"all")) {
                  // Everything
                  XrdProofdTrace->What = (on) ? TRACE_ALL : 0;
               }
               // Next
               val = cfg.GetToken();
            }
         }
      }
   }
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::Config(const char *cfn)
{
   // Scan the config file

   TRACE(ACT, "Config: enter: file: " <<cfn);

   // Get the modification time
   struct stat st;
   if (stat(cfn, &st) != 0)
      return -1;
   TRACE(DBG, "Config: time of last modification: " << st.st_mtime);
   fgCfgFile.fMtime = st.st_mtime;

   XrdOucStream Config(&fgEDest, getenv("XRDINSTANCE"));
   char *var;
   int cfgFD, NoGo = 0;
   int nmTmp = -1, nmInternalWait = -1, nmMaxOldLogs = -1,
       nmPoolUrl = -1, nmNamespace = -1, nmSuperUsers = -1, nmSchedOpt = -1;

   // Open and attach the config file
   if ((cfgFD = open(cfn, O_RDONLY, 0)) < 0)
      return fgEDest.Emsg("Config", errno, "open config file", cfn);
   Config.Attach(cfgFD);

   // Process items
   char mess[512];
   char *val = 0;
   while ((var = Config.GetMyFirstWord())) {
      if (!(strncmp("xrootd.seclib", var, 13))) {
         if ((val = Config.GetToken()) && val[0]) {
            SafeFree(fgSecLib);
            fgSecLib = strdup(val);
         }
      } else if (!(strncmp("xpd.", var, 4)) && var[4]) {
         var += 4;
         // Get the value
         val = Config.GetToken();
         if (val && val[0]) {
            sprintf(mess,"Processing '%s = %s [if <pattern>]'", var, val);
            TRACE(DBG, "Config: " <<mess);
            // Treat first those not supporting 'if <pattern>'
            if (!strcmp("resource",var)) {
               // Specifies the resource broker
               if (!strcmp("static",val)) {
                  /* Using a config file; format of the remaining tokens is
                  // [<cfg_file>] [ucfg:<user_cfg_opt>] \
                  //              [wmx:<max_workers>] [selopt:<selection_mode>]
                  // where:
                  //         <cfg_file>          path to the config file
                  //                            [$ROOTSYS/proof/etc/proof.conf]
                  //         <user_cfg_opt>     "yes"/"no" enables/disables user
                  //                            private config files ["no"].
                  //                            If enable, the default file path
                  //                            is $HOME/.proof.conf (can be changed
                  //                            as option to TProof::Open() ).
                  //         <max_workers>       maximum number of workers per user
                  //                            [all]
                  //         <selection_mode>   selection mode in case not all the
                  //                            workers have to be allocated.
                  //                            Options: "rndm", "rrobin"
                  //                            ["rrobin"] */
                  while ((val = Config.GetToken()) && val[0]) {
                     XrdOucString s(val);
                     if (s.beginswith("ucfg:")) {
                        fgWorkerUsrCfg = s.endswith("yes") ? 1 : 0;
                     }
                  }
               }

            } else if (!strcmp("groupfile",var)) {

               // Defines file with the group info
               fgGroupsMgr.Config(val);

            } else if (!strcmp("multiuser",var)) {
               // Multi-user option
               int mu = strtol(val,0,10);
               fgMultiUser = (mu == 1) ? 1 : fgMultiUser;

            } else if (!strcmp("priority",var)) {
               // Priority change directive: get delta_priority
               int dp = strtol(val,0,10);
               XrdProofdPriority *p = new XrdProofdPriority("*", dp);
               // Check if an 'if' condition is present
               if ((val = Config.GetToken()) && !strncmp(val,"if",2)) {
                  if ((val = Config.GetToken()) && val[0]) {
                     p->fUser = val;
                  }
               }
               // Add to the list
               fgPriorities.push_back(p);
            } else if (!strcmp("seclib",var)) {
               // Record the path
               SafeFree(fgSecLib);
               fgSecLib = strdup(val);
            } else if (!strcmp("shutdown",var)) {
               // Shutdown option
               int dp = strtol(val,0,10);
               if (dp >= 0 && dp <= 2)
                  fgShutdownOpt = dp;
               // Shutdown delay
               if ((val = Config.GetToken())) {
                  int l = strlen(val);
                  int f = 1;
                  XrdOucString tval = val;
                  // Parse
                  if (val[l-1] == 's') {
                     val[l-1] = 0;
                  } else if (val[l-1] == 'm') {
                     f = 60;
                     val[l-1] = 0;
                  } else if (val[l-1] == 'h') {
                     f = 3600;
                     val[l-1] = 0;
                  } else if (val[l-1] < 48 || val[l-1] > 57) {
                     f = -1;
                  }
                  if (f > 0) {
                     int de = strtol(val,0,10);
                     if (de > 0)
                        fgShutdownDelay = de * f;
                  }
               }
            } else if (!strcmp("rootsys",var)) {
               // Two tokens may be meaningful
               XrdOucString dir = val;
               val = Config.GetToken();
               XrdOucString tag = val;
               bool ok = 1;
               if (tag == "if") {
                  tag = "";
                  // Conditional
                  Config.RetToken();
                  ok = (XrdProofdAux::CheckIf(&Config, fgMgr.Host()) > 0) ? 1 : 0;
               }
               if (ok)
                  // Add to the list; it will be validated later on
                  fgROOT.push_back(new XrdROOT(dir.c_str(), tag.c_str()));
            } else if (!strcmp("putenv",var)) {
               // Env variable to exported to 'proofserv'
               if (fgProofServEnvs.length() > 0)
                  fgProofServEnvs += ',';
               fgProofServEnvs += val;
            } else if (!strcmp("putrc",var)) {
               // rootrc variable to be passed to 'proofserv':
               if (fgProofServRCs.length() > 0)
                  fgProofServRCs += ',';
               fgProofServRCs += val;
               while ((val = Config.GetToken()) && val[0]) {
                  fgProofServRCs += ' ';
                  fgProofServRCs += val;
               }
            //
            // The following ones support the 'if <pattern>'
            } else {
               // Save 'val' first
               XrdOucString tval = val;
               // Number of matching chars: the parameter will be updated only
               // if condition is absent or equivalent/better matching
               int nm = XrdProofdAux::CheckIf(&Config, fgMgr.Host());
               // Now check
               if (!strcmp("tmp",var)) {
                  // TMP directory
                  XPDSETSTRING(nm, nmTmp, fgTMPdir, tval);
               } else if (!strcmp("intwait",var)) {
                  // Internal time out
                  XPDSETINT(nm, nmInternalWait, fgInternalWait, tval);
               } else if (!strcmp("maxoldlogs",var)) {
                  // Max number of sessions per user
                  int maxoldlogs = XPC_DEFMAXOLDLOGS;
                  XPDSETINT(nm, nmMaxOldLogs, maxoldlogs, tval);
                  XrdProofdClient::SetMaxOldLogs(maxoldlogs);
               } else if (!strcmp("poolurl",var)) {
                  // Local pool entry point
                  XPDSETSTRING(nm, nmPoolUrl, fgPoolURL, tval);
               } else if (!strcmp("namespace",var)) {
                  // Local namespace
                  XPDSETSTRING(nm, nmNamespace, fgNamespace, tval);
               } else if (!strcmp("superusers",var)) {
                  // Superusers
                  XPDSETSTRING(nm, nmSuperUsers, fgSuperUsers, tval);
               } else if (!strcmp("allowedusers",var)) {
                  // Users allowed to use the cluster
                  fgAllowedUsers = tval;
                  fgOperationMode = kXPD_OpModeControlled;

               } else if (!strcmp("schedopt",var)) {
                  if (XPDCOND(nm, nmSchedOpt)) {
                     // Defines scheduling options
                     while (val && val[0]) {
                        XrdOucString o = val;
                        if (o.beginswith("overall:")) {
                           // The overall inflating factor
                           o.replace("overall:","");
                           float of = 1.;
                           sscanf(o.c_str(), "%f", &of);
                           fgOverallInflate = (of >= 1.) ? of : fgOverallInflate;
                        } else {
                           if (o == "fraction")
                              fgSchedOpt = kXPD_sched_fraction;
                           else if (o == "priority")
                              fgSchedOpt = kXPD_sched_priority;
                        }
                        // Next
                        val = Config.GetToken();
                     }
                     // New reference
                     nmSchedOpt = nm;
                  }
               }
            }
         } else {
            sprintf(mess,"%s not specified", var);
            fgEDest.Emsg("Config", mess);
         }
      }
   }

   // Change/DonotChange ownership when logging clients
   fgChangeOwn = (fgMultiUser && getuid()) ? 0 : 1;

   return NoGo;
}

//______________________________________________________________________________
int XrdProofdProtocol::Reconfig()
{
   // Rescan the config file, but only if it changed.
   // Warning: changes in the authentication policy need to log-off all users
   // and reconnect them according to the new policy: these changes are
   // ignored here.
   // TODO: this method overlaps with Configure, so the overlapping part
   // should be moved to some new method.

   // Check inputs
   if (fgCfgFile.fName.length() <= 0) {
      TRACE(XERR, "Reconfig: config file undefined!!!");
      return -1;
   }

   // Get the modification time
   struct stat st;
   if (stat(fgCfgFile.fName.c_str(), &st) != 0) {
      TRACE(XERR, "Reconfig: cant stat config file " << fgCfgFile.fName <<
                                         ": errno: " << errno);
      return -1;
   }
   TRACE(DBG, "Reconfig: time of last modification: " << st.st_mtime<<
                                               " vs " << fgCfgFile.fMtime);

   // File must have changed
   if (st.st_mtime <= fgCfgFile.fMtime) {
      TRACE(HDBG, "Reconfig: file "<< fgCfgFile.fName << " unchanged: do nothing");
      return 0;
   }

   XrdSysMutexHelper mtxh(&fgXPDMutex);

   // Reconfigure tracing
   TraceConfig(fgCfgFile.fName.c_str());

   // Reconfigure the manager
   fgMgr.Config(fgCfgFile.fName.c_str(), &fgEDest);

   TRACE(HDBG, "Reconfig: file " << fgCfgFile.fName << " changed since last check: rescan");
   fgCfgFile.fMtime = st.st_mtime;

   XrdOucStream Config(&fgEDest, getenv("XRDINSTANCE"));
   char *var;
   int cfgFD;
   int nmTmp = -1, nmInternalWait = -1, nmMaxOldLogs = -1,
       nmPoolUrl = -1, nmNamespace = -1, nmSuperUsers = -1;

   // Open and attach the config file
   if ((cfgFD = open(fgCfgFile.fName.c_str(), O_RDONLY, 0)) < 0)
      return fgEDest.Emsg("Reconfig", errno, "open config file", fgCfgFile.fName.c_str());
   Config.Attach(cfgFD);

   // Temporary list of ROOT versions
   std::list<XrdROOT *> tROOT;

   // Reset what needs to be reset
   std::list<XrdOucString *>::iterator si = fgMastersAllowed.begin();
   while (si != fgMastersAllowed.end()) {
      delete *si;
      si = fgMastersAllowed.erase(si);
   }
   std::list<XrdProofdPriority *>::iterator pi = fgPriorities.begin();
   while (pi != fgPriorities.end()) {
      delete *pi;
      pi = fgPriorities.erase(pi);
   }
   fgShutdownOpt = 1;
   fgShutdownDelay = 0;
   fgInternalWait = 5;
   XrdProofdClient::SetMaxOldLogs(10);
   SafeFree(fgSuperUsers);
   fgAllowedUsers = "";
   fgOperationMode = kXPD_OpModeOpen;
   fgProofServEnvs = "";
   fgProofServRCs = "";
   // Reset the group info
   fgGroupsMgr.Config(0);

   // Process items
   char mess[512];
   char *val = 0;
   while ((var = Config.GetMyFirstWord())) {
      if (!(strncmp("xpd.", var, 4)) && var[4]) {
         var += 4;
         // Get the value
         val = Config.GetToken();
         if (val && val[0]) {
            sprintf(mess,"Processing '%s = %s [if <pattern>]'", var, val);
            TRACE(HDBG, "Reconfig: " <<mess);
            // Treat first those not supporting 'if <pattern>'
            if (!strcmp("resource",var)) {
               // Specifies the resource broker
               if (!strcmp("static",val)) {
                  /* Using a config file; format of the remaining tokens is
                  // [<cfg_file>] [ucfg:<user_cfg_opt>] \
                  //              [wmx:<max_workers>] [selopt:<selection_mode>]
                  // where:
                  //         <cfg_file>          path to the config file
                  //                            [$ROOTSYS/proof/etc/proof.conf]
                  //         <user_cfg_opt>     "yes"/"no" enables/disables user
                  //                            private config files ["no"].
                  //                            If enable, the default file path
                  //                            is $HOME/.proof.conf (can be changed
                  //                            as option to TProof::Open() ).
                  //         <max_workers>       maximum number of workers per user
                  //                            [all]
                  //         <selection_mode>   selection mode in case not all the
                  //                            workers have to be allocated.
                  //                            Options: "rndm", "rrobin"
                  //                            ["rrobin"] */
                  while ((val = Config.GetToken()) && val[0]) {
                     XrdOucString s(val);
                     if (s.beginswith("ucfg:")) {
                        fgWorkerUsrCfg = s.endswith("yes") ? 1 : 0;
                     }
                  }
               }

            } else if (!strcmp("groupfile",var)) {

               // Defines file with the group info
               fgGroupsMgr.Config(val);

            } else if (!strcmp("multiuser",var)) {
               // Multi-user option
               int mu = strtol(val,0,10);
               fgMultiUser = (mu == 1) ? 1 : fgMultiUser;

            } else if (!strcmp("priority",var)) {
               // Priority change directive: get delta_priority
               int dp = strtol(val,0,10);
               XrdProofdPriority *p = new XrdProofdPriority("*", dp);
               // Check if an 'if' condition is present
               if ((val = Config.GetToken()) && !strncmp(val,"if",2)) {
                  if ((val = Config.GetToken()) && val[0]) {
                     p->fUser = val;
                  }
               }
               // Add to the list
               fgPriorities.push_back(p);
            } else if (!strcmp("seclib",var) || !strncmp("sec.",var,4)) {
               // Cannot reconfigure security on the fly: ignore
            } else if (!strcmp("shutdown",var)) {
               // Shutdown option
               int dp = strtol(val,0,10);
               if (dp >= 0 && dp <= 2)
                  fgShutdownOpt = dp;
               // Shutdown delay
               if ((val = Config.GetToken())) {
                  int l = strlen(val);
                  int f = 1;
                  XrdOucString tval = val;
                  // Parse
                  if (val[l-1] == 's') {
                     val[l-1] = 0;
                  } else if (val[l-1] == 'm') {
                     f = 60;
                     val[l-1] = 0;
                  } else if (val[l-1] == 'h') {
                     f = 3600;
                     val[l-1] = 0;
                  } else if (val[l-1] < 48 || val[l-1] > 57) {
                     f = -1;
                  }
                  if (f > 0) {
                     int de = strtol(val,0,10);
                     if (de > 0)
                        fgShutdownDelay = de * f;
                  }
               }
            } else if (!strcmp("rootsys",var)) {
               // Two tokens may be meaningful
               XrdOucString dir = val;
               val = Config.GetToken();
               XrdOucString tag = val;
               bool ok = 1;
               if (tag == "if") {
                  tag = "";
                  // Conditional
                  Config.RetToken();
                  ok = (XrdProofdAux::CheckIf(&Config, fgMgr.Host()) > 0) ? 1 : 0;
               }
               if (ok) {
                  // Init an instance (it will create a tag, if needed)
                  XrdROOT *r = new XrdROOT(dir.c_str(), tag.c_str());
                  // Add to the list (we validate it later)
                  tROOT.push_back(r);
               }
            } else if (!strcmp("putenv",var)) {
               // Env variable to exported to 'proofserv'
               if (fgProofServEnvs.length() > 0)
                  fgProofServEnvs += ',';
               fgProofServEnvs += val;
            } else if (!strcmp("putrc",var)) {
               // rootrc variable to be passed to 'proofserv':
               if (fgProofServRCs.length() > 0)
                  fgProofServRCs += ',';
               fgProofServRCs += val;
               while ((val = Config.GetToken()) && val[0]) {
                  fgProofServRCs += ' ';
                  fgProofServRCs += val;
               }
            //
            // The following ones support the 'if <pattern>'
            } else {
               // Save 'val' first
               XrdOucString tval = val;
               // Number of matching chars: the parameter will be updated only
               // if condition is absent or equivalent/better matching
               int nm = XrdProofdAux::CheckIf(&Config, fgMgr.Host());
               // Now check
               if (!strcmp("tmp",var)) {
                  // TMP directory
                  XPDADOPTSTRING(nm, nmTmp, fgTMPdir, tval);
               } else if (!strcmp("intwait",var)) {
                  // Internal time out
                  XPDSETINT(nm, nmInternalWait, fgInternalWait, tval);
               } else if (!strcmp("maxoldlogs",var)) {
                  // Max number of sessions per user
                  int maxoldlogs = XPC_DEFMAXOLDLOGS;
                  XPDSETINT(nm, nmMaxOldLogs, maxoldlogs, tval);
                  XrdProofdClient::SetMaxOldLogs(maxoldlogs);
               } else if (!strcmp("allow",var)) {
                  // Masters allowed to connect
                  if (nm == -1 || nm > 0)
                     fgMastersAllowed.push_back(new XrdOucString(tval));
               } else if (!strcmp("poolurl",var)) {
                  // Local pool entry point
                  XPDADOPTSTRING(nm, nmPoolUrl, fgPoolURL, tval);
               } else if (!strcmp("namespace",var)) {
                  // Local namespace
                  XPDADOPTSTRING(nm, nmNamespace, fgNamespace, tval);
               } else if (!strcmp("superusers",var)) {
                  // Superusers
                  XPDSETSTRING(nm, nmSuperUsers, fgSuperUsers, tval);
               } else if (!strcmp("allowedusers",var)) {
                  // Users allowed to use the cluster
                  fgAllowedUsers = tval;
                  fgOperationMode = kXPD_OpModeControlled;
               }
            }
         } else {
            sprintf(mess,"%s not specified", var);
            fgEDest.Emsg("Reconfig", mess);
         }
      }
   }

   // Save ROOT version info
   if (tROOT.size() <= 0) {
      if (getenv("ROOTSYS")) {
         XrdROOT *r = new XrdROOT(getenv("ROOTSYS"), "");
         tROOT.push_back(r);
      }
   }
   // Validate new list of ROOT sys
   std::list<XrdROOT *>::iterator tri;
   if (tROOT.size() > 0) {
      for (tri = tROOT.begin(); tri != tROOT.end();) {
         // Check if already validated
         std::list<XrdROOT *>::iterator ori;
         for (ori = fgROOT.begin(); ori != fgROOT.end(); ori++) {
            if ((*ori)->Match((*tri)->Tag(), (*tri)->Dir()))
               if ((*ori)->IsValid())
                  (*tri)->SetValid();
         }
         // If not, try validation
         if (!(*tri)->IsValid() && !(*tri)->IsInvalid())
            if ((*tri)->Validate()) {
               fgEDest.Say(0, "Reconfig: validation OK for: ", (*tri)->Export());
            } else {
               fgEDest.Say(0, "Reconfig: cannot validate: ", (*tri)->Export());
            }
         // Remove if invalid
         if ((*tri)->IsValid()) {
            ++tri;
         } else {
            delete *tri;
            tri = tROOT.erase(tri);
         }
      }
   }

   // Put in the official list
   if (tROOT.size() <= 0) {
      fgEDest.Say(0, "Reconfig: at least one ROOT version needed: ignore changes");
   } else {
      // Reassign ROOTsys info to existing clients
      std::list<XrdProofdClient *>::iterator pci;
      for (pci = fgProofdClients.begin(); pci != fgProofdClients.end(); ++pci) {
         // Attached ROOT version
         XrdROOT *r = (*pci)->ROOT();
         if (r) {
            // Search the new list for compatible instances
            (*pci)->SetROOT((XrdROOT *)0);
            for (tri = tROOT.begin(); tri != tROOT.end();) {
               if ((*tri)->Match(r->Dir(),r->Tag())) {
                  (*pci)->SetROOT(*tri);
                  break;
               }
            }
            if (!((*pci)->ROOT()))
               // Set the new default
               (*pci)->SetROOT(tROOT.front());
         }
      }

      // Cleanup official list first
      for (tri = fgROOT.begin(); tri != fgROOT.end();) {
          delete *tri;
          tri = fgROOT.erase(tri);
      }
      // Fill it with new elements, avoiding duplications
      XrdOucString tags;
      for (tri = tROOT.begin(); tri != tROOT.end();) {
          XrdOucString tag("|"); tag += (*tri)->Tag(); tag += "|";
          if (tags.find(tag) == STR_NPOS) {
             tags += tag;
             fgROOT.push_back(*tri);
             ++tri;
          } else {
             delete *tri;
             tri = fgROOT.erase(tri);
          }
      }
   }

   // Superusers: add default
   if (fgSuperUsers) {
      int l = strlen(fgSuperUsers);
      char *su = (char *) malloc(l + strlen(fgMgr.EffectiveUser()) + 2);
      if (su) {
         sprintf(su, "%s,%s", fgMgr.EffectiveUser(), fgSuperUsers);
         free(fgSuperUsers);
         fgSuperUsers = su;
      } else {
         // Failure: stop
         fgEDest.Say(0, "Reconfig: no memory for superuser list - stop");
         return 0;
      }
   } else {
      fgSuperUsers = strdup(fgMgr.EffectiveUser());
   }

   // Notify controlled mode, if such
   if (fgOperationMode == kXPD_OpModeControlled) {
      fgAllowedUsers += ',';
      fgAllowedUsers += fgSuperUsers;
      XrdOucString mp = "Reconfig: running in controlled access mode: users allowed: ";
      mp += fgAllowedUsers;
      fgEDest.Say(0, mp.c_str());
   }

   // Re-assign groups
   if (fgGroupsMgr.Num() > 0) {
      std::list<XrdProofdClient *>::iterator pci;
      for (pci = fgProofdClients.begin(); pci != fgProofdClients.end(); ++pci) {
         // Find first client
         XrdProofdProtocol *c = 0;
         int ic = 0;
         while (ic < (int) (*pci)->Clients()->size())
            if ((c = (*pci)->Clients()->at(ic++)))
               break;
         if (c)
            (*pci)->SetGroup(fgGroupsMgr.GetUserGroup(c->fClientID, c->fGroupID));
      }
   }

   // Change/DonotChange ownership when logging clients
   fgChangeOwn = (fgMultiUser && getuid()) ? 0 : 1;

   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::Process(XrdLink *)
{
   // Process the information received on the active link.
   // (We ignore the argument here)

   int rc = 0;
   TRACEP(REQ, "Process: enter: instance: " << this);

   // Read the next request header
   if ((rc = GetData("request", (char *)&fRequest, sizeof(fRequest))) != 0)
      return rc;
   TRACEP(DBG, "Process: after GetData: rc: " << rc);

   // Deserialize the data
   fRequest.header.requestid = ntohs(fRequest.header.requestid);
   fRequest.header.dlen      = ntohl(fRequest.header.dlen);

   // The stream ID for the reply
   { XrdSysMutexHelper mh(fResponse.fMutex);
      fResponse.Set(fRequest.header.streamid);
      fResponse.Set(fLink);
   }
   unsigned short sid;
   memcpy((void *)&sid, (const void *)&(fRequest.header.streamid[0]), 2);
   TRACEP(DBG, "Process: sid: " << sid <<
               ", req: " <<fRequest.header.requestid <<
               ", dlen: " <<fRequest.header.dlen);

   // Every request has an associated data length. It better be >= 0 or we won't
   // be able to know how much data to read.
   if (fRequest.header.dlen < 0) {
      fResponse.Send(kXR_ArgInvalid, "Process: Invalid request data length");
      return fLink->setEtext("Process: protocol data length error");
   }

   // Read any argument data at this point, except when the request is to forward
   // a buffer: the argument may have to be segmented and we're not prepared to do
   // that here.
   if (fRequest.header.requestid != kXP_sendmsg && fRequest.header.dlen) {
      if (GetBuff(fRequest.header.dlen+1) != 1) {
         fResponse.Send(kXR_ArgTooLong, "fRequest.argument is too long");
         return 0;
      }
      if ((rc = GetData("arg", fArgp->buff, fRequest.header.dlen)))
         return rc;
      fArgp->buff[fRequest.header.dlen] = '\0';
   }

   // Continue with request processing at the resume point
   return Process2();
}

//______________________________________________________________________________
int XrdProofdProtocol::Process2()
{
   // Local processing method: here the request is dispatched to the appropriate
   // method

   TRACEP(REQ, "Process2: enter: req id: " << fRequest.header.requestid);

   // If the user is not yet logged in, restrict what the user can do
   if (!fStatus || !(fStatus & XPD_LOGGEDIN))
      switch(fRequest.header.requestid) {
      case kXP_auth:
         return Auth();
      case kXP_login:
         return Login();
      default:
         TRACEP(XERR,"Process2: invalid request: " <<fRequest.header.requestid);
         fResponse.Send(kXR_InvalidRequest,"Invalid request; user not logged in");
         return fLink->setEtext("protocol sequence error 1");
      }

   // Once logged-in, the user can request the real actions
   XrdOucString emsg("Invalid request code: ");
   switch(fRequest.header.requestid) {
   case kXP_create:
      if (fSrvType != kXPD_Admin)
         return Create();
      else
         emsg += "'admin' role not allowd to process 'create'";
      break;
   case kXP_destroy:
      if (fSrvType != kXPD_Admin)
         return Destroy();
      else
         emsg += "'admin' role not allowd to process 'destroy'";
      break;
   case kXP_sendmsg:
      return SendMsg();
   case kXP_attach:
       if (fSrvType != kXPD_Admin)
         return Attach();
      else
         emsg += "'admin' role not allowd to process 'attach'";
      break;
   case kXP_detach:
      if (fSrvType != kXPD_Admin)
         return Detach();
      else
         emsg += "'admin' role not allowd to process 'detach'";
      break;
   case kXP_admin:
      return Admin();
   case kXP_interrupt:
      if (fSrvType != kXPD_Admin)
         return Interrupt();
      else
         emsg += "'admin' role not allowd to process 'interrupt'";
      break;
   case kXP_ping:
      return Ping();
   case kXP_urgent:
      return Urgent();
   case kXP_readbuf:
      return ReadBuffer();
   default:
      emsg += fRequest.header.requestid;
      break;
   }

   // Whatever we have, it's not valid
   fResponse.Send(kXR_InvalidRequest, emsg.c_str());
   return 0;
}

//______________________________________________________________________
void XrdProofdProtocol::Recycle(XrdLink *, int, const char *)
{
   // Recycle call. Release the instance and give it back to the stack.

   const char *srvtype[6] = {"ANY", "Worker", "Master",
                             "TopMaster", "Internal", "Admin"};

   // Document the disconnect
   TRACEI(REQ,"Recycle: enter: instance: " <<this<<", type: "<<srvtype[fSrvType+1]);

   // If we have a buffer, release it
   if (fArgp) {
      fgBPool->Release(fArgp);
      fArgp = 0;
   }

   // Flag for internal connections: those deserve a different treatment
   bool proofsrv = (fSrvType == kXPD_Internal) ? 1 : 0;

   // Locate the client instance
   XrdProofdClient *pmgr = fPClient;

   if (pmgr) {

      if (!proofsrv) {

         // Reset the corresponding client slot in the list of this client
         // Count the remaining top clients
         int nc = 0;
         int ic = 0;
         for (ic = 0; ic < (int) pmgr->Clients()->size(); ic++) {
            if (this == pmgr->Clients()->at(ic))
               pmgr->ResetClient(ic);
            else if (pmgr->Clients()->at(ic) && pmgr->Clients()->at(ic)->fTopClient)
               nc++;
         }

         // If top master ...
         if (fSrvType == kXPD_TopMaster) {
            XrdSysMutexHelper mtxh(pmgr->Mutex());
            // Loop over servers sessions associated to this client and update
            // their attached client vectors
            if (pmgr->ProofServs()->size() > 0) {
               XrdProofServProxy *psrv = 0;
               int is = 0;
               for (is = 0; is < (int) pmgr->ProofServs()->size(); is++) {
                  if ((psrv = pmgr->ProofServs()->at(is))) {
                     // Release CIDs in attached sessions: loop over attached clients
                     XrdClientID *cid = 0;
                     int ic = 0;
                     for (ic = 0; ic < (int) psrv->Clients()->size(); ic++) {
                        if ((cid = psrv->Clients()->at(ic))) {
                           if (cid->fP == this)
                              cid->Reset();
                        }
                     }
                  }
               }
            }

            // If no more clients schedule a shutdown at the PROOF session
            // by the sending the appropriate information
            if (nc <= 0 && pmgr->ProofServs()->size() > 0) {
               XrdProofServProxy *psrv = 0;
               int is = 0;
               for (is = 0; is < (int) pmgr->ProofServs()->size(); is++) {
                  if ((psrv = pmgr->ProofServs()->at(is)) && psrv->IsValid() &&
                       psrv->SrvType() == kXPD_TopMaster &&
                      (psrv->Status() == kXPD_idle || psrv->Status() == kXPD_running)) {
                     if (psrv->SetShutdownTimer(fgShutdownOpt, fgShutdownDelay) != 0) {
                        // Just notify locally: link is closed!
                        XrdOucString msg("Recycle: could not send shutdown info to proofsrv");
                        TRACEI(XERR, msg.c_str());
                     }
                  }
               }
            }

         } else {

            // We cannot continue if the top master went away: we cleanup the session
            XrdSysMutexHelper mtxh(pmgr->Mutex());
            if (pmgr->ProofServs()->size() > 0) {
               XrdProofServProxy *psrv = 0;
               int is = 0;
               for (is = 0; is < (int) pmgr->ProofServs()->size(); is++) {
                  if ((psrv = pmgr->ProofServs()->at(is)) && psrv->IsValid()
                      && psrv->SrvType() != kXPD_TopMaster) {

                     TRACEI(HDBG, "Recycle: found: " << psrv << " (t:"<<psrv->SrvType() <<
                                  ",nc:"<<psrv->Clients()->size()<<")");

                     XrdSysMutexHelper xpmh(psrv->Mutex());

                     // Send a terminate signal to the proofserv
                     if (LogTerminatedProc(psrv->TerminateProofServ()) != 0)
                        // Try hard kill
                        if (LogTerminatedProc(KillProofServ(psrv->SrvID(), 1)) != 0) {
                           TRACEI(XERR, "Recycle: problems terminating proofsrv");
                        }

                     // Reset instance
                     psrv->Reset();
                  }
               }
            }
         }

      } else {

         // Internal connection: we need to remove this instance from the list
         // of proxy servers and to notify the attached clients.
         // Loop over servers sessions associated to this client and locate
         // the one corresponding to this proofserv instance
         XrdSysMutexHelper mtxh(pmgr->Mutex());
         if (pmgr->ProofServs()->size() > 0) {
            XrdProofServProxy *psrv = 0;
            int is = 0;
            for (is = 0; is < (int) pmgr->ProofServs()->size(); is++) {
               if ((psrv = pmgr->ProofServs()->at(is)) && (psrv->Link() == fLink)) {

               TRACEI(HDBG, "Recycle: found: " << psrv << " (v:" << psrv->IsValid() <<
                            ",t:"<<psrv->SrvType() << ",nc:"<<psrv->Clients()->size()<<")");

                  XrdSysMutexHelper xpmh(psrv->Mutex());

                  // Tell other attached clients, if any, that this session is gone
                  if (psrv->Clients()->size() > 0) {
                     char msg[512] = {0};
                     snprintf(msg, 512, "Recycle: session: %s terminated by peer",
                                         psrv->Tag());
                     int len = strlen(msg);
                     int ic = 0;
                     XrdProofdProtocol *p = 0;
                     for (ic = 0; ic < (int) psrv->Clients()->size(); ic++) {
                        // Send message
                        if ((p = psrv->Clients()->at(ic)->fP)) {
                           unsigned short sid;
                           p->fResponse.GetSID(sid);
                           p->fResponse.Set(psrv->Clients()->at(ic)->fSid);
                           p->fResponse.Send(kXR_attn, kXPD_errmsg, msg, len);
                           p->fResponse.Set(sid);
                        }
                     }
                  }

                  // Send a terminate signal to the proofserv
                  LogTerminatedProc(KillProofServ(psrv->SrvID()));

                  // Reset instance
                  psrv->Reset();
               }
            }
         }
      }
   }

   // Set fields to starting point (debugging mostly)
   Reset();

   // Push ourselves on the stack
   fgProtStack.Push(&fProtLink);
}

//__________________________________________________________________________
char *XrdProofdProtocol::FilterSecConfig(const char *cfn, int &nd)
{
   // Grep directives of the form "xpd.sec...", "sec.protparm" and
   // "sec.protocol" from file 'cfn' and save them in a temporary file,
   // stripping off the prefix "xpd." when needed.
   // If any such directory is found, the full path of the temporary file
   // is returned, with the number of directives found in 'nd'.
   // Otherwise 0 is returned and '-errno' specified in nd.
   // The caller has the responsability to unlink the temporary file and
   // to release the memory allocated for the path.

   static const char *pfx[] = { "xpd.sec.", "sec.protparm", "sec.protocol" };
   char *rcfn = 0;

   TRACE(ACT, "FilterSecConfig: enter");

   // Make sure that we got an input file path and that we can open the
   // associated path.
   FILE *fin = 0;
   if (!cfn || !(fin = fopen(cfn,"r"))) {
      nd = (errno > 0) ? -errno : -1;
      return rcfn;
   }

   // Read the directives: if an interesting one is found, we create
   // the output temporary file
   int fd = -1;
   char lin[2048];
   while (fgets(lin,sizeof(lin),fin)) {
      if (!strncmp(lin, pfx[0], strlen(pfx[0])) ||
          !strncmp(lin, pfx[1], strlen(pfx[1])) ||
          !strncmp(lin, pfx[2], strlen(pfx[2]))) {
         // Target directive found
         nd++;
         // Create the output file, if not yet done
         if (!rcfn) {
            rcfn = new char[strlen(fgTMPdir) + strlen("/xpdcfn_XXXXXX") + 2];
            sprintf(rcfn, "%s/xpdcfn_XXXXXX", fgTMPdir);
            if ((fd = mkstemp(rcfn)) < 0) {
               delete[] rcfn;
               nd = (errno > 0) ? -errno : -1;
               fclose(fin);
               rcfn = 0;
               return rcfn;
            }
         }
         XrdOucString slin = lin;
         // Strip the prefix "xpd."
         slin.replace("xpd.","");
         // Make keyword substitution
         XrdProofdProtocol::ResolveKeywords(slin, 0);
         // Write the line to the output file
         XrdProofdAux::Write(fd, slin.c_str(), slin.length());
      }
   }

   // Close files
   fclose(fin);
   close(fd);

   return rcfn;
}

//__________________________________________________________________________
int XrdProofdProtocol::GetWorkers(XrdOucString &lw, XrdProofServProxy *xps)
{
   // Get a list of workers from the available resource broker
   int rc = 0;

   TRACE(ACT, "GetWorkers: enter");

   // We need the scheduler at this point
   if (!fgProofSched) {
      fgEDest.Emsg("GetWorkers", "Scheduler undefined");
      return -1;
   }

   // Query the scheduler for the list of workers
   std::list<XrdProofWorker *> wrks;
   fgProofSched->GetWorkers(xps, &wrks);
   TRACE(DBG, "GetWorkers: list size: " << wrks.size());

   // The full list
   std::list<XrdProofWorker *>::iterator iw;
   for (iw = wrks.begin(); iw != wrks.end() ; iw++) {
      XrdProofWorker *w = *iw;
      // Add export version of the info
      lw += w->Export();
      // Add separator
      lw += '&';
      // Count
      xps->AddWorker(w);
      w->fProofServs.push_back(xps);
      w->fActive++;
   }

   return rc;
}

//__________________________________________________________________________
int XrdProofdClient::GetFreeServID()
{
   // Get next free server ID. If none is found, increase the vector size
   // and get the first new one

   TRACE(ACT,"GetFreeServID: enter");

   XrdSysMutexHelper mh(fMutex);

   TRACE(DBG,"GetFreeServID: size = "<<fProofServs.size()<<
              "; capacity = "<<fProofServs.capacity());
   int ic = 0;
   // Search for free places in the existing vector
   for (ic = 0; ic < (int)fProofServs.size() ; ic++) {
      if (fProofServs[ic] && !(fProofServs[ic]->IsValid())) {
         fProofServs[ic]->SetValid();
         return ic;
      }
   }

   // We may need to resize (double it)
   if (ic >= (int)fProofServs.capacity()) {
      int newsz = 2 * fProofServs.capacity();
      fProofServs.reserve(newsz);
   }

   // Allocate new element
   fProofServs.push_back(new XrdProofServProxy());

   TRACE(DBG,"GetFreeServID: size = "<<fProofServs.size()<<
              "; new capacity = "<<fProofServs.capacity()<<"; ic = "<<ic);

   // We are done
   return ic;
}

//______________________________________________________________________________
void XrdProofdClient::EraseServer(int psid)
{
   // Erase server with id psid from the list

   TRACE(ACT,"EraseServer: enter: psid: " << psid);

   XrdSysMutexHelper mh(fMutex);

   XrdProofServProxy *xps = 0;
   std::vector<XrdProofServProxy *>::iterator ip;
   for (ip = fProofServs.begin(); ip != fProofServs.end(); ++ip) {
      xps = *ip;
      if (xps && xps->Match(psid)) {
         fProofServs.erase(ip);
         break;
      }
   }
}

//______________________________________________________________________________
int XrdProofdProtocol::Login()
{
   // Process a login request

   int rc = 1;

   TRACEP(REQ, "Login: enter");

   // Check if there was any change in the configuration
   XrdProofdProtocol::Reconfig();

   // If this server is explicitely required to be a worker node or a
   // submaster, check whether the requesting host is allowed to connect
   if (fRequest.login.role[0] != 'i' &&
       fgMgr.SrvType() == kXPD_WorkerServer || fgMgr.SrvType() == kXPD_MasterServer) {
      if (!CheckMaster(fLink->Host())) {
         TRACEP(XERR,"Login: master not allowed to connect - "
                    "ignoring request ("<<fLink->Host()<<")");
         fResponse.Send(kXR_InvalidRequest,
                    "Login: master not allowed to connect - request ignored");
         return rc;
      }
   }

   // If this is the second call (after authentication) we just need
   // mapping
   if (fStatus == XPD_NEED_MAP) {

      // Check if this is a priviliged client
      char *p = 0;
      if ((p = (char *) strstr(fgSuperUsers, fClientID))) {
         if (p == fgSuperUsers || (p > fgSuperUsers && *(p-1) == ',')) {
            if (!(strncmp(p, fClientID, strlen(fClientID)))) {
               fSuperUser = 1;
               TRACEP(LOGIN,"Login: privileged user ");
            }
         }
      }
      // Acknowledge the client
      fResponse.Send();
      fStatus = XPD_LOGGEDIN;
      return MapClient(0);
   }

   // Make sure the user is not already logged in
   if ((fStatus & XPD_LOGGEDIN)) {
      fResponse.Send(kXR_InvalidRequest, "duplicate login; already logged in");
      return rc;
   }

   int i, pid;
   XrdOucString uname, gname;

   // Unmarshall the data
   pid = (int)ntohl(fRequest.login.pid);
   char un[9];
   for (i = 0; i < (int)sizeof(un)-1; i++) {
      if (fRequest.login.username[i] == '\0' || fRequest.login.username[i] == ' ')
         break;
      un[i] = fRequest.login.username[i];
   }
   un[i] = '\0';
   uname = un;

   // Longer usernames are in the attached buffer
   if (uname == "?>buf") {
      // Attach to buffer
      char *buf = fArgp->buff;
      int   len = fRequest.login.dlen;
      // Extract username
      uname.assign(buf,0,len-1);
      int iusr = uname.find("|usr:");
      if (iusr == -1) {
         TRACEP(XERR,"Login: long user name not found");
         fResponse.Send(kXR_InvalidRequest,"Login: long user name not found");
         return rc;
      }
      uname.erase(0,iusr+5);
      uname.erase(uname.find("|"));
   }

   // Extract group name, if specified (syntax is uname[:gname])
   int ig = uname.find(":");
   if (ig != -1) {
      gname.assign(uname, ig+1);
      uname.erase(ig);
      TRACEP(DBG,"Login: requested group: "<<gname);
   }

   // Here we check if the user is allowed to use the system
   // If not, we fail.
   XrdOucString emsg;
   if (CheckUser(uname.c_str(), fUI, emsg) != 0) {
      emsg.insert(": ", 0);
      emsg.insert(uname, 0);
      emsg.insert("Login: ClientID not allowed: ", 0);
      TRACEP(XERR, emsg.c_str());
      fResponse.Send(kXR_InvalidRequest, emsg.c_str());
      return rc;
   }

   // Check if user belongs to the group
   if (fgGroupsMgr.Num() > 0) {
      XrdProofGroup *g = 0;
      if (gname.length() > 0) {
         g = fgGroupsMgr.GetGroup(gname.c_str());
         if (!g) {
            emsg = "Login: group unknown: ";
            emsg += gname;
            TRACEP(XERR, emsg.c_str());
            fResponse.Send(kXR_InvalidRequest, emsg.c_str());
            return rc;
         } else if (strncmp(g->Name(),"default",7) &&
                   !g->HasMember(uname.c_str())) {
            emsg = "Login: user ";
            emsg += uname;
            emsg += " is not member of group ";
            emsg += gname;
            TRACEP(XERR, emsg.c_str());
            fResponse.Send(kXR_InvalidRequest, emsg.c_str());
            return rc;
         } else {
            if (TRACING(DBG)) {
               TRACEP(DBG,"Login: group: "<<gname<<" found");
               g->Print();
            }
         }
      } else {
         g = fgGroupsMgr.GetUserGroup(uname.c_str());
         gname = g ? g->Name() : "default";
      }
   }

   // Establish the ID for this link
   fLink->setID(uname.c_str(), pid);
   fCapVer = fRequest.login.capver[0];

   // Establish the ID for this client
   fClientID = new char[uname.length()+4];
   strcpy(fClientID, uname.c_str());
   TRACEI(LOGIN,"Login: ClientID = " << fClientID);

   // Establish the group ID for this client
   fGroupID = new char[gname.length()+4];
   strcpy(fGroupID, gname.c_str());
   TRACEI(LOGIN,"Login: GroupID = " << fGroupID);

   // Assert the workdir directory ...
   fUI.fWorkDir = fUI.fHomeDir;
   if (fgMgr.WorkDir() && strlen(fgMgr.WorkDir()) > 0) {
      // The user directory path will be <workdir>/<user>
      fUI.fWorkDir = fgMgr.WorkDir();
      if (!fUI.fWorkDir.endswith('/'))
         fUI.fWorkDir += "/";
      fUI.fWorkDir += fClientID;
   } else {
      // Default: $HOME/proof
      if (!fUI.fWorkDir.endswith('/'))
         fUI.fWorkDir += "/";
      fUI.fWorkDir += "proof";
      if (fUI.fUser != fClientID) {
         fUI.fWorkDir += "/";
         fUI.fWorkDir += fClientID;
      }
   }
   TRACEI(LOGIN,"Login: work dir = " << fUI.fWorkDir);

   // Make sure the directory exists
   if (XrdProofdAux::AssertDir(fUI.fWorkDir.c_str(), fUI, fgChangeOwn) == -1) {
      XrdOucString emsg("Login: unable to create work dir: ");
      emsg += fUI.fWorkDir;
      TRACEP(XERR, emsg);
      fResponse.Send(kXP_ServerError, emsg.c_str());
      return rc;
   }

   // On masters assert the dataset directory ...
   if ((fgMgr.SrvType() == kXPD_TopMaster || fgMgr.SrvType() == kXPD_AnyServer) &&
       fgMgr.DataSetDir()) {
      XrdOucString dsetdir = fgMgr.DataSetDir();
      if (dsetdir.length() > 0) {
         dsetdir += "/";
         dsetdir += fGroupID;
         dsetdir += "/";
         dsetdir += fClientID;
      } else {
         dsetdir += fUI.fWorkDir;
         dsetdir += "/datasets";
      }
      if (XrdProofdAux::AssertDir(dsetdir.c_str(), fUI, fgChangeOwn) == -1) {
         XrdOucString emsg("Login: unable to assert dataset dir: ");
         emsg += dsetdir;
         TRACEP(XERR, emsg);
         fResponse.Send(kXP_ServerError, emsg.c_str());
         return rc;
      }
   }

   // If strong authentication is required ...
   if (fgCIA) {
      // ... make sure that the directory for credentials exists in the sandbox ...
      XrdOucString credsdir = fUI.fWorkDir;
      credsdir += "/.creds";
      // Acquire user identity
      XrdSysPrivGuard pGuard((uid_t)fUI.fUid, (gid_t)fUI.fGid);
      if (!pGuard.Valid()) {
         XrdOucString emsg("Login: could not get privileges to create credential dir ");
         emsg += credsdir;
         TRACEP(XERR, emsg);
         fResponse.Send(kXP_ServerError, emsg.c_str());
         return rc;
      }
      if (XrdProofdAux::AssertDir(credsdir.c_str(), fUI, fgChangeOwn) == -1) {
         XrdOucString emsg("Login: unable to create credential dir: ");
         emsg += credsdir;
         TRACEP(XERR, emsg);
         fResponse.Send(kXP_ServerError, emsg.c_str());
         return rc;
      }
   }

   // Find out the server type: 'i', internal, means this is a proofsrv calling back.
   // For the time being authentication is required for clients only.
   bool needauth = 0;
   switch (fRequest.login.role[0]) {
   case 'A':
      fSrvType = kXPD_Admin;
      fResponse.Set("adm: ");
      break;
   case 'i':
      fSrvType = kXPD_Internal;
      fResponse.Set("int: ");
      break;
   case 'M':
      if (fgMgr.SrvType() == kXPD_AnyServer || fgMgr.SrvType() == kXPD_TopMaster) {
         fTopClient = 1;
         fSrvType = kXPD_TopMaster;
         needauth = 1;
         fResponse.Set("m2c: ");
      } else {
         TRACEP(XERR,"Login: top master mode not allowed - ignoring request");
         fResponse.Send(kXR_InvalidRequest,
                        "Server not allowed to be top master - ignoring request");
         return rc;
      }
      break;
   case 'm':
      if (fgMgr.SrvType() == kXPD_AnyServer || fgMgr.SrvType() == kXPD_MasterServer) {
         fSrvType = kXPD_MasterServer;
         needauth = 1;
         fResponse.Set("m2m: ");
      } else {
         TRACEP(XERR,"Login: submaster mode not allowed - ignoring request");
         fResponse.Send(kXR_InvalidRequest,
                        "Server not allowed to be submaster - ignoring request");
         return rc;
      }
      break;
   case 's':
      if (fgMgr.SrvType() == kXPD_AnyServer || fgMgr.SrvType() == kXPD_WorkerServer) {
         fSrvType = kXPD_WorkerServer;
         needauth = 1;
         fResponse.Set("w2m: ");
      } else {
         TRACEP(XERR,"Login: worker mode not allowed - ignoring request");
         fResponse.Send(kXR_InvalidRequest,
                        "Server not allowed to be worker - ignoring request");
         return rc;
      }
      break;
   default:
      TRACEP(XERR, "Login: unknown mode: '" << fRequest.login.role[0] <<"'");
      fResponse.Send(kXR_InvalidRequest, "Server type: invalide mode");
      return rc;
   }

   // Get the security token for this link. We will either get a token, a null
   // string indicating host-only authentication, or a null indicating no
   // authentication. We can then optimize of each case.
   if (needauth && fgCIA) {
      const char *pp = fgCIA->getParms(i, fLink->Name());
      if (pp && i ) {
         fResponse.Send((kXR_int32)XPROOFD_VERSBIN, (void *)pp, i);
         fStatus = (XPD_NEED_MAP | XPD_NEED_AUTH);
         return rc;
      } else {
         fResponse.Send((kXR_int32)XPROOFD_VERSBIN);
         fStatus = XPD_LOGGEDIN;
         if (pp) {
            fEntity.tident = fLink->ID;
            fClient = &fEntity;
         }
      }
   } else {
      rc = fResponse.Send((kXR_int32)XPROOFD_VERSBIN);
      fStatus = XPD_LOGGEDIN;

      // Check if this is a priviliged client
      char *p = 0;
      if ((p = (char *) strstr(fgSuperUsers, fClientID))) {
         if (p == fgSuperUsers || (p > fgSuperUsers && *(p-1) == ',')) {
            if (!(strncmp(p, fClientID, strlen(fClientID)))) {
               fSuperUser = 1;
               TRACEI(LOGIN,"Login: privileged user ");
            }
         }
      }
   }

   // Map the client
   return MapClient(1);
}

//______________________________________________________________________________
int XrdProofdProtocol::MapClient(bool all)
{
   // Process a login request
   int rc = 1;

   TRACEI(REQ,"MapClient: enter");

   // Flag for internal connections
   bool proofsrv = ((fSrvType == kXPD_Internal) && all) ? 1 : 0;

   // If call back from proofsrv, find out the target session
   short int psid = -1;
   char protver = -1;
   short int clientvers = -1;
   if (proofsrv) {
      memcpy(&psid, (const void *)&(fRequest.login.reserved[0]), 2);
      if (psid < 0) {
         TRACEP(XERR,"MapClient: proofsrv callback: sent invalid session id");
         fResponse.Send(kXR_InvalidRequest,
                        "MapClient: proofsrv callback: sent invalid session id");
         return rc;
      }
      protver = fRequest.login.capver[0];
      TRACEI(DBG,"MapClient: proofsrv callback for session: " <<psid);
   } else {
      // Get PROOF version run by client
      memcpy(&clientvers, (const void *)&(fRequest.login.reserved[0]), 2);
      TRACEI(DBG,"MapClient: PROOF version run by client: " <<clientvers);
   }

   // Now search for an existing manager session for this ClientID
   XrdProofdClient *pmgr = 0;
   TRACEI(DBG,"MapClient: # of clients: "<<fgProofdClients.size());
   // This part may be not thread safe
   {  XrdSysMutexHelper mtxh(&fgXPDMutex);
      if (fgProofdClients.size() > 0) {
         std::list<XrdProofdClient *>::iterator i;
         for (i = fgProofdClients.begin(); i != fgProofdClients.end(); ++i) {
            if ((pmgr = *i) && pmgr->Match(fClientID, fGroupID))
               break;
            TRACEI(HDBG, "MapClient: client: "<<pmgr->ID()<< ", group: "<<
                         ((pmgr->Group()) ? pmgr->Group()->Name() : "---"));
            pmgr = 0;
         }
      }
   }

   // Map the existing session, if found
   if (pmgr && pmgr->IsValid()) {
      // Save as reference proof mgr
      fPClient = pmgr;
      TRACEI(DBG,"MapClient: matching client: "<<pmgr);

      // If proofsrv, locate the target session
      if (proofsrv) {
         XrdProofServProxy *psrv = 0;
         int is = 0;
         for (is = 0; is < (int) pmgr->ProofServs()->size(); is++) {
            if ((psrv = pmgr->ProofServs()->at(is)) && psrv->Match(psid))
               break;
            psrv = 0;
         }
         if (!psrv) {
            TRACEP(XERR, "MapClient: proofsrv callback:"
                        " wrong target session: protocol error");
            fResponse.Send(kXP_nosession, "MapClient: proofsrv callback:"
                           " wrong target session: protocol error");
            return -1;
         } else {
            // Set the protocol version
            psrv->SetProtVer(protver);
            // Assign this link to it
            psrv->SetLink(fLink);
            psrv->ProofSrv()->Set(fRequest.header.streamid);
            psrv->ProofSrv()->Set(fLink);
            // Set Trace ID
            XrdOucString tid(" : xrd->");
            tid += psrv->Ordinal();
            tid += " ";
            psrv->ProofSrv()->Set(tid.c_str());
            TRACEI(DBG,"MapClient: proofsrv callback:"
                       " link assigned to target session "<<psid);
         }
      } else {

         // Make sure that the version is filled correctly (if an admin operation
         // was run before this may still be -1 on workers)
         pmgr->SetClientVers(clientvers);

         // The index of the next free slot will be the unique ID
         fCID = pmgr->GetClientID(this);

         // If any PROOF session in shutdown state exists, stop the related
         // shutdown timers
         if (pmgr->ProofServs()->size() > 0) {
            XrdProofServProxy *psrv = 0;
            int is = 0;
            for (is = 0; is < (int) pmgr->ProofServs()->size(); is++) {
               if ((psrv = pmgr->ProofServs()->at(is)) &&
                    psrv->IsValid() && (psrv->SrvType() == kXPD_TopMaster) &&
                    psrv->IsShutdown()) {
                  if (psrv->SetShutdownTimer(fgShutdownOpt, fgShutdownDelay, 0) != 0) {
                     XrdOucString msg("MapClient: could not stop shutdown timer in proofsrv ");
                     msg += psrv->SrvID();
                     msg += "; status: ";
                     msg += psrv->StatusAsString();
                     fResponse.Send(kXR_attn, kXPD_srvmsg, (void *) msg.c_str(), msg.length());
                  }
               }
            }
         }
      }

   } else {

      // Proofsrv callbacks need something to attach to
      if (proofsrv) {
         TRACEI(XERR, "MapClient: proofsrv callback:"
                     " no manager to attach to: protocol error");
         return -1;
      }

      // This part may be not thread safe
      {  XrdSysMutexHelper mtxh(&fgXPDMutex);

         // Make sure that no zombie proofserv is around
         CleanupProofServ(0, fClientID);
         if (!pmgr) {
            // No existing session: create a new one
            pmgr = new XrdProofdClient(fClientID, clientvers, fUI);
            pmgr->SetROOT(fgROOT.front());
            // Locate and set the group, if any
            if (fgGroupsMgr.Num() > 0)
               pmgr->SetGroup(fgGroupsMgr.GetUserGroup(fClientID, fGroupID));
            // Add to the list
            fgProofdClients.push_back(pmgr);
         } else {
            // An instance not yet valid exists already: fill it
            pmgr->SetClientVers(clientvers);
            pmgr->SetWorkdir(fUI.fWorkDir.c_str());
            if (!(pmgr->ROOT()))
               pmgr->SetROOT(fgROOT.front());
         }
         // Save as reference proof mgr
         fPClient = pmgr;
      }

      // No existing session: create a new one
      if (pmgr && (pmgr->CreateUNIXSock(&fgEDest, fgTMPdir) == 0)) {

         TRACEI(DBG,"MapClient: NEW client: "<<pmgr<<
                    ", group: "<<((pmgr->Group()) ? pmgr->Group()->Name() : "???"));

         // The index of the next free slot will be the unique ID
         fCID = pmgr->GetClientID(this);

         // Reference Stream ID
         unsigned short sid;
         memcpy((void *)&sid, (const void *)&(fRequest.header.streamid[0]), 2);
         pmgr->SetRefSid(sid);

         // Check if old sessions are still flagged as active
         XrdOucString tobemv;

         // Get list of session working dirs flagged as active,
         // and check if they have to be deactivated
         std::list<XrdOucString *> sactlst;
         if (pmgr->GetSessionDirs(1, &sactlst) == 0) {
            std::list<XrdOucString *>::iterator i;
            for (i = sactlst.begin(); i != sactlst.end(); ++i) {
               char *p = (char *) strrchr((*i)->c_str(), '-');
               if (p) {
                  int pid = strtol(p+1, 0, 10);
                  if (!VerifyProcessByID(pid)) {
                     tobemv += (*i)->c_str();
                     tobemv += '|';
                  }
               }
            }
         }
         // Clean up the list
         sactlst.clear();

         // To avoid dead locks we must close the file and do the mv actions after
         XrdOucString fnact = fUI.fWorkDir;
         fnact += "/.sessions";
         FILE *f = fopen(fnact.c_str(), "r");
         if (f) {
            char ln[1024];
            while (fgets(ln, sizeof(ln), f)) {
               if (ln[strlen(ln)-1] == '\n')
                  ln[strlen(ln)-1] = 0;
               char *p = strrchr(ln, '-');
               if (p) {
                  int pid = strtol(p+1, 0, 10);
                  if (!VerifyProcessByID(pid)) {
                     tobemv += ln;
                     tobemv += '|';
                  }
               }
            }
            fclose(f);
         }

         // Instance can be considered valid by now
         pmgr->SetValid();

         TRACEI(DBG,"MapClient: client "<<pmgr<<" added to the list (ref sid: "<< sid<<")");

         XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
         if (XpdBadPGuard(pGuard, fUI.fUid) && fgChangeOwn) {
            TRACEI(XERR, "MapClient: could not get privileges");
            return -1;
         }

         // Mv inactive sessions, if needed
         if (tobemv.length() > 0) {
            char del = '|';
            XrdOucString tag;
            int from = 0;
            while ((from = tobemv.tokenize(tag, from, del)) != -1) {
               if (fPClient->MvOldSession(tag.c_str()) == -1)
                  TRACEI(REQ, "MapClient: problems recording session as old in sandbox");
            }
         }

         // Set ownership of the socket file to the client
         if (fgChangeOwn && chown(pmgr->UNIXSockPath(), fUI.fUid, fUI.fGid) == -1) {
            TRACEI(XERR, "MapClient: cannot set user ownership"
                               " on UNIX socket (errno: "<<errno<<")");
            return -1;
         }

      } else {
         // Remove from the list
         fgProofdClients.remove(pmgr);
         SafeDelete(pmgr);
         fPClient = 0;
         TRACEP(DBG,"MapClient: cannot instantiate XrdProofdClient");
         fResponse.Send(kXP_ServerError,
                        "MapClient: cannot instantiate XrdProofdClient");
         return rc;
      }
   }

   if (!proofsrv) {
      TRACEI(DBG,"MapClient: fCID: "<<fCID<<", size: "<<fPClient->Clients()->size()<<
                 ", capacity: "<<fPClient->Clients()->capacity());
   }

   // Document this login
   if (!(fStatus & XPD_NEED_AUTH))
      fgEDest.Log(XPD_LOG_01, ":MapClient", fLink->ID, "login");

   return rc;
}

//_____________________________________________________________________________
int XrdProofdProtocol::CheckUser(const char *usr,
                                 XrdProofUI &ui, XrdOucString &e)
{
   // Check if the user is allowed to use the system
   // Return 0 if OK, -1 if not.

   // No 'root' logins
   if (!usr || strlen(usr) <= 0) {
      e = "Login: 'usr' string is undefined ";
      return -1;
   }

   // No 'root' logins
   if (strlen(usr) == 4 && !strcmp(usr, "root")) {
      e = "Login: 'root' logins not accepted ";
      return -1;
   }

   // Here we check if the user is known locally.
   // If not, we fail for now.
   // In the future we may try to get a temporary account
   if (fgChangeOwn) {
      if (XrdProofdAux::GetUserInfo(usr, ui) != 0) {
         e = "Login: unknown ClientID: ";
         e += usr;
         return -1;
      }
   } else {
      if (XrdProofdAux::GetUserInfo(geteuid(), ui) != 0) {
         e = "Login: problems getting user info for id: ";
         e += (int)geteuid();
         return -1;
      }
   }

   // If we are in controlled mode we have to check if the user in the
   // authorized list; otherwise we fail. Privileged users are always
   // allowed to connect.
   if (fgOperationMode == kXPD_OpModeControlled) {
      bool notok = 1;
      XrdOucString us;
      int from = 0;
      while ((from = fgAllowedUsers.tokenize(us, from, ',')) != -1) {
         if (us == usr) {
            notok = 0;
            break;
         }
      }
      if (notok) {
         e = "Login: controlled operations:"
             " user not currently authorized to log in: ";
         e += usr;
         return -1;
      }
   }

   // OK
   return 0;
}



//_____________________________________________________________________________
int XrdProofdProtocol::Auth()
{
   // Analyse client authentication info

   struct sockaddr netaddr;
   XrdSecCredentials cred;
   XrdSecParameters *parm = 0;
   XrdOucErrInfo     eMsg;
   const char *eText;
   int rc;

   TRACEP(REQ,"Auth: enter");

   // Ignore authenticate requests if security turned off
   if (!fgCIA)
      return fResponse.Send();
   cred.size   = fRequest.header.dlen;
   cred.buffer = fArgp->buff;

   // If we have no auth protocol, try to get it
   if (!fAuthProt) {
      fLink->Name(&netaddr);
      if (!(fAuthProt = fgCIA->getProtocol(fLink->Host(), netaddr, &cred, &eMsg))) {
         eText = eMsg.getErrText(rc);
         TRACEP(XERR,"Auth: user authentication failed; "<<eText);
         fResponse.Send(kXR_NotAuthorized, eText);
         return -EACCES;
      }
      fAuthProt->Entity.tident = fLink->ID;
   }

   // Now try to authenticate the client using the current protocol
   if (!(rc = fAuthProt->Authenticate(&cred, &parm, &eMsg))) {
      const char *msg = (fStatus & XPD_ADMINUSER ? "admin login as" : "login as");
      rc = fResponse.Send();
      fStatus &= ~XPD_NEED_AUTH;
      fClient = &fAuthProt->Entity;
      if (fClient->name)
         fgEDest.Log(XPD_LOG_01, ":Auth", fLink->ID, msg, fClient->name);
      else
         fgEDest.Log(XPD_LOG_01, ":Auth", fLink->ID, msg, " nobody");
      return rc;
   }

   // If we need to continue authentication, tell the client as much
   if (rc > 0) {
      TRACEP(DBG, "Auth: more auth requested; sz: " <<(parm ? parm->size : 0));
      if (parm) {
         rc = fResponse.Send(kXR_authmore, parm->buffer, parm->size);
         delete parm;
         return rc;
      }
      if (fAuthProt) {
         fAuthProt->Delete();
         fAuthProt = 0;
      }
      TRACEP(XERR,"Auth: security requested additional auth w/o parms!");
      fResponse.Send(kXR_ServerError,"invalid authentication exchange");
      return -EACCES;
   }

   // We got an error, bail out
   if (fAuthProt) {
      fAuthProt->Delete();
      fAuthProt = 0;
   }
   eText = eMsg.getErrText(rc);
   TRACEP(XERR, "Auth: user authentication failed; "<<eText);
   fResponse.Send(kXR_NotAuthorized, eText);
   return -EACCES;
}

//______________________________________________________________________________
int XrdProofdProtocol::GetBuff(int quantum)
{
   // Allocate a buffer to handle quantum bytes

   TRACE(ACT, "GetBuff: enter");

   // The current buffer may be sufficient for the current needs
   if (!fArgp || quantum > fArgp->bsize)
      fhcNow = fhcPrev;
   else if (quantum >= fhalfBSize || fhcNow-- > 0)
      return 1;
   else if (fhcNext >= fhcMax)
      fhcNow = fhcMax;
   else {
      int tmp = fhcPrev;
      fhcNow = fhcNext;
      fhcPrev = fhcNext;
      fhcNext = tmp + fhcNext;
   }

   // We need a new buffer
   if (fArgp)
      fgBPool->Release(fArgp);
   if ((fArgp = fgBPool->Obtain(quantum)))
      fhalfBSize = fArgp->bsize >> 1;
   else
      return fResponse.Send(kXR_NoMemory, "insufficient memory for requested buffer");

   // Success
   return 1;
}

//______________________________________________________________________________
int XrdProofdProtocol::GetData(const char *dtype, char *buff, int blen)
{
   // Get data from the open link

   int rlen;

   // Read the data but reschedule the link if we have not received all of the
   // data within the timeout interval.
   TRACEI(ACT, "GetData: dtype: "<<(dtype ? dtype : " - ")<<", blen: "<<blen);

   rlen = fLink->Recv(buff, blen, fgReadWait);

   if (rlen  < 0)
      if (rlen != -ENOMSG) {
         XrdOucString emsg = "GetData: link read error: errno: ";
         emsg += -rlen;
         TRACEI(XERR, emsg.c_str());
         return fLink->setEtext(emsg.c_str());
      } else {
         TRACEI(DBG, "GetData: connection closed by peer (errno: "<<-rlen<<")");
         return -1;
      }
   if (rlen < blen) {
      fBuff = buff+rlen; fBlen = blen-rlen;
      TRACEI(XERR, "GetData: " << dtype <<
                  " timeout; read " <<rlen <<" of " <<blen <<" bytes");
      return 1;
   }
   TRACEI(DBG, "GetData: rlen: "<<rlen);

   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::Attach()
{
   // Handle a request to attach to an existing session

   int psid = -1, rc = 1;

   // Unmarshall the data
   psid = ntohl(fRequest.proof.sid);
   TRACEI(REQ, "Attach: psid: "<<psid<<", fCID = "<<fCID);

   // Find server session
   XrdProofServProxy *xps = 0;
   if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
       !(xps = fPClient->ProofServs()->at(psid))) {
      TRACEP(XERR, "Attach: session ID not found");
      fResponse.Send(kXR_InvalidRequest,"session ID not found");
      return rc;
   }
   TRACEP(DBG, "Attach: xps: "<<xps<<", status: "<< xps->Status());

   // Stream ID
   unsigned short sid;
   memcpy((void *)&sid, (const void *)&(fRequest.header.streamid[0]), 2);

   // We associate this instance to the corresponding slot in the
   // session vector of attached clients
   XrdClientID *csid = xps->GetClientID(fCID);
   csid->fP = this;
   csid->fSid = sid;

   // Take parentship, if orphalin
   if (!(xps->Parent()))
      xps->SetParent(csid);

   // Notify to user
   if (fSrvType == kXPD_TopMaster) {
      // Send also back the data pool url
      XrdOucString dpu = fgPoolURL;
      if (!dpu.endswith('/'))
         dpu += '/';
      dpu += fgNamespace;
      fResponse.Send(psid, xps->ROOT()->SrvProtVers(), (kXR_int16)XPROOFD_VERSBIN,
                     (void *) dpu.c_str(), dpu.length());
   } else
      fResponse.Send(psid, xps->ROOT()->SrvProtVers(), (kXR_int16)XPROOFD_VERSBIN);

   // Send saved query num message
   if (xps->QueryNum()) {
      TRACEP(XERR, "Attach: sending query num message ("<<
                  xps->QueryNum()->fSize<<" bytes)");
      fResponse.Send(kXR_attn, kXPD_msg,
                     xps->QueryNum()->fBuff, xps->QueryNum()->fSize);
   }
   // Send saved start processing message, if not idle
   if (xps->Status() == kXPD_running && xps->StartMsg()) {
      TRACEP(XERR, "Attach: sending start process message ("<<
                  xps->StartMsg()->fSize<<" bytes)");
      fResponse.Send(kXR_attn, kXPD_msg,
                     xps->StartMsg()->fBuff, xps->StartMsg()->fSize);
   }

   // Over
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::Detach()
{
   // Handle a request to detach from an existing session

   int psid = -1, rc = 1;

   XrdSysMutexHelper mh(fMutex);

   // Unmarshall the data
   psid = ntohl(fRequest.proof.sid);
   TRACEI(REQ, "Detach: psid: "<<psid);

   // Find server session
   XrdProofServProxy *xps = 0;
   if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
       !(xps = fPClient->ProofServs()->at(psid))) {
      TRACEP(XERR, "Detach: session ID not found");
      fResponse.Send(kXR_InvalidRequest,"session ID not found");
      return rc;
   }
   TRACEP(DBG, "Detach: xps: "<<xps<<", status: "<< xps->Status()<<
               ", # clients: "<< xps->Clients()->size());

   XrdSysMutexHelper xpmh(xps->Mutex());

   // Remove this from the list of clients
   std::vector<XrdClientID *>::iterator i;
   for (i = xps->Clients()->begin(); i != xps->Clients()->end(); ++i) {
      if (*i) {
         if ((*i)->fP == this) {
            delete (*i);
            xps->Clients()->erase(i);
            break;
         }
      }
   }

   // Notify to user
   fResponse.Send();

   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::Destroy()
{
   // Handle a request to shutdown an existing session

   int psid = -1, rc = 1;

   XrdSysMutexHelper mh(fPClient->Mutex());

   // Unmarshall the data
   psid = ntohl(fRequest.proof.sid);
   TRACEI(REQ, "Destroy: psid: "<<psid);

   // Find server session
   XrdProofServProxy *xpsref = 0;
   if (psid > -1) {
      // Request for a specific session
      if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
          !(xpsref = fPClient->ProofServs()->at(psid))) {
         TRACEP(XERR, "Destroy: reference session ID not found");
         fResponse.Send(kXR_InvalidRequest,"reference session ID not found");
         return rc;
      }
   }

   // Loop over servers
   XrdProofServProxy *xps = 0;
   int is = 0;
   for (is = 0; is < (int) fPClient->ProofServs()->size(); is++) {

      if ((xps = fPClient->ProofServs()->at(is)) && (xpsref == 0 || xps == xpsref)) {

         TRACEI(DBG, "Destroy: xps: "<<xps<<", status: "<< xps->Status()<<", pid: "<<xps->SrvID());

         {  XrdSysMutexHelper xpmh(xps->Mutex());

            if (xps->SrvType() == kXPD_TopMaster) {
               // Tell other attached clients, if any, that this session is gone
               if (fTopClient && xps->Clients()->size() > 0) {
                  char msg[512] = {0};
                  snprintf(msg, 512, "Destroy: session: %s destroyed by: %s",
                           xps->Tag(), fLink->ID);
                  int len = strlen(msg);
                  int ic = 0;
                  XrdProofdProtocol *p = 0;
                  for (ic = 0; ic < (int) xps->Clients()->size(); ic++) {
                     if ((p = xps->Clients()->at(ic)->fP) &&
                         (p != this) && p->fTopClient) {
                        unsigned short sid;
                        p->fResponse.GetSID(sid);
                        p->fResponse.Set(xps->Clients()->at(ic)->fSid);
                        p->fResponse.Send(kXR_attn, kXPD_srvmsg, msg, len);
                        p->fResponse.Set(sid);
                     }
                  }
               }
            }

            // Send a terminate signal to the proofserv
            if (LogTerminatedProc(xps->TerminateProofServ()) != 0)
               if (LogTerminatedProc(KillProofServ(xps->SrvID(), 1)) != 0) {
                  TRACEI(XERR, "Destroy: problems terminating request to proofsrv");
               }

            // Reset instance
            xps->Reset();

            // If single delete we are done
            if ((xpsref != 0 && (xps == xpsref)))
               break;
         }
      }

   }

   // Notify to user
   fResponse.Send();

   // Over
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::SaveAFSkey(XrdSecCredentials *c, const char *dir)
{
   // Save the AFS key, if any, for usage in proofserv in file 'dir'/.afs .
   // Return 0 on success, -1 on error.

   // Check file name
   if (!dir || strlen(dir) <= 0) {
      TRACE(XERR, "SaveAFSkey: dir name undefined");
      return -1;
   }

   // Check credentials
   if (!c) {
      TRACE(XERR, "SaveAFSkey: credentials undefined");
      return -1;
   }

   // Decode credentials
   int lout = 0;
   char *out = new char[c->size];
   if (XrdSutFromHex(c->buffer, out, lout) != 0) {
      TRACE(XERR, "SaveAFSkey: problems unparsing hex string");
      delete [] out;
      return -1;
   }

   // Locate the key
   char *key = out + 5;
   if (strncmp(key, "afs:", 4)) {
      TRACE(DBG, "SaveAFSkey: string does not contain an AFS key");
      delete [] out;
      return 0;
   }
   key += 4;

   // Filename
   XrdOucString fn = dir;
   fn += "/.afs";
   // Open the file, truncatin g if already existing
   int fd = open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
   if (fd <= 0) {
      TRACE(XERR, "SaveAFSkey: problems creating file - errno: " << errno);
      delete [] out;
      return -1;
   }
   // Make sure it is protected
   if (fchmod(fd, 0600) != 0) {
      TRACE(XERR, "SaveAFSkey: problems setting file permissions to 0600 - errno: " << errno);
      delete [] out;
      close(fd);
      return -1;
   }
   // Write out the key
   int rc = 0;
   int lkey = lout - 9;
   if (XrdProofdAux::Write(fd, key, lkey) != lkey) {
      TRACE(XERR, "SaveAFSkey: problems writing to file - errno: " << errno);
      rc = -1;
   }

   // Cleanup
   delete [] out;
   close(fd);
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::SetProofServEnvOld(int psid, int loglevel, const char *cfg)
{
   // Set environment for proofserv; old version preparing the environment for
   // proofserv protocol version <= 13. Needed for backward compatibility.

   char *ev = 0;

   MTRACE(REQ,  "xpd:child: ", "SetProofServEnv: enter: psid: "<<psid<<
                      ", log: "<<loglevel);

   // Make sure the principal client is defined
   if (!fPClient) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: principal client undefined - cannot continue");
      return -1;
   }

   // Set basic environment for proofserv
   if (SetProofServEnv(fPClient->ROOT()) != 0) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnvOld: problems setting basic environment - exit");
      return -1;
   }

   // Session proxy
   XrdProofServProxy *xps = fPClient->ProofServs()->at(psid);
   if (!xps) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnvOld: unable to get instance of proofserv proxy");
      return -1;
   }

   // Work directory
   XrdOucString udir = fPClient->Workdir();
   MTRACE(DBG, "xpd:child: ",
               "SetProofServEnvOld: working dir for "<<fClientID<<" is: "<<udir);

   // Session tag
   char hn[64], stag[512];
#if defined(XPD__SUNCC)
   sysinfo(SI_HOSTNAME, hn, sizeof(hn));
#else
   gethostname(hn, sizeof(hn));
#endif
   XrdOucString host = hn;
   if (host.find(".") != STR_NPOS)
      host.erase(host.find("."));
   sprintf(stag,"%s-%d-%d",host.c_str(),(int)time(0),getpid());

   // Session dir
   XrdOucString logdir = udir;
   if (fSrvType == kXPD_TopMaster) {
      logdir += "/session-";
      logdir += stag;
      xps->SetTag(stag);
   } else {
      logdir += "/";
      logdir += xps->Tag();
   }
   MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: log dir "<<logdir);
   // Make sure the directory exists
   if (XrdProofdAux::AssertDir(logdir.c_str(), fUI, fgChangeOwn) == -1) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnvOld: unable to create log dir: "<<logdir);
      return -1;
   }
   // The session dir (sandbox) depends on the role
   XrdOucString sessdir = logdir;
   if (fSrvType == kXPD_WorkerServer)
      sessdir += "/worker-";
   else
      sessdir += "/master-";
   sessdir += xps->Ordinal();
   sessdir += "-";
   sessdir += stag;
   ev = new char[strlen("ROOTPROOFSESSDIR=")+sessdir.length()+2];
   sprintf(ev, "ROOTPROOFSESSDIR=%s", sessdir.c_str());
   putenv(ev);
   MTRACE(DBG,  "xpd:child: ", "SetProofServEnvOld: "<<ev);

   // Log level
   ev = new char[strlen("ROOTPROOFLOGLEVEL=")+5];
   sprintf(ev, "ROOTPROOFLOGLEVEL=%d", loglevel);
   putenv(ev);
   MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);

   // Ordinal number
   ev = new char[strlen("ROOTPROOFORDINAL=")+strlen(xps->Ordinal())+2];
   sprintf(ev, "ROOTPROOFORDINAL=%s", xps->Ordinal());
   putenv(ev);
   MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);

   // ROOT Version tag if not the default one
   ev = new char[strlen("ROOTVERSIONTAG=")+strlen(fPClient->ROOT()->Tag())+2];
   sprintf(ev, "ROOTVERSIONTAG=%s", fPClient->ROOT()->Tag());
   putenv(ev);
   MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);

   // Create the env file
   MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: creating env file");
   XrdOucString envfile = sessdir;
   envfile += ".env";
   FILE *fenv = fopen(envfile.c_str(), "w");
   if (!fenv) {
      MTRACE(XERR, "xpd:child: ",
                  "SetProofServEnvOld: unable to open env file: "<<envfile);
      return -1;
   }
   MTRACE(DBG, "xpd:child: ",
               "SetProofServEnvOld: environment file: "<< envfile);

   // Forwarded sec credentials, if any
   if (fAuthProt) {

      // Additional envs possibly set by the protocol for next application
      XrdOucString secenvs(getenv("XrdSecENVS"));
      if (secenvs.length() > 0) {
         // Go through the list
         XrdOucString env;
         int from = 0;
         while ((from = secenvs.tokenize(env, from, ',')) != -1) {
            if (env.length() > 0) {
               // Set the env now
               ev = new char[env.length()+1];
               strncpy(ev, env.c_str(), env.length());
               ev[env.length()] = 0;
               putenv(ev);
               fprintf(fenv, "%s\n", ev);
               MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);
            }
         }
      }

      // The credential buffer, if any
      XrdSecCredentials *creds = fAuthProt->getCredentials();
      if (creds) {
         int lev = strlen("XrdSecCREDS=")+creds->size;
         ev = new char[lev+1];
         strcpy(ev, "XrdSecCREDS=");
         memcpy(ev+strlen("XrdSecCREDS="), creds->buffer, creds->size);
         ev[lev] = 0;
         putenv(ev);
         MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: XrdSecCREDS set");

         // If 'pwd', save AFS key, if any
         if (!strncmp(fAuthProt->Entity.prot, "pwd", 3)) {
            XrdOucString credsdir = udir;
            credsdir += "/.creds";
            // Make sure the directory exists
            if (!XrdProofdAux::AssertDir(credsdir.c_str(), fUI, fgChangeOwn)) {
               if (SaveAFSkey(creds, credsdir.c_str()) == 0) {
                  ev = new char[strlen("ROOTPROOFAFSCREDS=")+credsdir.length()+strlen("/.afs")+2];
                  sprintf(ev, "ROOTPROOFAFSCREDS=%s/.afs", credsdir.c_str());
                  putenv(ev);
                  fprintf(fenv, "ROOTPROOFAFSCREDS has been set\n");
                  MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: " << ev);
               } else {
                  MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: problems in saving AFS key");
               }
            } else {
               MTRACE(XERR, "xpd:child: ",
                            "SetProofServEnvOld: unable to create creds dir: "<<credsdir);
               return -1;
            }
         }
      }
   }

   // Set ROOTSYS
   fprintf(fenv, "ROOTSYS=%s\n", xps->ROOT()->Dir());

   // Set conf dir
   fprintf(fenv, "ROOTCONFDIR=%s\n", xps->ROOT()->Dir());

   // Set TMPDIR
   fprintf(fenv, "ROOTTMPDIR=%s\n", fgTMPdir);

   // Port (really needed?)
   fprintf(fenv, "ROOTXPDPORT=%d\n", fgMgr.Port());

   // Work dir
   fprintf(fenv, "ROOTPROOFWORKDIR=%s\n", udir.c_str());

   // Session tag
   fprintf(fenv, "ROOTPROOFSESSIONTAG=%s\n", stag);

   // Whether user specific config files are enabled
   if (fgWorkerUsrCfg)
      fprintf(fenv, "ROOTUSEUSERCFG=1\n");

   // Set Open socket
   fprintf(fenv, "ROOTOPENSOCK=%s\n", fPClient->UNIXSockPath());

   // Entity
   fprintf(fenv, "ROOTENTITY=%s@%s\n", fClientID, fLink->Host());

   // Session ID
   fprintf(fenv, "ROOTSESSIONID=%d\n", psid);

   // Client ID
   fprintf(fenv, "ROOTCLIENTID=%d\n", fCID);

   // Client Protocol
   fprintf(fenv, "ROOTPROOFCLNTVERS=%d\n", fPClient->Version());

   // Ordinal number
   fprintf(fenv, "ROOTPROOFORDINAL=%s\n", xps->Ordinal());

   // ROOT version tag if different from the default one
   if (getenv("ROOTVERSIONTAG"))
      fprintf(fenv, "ROOTVERSIONTAG=%s\n", getenv("ROOTVERSIONTAG"));

   // Config file
   if (cfg && strlen(cfg) > 0)
      fprintf(fenv, "ROOTPROOFCFGFILE=%s\n", cfg);

   // Log file in the log dir
   XrdOucString logfile = sessdir;
   logfile += ".log";
   fprintf(fenv, "ROOTPROOFLOGFILE=%s\n", logfile.c_str());
   xps->SetFileout(logfile.c_str());

   // Additional envs (xpd.putenv directive)
   if (fgProofServEnvs.length() > 0) {
      // Go through the list
      XrdOucString env;
      int from = 0;
      while ((from = fgProofServEnvs.tokenize(env, from, ',')) != -1) {
         if (env.length() > 0) {
            // Resolve keywords
            ResolveKeywords(env, fPClient);
            // Set the env now
            ev = new char[env.length()+1];
            strncpy(ev, env.c_str(), env.length());
            ev[env.length()] = 0;
            putenv(ev);
            fprintf(fenv, "%s\n", ev);
            MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);
         }
      }
   }

   // Set the user envs
   if (xps->UserEnvs() &&
       strlen(xps->UserEnvs()) && strstr(xps->UserEnvs(),"=")) {
      // The single components
      XrdOucString ue = xps->UserEnvs();
      XrdOucString env, namelist;
      int from = 0, ieq = -1;
      while ((from = ue.tokenize(env, from, ',')) != -1) {
         if (env.length() > 0 && (ieq = env.find('=')) != -1) {
            ev = new char[env.length()+1];
            strncpy(ev, env.c_str(), env.length());
            ev[env.length()] = 0;
            putenv(ev);
            fprintf(fenv, "%s\n", ev);
            MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);
            env.erase(ieq);
            if (namelist.length() > 0)
               namelist += ',';
            namelist += env;
         }
      }
      // The list of names, ','-separated
      ev = new char[strlen("PROOF_ALLVARS=") + namelist.length() + 2];
      sprintf(ev, "PROOF_ALLVARS=%s", namelist.c_str());
      putenv(ev);
      fprintf(fenv, "%s\n", ev);
      MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: "<<ev);
   }

   // Close file
   fclose(fenv);

   // Create or Update symlink to last session
   TRACEI(DBG, "SetProofServEnvOld: creating symlink");
   XrdOucString syml = udir;
   if (fSrvType == kXPD_WorkerServer)
      syml += "/last-worker-session";
   else
      syml += "/last-master-session";
   if (XrdProofdAux::SymLink(logdir.c_str(), syml.c_str()) != 0) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnvOld: problems creating symlink to "
                    " last session (errno: "<<errno<<")");
   }

   // We are done
   MTRACE(DBG, "xpd:child: ", "SetProofServEnvOld: done");
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::SetProofServEnv(XrdROOT *r)
{
   // Set basic environment accordingly to 'r'

   char *ev = 0;

   MTRACE(REQ, "xpd:child: ",
               "SetProofServEnv: enter: ROOT dir: "<< (r ? r->Dir() : "*** undef ***"));

   if (r) {
      char *rootsys = (char *) r->Dir();
#ifndef ROOTLIBDIR
      char *ldpath = new char[32 + strlen(rootsys) + fgBareLibPath.length()];
      if (fgBareLibPath.length() > 0)
         sprintf(ldpath, "%s=%s/lib:%s", XPD_LIBPATH, rootsys, fgBareLibPath.c_str());
      else
         sprintf(ldpath, "%s=%s/lib", XPD_LIBPATH, rootsys);
      putenv(ldpath);
#endif
      // Set ROOTSYS
      ev = new char[15 + strlen(rootsys)];
      sprintf(ev, "ROOTSYS=%s", rootsys);
      putenv(ev);

      // Set conf dir
      ev = new char[20 + strlen(rootsys)];
      sprintf(ev, "ROOTCONFDIR=%s", rootsys);
      putenv(ev);

      // Set TMPDIR
      ev = new char[20 + strlen(fgTMPdir)];
      sprintf(ev, "TMPDIR=%s", fgTMPdir);
      putenv(ev);

      // Done
      return 0;
   }

   // Bad input
   MTRACE(REQ,  "xpd:child: ", "SetProofServEnv: XrdROOT instance undefined!");
   return -1;
}

//______________________________________________________________________________
int XrdProofdProtocol::SetProofServEnv(int psid, int loglevel, const char *cfg)
{
   // Set environment for proofserv

   char *ev = 0;

   MTRACE(REQ,  "xpd:child: ", "SetProofServEnv: enter: psid: "<<psid<<
                      ", log: "<<loglevel);

   // Make sure the principal client is defined
   if (!fPClient) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: principal client undefined - cannot continue");
      return -1;
   }

   // Old proofservs expect different settings
   if (fPClient->ROOT() && fPClient->ROOT()->SrvProtVers() < 14)
      return SetProofServEnvOld(psid, loglevel, cfg);

   // Session proxy
   XrdProofServProxy *xps = fPClient->ProofServs()->at(psid);
   if (!xps) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: unable to get instance of proofserv proxy");
      return -1;
   }

   // Client sandbox
   XrdOucString udir = fPClient->Workdir();
   MTRACE(DBG, "xpd:child: ",
               "SetProofServEnv: sandbox for "<<fClientID<<" is: "<<udir);

   // Create and log into the directory reserved to this session:
   // the unique tag will identify it
   char hn[64], stag[512];
#if defined(XPD__SUNCC)
   sysinfo(SI_HOSTNAME, hn, sizeof(hn));
#else
   gethostname(hn, sizeof(hn));
#endif
   XrdOucString host = hn;
   if (host.find(".") != STR_NPOS)
      host.erase(host.find("."));
   sprintf(stag,"%s-%d-%d",host.c_str(),(int)time(0),getpid());

   // Session dir
   XrdOucString sessiondir = udir;
   if (fSrvType == kXPD_TopMaster) {
      sessiondir += "/session-";
      sessiondir += stag;
      xps->SetTag(stag);
   } else {
      sessiondir += "/";
      sessiondir += xps->Tag();
   }
   MTRACE(DBG, "xpd:child: ", "SetProofServEnv: session dir "<<sessiondir);
   // Make sure the directory exists ...
   if (XrdProofdAux::AssertDir(sessiondir.c_str(), fUI, fgChangeOwn) == -1) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: unable to create log dir: "<<sessiondir);
      return -1;
   }
   // ... and log into it
   if (XrdProofdAux::ChangeToDir(sessiondir.c_str(), fUI, fgChangeOwn) != 0) {
      MTRACE(XERR, "xpd:child: ", "SetProofServEnv: couldn't change directory to "<<
                   sessiondir);
      return -1;
   }

   // Set basic environment for proofserv
   if (SetProofServEnv(fPClient->ROOT()) != 0) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: problems setting basic environment - exit");
      return -1;
   }

   // The session working dir depends on the role
   XrdOucString swrkdir = sessiondir;
   if (fSrvType == kXPD_WorkerServer)
      swrkdir += "/worker-";
   else
      swrkdir += "/master-";
   swrkdir += xps->Ordinal();
   swrkdir += "-";
   swrkdir += stag;

   // Create the rootrc and env files
   MTRACE(DBG, "xpd:child: ", "SetProofServEnv: creating env file");
   XrdOucString rcfile = swrkdir;
   rcfile += ".rootrc";
   FILE *frc = fopen(rcfile.c_str(), "w");
   if (!frc) {
      MTRACE(XERR, "xpd:child: ",
                  "SetProofServEnv: unable to open rootrc file: "<<rcfile);
      return -1;
   }
   // Symlink to session.rootrc
   if (XrdProofdAux::SymLink(rcfile.c_str(), "session.rootrc") != 0) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: problems creating symlink to"
                    "'session.rootrc' (errno: "<<errno<<")");
   }
   MTRACE(DBG, "xpd:child: ",
               "SetProofServEnv: session rootrc file: "<< rcfile);

   // Port
   fprintf(frc,"# XrdProofdProtocol listening port\n");
   fprintf(frc, "ProofServ.XpdPort: %d\n", fgMgr.Port());

   // The session working dir depends on the role
   fprintf(frc,"# The session working dir\n");
   fprintf(frc,"ProofServ.SessionDir: %s\n", swrkdir.c_str());

   // Log / Debug level
   fprintf(frc,"# Proof Log/Debug level\n");
   fprintf(frc,"Proof.DebugLevel: %d\n", loglevel);

   // Ordinal number
   fprintf(frc,"# Ordinal number\n");
   fprintf(frc,"ProofServ.Ordinal: %s\n", xps->Ordinal());

   // ROOT Version tag
   if (fPClient->ROOT()) {
      fprintf(frc,"# ROOT Version tag\n");
      fprintf(frc,"ProofServ.RootVersionTag: %s\n", fPClient->ROOT()->Tag());
   }
   // Proof group
   if (fPClient->Group()) {
      fprintf(frc,"# Proof group\n");
      fprintf(frc,"ProofServ.ProofGroup: %s\n", fPClient->Group()->Name());
      // Make sure that the related dataset dir exists
      // The disk quota manager requires that dataset metadata info under
      // <user_datasetdir> = <global_datasetdir>/<group>/<user>
      if ((fgMgr.SrvType() == kXPD_TopMaster || fgMgr.SrvType() == kXPD_AnyServer) &&
         fgMgr.DataSetDir()) {
         XrdOucString dsetdir = fgMgr.DataSetDir();
         if (dsetdir.length() > 0) {
            dsetdir += "/";
            dsetdir += fGroupID;
            dsetdir += "/";
            dsetdir += fClientID;
         } else {
            dsetdir += fUI.fWorkDir;
            dsetdir += "/datasets";
         }
         if (XrdProofdAux::AssertDir(dsetdir.c_str(), fUI, fgChangeOwn) == -1) {
            MTRACE(XERR, "xpd:child: ",
                         "SetProofServEnv: unable to assert dataset dir: "<<dsetdir);
            return -1;
         }
         fprintf(frc,"# User's dataset dir\n");
         fprintf(frc,"ProofServ.DataSetDir: %s\n", dsetdir.c_str());
         // Export also the dataset root (for location purposes)
         fprintf(frc,"# Global root for datasets\n");
         fprintf(frc,"ProofServ.DataSetRoot: %s\n", fgMgr.DataSetDir());
      }
   }

   // Work dir
   fprintf(frc,"# Users sandbox\n");
   fprintf(frc, "ProofServ.Sandbox: %s\n", udir.c_str());

   // Session tag
   fprintf(frc,"# Session tag\n");
   fprintf(frc, "ProofServ.SessionTag: %s\n", stag);

   // Whether user specific config files are enabled
   if (fgWorkerUsrCfg) {
      fprintf(frc,"# Whether user specific config files are enabled\n");
      fprintf(frc, "ProofServ.UseUserCfg: 1\n");
   }
   // Set Open socket
   fprintf(frc,"# Open socket\n");
   fprintf(frc, "ProofServ.OpenSock: %s\n", fPClient->UNIXSockPath());
   // Entity
   fprintf(frc,"# Entity\n");
   if (fGroupID && strlen(fGroupID) > 0)
      fprintf(frc, "ProofServ.Entity: %s:%s@%s\n", fClientID, fGroupID, fLink->Host());
   else
      fprintf(frc, "ProofServ.Entity: %s@%s\n", fClientID, fLink->Host());


   // Session ID
   fprintf(frc,"# Session ID\n");
   fprintf(frc, "ProofServ.SessionID: %d\n", psid);

   // Client ID
   fprintf(frc,"# Client ID\n");
   fprintf(frc, "ProofServ.ClientID: %d\n", fCID);

   // Client Protocol
   fprintf(frc,"# Client Protocol\n");
   fprintf(frc, "ProofServ.ClientVersion: %d\n", fPClient->Version());

   // Config file
   if (cfg && strlen(cfg) > 0) {
      fprintf(frc,"# Config file\n");
      fprintf(frc, "ProofServ.ProofConfFile: %s\n", cfg);
   }

   // Additional rootrcs (xpd.putrc directive)
   if (fgProofServRCs.length() > 0) {
      fprintf(frc,"# Additional rootrcs (xpd.putrc directives)\n");
      // Go through the list
      XrdOucString rc;
      int from = 0;
      while ((from = fgProofServRCs.tokenize(rc, from, ',')) != -1)
         if (rc.length() > 0)
            fprintf(frc, "%s\n", rc.c_str());
   }

   // Done with this
   fclose(frc);

   // Now save the exported env variables, for the record
   XrdOucString envfile = swrkdir;
   envfile += ".env";
   FILE *fenv = fopen(envfile.c_str(), "w");
   if (!fenv) {
      MTRACE(XERR, "xpd:child: ",
                  "SetProofServEnv: unable to open env file: "<<envfile);
      return -1;
   }
   MTRACE(DBG, "xpd:child: ", "SetProofServEnv: environment file: "<< envfile);

   // Forwarded sec credentials, if any
   if (fAuthProt) {

      // Additional envs possibly set by the protocol for next application
      XrdOucString secenvs(getenv("XrdSecENVS"));
      if (secenvs.length() > 0) {
         // Go through the list
         XrdOucString env;
         int from = 0;
         while ((from = secenvs.tokenize(env, from, ',')) != -1) {
            if (env.length() > 0) {
               // Set the env now
               ev = new char[env.length()+1];
               strncpy(ev, env.c_str(), env.length());
               ev[env.length()] = 0;
               putenv(ev);
               fprintf(fenv, "%s\n", ev);
               MTRACE(DBG, "xpd:child: ", "SetProofServEnv: "<<ev);
            }
         }
      }

      // The credential buffer, if any
      XrdSecCredentials *creds = fAuthProt->getCredentials();
      if (creds) {
         int lev = strlen("XrdSecCREDS=")+creds->size;
         ev = new char[lev+1];
         strcpy(ev, "XrdSecCREDS=");
         memcpy(ev+strlen("XrdSecCREDS="), creds->buffer, creds->size);
         ev[lev] = 0;
         putenv(ev);
         MTRACE(DBG, "xpd:child: ", "SetProofServEnv: XrdSecCREDS set");

         // If 'pwd', save AFS key, if any
         if (!strncmp(fAuthProt->Entity.prot, "pwd", 3)) {
            XrdOucString credsdir = udir;
            credsdir += "/.creds";
            // Make sure the directory exists
            if (!XrdProofdAux::AssertDir(credsdir.c_str(), fUI, fgChangeOwn)) {
               if (SaveAFSkey(creds, credsdir.c_str()) == 0) {
                  ev = new char[strlen("ROOTPROOFAFSCREDS=")+credsdir.length()+strlen("/.afs")+2];
                  sprintf(ev, "ROOTPROOFAFSCREDS=%s/.afs", credsdir.c_str());
                  putenv(ev);
                  fprintf(fenv, "ROOTPROOFAFSCREDS has been set\n");
                  MTRACE(DBG, "xpd:child: ", "SetProofServEnv: " << ev);
               } else {
                  MTRACE(DBG, "xpd:child: ", "SetProofServEnv: problems in saving AFS key");
               }
            } else {
               MTRACE(XERR, "xpd:child: ",
                            "SetProofServEnv: unable to create creds dir: "<<credsdir);
               return -1;
            }
         }
      }
   }

   // Library path
   fprintf(fenv, "%s=%s\n", XPD_LIBPATH, getenv(XPD_LIBPATH));

   // ROOTSYS
   fprintf(fenv, "ROOTSYS=%s\n", xps->ROOT()->Dir());

   // Conf dir
   fprintf(fenv, "ROOTCONFDIR=%s\n", xps->ROOT()->Dir());

   // TMPDIR
   fprintf(fenv, "TMPDIR=%s\n", fgTMPdir);

   // ROOT version tag (needed in building packages)
   ev = new char[strlen("ROOTVERSIONTAG=")+strlen(fPClient->ROOT()->Tag())+2];
   sprintf(ev, "ROOTVERSIONTAG=%s", fPClient->ROOT()->Tag());
   putenv(ev);
   fprintf(fenv, "%s\n", ev);

   // Log file in the log dir
   XrdOucString logfile = swrkdir;
   logfile += ".log";
   ev = new char[strlen("ROOTPROOFLOGFILE=")+logfile.length()+2];
   sprintf(ev, "ROOTPROOFLOGFILE=%s", logfile.c_str());
   putenv(ev);
   fprintf(fenv, "%s\n", ev);
   xps->SetFileout(logfile.c_str());

   // Xrootd config file
   ev = new char[strlen("XRDCF=")+fgCfgFile.fName.length()+2];
   sprintf(ev, "XRDCF=%s", fgCfgFile.fName.c_str());
   putenv(ev);
   fprintf(fenv, "%s\n", ev);

   // Additional envs (xpd.putenv directive)
   if (fgProofServEnvs.length() > 0) {
      // Go through the list
      XrdOucString env;
      int from = 0;
      while ((from = fgProofServEnvs.tokenize(env, from, ',')) != -1) {
         if (env.length() > 0) {
            // Resolve keywords
            ResolveKeywords(env, fPClient);
            // Set the env now
            ev = new char[env.length()+1];
            strncpy(ev, env.c_str(), env.length());
            ev[env.length()] = 0;
            putenv(ev);
            fprintf(fenv, "%s\n", ev);
            MTRACE(DBG, "xpd:child: ", "SetProofServEnv: "<<ev);
         }
      }
   }

   // Set the user envs
   if (xps->UserEnvs() &&
       strlen(xps->UserEnvs()) && strstr(xps->UserEnvs(),"=")) {
      // The single components
      XrdOucString ue = xps->UserEnvs();
      XrdOucString env, namelist;
      int from = 0, ieq = -1;
      while ((from = ue.tokenize(env, from, ',')) != -1) {
         if (env.length() > 0 && (ieq = env.find('=')) != -1) {
            ev = new char[env.length()+1];
            strncpy(ev, env.c_str(), env.length());
            ev[env.length()] = 0;
            putenv(ev);
            fprintf(fenv, "%s\n", ev);
            MTRACE(DBG, "xpd:child: ", "SetProofServEnv: "<<ev);
            env.erase(ieq);
            if (namelist.length() > 0)
               namelist += ',';
            namelist += env;
         }
      }
      // The list of names, ','-separated
      ev = new char[strlen("PROOF_ALLVARS=") + namelist.length() + 2];
      sprintf(ev, "PROOF_ALLVARS=%s", namelist.c_str());
      putenv(ev);
      fprintf(fenv, "%s\n", ev);
      MTRACE(DBG, "xpd:child: ", "SetProofServEnv: "<<ev);
   }

   // Close file
   fclose(fenv);

   // Create or Update symlink to last session
   TRACEI(DBG, "SetProofServEnv: creating symlink");
   XrdOucString syml = udir;
   if (fSrvType == kXPD_WorkerServer)
      syml += "/last-worker-session";
   else
      syml += "/last-master-session";
   if (XrdProofdAux::SymLink(sessiondir.c_str(), syml.c_str()) != 0) {
      MTRACE(XERR, "xpd:child: ",
                   "SetProofServEnv: problems creating symlink to "
                    " last session (errno: "<<errno<<")");
   }

   // We are done
   MTRACE(DBG, "xpd:child: ", "SetProofServEnv: done");
   return 0;
}

//_________________________________________________________________________________
int XrdProofdProtocol::Create()
{
   // Handle a request to create a new session

   int psid = -1, rc = 1;

   TRACEI(REQ, "Create: enter");
   XrdSysMutexHelper mh(fPClient->Mutex());

   // Allocate next free server ID and fill in the basic stuff
   psid = fPClient->GetFreeServID();
   XrdProofServProxy *xps = fPClient->ProofServs()->at(psid);
   xps->SetClient((const char *)fClientID);
   xps->SetID(psid);
   xps->SetSrvType(fSrvType);

   // Prepare the stream identifier
   unsigned short sid;
   memcpy((void *)&sid, (const void *)&(fRequest.header.streamid[0]), 2);
   // We associate this instance to the corresponding slot in the
   // session vector of attached clients
   XrdClientID *csid = xps->GetClientID(fCID);
   csid->fP = this;
   csid->fSid = sid;
   // Take parentship, if orphalin
   xps->SetParent(csid);

   // Unmarshall log level
   int loglevel = ntohl(fRequest.proof.int1);

   // Parse buffer
   char *buf = fArgp->buff;
   int   len = fRequest.proof.dlen;

   // Extract session tag
   XrdOucString tag(buf,len);
   tag.erase(tag.find('|'));
   xps->SetTag(tag.c_str());
   TRACEI(DBG, "Create: tag: "<<tag);

   // Extract ordinal number
   XrdOucString ord = "0";
   if ((fSrvType == kXPD_WorkerServer) || (fSrvType == kXPD_MasterServer)) {
      ord.assign(buf,0,len-1);
      int iord = ord.find("|ord:");
      if (iord != STR_NPOS) {
         ord.erase(0,iord+5);
         ord.erase(ord.find("|"));
      } else
         ord = "0";
   }
   xps->SetOrdinal(ord.c_str());

   // Extract config file, if any (for backward compatibility)
   XrdOucString cffile;
   cffile.assign(buf,0,len-1);
   int icf = cffile.find("|cf:");
   if (icf != STR_NPOS) {
      cffile.erase(0,icf+4);
      cffile.erase(cffile.find("|"));
   } else
      cffile = "";

   // Extract user envs, if any
   XrdOucString uenvs;
   uenvs.assign(buf,0,len-1);
   int ienv = uenvs.find("|envs:");
   if (ienv != STR_NPOS) {
      uenvs.erase(0,ienv+6);
      uenvs.erase(uenvs.find("|"));
      xps->SetUserEnvs(uenvs.c_str());
   } else
      uenvs = "";

   // The ROOT version to be used
   xps->SetROOT(fPClient->ROOT());
   XPDPRT("Create: using ROOT version: "<<xps->ROOT()->Export());
   if (fSrvType == kXPD_TopMaster) {
      // Notify the client if using a version different from the default one
      if (fPClient->ROOT() != fgROOT.front()) {
         XrdOucString msg("++++ Using NON-default ROOT version: ");
         msg += xps->ROOT()->Export();
         msg += " ++++\n";
         fResponse.Send(kXR_attn, kXPD_srvmsg, (char *) msg.c_str(), msg.length());
      }
   }

   // Notify
   TRACEI(DBG, "Create: {ord,cfg,psid,cid,log}: {"<<ord<<","<<cffile<<","<<psid
                                                  <<","<<fCID<<","<<loglevel<<"}");
   if (uenvs.length() > 0)
      TRACEI(DBG, "Create: user envs: "<<uenvs);

   // Here we fork: for some weird problem on SMP machines there is a
   // non-zero probability for a deadlock situation in system mutexes.
   // The semaphore seems to have solved the problem.
   if (fgForkSem.Wait(10) != 0) {
      xps->Reset();
      // Timeout acquire fork semaphore
      fResponse.Send(kXR_ServerError, "timed-out acquiring fork semaphore");
      return rc;
   }

   // Pipe to communicate status of setup
   int fp[2];
   if (pipe(fp) != 0) {
      xps->Reset();
      // Failure creating pipe
      fResponse.Send(kXR_ServerError,
                     "unable to create pipe for status-of-setup communication");
      return rc;
   }

   // Fork an agent process to handle this session
   int pid = -1;
   TRACEI(FORK,"Forking external proofsrv: UNIX sock: "<<fPClient->UNIXSockPath());
   if (!(pid = fgSched->Fork("proofsrv"))) {

      int setupOK = 0;

      MTRACE(FORK, "xpd: ", "child process");

      // We set to the user environment
      if (SetUserEnvironment() != 0) {
         MTRACE(XERR, "xpd:child: ",
                      "Create: SetUserEnvironment did not return OK - EXIT");
         write(fp[1], &setupOK, sizeof(setupOK));
         close(fp[0]);
         close(fp[1]);
         exit(1);
      }

      char *argvv[6] = {0};

      // We add our PID to be able to identify processes coming from us
      char cpid[10] = {0};
      sprintf(cpid, "%d", getppid());

      // Log level
      char clog[10] = {0};
      sprintf(clog, "%d", loglevel);

      // start server
      argvv[0] = (char *) xps->ROOT()->PrgmSrv();
      argvv[1] = (char *)((fSrvType == kXPD_WorkerServer) ? "proofslave"
                       : "proofserv");
      argvv[2] = (char *)"xpd";
      argvv[3] = (char *)cpid;
      argvv[4] = (char *)clog;
      argvv[5] = 0;

      // Set environment for proofserv
      if (SetProofServEnv(psid, loglevel, cffile.c_str()) != 0) {
         MTRACE(XERR, "xpd:child: ",
                      "Create: SetProofServEnv did not return OK - EXIT");
         write(fp[1], &setupOK, sizeof(setupOK));
         close(fp[0]);
         close(fp[1]);
         exit(1);
      }

      // Setup OK: now we go
      // Communicate the logfile path
      int lfout = strlen(xps->Fileout());
      write(fp[1], &lfout, sizeof(lfout));
      if (lfout > 0) {
         int n, ns = 0;
         char *buf = (char *) xps->Fileout();
         for (n = 0; n < lfout; n += ns) {
            if ((ns = write(fp[1], buf + n, lfout - n)) <= 0) {
               MTRACE(XERR, "xpd:child: ",
                            "Create: SetProofServEnv did not return OK - EXIT");
               write(fp[1], &setupOK, sizeof(setupOK));
               close(fp[0]);
               close(fp[1]);
               exit(1);
            }
         }
      }

      // Cleanup
      close(fp[0]);
      close(fp[1]);

      MTRACE(LOGIN,"xpd:child: ", "Create: fClientID: "<<fClientID<<
                         ", uid: "<<getuid()<<", euid:"<<geteuid());
      // Run the program
      execv(xps->ROOT()->PrgmSrv(), argvv);

      // We should not be here!!!
      MERROR("xpd:child: ", "Create: returned from execv: bad, bad sign !!!");
      exit(1);
   }

   TRACEP(FORK,"Parent process");

   // Wakeup colleagues
   fgForkSem.Post();

   // parent process
   if (pid < 0) {
      xps->Reset();
      // Failure in forking
      fResponse.Send(kXR_ServerError, "could not fork agent");
      close(fp[0]);
      close(fp[1]);
      return rc;
   }

   // Read status-of-setup from pipe
   XrdOucString emsg;
   int setupOK = 0;
   if (read(fp[0], &setupOK, sizeof(setupOK)) == sizeof(setupOK)) {
   // now we wait for the callback to be (successfully) established

      if (setupOK > 0) {
         // Receive path of the log file
         int lfout = setupOK;
         char *buf = new char[lfout + 1];
         int n, nr = 0;
         for (n = 0; n < lfout; n += nr) {
            while ((nr = read(fp[0], buf + n, lfout - n)) == -1 && errno == EINTR)
               errno = 0;   // probably a SIGCLD that was caught
            if (nr == 0)
               break;          // EOF
            if (nr < 0) {
               // Failure
               setupOK= -1;
               emsg += ": failure receiving logfile path";
               break;
            }
         }
         if (setupOK > 0) {
            buf[lfout] = 0;
            xps->SetFileout(buf);
            // Set also the session tag
            XrdOucString stag(buf);
            stag.erase(stag.rfind('/'));
            stag.erase(0, stag.find("session-") + strlen("session-"));
            xps->SetTag(stag.c_str());
         }
         delete[] buf;
      } else {
         emsg += ": proofserv startup failed";
      }
   } else {
      emsg += ": problems receiving status-of-setup after forking";
   }

   // Cleanup
   close(fp[0]);
   close(fp[1]);

   // Notify to user
   if (setupOK > 0) {
      if (fSrvType == kXPD_TopMaster) {
         // Send also back the data pool url
         XrdOucString dpu = fgPoolURL;
         if (!dpu.endswith('/'))
            dpu += '/';
         dpu += fgNamespace;
         fResponse.Send(psid, xps->ROOT()->SrvProtVers(), (kXR_int16)XPROOFD_VERSBIN,
                       (void *) dpu.c_str(), dpu.length());
      } else
         fResponse.Send(psid, xps->ROOT()->SrvProtVers(), (kXR_int16)XPROOFD_VERSBIN);
   } else {
      // Failure
      emsg += ": failure setting up proofserv" ;
      xps->Reset();
      KillProofServ(pid, 1);
      fResponse.Send(kXR_ServerError, emsg.c_str());
      return rc;
   }
   // UNIX Socket is saved now
   fPClient->SetUNIXSockSaved();

   // now we wait for the callback to be (successfully) established
   TRACEP(FORK, "Create: server launched: wait for callback ");

   // We will get back a peer to initialize a link
   XrdNetPeer peerpsrv;
   XrdLink   *linkpsrv = 0;
   int lnkopts = 0;

   // Perform regular accept
   if (!(fPClient->UNIXSock()->Accept(peerpsrv, XRDNET_NODNTRIM, fgInternalWait))) {

      // We need the right privileges to do this
      XrdOucString msg("did not receive callback: ");
      if (KillProofServ(pid, 1) != 0)
         msg += "process could not be killed";
      else
         msg += "process killed";
      fResponse.Send(kXR_attn, kXPD_errmsg, (char *) msg.c_str(), msg.length());

      xps->Reset();
      return rc;
   }
   // Make sure we have the full host name
   if (peerpsrv.InetName) {
      char *ptmp = peerpsrv.InetName;
      peerpsrv.InetName = XrdNetDNS::getHostName("localhost");
      free(ptmp);
   }

   // Allocate a new network object
   if (!(linkpsrv = XrdLink::Alloc(peerpsrv, lnkopts))) {

      // We need the right privileges to do this
      XrdOucString msg("could not allocate network object: ");
      if (KillProofServ(pid, 0) != 0)
         msg += "process could not be killed";
      else
         msg += "process killed";
      fResponse.Send(kXR_attn, kXPD_errmsg, (char *) msg.c_str(), msg.length());

      xps->Reset();
      return rc;

   } else {

      // Keep buffer after object goes away
      peerpsrv.InetBuff = 0;
      TRACEP(DBG, "Accepted connection from " << peerpsrv.InetName);

      // Get a protocol object off the stack (if none, allocate a new one)
      XrdProtocol *xp = Match(linkpsrv);
      if (!xp) {

         // We need the right privileges to do this
         XrdOucString msg("match failed: protocol error: ");
         if (KillProofServ(pid, 0) != 0)
            msg += "process could not be killed";
         else
            msg += "process killed";
         fResponse.Send(kXR_attn, kXPD_errmsg, (char *) msg.c_str(), msg.length());

         linkpsrv->Close();
         xps->Reset();
         return rc;
      }

      // Take a short-cut and process the initial request as a sticky request
      xp->Process(linkpsrv);
      if (xp->Process(linkpsrv) != 1) {
         // We need the right privileges to do this
         XrdOucString msg("handshake with internal link failed: ");
         if (KillProofServ(pid, 0) != 0)
            msg += "process could not be killed";
         else
            msg += "process killed";
         fResponse.Send(kXR_attn, kXPD_errmsg, (char *) msg.c_str(), msg.length());

         linkpsrv->Close();
         xps->Reset();
         return rc;
      }

      // Attach this link to the appropriate poller and enable it.
      if (!XrdPoll::Attach(linkpsrv)) {

         // We need the right privileges to do this
         XrdOucString msg("could not attach new internal link to poller: ");
         if (KillProofServ(pid, 0) != 0)
            msg += "process could not be killed";
         else
            msg += "process killed";
         fResponse.Send(kXR_attn, kXPD_errmsg, (char *) msg.c_str(), msg.length());

         linkpsrv->Close();
         xps->Reset();
         return rc;
      }

      // Tight this protocol instance to the link
      linkpsrv->setProtocol(xp);

      // Schedule it
      fgSched->Schedule((XrdJob *)linkpsrv);
   }

   // Set ID
   xps->SetSrv(pid);

   // Set the group, if any
   xps->SetGroup(fPClient->Group());

   // Change child process priority, if required
   if (fgPriorities.size() > 0) {
      XrdOucString usr(fClientID);
      int dp = 0;
      int nmmx = -1;
      std::list<XrdProofdPriority *>::iterator i;
      for (i = fgPriorities.begin(); i != fgPriorities.end(); ++i) {
         int nm = usr.matches((*i)->fUser.c_str());
         if (nm >= nmmx) {
            nmmx = nm;
            dp = (*i)->fDeltaPriority;
         }
      }
      if (nmmx > -1) {
         // Changing child process priority for this user
         if (xps->ChangeProcessPriority(dp) != 0) {
            TRACEI(XERR, "Create: problems changing child process priority");
         } else {
            TRACEI(DBG, "Create: priority of the child process changed by "
                        << dp << " units");
         }
      }
   }
   TRACEI(DBG, "Create: xps: "<<xps<<", ClientID: "<<(int *)(xps->Parent())<<" (sid: "<<sid<<")");

   // Record this session in the sandbox
   if (fSrvType != kXPD_Internal) {

      XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
      if (XpdBadPGuard(pGuard, fUI.fUid)) {
         TRACEI(REQ, "Create: could not get privileges to run AddNewSession");
      } else {
         if (fPClient->AddNewSession(xps->Tag()) == -1)
            TRACEI(REQ, "Create: problems recording session in sandbox");
      }
   }

   // Over
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::SendData(XrdProofdResponse *resp,
                                kXR_int32 sid, XrdSrvBuffer **buf)
{
   // Send data over the open link. Segmentation is done here, if required.

   int rc = 1;

   TRACEI(ACT, "SendData: enter: length: "<<fRequest.header.dlen<<" bytes ");

   // Buffer length
   int len = fRequest.header.dlen;

   // Quantum size
   int quantum = (len > fgMaxBuffsz ? fgMaxBuffsz : len);

   // Make sure we have a large enough buffer
   if (!fArgp || quantum < fhalfBSize || quantum > fArgp->bsize) {
      if ((rc = GetBuff(quantum)) <= 0)
         return rc;
   } else if (fhcNow < fhcNext)
      fhcNow++;

   // Now send over all of the data as unsolicited messages
   while (len > 0) {
      if ((rc = GetData("data", fArgp->buff, quantum)))
         return rc;
      if (buf && !(*buf))
         *buf = new XrdSrvBuffer(fArgp->buff, quantum, 1);
      // Send
      if (sid > -1) {
         if (resp->Send(kXR_attn, kXPD_msgsid, sid, fArgp->buff, quantum))
            return 1;
      } else {
         if (resp->Send(kXR_attn, kXPD_msg, fArgp->buff, quantum))
            return 1;
      }
      // Next segment
      len -= quantum;
      if (len < quantum)
         quantum = len;
   }

   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::SendDataN(XrdProofServProxy *xps,
                                 XrdSrvBuffer **buf)
{
   // Send data over the open client links of session 'xps'.
   // Used when all the connected clients are eligible to receive the message.
   // Segmentation is done here, if required.

   int rc = 1;

   TRACEI(ACT, "SendDataN: enter: length: "<<fRequest.header.dlen<<" bytes ");

   // Buffer length
   int len = fRequest.header.dlen;

   // Quantum size
   int quantum = (len > fgMaxBuffsz ? fgMaxBuffsz : len);

   // Make sure we have a large enough buffer
   if (!fArgp || quantum < fhalfBSize || quantum > fArgp->bsize) {
      if ((rc = GetBuff(quantum)) <= 0)
         return rc;
   } else if (fhcNow < fhcNext)
      fhcNow++;

   // Now send over all of the data as unsolicited messages
   while (len > 0) {
      if ((rc = GetData("data", fArgp->buff, quantum)))
         return rc;
      if (buf && !(*buf))
         *buf = new XrdSrvBuffer(fArgp->buff, quantum, 1);
      // Broadcast
      XrdClientID *csid = 0;
      int ic = 0;
      for (ic = 0; ic < (int) xps->Clients()->size(); ic++) {
         if ((csid = xps->Clients()->at(ic)) && csid->fP) {
            XrdProofdResponse& resp = csid->fP->fResponse;
            int rs = 0;
            {  XrdSysMutexHelper mhp(resp.fMutex);
               unsigned short sid;
               resp.GetSID(sid);
               TRACEI(HDBG, "SendDataN: INTERNAL: this sid: "<<sid<<
                            "; client sid:"<<csid->fSid);
               resp.Set(csid->fSid);
               rs = resp.Send(kXR_attn, kXPD_msg, fArgp->buff, quantum);
               resp.Set(sid);
            }
            if (rs)
               return 1;
         }
      }

      // Next segment
      len -= quantum;
      if (len < quantum)
         quantum = len;
   }

   // Done
   return 0;
}

//_____________________________________________________________________________
int XrdProofdProtocol::SendMsg()
{
   // Handle a request to forward a message to another process

   static const char *crecv[4] = {"master proofserv", "top master",
                                  "client", "undefined"};
   int rc = 1;

   XrdSysMutexHelper mh(fResponse.fMutex);

   // Unmarshall the data
   int psid = ntohl(fRequest.sendrcv.sid);
   int opt = ntohl(fRequest.sendrcv.opt);
   bool external = !(opt & kXPD_internal);

   // Find server session
   XrdProofServProxy *xps = 0;
   if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
       !(xps = fPClient->ProofServs()->at(psid))) {
      TRACEP(XERR, "SendMsg: session ID not found: "<< psid);
      fResponse.Send(kXR_InvalidRequest,"session ID not found");
      return rc;
   }

   // Forward message as unsolicited
   int len = fRequest.header.dlen;

   // Notify
   TRACEP(DBG, "SendMsg: psid: "<<psid<<", xps: "<<xps<<", status: "<<xps->Status()<<
               ", cid: "<<fCID);

   if (external) {

      if (opt & kXPD_process) {
         TRACEP(DBG, "SendMsg: INT: setting proofserv in 'running' state");
         xps->SetStatus(kXPD_running);
         // Update global list of active sessions
         fgMgr.AddActiveSession(xps);
         // Update counters in client instance
         fPClient->CountSession(1, (xps->SrvType() == kXPD_WorkerServer));
         // Notify
         TRACE(INFLT, fPClient->ID()<<": kXPD_process: act w: "<<
                      fPClient->WorkerProofServ() <<": act m: "<<
                      fPClient->MasterProofServ())
         // Update group info, if any
         XrdSysMutexHelper mtxh(&fgXPDMutex);
         if (fPClient->Group())
            fPClient->Group()->Count((const char *) fPClient->ID());
         // Recalculate the inflate factors
         SetInflateFactors();
      }

      // Send to proofsrv our client ID
      if (fCID == -1) {
         fResponse.Send(kXR_ServerError,"EXT: getting clientSID");
         return rc;
      }
      if (SendData(xps->ProofSrv(), fCID)) {
         fResponse.Send(kXR_ServerError,"EXT: sending message to proofserv");
         return rc;
      }
      // Notify to user
      fResponse.Send();
      TRACEP(DBG, "SendMsg: EXT: message sent to proofserv ("<<len<<" bytes)");

   } else {

      bool saveStartMsg = 0;
      XrdSrvBuffer *savedBuf = 0;
      // Additional info about the message
      if (opt & kXPD_setidle) {
         TRACEP(DBG, "SendMsg: INT: setting proofserv in 'idle' state");
         xps->SetStatus(kXPD_idle);
         xps->SetSchedRoundRobin(0);
         // Update global list of active sessions
         fgMgr.RemoveActiveSession(xps);
         // Clean start processing message, if any
         xps->DeleteStartMsg();
         // Update counters in client instance
         fPClient->CountSession(-1, (xps->SrvType() == kXPD_WorkerServer));
         // Notify
         TRACE(INFLT, fPClient->ID()<<": kXPD_setidle: act w: "<<
                      fPClient->WorkerProofServ()<<": act m: "<<
                      fPClient->MasterProofServ())
         // Update group info, if any
         XrdSysMutexHelper mtxh(&fgXPDMutex);
         if (fPClient->Group()) {
            if (!(fPClient->WorkerProofServ()+fPClient->MasterProofServ()))
               fPClient->Group()->Count((const char *) fPClient->ID(), -1);
         }
         // Recalculate the inflate factors
         SetInflateFactors();
      } else if (opt & kXPD_querynum) {
         TRACEI(DBG, "SendMsg: INT: got message with query number");
         // Save query num message for later clients
         savedBuf = xps->QueryNum();
      } else if (opt & kXPD_startprocess) {
         TRACEI(DBG, "SendMsg: INT: setting proofserv in 'running' state");
         xps->SetStatus(kXPD_running);
         // Save start processing message for later clients
         xps->DeleteStartMsg();
         saveStartMsg = 1;
         // Update counters in client instance
         fPClient->CountSession(1, (xps->SrvType() == kXPD_WorkerServer));
      } else if (opt & kXPD_logmsg) {
         // We broadcast log messages only not idle to catch the
         // result from processing
         if (xps->Status() == kXPD_running) {
            TRACEI(DBG, "SendMsg: INT: broadcasting log message");
            opt |= kXPD_fb_prog;
         }
      }
      bool fbprog = (opt & kXPD_fb_prog);

      if (!fbprog) {
         // Get ID of the client
         int cid = ntohl(fRequest.sendrcv.cid);
         TRACEP(DBG, "SendMsg: INT: client ID: "<<cid);

         // Get corresponding instance
         XrdClientID *csid = 0;
         if (!xps || !INRANGE(cid, xps->Clients()) ||
             !(csid = xps->Clients()->at(cid))) {
            TRACEP(XERR, "SendMsg: INT: client ID not found (cid: "<<cid<<
                        ", size: "<<xps->Clients()->size()<<")");
            fResponse.Send(kXR_InvalidRequest,"Client ID not found");
            return rc;
         }
         if (!csid || !(csid->fP)) {
            TRACEP(XERR, "SendMsg: INT: client not connected: csid: "<<csid<<
                        ", cid: "<<cid<<", fSid: " << csid->fSid);
            // Notify to proofsrv
            fResponse.Send();
            return rc;
         }

         //
         // The message is strictly for the client requiring it
         int rs = 0;
         {  XrdSysMutexHelper mhp(csid->fP->fResponse.fMutex);
            unsigned short sid;
            csid->fP->fResponse.GetSID(sid);
            TRACEP(DBG, "SendMsg: INT: this sid: "<<sid<<
                        ", client sid: "<<csid->fSid);
            csid->fP->fResponse.Set(csid->fSid);
            rs = SendData(&(csid->fP->fResponse), -1, &savedBuf);
            csid->fP->fResponse.Set(sid);
         }
         if (rs) {
            fResponse.Send(kXR_ServerError,
                           "SendMsg: INT: sending message to client"
                           " or master proofserv");
            return rc;
         }
      } else {
         // Send to all connected clients
         if (SendDataN(xps, &savedBuf)) {
            fResponse.Send(kXR_ServerError,
                           "SendMsg: INT: sending message to client"
                           " or master proofserv");
            return rc;
         }
      }
      // Save start processing messages, if required
      if (saveStartMsg)
         xps->SetStartMsg(savedBuf);

      TRACEP(DBG, "SendMsg: INT: message sent to "<<crecv[xps->SrvType()]<<
                  " ("<<len<<" bytes)");
      // Notify to proofsrv
      fResponse.Send();
   }

   // Over
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::Urgent()
{
   // Handle generic request of a urgent message to be forwarded to the server
   unsigned int rc = 1;

   // Unmarshall the data
   int psid = ntohl(fRequest.proof.sid);
   int type = ntohl(fRequest.proof.int1);
   int int1 = ntohl(fRequest.proof.int2);
   int int2 = ntohl(fRequest.proof.int3);

   TRACEP(REQ, "Urgent: enter: psid: "<<psid<<", type: "<< type);

   // Find server session
   XrdProofServProxy *xps = 0;
   if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
       !(xps = fPClient->ProofServs()->at(psid))) {
      TRACEP(XERR, "Urgent: session ID not found");
      fResponse.Send(kXR_InvalidRequest,"Urgent: session ID not found");
      return rc;
   }

   TRACEP(DBG, "Urgent: xps: "<<xps<<", status: "<<xps->Status());

   // Check ID matching
   if (!xps->Match(psid)) {
      fResponse.Send(kXP_InvalidRequest,"Urgent: IDs do not match - do nothing");
      return rc;
   }

   // Prepare buffer
   int len = 3 *sizeof(kXR_int32);
   char *buf = new char[len];
   // Type
   kXR_int32 itmp = static_cast<kXR_int32>(htonl(type));
   memcpy(buf, &itmp, sizeof(kXR_int32));
   // First info container
   itmp = static_cast<kXR_int32>(htonl(int1));
   memcpy(buf + sizeof(kXR_int32), &itmp, sizeof(kXR_int32));
   // Second info container
   itmp = static_cast<kXR_int32>(htonl(int2));
   memcpy(buf + 2 * sizeof(kXR_int32), &itmp, sizeof(kXR_int32));
   // Send over
   if (xps->ProofSrv()->Send(kXR_attn, kXPD_urgent, buf, len) != 0) {
      fResponse.Send(kXP_ServerError,
                     "Urgent: could not propagate request to proofsrv");
      return rc;
   }

   // Notify to user
   fResponse.Send();
   TRACEP(DBG, "Urgent: request propagated to proofsrv");

   // Over
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::Admin()
{
   // Handle generic request of administrative type

   int rc = 1;

   // Unmarshall the data
   //
   int psid = ntohl(fRequest.proof.sid);
   int type = ntohl(fRequest.proof.int1);

   TRACEI(REQ, "Admin: enter: type: "<<type<<", psid: "<<psid);

   if (type == kQuerySessions) {

      XrdProofServProxy *xps = 0;
      int ns = 0;
      std::vector<XrdProofServProxy *>::iterator ip;
      for (ip = fPClient->ProofServs()->begin(); ip != fPClient->ProofServs()->end(); ++ip)
         if ((xps = *ip) && xps->IsValid() && (xps->SrvType() == kXPD_TopMaster)) {
            ns++;
            TRACEI(XERR, "Admin: found: " << xps << "(" << xps->IsValid() <<")");
         }

      // Generic info about all known sessions
      int len = (kXPROOFSRVTAGMAX+kXPROOFSRVALIASMAX+30)* (ns+1);
      char *buf = new char[len];
      if (!buf) {
         TRACEP(XERR, "Admin: no resources for results");
         fResponse.Send(kXR_NoMemory, "Admin: out-of-resources for results");
         return rc;
      }
      sprintf(buf, "%d", ns);

      xps = 0;
      for (ip = fPClient->ProofServs()->begin(); ip != fPClient->ProofServs()->end(); ++ip) {
         if ((xps = *ip) && xps->IsValid() && (xps->SrvType() == kXPD_TopMaster)) {
            sprintf(buf,"%s | %d %s %s %d %d",
                    buf, xps->ID(), xps->Tag(), xps->Alias(),
                    xps->Status(), xps->GetNClients());
         }
      }
      TRACEP(DBG, "Admin: sending: "<<buf);

      // Send back to user
      fResponse.Send(buf,strlen(buf)+1);
      if (buf) delete[] buf;

   } else if (type == kQueryLogPaths) {

      int ridx = ntohl(fRequest.proof.int2);

      // Find out for which session is this request
      char *stag = 0;
      int len = fRequest.header.dlen;
      if (len > 0) {
         char *buf = fArgp->buff;
         if (buf[0] != '*') {
            stag = new char[len+1];
            memcpy(stag, buf, len);
            stag[len] = 0;
         }
      }

      XrdOucString tag = (!stag && ridx >= 0) ? "last" : stag;
      if (!stag && fPClient->GuessTag(tag, ridx) != 0) {
         TRACEP(XERR, "Admin: query sess logs: session tag not found");
         fResponse.Send(kXR_InvalidRequest,"Admin: query log: session tag not found");
         return rc;
      }

      // Return message
      XrdOucString rmsg;

      // The session tag first
      rmsg += tag; rmsg += "|";

      // The pool URL second
      rmsg += fgPoolURL; rmsg += "|";

      // Locate the local log file
      XrdOucString sdir(fPClient->Workdir());
      sdir += "/session-";
      sdir += tag;

      // Open dir
      DIR *dir = opendir(sdir.c_str());
      if (!dir) {
         XrdOucString msg("Admin: cannot open dir ");
         msg += sdir; msg += " (errno: "; msg += errno; msg += ")";
         TRACEP(XERR, msg.c_str());
         fResponse.Send(kXR_InvalidRequest, msg.c_str());
         return rc;
      }
      // Scan the directory
      bool found = 0;
      struct dirent *ent = 0;
      while ((ent = (struct dirent *)readdir(dir))) {
         if (!strncmp(ent->d_name, "master-", 7) &&
              strstr(ent->d_name, ".log")) {
            rmsg += "|0 proof://"; rmsg += fgMgr.Host(); rmsg += ':';
            rmsg += fgMgr.Port(); rmsg += '/';
            rmsg += sdir; rmsg += '/'; rmsg += ent->d_name;
            found = 1;
            break;
         }
      }
      // Close dir
      closedir(dir);

      // Now open the workers file
      XrdOucString wfile(sdir);
      wfile += "/.workers";
      FILE *f = fopen(wfile.c_str(), "r");
      if (f) {
         char ln[2048];
         while (fgets(ln, sizeof(ln), f)) {
            if (ln[strlen(ln)-1] == '\n')
               ln[strlen(ln)-1] = 0;
            // Locate status and url
            char *ps = strchr(ln, ' ');
            if (ps) {
               *ps = 0;
               ps++;
               // Locate ordinal
               char *po = strchr(ps, ' ');
               if (po) {
                  po++;
                  // Locate path
                  char *pp = strchr(po, ' ');
                  if (pp) {
                     *pp = 0;
                     pp++;
                     // Record now
                     rmsg += "|"; rmsg += po;
                     rmsg += " "; rmsg += ln; rmsg += '/';
                     rmsg += pp;
                  }
               }
            }
         }
         fclose(f);
      }

      // Send back to user
      fResponse.Send((void *) rmsg.c_str(), rmsg.length()+1);

   } else if (type == kCleanupSessions) {

      // Target client (default us)
      XrdProofdClient *tgtclnt = fPClient;

      // Server type to clean
      int srvtype = ntohl(fRequest.proof.int2);

      // If super user we may be requested to cleanup everything
      bool all = 0;
      char *usr = 0;
      bool clntfound = 1;
      if (fSuperUser) {
         int what = ntohl(fRequest.proof.int2);
         all = (what == 1) ? 1 : 0;

         if (!all) {
            // Get a user name, if any.
            // A super user can ask cleaning for clients different from itself
            char *buf = 0;
            int len = fRequest.header.dlen;
            if (len > 0) {
               clntfound = 0;
               buf = fArgp->buff;
               len = (len < 9) ? len : 8;
            } else {
               buf = fClientID;
               len = strlen(fClientID);
            }
            if (len > 0) {
               usr = new char[len+1];
               memcpy(usr, buf, len);
               usr[len] = '\0';
               // Group info, if any
               char *grp = strstr(usr, ":");
               if (grp)
                  *grp++ = 0;
               // Find the client instance
               XrdProofdClient *c = 0;
               std::list<XrdProofdClient *>::iterator i;
               for (i = fgProofdClients.begin(); i != fgProofdClients.end(); ++i) {
                  if ((c = *i) && c->Match(usr,grp)) {
                     tgtclnt = c;
                     clntfound = 1;
                     break;
                  }
               }
               TRACEI(DBG, "Admin: CleanupSessions: superuser, cleaning usr: "<< usr);
            }
         } else {
            TRACEI(DBG, "Admin: CleanupSessions: superuser, all sessions cleaned");
         }
      } else {
         // Define the user name for later transactions (their executed under
         // the admin name)
         int len = strlen(tgtclnt->ID()) + 1;
         usr = new char[len+1];
         memcpy(usr, tgtclnt->ID(), len);
         usr[len] = '\0';
      }

      // We cannot continue if we do not have anything to clean
      if (!clntfound) {
         TRACEI(DBG, "Admin: specified client has no sessions - do nothing");
      }

      if (clntfound) {

         // The clients to cleaned
         std::list<XrdProofdClient *> *clnts;
         if (all) {
            // The full list
            clnts = &fgProofdClients;
         } else {
            clnts = new std::list<XrdProofdClient *>;
            clnts->push_back(tgtclnt);
         }

         // List of process IDs asked to terminate
         std::list<int *> signalledpid;

         // Loop over them
         XrdProofdClient *c = 0;
         std::list<XrdProofdClient *>::iterator i;
         for (i = clnts->begin(); i != clnts->end(); ++i) {
            if ((c = *i)) {

               // This part may be not thread safe
               XrdSysMutexHelper mh(c->Mutex());

               // Notify the attached clients that we are going to cleanup
               XrdOucString msg = "Admin: CleanupSessions: cleaning up client: requested by: ";
               msg += fLink->ID;
               int ic = 0;
               XrdProofdProtocol *p = 0;
               for (ic = 0; ic < (int) c->Clients()->size(); ic++) {
                  if ((p = c->Clients()->at(ic)) && (p != this) && p->fTopClient) {
                     unsigned short sid;
                     p->fResponse.GetSID(sid);
                     p->fResponse.Set(c->RefSid());
                     p->fResponse.Send(kXR_attn, kXPD_srvmsg, (char *) msg.c_str(), msg.length());
                     p->fResponse.Set(sid);
                     // Close the link, so that the associated protocol instance
                     // can be recycled
                     p->fLink->Close();
                  }
               }

               // Loop over client sessions and terminated them
               int is = 0;
               XrdProofServProxy *s = 0;
               for (is = 0; is < (int) c->ProofServs()->size(); is++) {
                  if ((s = c->ProofServs()->at(is)) && s->IsValid() &&
                     s->SrvType() == srvtype) {
                     int *pid = new int;
                     *pid = s->SrvID();
                     TRACEI(HDBG, "Admin: CleanupSessions: terminating " << *pid);
                     if (s->TerminateProofServ() != 0) {
                        if (KillProofServ(*pid, 0) != 0) {
                           XrdOucString msg = "Admin: CleanupSessions: WARNING: process ";
                           msg += *pid;
                           msg += " could not be signalled for termination";
                           TRACEI(XERR, msg.c_str());
                        } else
                           signalledpid.push_back(pid);
                     } else
                        signalledpid.push_back(pid);
                     // Reset session proxy
                     s->Reset();
                  }
               }
            }
         }

         // Now we give sometime to sessions to terminate (10 sec).
         // We check the status every second
         int nw = 10;
         int nleft = signalledpid.size();
         while (nw-- && nleft > 0) {

            // Loop over the list of processes requested to terminate
            std::list<int *>::iterator ii;
            for (ii = signalledpid.begin(); ii != signalledpid.end(); )
               if (XrdProofdProtocol::VerifyProcessByID(*(*ii)) == 0) {
                  nleft--;
                  delete (*ii);
                  ii = signalledpid.erase(ii);
               } else
                  ++ii;

            // Wait a bit before retrying
            sleep(1);
         }
      }

      // Now we cleanup what left (any zombies or super resistent processes)
      CleanupProofServ(all, usr);

      // Cleanup all possible sessions around
      fgMgr.Broadcast(type, usr, &fResponse);

      // Cleanup usr
      SafeDelArray(usr);

      // Acknowledge user
      fResponse.Send();

   } else if (type == kSessionTag) {

      //
      // Specific info about a session
      XrdProofServProxy *xps = 0;
      if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
          !(xps = fPClient->ProofServs()->at(psid))) {
         TRACEP(XERR, "Admin: session ID not found");
         fResponse.Send(kXR_InvalidRequest,"Admin: session ID not found");
         return rc;
      }

      // Set session tag
      const char *msg = (const char *) fArgp->buff;
      int   len = fRequest.header.dlen;
      if (len > kXPROOFSRVTAGMAX - 1)
         len = kXPROOFSRVTAGMAX - 1;

      // Save tag
      if (len > 0 && msg) {
         xps->SetTag(msg, len);
         TRACEI(DBG, "Admin: session tag set to: "<<xps->Tag());
      }

      // Acknowledge user
      fResponse.Send();

   } else if (type == kSessionAlias) {

      //
      // Specific info about a session
      XrdProofServProxy *xps = 0;
      if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
          !(xps = fPClient->ProofServs()->at(psid))) {
         TRACEP(XERR, "Admin: session ID not found");
         fResponse.Send(kXR_InvalidRequest,"Admin: session ID not found");
         return rc;
      }

      // Set session alias
      const char *msg = (const char *) fArgp->buff;
      int   len = fRequest.header.dlen;
      if (len > kXPROOFSRVALIASMAX - 1)
         len = kXPROOFSRVALIASMAX - 1;

      // Save tag
      if (len > 0 && msg) {
         xps->SetAlias(msg, len);
         TRACEP(DBG, "Admin: session alias set to: "<<xps->Alias());
      }

      // Acknowledge user
      fResponse.Send();

   } else if (type == kGroupProperties) {
      //
      // Specific info about a session
      XrdProofServProxy *xps = 0;
      if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
          !(xps = fPClient->ProofServs()->at(psid))) {
         TRACEP(XERR, "Admin: session ID not found");
         fResponse.Send(kXR_InvalidRequest,"Admin: session ID not found");
         return rc;
      }

      // User's group
      int   len = fRequest.header.dlen;
      char *grp = new char[len+1];
      memcpy(grp, fArgp->buff, len);
      grp[len] = 0;

      // Make sure is the current one of the user
      XrdProofGroup *g = xps->Group();
      if (g && strcmp(grp, g->Name())) {
         TRACEP(XERR, "Admin: received group does not match the user's one");
         fResponse.Send(kXR_InvalidRequest,
                      "Admin: received group does not match the user's one");
         return rc;
      }

      // Set the priority
      int priority = ntohl(fRequest.proof.int2);
      g->SetPriority(priority);

      // Make sure scheduling is ON
      fgSchedOpt = kXPD_sched_priority;
      fgOverallInflate = 1.05;

      // Notify
      TRACEP(DBG, "Admin: priority for group '"<< grp<<"' has been set to "<<priority);

      // Acknowledge user
      fResponse.Send();

   } else if (type == kGetWorkers) {

      // Find server session
      XrdProofServProxy *xps = 0;
      if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
          !(xps = fPClient->ProofServs()->at(psid))) {
         TRACEP(XERR, "Admin: session ID not found");
         fResponse.Send(kXR_InvalidRequest,"session ID not found");
         return rc;
      }

      // We should query the chosen resource provider
      XrdOucString wrks;

      if (GetWorkers(wrks, xps) !=0 ) {
         // Something wrong
         fResponse.Send(kXR_InvalidRequest,"Admin: GetWorkers failed");
         return rc;
      } else {
         // Send buffer
         char *buf = (char *) wrks.c_str();
         int len = wrks.length() + 1;
         TRACEP(DBG, "Admin: GetWorkers: sending: "<<buf);

         // Send back to user
         fResponse.Send(buf, len);
      }
   } else if (type == kQueryWorkers) {

      // Send back a list of potentially available workers
      XrdOucString sbuf(1024);
      fgProofSched->ExportInfo(sbuf);

      // Send buffer
      char *buf = (char *) sbuf.c_str();
      int len = sbuf.length() + 1;
      TRACEP(DBG, "Admin: QueryWorkers: sending: "<<buf);

      // Send back to user
      fResponse.Send(buf, len);

   } else if (type == kQueryROOTVersions) {

      // The total length first
      int len = 0;
      std::list<XrdROOT *>::iterator ip;
      for (ip = fgROOT.begin(); ip != fgROOT.end(); ++ip) {
         len += strlen((*ip)->Export());
         len += 5;
      }

      // Generic info about all known sessions
      char *buf = new char[len+2];
      char *pw = buf;
      for (ip = fgROOT.begin(); ip != fgROOT.end(); ++ip) {
         if (fPClient->ROOT() == *ip)
            memcpy(pw, "  * ", 4);
         else
            memcpy(pw, "    ", 4);
         pw += 4;
         const char *ex = (*ip)->Export();
         int lex = strlen(ex);
         memcpy(pw, ex, lex);
         pw[lex] = '\n';
         pw += (lex+1);
      }
      *pw = 0;
      TRACEP(DBG, "Admin: sending: "<<buf);

      // Send back to user
      fResponse.Send(buf,strlen(buf)+1);
      if (buf) delete[] buf;

   } else if (type == kROOTVersion) {

      // Change default ROOT version
      const char *t = (const char *) fArgp->buff;
      int len = fRequest.header.dlen;
      XrdOucString tag(t,len);

      // If a user name is given separate it out and check if
      // we can do the operation
      XrdOucString usr;
      if (tag.beginswith("u:")) {
         usr = tag;
         usr.erase(usr.rfind(' '));
         usr.replace("u:","");
         TRACEI(DBG, "Admin: ROOTVersion: request is for user: "<< usr);
         // Isolate the tag
         tag.erase(0,tag.find(' ') + 1);
      }
      TRACEP(DBG, "Admin: ROOTVersion: version tag: "<< tag);

      // If the action is requested for a user different from us we
      // must be 'superuser'
      XrdProofdClient *c = fPClient;
      XrdOucString grp;
      if (usr.length() > 0) {
         // Separate group info, if any
         if (usr.find(':') != STR_NPOS) {
            grp = usr;
            grp.erase(grp.rfind(':'));
            usr.erase(0,usr.find(':') + 1);
         } else {
            XrdProofGroup *g = fgGroupsMgr.GetUserGroup(usr.c_str());
            grp = g ? g->Name() : "default";
         }
         if (usr != fPClient->ID()) {
            if (!fSuperUser) {
               usr.insert("Admin: not allowed to change settings for usr '", 0);
               usr += "'";
               TRACEI(XERR, usr.c_str());
               fResponse.Send(kXR_InvalidRequest, usr.c_str());
               return rc;
            }
            // Lookup the list
            c = 0;
            std::list<XrdProofdClient *>::iterator i;
            for (i = fgProofdClients.begin(); i != fgProofdClients.end(); ++i) {
               if ((*i)->Match(usr.c_str(), grp.c_str())) {
                  c = (*i);
                  break;
               }
            }
            if (!c) {
               // Is this a potential user?
               XrdOucString emsg;
               XrdProofUI ui;
               if (CheckUser(usr.c_str(), ui, emsg) != 0) {
                  // No: fail
                  emsg.insert(": ", 0);
                  emsg.insert(usr, 0);
                  emsg.insert("Admin: user not found: ", 0);
                  TRACEP(XERR, emsg.c_str());
                  fResponse.Send(kXR_InvalidRequest, emsg.c_str());
                  return rc;
               } else {
                  // Yes: create an (invalid) instance of XrdProofdClient:
                  // It would be validated on the first valid login
                  c = new XrdProofdClient(usr.c_str(), (short int) -1, ui);
                  // Locate and set the group, if any
                  if (fgGroupsMgr.Num() > 0)
                     c->SetGroup(fgGroupsMgr.GetUserGroup(usr.c_str(), grp.c_str()));
                  // Add to the list
                  fgProofdClients.push_back(c);
                  TRACEP(DBG, "Admin: instance for {client, group} = {"<<usr<<", "<<
                              grp<<"} created and added to the list ("<<c<<")");
               }
            }
         }
      }

      // Search in the list
      bool ok = 0;
      std::list<XrdROOT *>::iterator ip;
      for (ip = fgROOT.begin(); ip != fgROOT.end(); ++ip) {
         if ((*ip)->MatchTag(tag.c_str())) {
            c->SetROOT(*ip);
            ok = 1;
            break;
         }
      }

      // Notify
      TRACEP(DBG, "Admin: default changed to "<<tag<<" for {client, group} = {"<<
                  usr<<", "<<grp<<"} ("<<c<<")");

      // forward down the tree, if not leaf
      if (fgMgr.SrvType() != kXPD_WorkerServer) {
         XrdOucString buf("u:");
         buf += c->ID();
         buf += " ";
         buf += tag;
         fgMgr.Broadcast(type, buf.c_str(), &fResponse);
      }

      if (ok) {
         // Acknowledge user
         fResponse.Send();
      } else {
         tag.insert("Admin: tag '", 0);
         tag += "' not found in the list of available ROOT versions";
         TRACEP(XERR, tag.c_str());
         fResponse.Send(kXR_InvalidRequest, tag.c_str());
      }
   } else {
      TRACEP(XERR, "Admin: unknown request type");
      fResponse.Send(kXR_InvalidRequest,"Admin: unknown request type");
      return rc;
   }

   // Over
   return rc;
}

//___________________________________________________________________________
int XrdProofdProtocol::Interrupt()
{
   // Handle an interrupt request

   unsigned int rc = 1;

   // Unmarshall the data
   int psid = ntohl(fRequest.interrupt.sid);
   int type = ntohl(fRequest.interrupt.type);
   TRACEI(REQ, "Interrupt: psid: "<<psid<<", type:"<<type);

   // Find server session
   XrdProofServProxy *xps = 0;
   if (!fPClient || !INRANGE(psid, fPClient->ProofServs()) ||
       !(xps = fPClient->ProofServs()->at(psid))) {
      TRACEP(XERR, "Interrupt: session ID not found");
      fResponse.Send(kXR_InvalidRequest,"nterrupt: session ID not found");
      return rc;
   }

   if (xps) {

      // Check ID matching
      if (!xps->Match(psid)) {
         fResponse.Send(kXP_InvalidRequest,"Interrupt: IDs do not match - do nothing");
         return rc;
      }

      TRACEP(DBG, "Interrupt: xps: "<<xps<<", internal link "<<xps->Link()<<
                  ", proofsrv ID: "<<xps->SrvID());

      // Propagate the type as unsolicited
      if (xps->ProofSrv()->Send(kXR_attn, kXPD_interrupt, type) != 0) {
         fResponse.Send(kXP_ServerError,
                        "Interrupt: could not propagate interrupt code to proofsrv");
         return rc;
      }

      // Notify to user
      fResponse.Send();
      TRACEP(DBG, "Interrupt: interrupt propagated to proofsrv");
   }

   // Over
   return rc;
}

//___________________________________________________________________________
int XrdProofdProtocol::Ping()
{
   // Handle a ping request

   int rc = 1;

   // Unmarshall the data
   int psid = ntohl(fRequest.sendrcv.sid);
   int opt = ntohl(fRequest.sendrcv.opt);

   TRACEI(REQ, "Ping: psid: "<<psid<<", opt: "<<opt);

   // Find server session
   XrdProofServProxy *xps = 0;
   if (!fPClient || !INRANGE(psid,fPClient->ProofServs()) ||
       !(xps = fPClient->ProofServs()->at(psid))) {
      TRACEP(XERR, "Ping: session ID not found");
      fResponse.Send(kXR_InvalidRequest,"session ID not found");
      return rc;
   }

   kXR_int32 pingres = 0;
   if (xps) {
      TRACEI(DBG, "Ping: xps: "<<xps<<", status: "<<xps->Status());

      // Type of connection
      bool external = !(opt & kXPD_internal);

      if (external) {
         TRACEI(DBG, "Ping: EXT: psid: "<<psid);

         // Send the request
         if ((pingres = (kXR_int32) xps->VerifyProofServ(fgInternalWait)) == -1) {
            TRACEP(XERR, "Ping: EXT: could not verify proofsrv");
            fResponse.Send(kXR_ServerError, "EXT: could not verify proofsrv");
            return rc;
         }

         // Notify the client
         TRACEP(DBG, "Ping: EXT: ping notified to client");
         fResponse.Send(kXR_ok, pingres);
         return rc;

      } else {
         TRACEI(DBG, "Ping: INT: psid: "<<psid);

         // If a semaphore is waiting, post it
         if (xps->PingSem())
            xps->PingSem()->Post();


         // Just notify to user
         pingres = 1;
         TRACEP(DBG, "Ping: INT: ping notified to client");
         fResponse.Send(kXR_ok, pingres);
         return rc;
      }
   }

   // Failure
   TRACEP(XERR, "Ping: session ID not found");
   fResponse.Send(kXR_ok, pingres);
   return rc;
}

//___________________________________________________________________________
int XrdProofdProtocol::SetUserEnvironment()
{
   // Set user environment: set effective user and group ID of the process
   // to the ones of the owner of this protocol instnace and change working
   // dir to the sandbox.
   // Return 0 on success, -1 if enything goes wrong.

   MTRACE(ACT, "xpd:child: ", "SetUserEnvironment: enter");

   if (XrdProofdAux::ChangeToDir(fPClient->Workdir(), fUI, fgChangeOwn) != 0) {
      MTRACE(XERR, "xpd:child: ", "SetUserEnvironment: couldn't change directory to "<<
                   fPClient->Workdir());
      return -1;
   }

   // set HOME env
   char *h = new char[8 + strlen(fPClient->Workdir())];
   sprintf(h, "HOME=%s", fPClient->Workdir());
   putenv(h);
   MTRACE(XERR, "xpd:child: ", "SetUserEnvironment: set "<<h);

   // Set access control list from /etc/initgroup
   // (super-user privileges required)
   MTRACE(DBG, "xpd:child: ", "SetUserEnvironment: setting ACLs");
   if (fgChangeOwn && (int) geteuid() != fUI.fUid) {

      XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
      if (XpdBadPGuard(pGuard, fUI.fUid)) {
         MTRACE(XERR, "xpd:child: ", "SetUserEnvironment: could not get privileges");
         return -1;
      }

      initgroups(fUI.fUser.c_str(), fUI.fGid);
   }

   if (fgChangeOwn) {
      // acquire permanently target user privileges
      MTRACE(DBG, "xpd:child: ", "SetUserEnvironment: acquire target user identity");
      if (XrdSysPriv::ChangePerm((uid_t)fUI.fUid, (gid_t)fUI.fGid) != 0) {
         MTRACE(XERR, "xpd:child: ",
                      "SetUserEnvironment: can't acquire "<< fUI.fUser <<" identity");
         return -1;
      }
   }

   // Save UNIX path in the sandbox for later cleaning
   // (it must be done after sandbox login)
   fPClient->SaveUNIXPath();

   // We are done
   MTRACE(DBG, "xpd:child: ", "SetUserEnvironment: done");
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::VerifyProcessByID(int pid, const char *pname)
{
   // Check if 'proofserv' (or a process named 'pname') process 'pid' is still
   // in the process table.
   // For {linux, sun, macosx} it uses the system info; for other systems it
   // invokes the command shell 'ps ax' via popen.
   // Return 1 if running, 0 if not running, -1 if the check could not be run.

   int rc = 0;

   TRACE(ACT, "VerifyProcessByID: enter: pid: "<<pid);

   // Check input consistency
   if (pid < 0) {
      TRACE(XERR, "VerifyProcessByID: invalid pid");
      return -1;
   }

   // Name
   const char *pn = (pname && strlen(pname) > 0) ? pname : "proofserv";

#if defined(linux)
   // Look for the relevant /proc dir
   XrdOucString fn("/proc/");
   fn += pid;
   fn += "/stat";
   FILE *ffn = fopen(fn.c_str(), "r");
   if (!ffn) {
      if (errno == ENOENT) {
         TRACE(DBG, "VerifyProcessByID: process does not exists anymore");
         return 0;
      } else {
         XrdOucString emsg("VerifyProcessByID: cannot open ");
         emsg += fn;
         emsg += ": errno: ";
         emsg += errno;
         TRACE(XERR, emsg.c_str());
         return -1;
      }
   }
   // Read status line
   char line[2048] = { 0 };
   if (fgets(line, sizeof(line), ffn)) {
      if (strstr(line, pn))
         // Still there
         rc = 1;
   } else {
      XrdOucString emsg("VerifyProcessByID: cannot read ");
      emsg += fn;
      emsg += ": errno: ";
      emsg += errno;
      TRACE(XERR, emsg.c_str());
      fclose(ffn);
      return -1;
   }
   // Close the file
   fclose(ffn);

#elif defined(__sun)

   // Look for the relevant /proc dir
   XrdOucString fn("/proc/");
   fn += pid;
   fn += "/psinfo";
   int ffd = open(fn.c_str(), O_RDONLY);
   if (ffd <= 0) {
      if (errno == ENOENT) {
         TRACE(DBG, "VerifyProcessByID: process does not exists anymore");
         return 0;
      } else {
         XrdOucString emsg("VerifyProcessByID: cannot open ");
         emsg += fn;
         emsg += ": errno: ";
         emsg += errno;
         TRACE(XERR, emsg.c_str());
         return -1;
      }
   }
   // Get the information
   psinfo_t psi;
   if (read(ffd, &psi, sizeof(psinfo_t)) != sizeof(psinfo_t)) {
      XrdOucString emsg("VerifyProcessByID: cannot read ");
      emsg += fn;
      emsg += ": errno: ";
      emsg += errno;
      TRACE(XERR, emsg.c_str());
      close(ffd);
      return -1;
   }

   // Verify now
   if (strstr(psi.pr_fname, pn))
      // The process is still there
      rc = 1;

   // Close the file
   close(ffd);

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)

   // Get the proclist
   kinfo_proc *pl = 0;
   int np;
   int ern = 0;
   if ((ern = GetMacProcList(&pl, np)) != 0) {
      XrdOucString emsg("VerifyProcessByID: cannot get the process list: errno: ");
      emsg += ern;
      TRACE(XERR, emsg.c_str());
      return -1;
   }

   // Loop over the list
   while (np--) {
      if (pl[np].kp_proc.p_pid == pid &&
          strstr(pl[np].kp_proc.p_comm, pn)) {
         // Process still exists
         rc = 1;
         break;
      }
   }
   // Cleanup
   free(pl);
#else
   // Use the output of 'ps ax' as a backup solution
   XrdOucString cmd = "ps ax | grep proofserv 2>/dev/null";
   if (pname && strlen(pname))
      cmd.replace("proofserv", pname);
   FILE *fp = popen(cmd.c_str(), "r");
   if (fp != 0) {
      char line[2048] = { 0 };
      while (fgets(line, sizeof(line), fp)) {
         if (pid == XrdProofdAux::GetLong(line)) {
            // Process still running
            rc = 1;
            break;
         }
      }
      pclose(fp);
   } else {
      // Error executing the command
      return -1;
   }
#endif
   // Done
   return rc;
}

//______________________________________________________________________________
int XrdProofdProtocol::TrimTerminatedProcesses()
{
   // Check if the terminated processed have really exited the process
   // table; return number of processes still being terminated

   int np = 0;

   // Cleanup the list of terminated or killed processes
   if (fgTerminatedProcess.size() > 0) {
      std::list<XrdProofdPInfo *>::iterator i;
      for (i = fgTerminatedProcess.begin(); i != fgTerminatedProcess.end();) {
         XrdProofdPInfo *xi = (*i);
         if (VerifyProcessByID(xi->pid, xi->pname.c_str()) == 0) {
            TRACE(HDBG,"VerifyProcessByID: freeing: "<<xi<<" ("<<xi->pid<<", "<<xi->pname<<")");
            // Cleanup the integer
            delete *i;
            // Process has terminated: remove it from the list
            i = fgTerminatedProcess.erase(i);
         } else {
            // Count
            np++;
            // Goto next
            i++;
         }
      }
   }

   // Done
   return np;
}

//______________________________________________________________________________
int XrdProofdProtocol::CleanupProofServ(bool all, const char *usr)
{
   // Cleanup (kill) all 'proofserv' processes from the process table.
   // Only the processes associated with 'usr' are killed,
   // unless 'all' is TRUE, in which case all 'proofserv' instances are
   // terminated (this requires superuser privileges).
   // Super users can also terminated all processes fo another user (specified
   // via usr).
   // Return number of process notified for termination on success, -1 otherwise

   TRACE(ACT, "CleanupProofServ: enter: all: "<<all<<
               ", usr: " << (usr ? usr : "undef"));
   int nk = 0;

   // Name
   const char *pn = "proofserv";

   // Uid
   int refuid = -1;
   if (!all) {
      if (!usr) {
         TRACE(DBG, "CleanupProofServ: usr must be defined for all = FALSE");
         return -1;
      }
      XrdProofUI ui;
      if (XrdProofdAux::GetUserInfo(usr, ui) != 0) {
         TRACE(DBG, "CleanupProofServ: problems getting info for user " << usr);
         return -1;
      }
      refuid = ui.fUid;
   }

#if defined(linux)
   // Loop over the "/proc" dir
   DIR *dir = opendir("/proc");
   if (!dir) {
      XrdOucString emsg("CleanupProofServ: cannot open /proc - errno: ");
      emsg += errno;
      TRACE(DBG, emsg.c_str());
      return -1;
   }

   struct dirent *ent = 0;
   while ((ent = readdir(dir))) {
      if (DIGIT(ent->d_name[0])) {
         XrdOucString fn("/proc/", 256);
         fn += ent->d_name;
         fn += "/status";
         // Open file
         FILE *ffn = fopen(fn.c_str(), "r");
         if (!ffn) {
            XrdOucString emsg("CleanupProofServ: cannot open file ");
            emsg += fn; emsg += " - errno: "; emsg += errno;
            TRACE(HDBG, emsg.c_str());
            continue;
         }
         // Read info
         bool xname = 1, xpid = 1, xppid = 1;
         bool xuid = (all) ? 0 : 1;
         int pid = -1;
         int ppid = -1;
         char line[2048] = { 0 };
         while (fgets(line, sizeof(line), ffn) &&
               (xname || xpid || xppid || xuid)) {
            // Check name
            if (xname && strstr(line, "Name:")) {
               if (!strstr(line, pn))
                  break;
               xname = 0;
            }
            if (xpid && strstr(line, "Pid:")) {
               pid = (int) XrdProofdAux::GetLong(&line[strlen("Pid:")]);
               xpid = 0;
            }
            if (xppid && strstr(line, "PPid:")) {
               ppid = (int) XrdProofdAux::GetLong(&line[strlen("PPid:")]);
               // Parent process must be us or be dead
               if (ppid != getpid() &&
                   XrdProofdProtocol::VerifyProcessByID(ppid, "xrootd"))
                  // Process created by another running xrootd
                  break;
               xppid = 0;
            }
            if (xuid && strstr(line, "Uid:")) {
               int uid = (int) XrdProofdAux::GetLong(&line[strlen("Uid:")]);
               if (refuid == uid)
                  xuid = 0;
            }
         }
         // Close the file
         fclose(ffn);
         // If this is a good candidate, kill it
         if (!xname && !xpid && !xppid && !xuid) {
            if (KillProofServ(pid, 1) == 0)
               nk++;
         }
      }
   }
   // Close the directory
   closedir(dir);

#elif defined(__sun)

   // Loop over the "/proc" dir
   DIR *dir = opendir("/proc");
   if (!dir) {
      XrdOucString emsg("CleanupProofServ: cannot open /proc - errno: ");
      emsg += errno;
      TRACE(DBG, emsg.c_str());
      return -1;
   }

   struct dirent *ent = 0;
   while ((ent = readdir(dir))) {
      if (DIGIT(ent->d_name[0])) {
         XrdOucString fn("/proc/", 256);
         fn += ent->d_name;
         fn += "/psinfo";
         // Open file
         int ffd = open(fn.c_str(), O_RDONLY);
         if (ffd <= 0) {
            XrdOucString emsg("CleanupProofServ: cannot open file ");
            emsg += fn; emsg += " - errno: "; emsg += errno;
            TRACE(HDBG, emsg.c_str());
            continue;
         }
         // Read info
         bool xname = 1;
         bool xuid = (all) ? 0 : 1;
         bool xppid = 1;
         // Get the information
         psinfo_t psi;
         if (read(ffd, &psi, sizeof(psinfo_t)) != sizeof(psinfo_t)) {
            XrdOucString emsg("CleanupProofServ: cannot read ");
            emsg += fn; emsg += ": errno: "; emsg += errno;
            TRACE(XERR, emsg.c_str());
            close(ffd);
            continue;
         }
         // Close the file
         close(ffd);

         // Check name
         if (xname) {
            if (!strstr(psi.pr_fname, pn))
               continue;
            xname = 0;
         }
         // Check uid, if required
         if (xuid) {
            if (refuid == psi.pr_uid)
               xuid = 0;
         }
         // Parent process must be us or be dead
         int ppid = psi.pr_ppid;
         if (ppid != getpid() &&
             XrdProofdProtocol::VerifyProcessByID(ppid, "xrootd")) {
             // Process created by another running xrootd
             continue;
             xppid = 0;
         }

         // If this is a good candidate, kill it
         if (!xname && !xppid && !xuid) {
            if (KillProofServ(psi.pr_pid, 1) == 0)
               nk++;
         }
      }
   }
   // Close the directory
   closedir(dir);

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)

   // Get the proclist
   kinfo_proc *pl = 0;
   int np;
   int ern = 0;
   if ((ern = GetMacProcList(&pl, np)) != 0) {
      XrdOucString emsg("CleanupProofServ: cannot get the process list: errno: ");
      emsg += ern;
      TRACE(XERR, emsg.c_str());
      return -1;
   }

   // Loop over the list
   int ii = np;
   while (ii--) {
      if (strstr(pl[ii].kp_proc.p_comm, pn)) {
         if (all || (int)(pl[ii].kp_eproc.e_ucred.cr_uid) == refuid) {
            // Parent process must be us or be dead
            int ppid = pl[ii].kp_eproc.e_ppid;
            bool xppid = 0;
            if (ppid != getpid()) {
               int jj = np;
               while (jj--) {
                  if (strstr(pl[jj].kp_proc.p_comm, "xrootd") &&
                      pl[jj].kp_proc.p_pid == ppid) {
                      xppid = 1;
                      break;
                  }
               }
            }
            if (!xppid)
               // Good candidate to be shot
               if (KillProofServ(pl[np].kp_proc.p_pid, 1))
                  nk++;
         }
      }
   }
   // Cleanup
   free(pl);
#else
   // For the remaining cases we use 'ps' via popen to localize the processes

   // Build command
   XrdOucString cmd = "ps ";
   bool busr = 0;
   const char *cusr = (usr && strlen(usr) && fSuperUser) ? usr : (const char *)fClientID;
   if (all) {
      cmd += "ax";
   } else {
      cmd += "-U ";
      cmd += cusr;
      cmd += " -u ";
      cmd += cusr;
      cmd += " -f";
      busr = 1;
   }
   cmd += " | grep proofserv 2>/dev/null";

   // Our parent ID as a string
   char cpid[10];
   sprintf(cpid, "%d", getpid());

   // Run it ...
   XrdOucString pids = ":";
   FILE *fp = popen(cmd.c_str(), "r");
   if (fp != 0) {
      char line[2048] = { 0 };
      while (fgets(line, sizeof(line), fp)) {
         // Parse line: make sure that we are the parent
         char *px = strstr(line, "xpd");
         if (!px)
            // Not xpd: old proofd ?
            continue;
         char *pi = strstr(px+3, cpid);
         if (!pi) {
            // Not started by us: check if the parent is still running
            pi = px + 3;
            int ppid = (int) XrdProofdAux::GetLong(pi);
            TRACE(HDBG, "CleanupProofServ: found alternative parent ID: "<< ppid);
            // If still running then skip
            if (VerifyProcessByID(ppid, "xrootd"))
               continue;
         }
         // Get pid now
         int from = 0;
         if (busr)
            from += strlen(cusr);
         int pid = (int) XrdProofdAux::GetLong(&line[from]);
         // Kill it
         if (KillProofServ(pid, 1) == 0)
            nk++;
      }
      pclose(fp);
   } else {
      // Error executing the command
      return -1;
   }
#endif

   // Done
   return nk;
}

//______________________________________________________________________________
int XrdProofdProtocol::LogTerminatedProc(int pid)
{
   // Add 'pid' to the global list of processes for which termination was
   // requested.
   // returns 0 on success, -1 in case pid <= 0 .

   if (pid > 0) {
      XrdSysMutexHelper mtxh(&fgXPDMutex);
      fgTerminatedProcess.push_back(new XrdProofdPInfo(pid, "proofserv"));
      TRACE(DBG, "LogTerminatedProc: process ID "<<pid<<
                 " signalled and pushed back");
      return 0;
   }
   return -1;
}

//______________________________________________________________________________
int XrdProofdProtocol::KillProofServ(int pid, bool forcekill)
{
   // Kill the process 'pid'.
   // A SIGTERM is sent, unless 'kill' is TRUE, in which case a SIGKILL is used.
   // If add is TRUE (default) the pid is added to the list of processes
   // requested to terminate.
   // Return 0 on success, -1 if not allowed or other errors occured.

   TRACE(ACT, "KillProofServ: enter: pid: "<<pid<< ", forcekill: "<< forcekill);

   if (pid > 0) {
      // We need the right privileges to do this
      XrdSysMutexHelper mtxh(&gSysPrivMutex);
      XrdSysPrivGuard pGuard((uid_t)0, (gid_t)0);
      if (XpdBadPGuard(pGuard, fUI.fUid) && fgChangeOwn) {
         XrdOucString msg = "KillProofServ: could not get privileges";
         TRACE(XERR, msg.c_str());
         return -1;
      } else {
         bool signalled = 1;
         if (forcekill)
            // Hard shutdown via SIGKILL
            if (kill(pid, SIGKILL) != 0) {
               if (errno != ESRCH) {
                  XrdOucString msg = "KillProofServ: kill(pid,SIGKILL) failed for process: ";
                  msg += pid;
                  msg += " - errno: ";
                  msg += errno;
                  TRACE(XERR, msg.c_str());
                  return -1;
               }
               signalled = 0;
            }
         else
            // Softer shutdown via SIGTERM
            if (kill(pid, SIGTERM) != 0) {
               if (errno != ESRCH) {
                  XrdOucString msg = "KillProofServ: kill(pid,SIGTERM) failed for process: ";
                  msg += pid;
                  msg += " - errno: ";
                  msg += errno;
                  TRACE(XERR, msg.c_str());
                  return -1;
               }
               signalled = 0;
            }
         // Add to the list of termination attempts
         if (signalled) {
            TRACE(DBG, "KillProofServ: "<<pid<<" signalled");
            if (fPClient) {
               // Record this session in the sandbox as old session
               XrdOucString tag = "-";
               tag += pid;
               if (fPClient->GuessTag(tag) == 0) {
                  if (fPClient->MvOldSession(tag.c_str()) == -1)
                     TRACE(XERR, "KillProofServ: problems recording session as old in sandbox");
               } else {
                     TRACE(DBG, "KillProofServ: problems guessing tag");
               }
            }
         } else {
            TRACE(DBG, "KillProofServ: process ID "<<pid<<" not found in the process table");
         }
      }
   } else {
      return -1;
   }

   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::ReadBuffer()
{
   // Process a readbuf request

   int rc = 1;
   XrdOucString emsg;

   // Find out the file name
   char *file = 0;
   int dlen = fRequest.header.dlen;
   if (dlen > 0 && fArgp->buff) {
      file = new char[dlen+1];
      memcpy(file, fArgp->buff, dlen);
      file[dlen] = 0;
   } else {
      emsg = "ReadBuffer: file name not not found";
      TRACEP(XERR, emsg);
      fResponse.Send(kXR_InvalidRequest, emsg.c_str());
      return rc;
   }

   // Unmarshall the data
   //
   kXR_int64 ofs = ntohll(fRequest.readbuf.ofs);
   int len = ntohl(fRequest.readbuf.len);
   TRACEI(REQ, "ReadBuffer: file: "<<file<<", ofs: "<<ofs<<", len: "<<len);

   // Check if local
   bool local = 0;
   XrdClientUrlInfo ui(file);
   if (ui.Host.length() > 0) {
      // Fully qualified name
      char *fqn = XrdNetDNS::getHostName(ui.Host.c_str());
      if (fqn && (strstr(fqn, "localhost") ||
                 !strcmp(fqn, "127.0.0.1") ||
                 !strcmp(fgMgr.Host(),fqn))) {
         memcpy(file, ui.File.c_str(), ui.File.length());
         file[ui.File.length()] = 0;
         local = 1;
         TRACEI(DBG, "ReadBuffer: file is LOCAL");
      }
      SafeFree(fqn);
   }

   // Get the buffer
   int lout = len;
   char *buf = (local) ? ReadBufferLocal(file, ofs, lout)
                       : ReadBufferRemote(file, ofs, lout);
   if (!buf) {
      emsg = "ReadBuffer: could not read buffer from ";
      emsg += (local) ? "local file " : "remote file ";
      emsg += file;
      TRACEP(XERR, emsg);
      fResponse.Send(kXR_InvalidRequest, emsg.c_str());
      return rc;
   }

   // Send back to user
   fResponse.Send(buf, lout);

   // Cleanup
   SafeFree(buf);

   // Done
   return rc;
}

//______________________________________________________________________________
char *XrdProofdProtocol::ReadBufferLocal(const char *file, kXR_int64 ofs, int &len)
{
   // Read a buffer of length 'len' at offset 'ofs' of local file 'file'; the
   // returned buffer must be freed by the caller.
   // Returns 0 in case of error.

   XrdOucString emsg;
   TRACEI(ACT, "ReadBufferLocal: file: "<<file<<", ofs: "<<ofs<<", len: "<<len);

   // Check input
   if (!file || strlen(file) <= 0) {
      TRACEI(XERR, "ReadBufferLocal: file path undefined!");
      return (char *)0;
   }

   // Open the file in read mode
   int fd = open(file, O_RDONLY);
   if (fd < 0) {
      emsg = "ReadBufferLocal: could not open ";
      emsg += file;
      TRACEI(XERR, emsg);
      return (char *)0;
   }

   // Size of the output
   struct stat st;
   if (fstat(fd, &st) != 0) {
      emsg = "ReadBufferLocal: could not get size of file with stat: errno: ";
      emsg += (int)errno;
      TRACEI(XERR, emsg);
      close(fd);
      return (char *)0;
   }
   off_t ltot = st.st_size;

   // Estimate offsets of the requested range
   // Start from ...
   kXR_int64 start = ofs;
   off_t fst = (start < 0) ? ltot + start : start;
   fst = (fst < 0) ? 0 : ((fst >= ltot) ? ltot - 1 : fst);
   // End at ...
   kXR_int64 end = fst + len;
   off_t lst = (end >= ltot) ? ltot : ((end > fst) ? end  : fst);
   TRACEI(DBG, "ReadBufferLocal: file size: "<<ltot<<
               ", read from: "<<fst<<" to "<<lst);

   // Number of bytes to be read
   len = lst - fst;

   // Output buffer
   char *buf = (char *)malloc(len + 1);
   if (!buf) {
      emsg = "ReadBufferLocal: could not allocate enough memory on the heap: errno: ";
      emsg += (int)errno;
      XPDERR(emsg);
      close(fd);
      return (char *)0;
   }

   // Reposition, if needed
   if (fst >= 0)
      lseek(fd, fst, SEEK_SET);

   int left = len;
   int pos = 0;
   int nr = 0;
   do {
      while ((nr = read(fd, buf + pos, left)) < 0 && errno == EINTR)
         errno = 0;
      TRACEI(HDBG, "ReadBufferLocal: read "<<nr<<" bytes: "<< buf);
      if (nr < 0) {
         TRACEI(XERR, "ReadBufferLocal: error reading from file: errno: "<< errno);
         break;
      }

      // Update counters
      pos += nr;
      left -= nr;

   } while (nr > 0 && left > 0);

   // Termination
   buf[len] = 0;

   // Close file
   close(fd);

   // Done
   return buf;
}

//______________________________________________________________________________
char *XrdProofdProtocol::ReadBufferRemote(const char *url,
                                          kXR_int64 ofs, int &len)
{
   // Send a read buffer request of length 'len' at offset 'ofs' for remote file
   // defined by 'url'; the returned buffer must be freed by the caller.
   // Returns 0 in case of error.

   TRACEI(ACT, "ReadBufferRemote: url: "<<(url ? url : "undef")<<
                                       ", ofs: "<<ofs<<", len: "<<len);

   // Check input
   if (!url || strlen(url) <= 0) {
      TRACEI(XERR, "ReadBufferRemote: url undefined!");
      return (char *)0;
   }

   // We try only once
   int maxtry_save = -1;
   int timewait_save = -1;
   XrdProofConn::GetRetryParam(maxtry_save, timewait_save);
   XrdProofConn::SetRetryParam(1, 1);

   // Open the connection
   XrdOucString msg = "readbuffer request from ";
   msg += fgMgr.Host();
   char m = 'A'; // log as admin
   XrdProofConn *conn = new XrdProofConn(url, m, -1, -1, 0, msg.c_str());

   char *buf = 0;
   if (conn && conn->IsValid()) {
      // Prepare request
      XPClientRequest reqhdr;
      memset(&reqhdr, 0, sizeof(reqhdr));
      conn->SetSID(reqhdr.header.streamid);
      reqhdr.header.requestid = kXP_readbuf;
      reqhdr.readbuf.ofs = ofs;
      reqhdr.readbuf.len = len;
      reqhdr.header.dlen = strlen(url);
      const void *btmp = (const void *) url;
      void **vout = (void **)&buf;
      // Send over
      XrdClientMessage *xrsp =
         conn->SendReq(&reqhdr, btmp, vout, "XrdProofdProtocol::ReadBufferRemote");

      // If positive answer
      if (xrsp && buf && (xrsp->DataLen() > 0)) {
         len = xrsp->DataLen();
      } else {
         SafeFree(buf);
      }

      // Clean the message
      SafeDelete(xrsp);

      // Close physically the connection
      conn->Close("S");
      // Delete it
      SafeDelete(conn);
   }

   // Restore original retry parameters
   XrdProofConn::SetRetryParam(maxtry_save, timewait_save);

   // Done
   return buf;
}

//__________________________________________________________________________
static int GetGroupsInfo(const char *, XrdProofGroup *g, void *s)
{
   // Check if user 'u' is memmebr of group 'grp'

   XpdGroupGlobal_t *glo = (XpdGroupGlobal_t *)s;

   if (glo) {
      if (g->Active() > 0) {
         // Set the min/max priorities
         if (glo->prmin == -1 || g->Priority() < glo->prmin)
            glo->prmin = g->Priority();
         if (glo->prmax == -1 || g->Priority() > glo->prmax)
            glo->prmax = g->Priority();
         // Set the draft fractions
         if (g->Fraction() > 0) {
            g->SetFracEff((float)(g->Fraction()));
            glo->totfrac += (float)(g->Fraction());
         } else {
            glo->nofrac += 1;
         }
      }
   } else {
      // Not enough info: stop
      return 1;
   }

   // Check next
   return 0;
}

//__________________________________________________________________________
static int SetGroupFracEff(const char *, XrdProofGroup *g, void *s)
{
   // Check if user 'u' is memmebr of group 'grp'

   XpdGroupEff_t *eff = (XpdGroupEff_t *)s;

   if (eff && eff->glo) {
      XpdGroupGlobal_t *glo = eff->glo;
      if (g->Active() > 0) {
         if (eff->opt == 0) {
            float ef = g->Priority() / (float)(glo->prmin);
            g->SetFracEff(ef);
         } else if (eff->opt == 1) {
            if (g->Fraction() < 0) {
               float ef = ((100. - glo->totfrac) / glo->nofrac);
               g->SetFracEff(ef);
            }
         } else if (eff->opt == 2) {
            if (g->FracEff() < 0) {
               // Share eff->cut (default 5%) between those with undefined fraction
               float ef = (eff->cut / glo->nofrac);
               g->SetFracEff(ef);
            } else {
               // renormalize
               float ef = g->FracEff() * eff->norm;
               g->SetFracEff(ef);
            }
         }
      }
   } else {
      // Not enough info: stop
      return 1;
   }

   // Check next
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::SetGroupEffectiveFractions()
{
   // Go through the list of active groups (those having at least a non-idle
   // member) and determine the effective resource fraction on the base of
   // the scheduling option and of priorities or nominal fractions.
   // Return 0 in case of success, -1 in case of error, 1 if every group
   // has the same priority so that the system scheduler should do the job.

   // Scheduling option
   bool opri = (fgSchedOpt == kXPD_sched_priority) ? 1 : 0;

   // Loop over groupd
   XpdGroupGlobal_t glo = {-1, -1, 0, 0.};
   fgGroupsMgr.Apply(GetGroupsInfo, &glo);

   XpdGroupEff_t eff = {0, &glo, 0.5, 1.};
   if (opri) {
      // In the priority scheme we need to enter effective fractions
      // proportional to the priority; they will get normalized later on.
      if (glo.prmin == glo.prmax) {
         // If everybody has the same priority, apply the
         // overall factor (if any) and leave the job to the system scheduler
         if (fgOverallInflate >= 1.01) {
            XrdSysMutexHelper mhp(fgMgr.Mutex());
            // Apply the factor to all the active sessions and return
            std::list<XrdProofServProxy *>::iterator svi;
            for (svi = fgMgr.GetActiveSessions()->begin();
                 svi != fgMgr.GetActiveSessions()->end(); svi++) {
               if ((*svi)->IsValid() && ((*svi)->Status() == kXPD_running)) {
                  int inflate = (int) (fgOverallInflate * 1000);
                  if ((*svi)->SetInflate(inflate,1) != 0)
                     TRACE(XERR, "SetGroupEffectiveFractions: problems setting inflate");
               }
            }
         }
         return 1;
      }

      // Set effective fractions
      fgGroupsMgr.ResetIter();
      eff.opt = 0;
      fgGroupsMgr.Apply(SetGroupFracEff, &eff);

   } else {
      // In the fraction scheme we need to fill up with the remaining resources
      // if at least one lower bound was found. And of course we need to restore
      // unitarity, if it was broken

      if (glo.totfrac < 100. && glo.nofrac > 0) {
         eff.opt = 1;
         fgGroupsMgr.Apply(SetGroupFracEff, &eff);
      } else if (glo.totfrac > 100) {
         // Leave 5% for unnamed or low priority groups
         eff.opt = 2;
         eff.norm = (glo.nofrac > 0) ? (100. - eff.cut)/glo.totfrac : 100./glo.totfrac ;
         fgGroupsMgr.Apply(SetGroupFracEff, &eff);
      }
   }

   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdProtocol::SetInflateFactors()
{
   // Recalculate inflate factors taking into account all active users
   // and their priorities. Return 0 on success, -1 otherwise.

   TRACE(INFLT,"---------------- SetInflateFactors ---------------------------");

   if (fgGroupsMgr.Num() <= 1 || fgSchedOpt == kXPD_sched_off)
      // Nothing to do
      return 0;

   // At least two active session
   int nact = fgMgr.GetActiveSessions()->size();
   if (nact <= 1) {
      // Reset inflate
      if (nact == 1) {
         XrdProofServProxy *srv = fgMgr.GetActiveSessions()->front();
         srv->SetInflate(1000,1);
      }
      // Nothing else to do
      return 0;
   }

   TRACE(INFLT,"enter: "<< fgGroupsMgr.Num()<<" groups, " <<
                           nact<<" active sessions");

   XrdSysMutexHelper mtxh(&fgXPDMutex);

   // Determine which groups are active and their effective fractions
   int rc = 0;
   if ((rc = SetGroupEffectiveFractions()) != 0) {
      if (rc == 1) {
         // If everybody has the same priority and no overall factor is applied
         // leave the job to the system scheduler
         TRACE(INFLT,"SetInflateFactors: every session has the same priority: no action ");
         return 0;
      } else {
         // Failure
         TRACE(XERR,"SetInflateFactors: failure from SetGroupEffectiveFractions");
         return -1;
      }
   }

   // Now create a list of active sessions sorted by decreasing effective fraction
   TRACE(INFLT,"--> creating a list of active sessions sorted by decreasing effective fraction ");
   float tf = 0.;
   std::list<XrdProofServProxy *>::iterator asvi, ssvi;
   std::list<XrdProofServProxy *> sorted;
   XrdSysMutexHelper mhp(fgMgr.Mutex());
   for (asvi = fgMgr.GetActiveSessions()->begin();
        asvi != fgMgr.GetActiveSessions()->end(); asvi++) {
      if ((*asvi)->IsValid() && ((*asvi)->Status() == kXPD_running)) {
         XrdProofdClient *c = (*asvi)->Parent()->fP->fPClient;
         XrdProofGroup *g = c->Group();
         TRACE(INFLT,"SetInflateFactors: group: "<<  g<<", client: "<<(*asvi)->Client());
         if (g && g->Active() > 0) {
            TRACE(INFLT,"SetInflateFactors: FracEff: "<< g->FracEff()<<" Active: "<<g->Active());
            float ef = g->FracEff() / g->Active();
            int nsrv = c->WorkerProofServ() + c->MasterProofServ();
            if (nsrv > 0) {
               ef /= nsrv;
               (*asvi)->SetFracEff(ef);
               tf += ef;
               bool pushed = 0;
               for (ssvi = sorted.begin() ; ssvi != sorted.end(); ssvi++) {
                   if (ef >= (*ssvi)->FracEff()) {
                      sorted.insert(ssvi, (*asvi));
                      pushed = 1;
                      break;
                   }
               }
               if (!pushed)
                  sorted.push_back((*asvi));
            } else {
               TRACE(XERR,"SetInflateFactors: "<<(*asvi)->Client()<<" ("<<c->ID()<<
                          "): no srv sessions for active client !!!"
                          " ===> Protocol error");
            }
         } else {
            if (g) {
               TRACE(XERR,"SetInflateFactors: "<<(*asvi)->Client()<<
                          ": inactive group for active session !!!"
                          " ===> Protocol error");
               g->Print();
            } else {
               TRACE(XERR,"SetInflateFactors: "<<(*asvi)->Client()<<
                          ": undefined group for active session !!!"
                          " ===> Protocol error");
            }
         }
      }
   }

   // Notify
   int i = 0;
   if (TRACING(INFLT) && TRACING(HDBG)) {
      for (ssvi = sorted.begin() ; ssvi != sorted.end(); ssvi++)
         XPDPRT("SetInflateFactors: "<< i++ <<" eff: "<< (*ssvi)->FracEff());
   }

   // Number of processors on this machine
   int ncpu = XrdProofdAux::GetNumCPUs();

   TRACE(INFLT,"--> calculating alpha factors");
   // Calculate alphas now
   float T = 0.;
   float *aa = new float[sorted.size()];
   int nn = sorted.size() - 1;
   i = nn;
   ssvi = sorted.end();
   while (ssvi != sorted.begin()) {
      --ssvi;
      // Normalized factor
      float f = (*ssvi)->FracEff() / tf;
      TRACE(INFLT, "    --> entry: "<< i<<" norm frac:"<< f);
      // The lowest priority gives the overall normalization
      if (i == nn) {
         aa[i] = (1. - f * (nn + 1)) / f ;
         T = (nn + 1 + aa[i]);
         TRACE(INFLT, "    --> aa["<<i<<"]: "<<aa[i]<<", T: "<<T);
      } else {
         float fr = f * T - 1.;
         float ar = 0;
         int j = 0;
         for (j = i+1 ; j < nn ; j++) {
            ar += (aa[j+1] - aa[j]) / (j+1);
         }
         aa[i] = aa[i+1] - (i+1) * (fr - ar);
         TRACE(INFLT, "    --> aa["<<i<<"]: "<<aa[i]<<", fr: "<<fr<<", ar: "<<ar);
      }

      // Round Robin scheduling to have time control
      (*ssvi)->SetSchedRoundRobin();

      // Inflate factor (taking into account the number of CPU's)
      int infl = (int)((1. + aa[i] /ncpu) * 1000 * fgOverallInflate);
      TRACE(INFLT, "    --> inflate factor for client "<<
            (*ssvi)->Client()<<" is "<<infl<<"( aa["<<i<<"]: "<<aa[i]<<")");
      (*ssvi)->SetInflate(infl, 1);
      // Go to next
      i--;
   }
   TRACE(INFLT,"------------ End of SetInflateFactors ------------------------");

   // Done
   return 0;
}
