// @(#)root/proofd:$Id$
// Author: G. Ganis Jan 2008

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_XrdProofdProofServMgr
#define ROOT_XrdProofdProofServMgr

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdProofdProofServMgr                                                  //
//                                                                      //
// Author: G. Ganis, CERN, 2008                                         //
//                                                                      //
// Class managing proofserv sessions manager.                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <list>

#ifdef OLDXRDOUC
#  include "XrdSysToOuc.h"
#  include "XrdOuc/XrdOucPthread.hh"
#else
#  include "XrdSys/XrdSysPthread.hh"
#endif

#include "XrdOuc/XrdOucHash.hh"
#include "XrdOuc/XrdOucString.hh"

#include "XrdProofdConfig.h"
#include "XrdProofdProofServ.h"

class XrdOucStream;
class XrdProtocol_Config;
class XrdProofdManager;
class XrdROOTMgr;
class XrdScheduler;
class XrdSysLogger;

class XpdClientSessions {
public:
   XrdSysRecMutex   fMutex;
   XrdProofdClient *fClient;
   std::list<XrdProofdProofServ *> fProofServs;
   XpdClientSessions(XrdProofdClient *c = 0) : fClient(c) { }
   int operator==(const XpdClientSessions c) { return (c.fClient == fClient) ? 1 : 0; }
};

class XrdProofSessionInfo {
public:
   time_t         fLastAccess;
   int            fPid;
   int            fID;
   int            fSrvType;
   int            fStatus;
   XrdOucString   fUser;
   XrdOucString   fGroup;
   XrdOucString   fUnixPath;
   XrdOucString   fTag;
   XrdOucString   fAlias;
   XrdOucString   fLogFile;
   XrdOucString   fOrdinal;
   XrdOucString   fUserEnvs;
   XrdOucString   fROOTTag;
   XrdOucString   fAdminPath;
   int            fSrvProtVers;

   XrdProofSessionInfo(XrdProofdClient *c, XrdProofdProofServ *s);
   XrdProofSessionInfo(const char *file) { ReadFromFile(file); }

   void FillProofServ(XrdProofdProofServ &s, XrdROOTMgr *rmgr);
   int ReadFromFile(const char *file);
   void Reset();
   int SaveToFile(const char *file);
};

class XrdProofdProofServMgr : public XrdProofdConfig {

   XrdProofdManager  *fMgr;
   XrdSysRecMutex     fMutex;
   XrdSysRecMutex     fRecoverMutex;
   XrdSysSemWait      fForkSem;   // To serialize fork requests
   XrdScheduler      *fSched;     // System scheduler
   XrdSysLogger      *fLogger;    // Error logger
   int                fInternalWait;   // Timeout on replies from proofsrv
   XrdOucString       fProofServEnvs;  // Additional envs to be exported before proofserv
   XrdOucString       fProofServRCs;   // Additional rcs to be passed to proofserv

   int                fShutdownOpt;    // What to do when a client disconnects
   int                fShutdownDelay;  // Delay shutdown by this (if enabled)

   XrdProofdPipe      fPipe;

   int                fCheckFrequency;
   int                fTerminationTimeOut;
   int                fVerifyTimeOut;
   int                fReconnectTime;
   int                fReconnectTimeOut;
   int                fRecoverTimeOut;
   int                fRecoverDeadline;

   int                fNextSessionsCheck; // Time of next sessions check

   XrdOucString       fActiAdminPath; // Active sessions admin area
   XrdOucString       fTermAdminPath; // Terminated sessions admin area

   XrdOucHash<XrdProofdProofServ> fSessions; // List of sessions
   std::list<XrdProofdProofServ *> fActiveSessions;     // List of active sessions (non-idle)
   std::list<XpdClientSessions *> *fRecoverClients; // List of client potentially recovering

   int                DoDirectiveProofServMgr(char *, XrdOucStream *, bool);
   int                DoDirectivePutEnv(char *, XrdOucStream *, bool);
   int                DoDirectivePutRc(char *, XrdOucStream *, bool);
   int                DoDirectiveShutdown(char *, XrdOucStream *, bool);

   int                PrepareSessionRecovering();
   int                ResolveSession(const char *fpid);

   // Session Admin path management
   int                AddSession(XrdProofdProtocol *p, XrdProofdProofServ *s);
   int                RmSession(const char *fpid);
   int                TouchSession(const char *fpid, const char *path = 0);
   int                VerifySession(const char *fpid, int to = -1, const char *path = 0);

public:
   XrdProofdProofServMgr(XrdProofdManager *mgr, XrdProtocol_Config *pi, XrdSysError *e);
   virtual ~XrdProofdProofServMgr() { }

   enum PSMProtocol { kSessionRemoval = 0, kClientDisconnect = 1, kCleanSessions = 2} ;

   XrdSysRecMutex   *Mutex() { return &fMutex; }

   int               Config(bool rcf = 0);
   int               DoDirective(XrdProofdDirective *d,
                                 char *val, XrdOucStream *cfg, bool rcf);
   void              RegisterDirectives();

   int               CheckFrequency() const { return fCheckFrequency; }
   int               InternalWait() const { return fInternalWait; }
   int               VerifyTimeOut() const { return fVerifyTimeOut; }

   inline int        NextSessionsCheck()
                        { XrdSysMutexHelper mhp(fMutex); return fNextSessionsCheck; }
   inline void       SetNextSessionsCheck(int t)
                        { XrdSysMutexHelper mhp(fMutex); fNextSessionsCheck = t; }

   bool              IsReconnecting();
   bool              IsClientRecovering(const char *usr, const char *grp, int &deadline);
   void              SetReconnectTime(bool on = 1);

   int               Process(XrdProofdProtocol *p);

   XrdProofdProofServ *Accept(XrdProofdClient *c, int to, XrdOucString &e);
   int               Attach(XrdProofdProtocol *p);
   int               Create(XrdProofdProtocol *p);
   int               Destroy(XrdProofdProtocol *p);
   int               Detach(XrdProofdProtocol *p);
   int               Recover(XpdClientSessions *cl);

   int               BroadcastPriorities();
   void              DisconnectFromProofServ(int pid);

   std::list<XrdProofdProofServ *> *ActiveSessions() { return &fActiveSessions; }
   XrdProofdProofServ *GetActiveSession(int pid);

   int               CleanupProofServ(bool all = 0, const char *usr = 0);

   void              GetTagDirs(XrdProofdProtocol *p, XrdProofdProofServ *xps,
                                XrdOucString &sesstag, XrdOucString &topsesstag,
                                XrdOucString &sessiondir, XrdOucString &sesswrkdir);

   int               SetProofServEnv(XrdProofdProtocol *p, void *in);
   int               SetProofServEnvOld(XrdProofdProtocol *p, void *in);

   int               SaveAFSkey(XrdSecCredentials *c, const char *fn);
   int               SetUserEnvironment(XrdProofdProtocol *p);

   static int        SetProofServEnv(XrdProofdManager *m, XrdROOT *r);

   inline XrdProofdPipe *Pipe() { return &fPipe; }

   // Checks run periodically by the cron job
   int               DeleteFromSessions(const char *pid);
   int               MvSession(const char *fpid);
   int               CheckActiveSessions();
   int               CheckTerminatedSessions();
   int               CleanClientSessions(const char *usr, int srvtype);
   int               RecoverActiveSessions();
};
#endif
