// @(#)root/proofd:$Id$
// Author: G. Ganis  June 2005

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_XrdProofdProtocol
#define ROOT_XrdProofdProtocol

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdProofdProtocol                                                    //
//                                                                      //
// Authors: G. Ganis, CERN, 2005                                        //
//                                                                      //
// XrdProtocol implementation to coordinate 'proofserv' applications.   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <vector>

// Version index: start from 1001 (0x3E9) to distinguish from 'proofd'
// To be increment when non-backward compatible changes are introduced
//  1001 (0x3E9) -> 1002 (0x3EA) : support for flexible env setting
//  1002 (0x3EA) -> 1003 (0x3EB) : many new features
//  1003 (0x3EB) -> 1004 (0x3EC) : restructuring
//  1004 (0x3EC) -> 1005 (0x3ED) : deeper restructuring
#define XPROOFD_VERSBIN 0x000003ED
#define XPROOFD_VERSION "0.5"

#ifdef OLDXRDOUC
#  include "XrdSysToOuc.h"
#  include "XrdOuc/XrdOucPthread.hh"
#else
#  include "XrdSys/XrdSysPthread.hh"
#endif
#include "Xrd/XrdLink.hh"
#include "Xrd/XrdObject.hh"
#include "Xrd/XrdProtocol.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdSec/XrdSecInterface.hh"
#include "XProofProtocol.h"

class XrdBuffer;
class XrdProofdClient;
class XrdProofdManager;
class XrdProofdResponse;
class XrdProofdProofServ;
class XrdSrvBuffer;

class XrdProofdProtocol : XrdProtocol {

public:
   XrdProofdProtocol();
   virtual ~XrdProofdProtocol() {} // Never gets destroyed

   void          DoIt() {}
   XrdProtocol  *Match(XrdLink *lp);
   int           Process(XrdLink *lp);
   void          Recycle(XrdLink *lp, int x, const char *y);
   int           Stats(char *buff, int blen, int do_sync);

   static int    Configure(char *parms, XrdProtocol_Config *pi);

   // Getters
   inline kXR_int32 CID() const { return fCID; }
   inline XrdProofdClient *Client() const { return fPClient; }
   inline int    ConnType() const { return fConnType; }
   inline const char *TraceID() const { return fTraceID.c_str(); }
   inline bool   Internal() { return (fConnType == kXPD_Internal) ? 1 : 0; }
   inline int    Pid() const { return fPid; }
   inline char   Status() const { return fStatus; }
   inline short int ProofProtocol() const { return fProofProtocol; }
   inline bool   SuperUser() const { return fSuperUser; }

   XrdProofdResponse *Response(kXR_unt16 rid);
   inline XPClientRequest *Request() const { return (XPClientRequest *)&fRequest; }
   inline XrdBuffer *Argp() const { return fArgp; }
   inline XrdLink *Link() const { return fLink; }
   inline XrdSecProtocol *AuthProt() const { return fAuthProt; }

   // Setters
   inline void   SetAdminPath(const char *p) { XrdSysMutexHelper mhp(fMutex); fAdminPath = p; }
   inline void   SetAuthEntity(XrdSecEntity *se = 0) { fSecEntity.tident = fLink->ID;
                                                       fSecClient = (se) ? se : &fSecEntity; }
   inline void   SetAuthProt(XrdSecProtocol *p) { fAuthProt = p; }
   inline void   SetClient(XrdProofdClient *c) { fPClient = c; }
   inline void   SetClntCapVer(unsigned char c) { fClntCapVer = c; }
   inline void   SetCID(kXR_int32 cid) { fCID = cid; }
   inline void   SetConnType(int ct) { fConnType = ct; }
   inline void   SetTraceID() { if (fLink) fTraceID.form("%s: ", fLink->ID); }
   inline void   SetPid(int pid) { fPid = pid; }
   inline void   SetProofProtocol(short int pp) { fProofProtocol = pp; }
   inline void   SetStatus(char s) { fStatus = s; }
   inline void   SetSuperUser(bool su = 1) { fSuperUser = su; }

   static XrdProofdManager *Mgr() { return fgMgr; }
   static int    EUidAtStartup() { return fgEUidAtStartup; }

 private:

   XrdBuffer    *GetBuff(int quantum, XrdBuffer *argp = 0);
   int           GetData(const char *dtype, char *buff, int blen);
   XrdProofdResponse *GetNewResponse(kXR_unt16 rid);
   int           Interrupt();
   int           Ping();
   int           Process2();
   void          Reset();
   int           SendData(XrdProofdProofServ *xps, kXR_int32 sid = -1, XrdSrvBuffer **buf = 0);
   int           SendDataN(XrdProofdProofServ *xps, XrdSrvBuffer **buf = 0);
   int           SendMsg();
   void          TouchAdminPath();
   int           Urgent();

   //
   // Protocol variables

   XrdObject<XrdProofdProtocol>  fProtLink;
   XrdBuffer                    *fArgp;

   XrdLink                      *fLink;
   int                           fPid;             // Remote ID of the connected process

   char                          fStatus;

   unsigned char                 fClntCapVer;
   short int                     fProofProtocol;   // PROOF protocol version run by client

   bool                          fSuperUser;       // TRUE for privileged clients (admins)

   XrdProofdClient              *fPClient;         // Our reference XrdProofdClient
   XrdOucString                  fAdminPath;       // Admin path for this client

   XrdOucString                  fTraceID;          // Tracing ID

   XrdSecEntity                 *fSecClient;
   XrdSecProtocol               *fAuthProt;
   XrdSecEntity                  fSecEntity;

   kXR_int32                     fConnType;        // Type of connection: Clnt-Mst, Mst-Mst, Mst-Wrk

   kXR_int32                     fCID;             // Reference ID of this client

   XrdSysRecMutex                fMutex;    // Local mutex

   //
   // These depend on the logical connection
   XPClientRequest               fRequest;  // handle client requests
   std::vector<XrdProofdResponse *> fResponses; // One per each logical connection

   //
   // Static area: general protocol managing section
   //
   static bool                   fgConfigDone;
   static int                    fgCount;
   static XrdObjectQ<XrdProofdProtocol> fgProtStack;
   static XrdBuffManager        *fgBPool;        // Buffer manager
   static int                    fgMaxBuffsz;    // Maximum buffer size we can have
   static XrdSysRecMutex         fgBMutex;       // Buffer management mutex

   static XrdSysError            fgEDest;     // Error message handler
   static XrdSysLogger          *fgLogger;    // Error logger

   static int                    fgEUidAtStartup; // Effective uid at startup

   //
   // Static area: protocol configuration section
   static int                    fgReadWait;
   static XrdProofdManager      *fgMgr;       // Cluster manager

   static void                   PostSession(int on, const char *u, const char *g, int pid);
};

#define XPD_SETRESP(p, x) \
   kXR_unt16 rid; \
   memcpy((void *)&rid, (const void *)&(p->Request()->header.streamid[0]), 2); \
   XrdProofdResponse *response = p->Response(rid); \
   if (!response) { \
      TRACEP(p, XERR, x << ": could not get Response instance for requid:"<< rid); \
      return rc; \
   }

#define XPD_SETRESPV(p, x) \
   kXR_unt16 rid; \
   memcpy((void *)&rid, (const void *)&(p->Request()->header.streamid[0]), 2); \
   XrdProofdResponse *response = p->Response(rid); \
   if (!response) { \
      TRACEP(p, XERR, x << ": could not get Response instance for requid:"<< rid); \
      return; \
   }

#endif
