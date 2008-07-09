// @(#)root/proofd:$Id$
// Author: G. Ganis  Jan 2008

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
#include "XrdProofdPlatform.h"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdProofdNetMgr                                                     //
//                                                                      //
// Authors: G. Ganis, CERN, 2008                                        //
//                                                                      //
// Manages connections between PROOF server daemons                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "XrdProofdNetMgr.h"

#include "Xrd/XrdBuffer.hh"
#include "XrdClient/XrdClientConst.hh"
#include "XrdClient/XrdClientEnv.hh"
#include "XrdClient/XrdClientMessage.hh"
#include "XrdNet/XrdNetDNS.hh"
#include "XrdOuc/XrdOucStream.hh"
#include "XrdSys/XrdSysPlatform.hh"

#include "XrdProofdManager.h"
#include "XrdProofdProtocol.h"
#include "XrdProofdResponse.h"
#include "XrdProofWorker.h"

// Tracing utilities
#include "XrdProofdTrace.h"

//______________________________________________________________________________
XrdProofdNetMgr::XrdProofdNetMgr(XrdProofdManager *mgr,
                                 XrdProtocol_Config *pi, XrdSysError *e)
                : XrdProofdConfig(pi->ConfigFN, e)
{
   // Constructor

   fMgr = mgr;
   fResourceType = kRTNone;
   fPROOFcfg.fName = "";
   fPROOFcfg.fMtime = 0;
   fWorkers.clear();
   fNodes.clear();
   fNumLocalWrks = XrdProofdAux::GetNumCPUs();
   fWorkerUsrCfg = 0;
   fRequestTO = 30;

   // Configuration directives
   RegisterDirectives();
}

//__________________________________________________________________________
void XrdProofdNetMgr::RegisterDirectives()
{
   // Register config directives

   Register("adminreqto", new XrdProofdDirective("adminreqto", this, &DoDirectiveClass));
   Register("resource", new XrdProofdDirective("resource", this, &DoDirectiveClass));
   Register("worker", new XrdProofdDirective("worker", this, &DoDirectiveClass));
   Register("localwrks", new XrdProofdDirective("localwrks", (void *)&fNumLocalWrks, &DoDirectiveInt));
}

//__________________________________________________________________________
XrdProofdNetMgr::~XrdProofdNetMgr()
{
   // Destructor

   // Cleanup the worker list
   // (the nodes list points to the same object, no cleanup is needed)
   std::list<XrdProofWorker *>::iterator w = fWorkers.begin();
   while (w != fWorkers.end()) {
      delete *w;
      w = fWorkers.erase(w);
   }
}

//__________________________________________________________________________
int XrdProofdNetMgr::Config(bool rcf)
{
   // Run configuration and parse the entered config directives.
   // Return 0 on success, -1 on error
   XPDLOC(NMGR, "NetMgr::Config")

   // Cleanup the worker list
   std::list<XrdProofWorker *>::iterator w = fWorkers.begin();
   while (w != fWorkers.end()) {
      delete *w;
      w = fWorkers.erase(w);
   }
   // Create a default master line
   XrdOucString mm("master ",128);
   mm += fMgr->Host();
   fWorkers.push_back(new XrdProofWorker(mm.c_str()));

   // Run first the configurator
   if (XrdProofdConfig::Config(rcf) != 0) {
      XPDERR("problems parsing file ");
      return -1;
   }

   XrdOucString msg;
   msg = (rcf) ? "re-configuring" : "configuring";
   TRACE(ALL, msg);

   if (fMgr->SrvType() != kXPD_Worker || fMgr->SrvType() == kXPD_AnyServer) {
      TRACE(ALL, "PROOF config file: " << 
            ((fPROOFcfg.fName.length() > 0) ? fPROOFcfg.fName.c_str() : "none"));
      if (fResourceType == kRTStatic) {
         // Initialize the list of workers if a static config has been required
         // Default file path, if none specified
         if (fPROOFcfg.fName.length() <= 0) {
            CreateDefaultPROOFcfg();
         } else {
            // Load file content in memory
            if (ReadPROOFcfg() != 0) {
               XPDERR("unable to find valid information in PROOF config file "<<
                      fPROOFcfg.fName);
               fPROOFcfg.fMtime = 0;
               return 0;
            }
         }
      } else if (fResourceType == kRTNone && fWorkers.size() <= 1) {
         // Nothing defined: use default
         CreateDefaultPROOFcfg();
      }
      msg.form("%d worker nodes defined", fWorkers.size() - 1);
      TRACE(ALL, msg);

      // Find unique nodes
      FindUniqueNodes();
   }

   if (fPROOFcfg.fName.length() <= 0)
      // Enable user config files
      fWorkerUsrCfg = 1;

   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdNetMgr::DoDirective(XrdProofdDirective *d,
                                       char *val, XrdOucStream *cfg, bool rcf)
{
   // Update the priorities of the active sessions.
   XPDLOC(NMGR, "NetMgr::DoDirective")

   if (!d)
      // undefined inputs
      return -1;

   if (d->fName == "resource") {
      return DoDirectiveResource(val, cfg, rcf);
   } else if (d->fName == "adminreqto") {
      return DoDirectiveAdminReqTO(val, cfg, rcf);
   } else if (d->fName == "worker") {
      return DoDirectiveWorker(val, cfg, rcf);
   }
   TRACE(XERR,"unknown directive: "<<d->fName);
   return -1;
}

//______________________________________________________________________________
int XrdProofdNetMgr::DoDirectiveAdminReqTO(char *val, XrdOucStream *cfg, bool)
{
   // Process 'adminreqto' directive

   if (!val)
      // undefined inputs
      return -1;

   // Check deprecated 'if' directive
   if (fMgr->Host() && cfg)
      if (XrdProofdAux::CheckIf(cfg, fMgr->Host()) == 0)
         return 0;

   // Timeout on requested broadcasted to workers; there are 4 attempts,
   // so the real timeout is 4 x fRequestTO
   int to = strtol(val, 0, 10);
   fRequestTO = (to > 0) ? to : fRequestTO;
   return 0;
}

//______________________________________________________________________________
int XrdProofdNetMgr::DoDirectiveResource(char *val, XrdOucStream *cfg, bool)
{
   // Process 'resource' directive
   XPDLOC(NMGR, "NetMgr::DoDirectiveResource")

   if (!val || !cfg)
      // undefined inputs
      return -1;

   if (!strcmp("static",val)) {
      // We just take the path of the config file here; the
      // rest is used by the static scheduler
      fResourceType = kRTStatic;
      while ((val = cfg->GetToken()) && val[0]) {
         XrdOucString s(val);
         if (s.beginswith("ucfg:")) {
            fWorkerUsrCfg = s.endswith("yes") ? 1 : 0;
         } else if (s.beginswith("wmx:")) {
         } else if (s.beginswith("selopt:")) {
         } else {
            // Config file
            fPROOFcfg.fName = val;
            if (fPROOFcfg.fName.beginswith("sm:")) {
               fPROOFcfg.fName.replace("sm:","");
            }
            XrdProofdAux::Expand(fPROOFcfg.fName);
            // Make sure it exists and can be read
            if (access(fPROOFcfg.fName.c_str(), R_OK)) {
               TRACE(XERR,"configuration file cannot be read: "<<
                          fPROOFcfg.fName);
               fPROOFcfg.fName = "";
               fPROOFcfg.fMtime = 0;
            }
         }
      }
   }
   return 0;
}

//______________________________________________________________________________
int XrdProofdNetMgr::DoDirectiveWorker(char *val, XrdOucStream *cfg, bool)
{
   // Process 'worker' directive
   XPDLOC(NMGR, "NetMgr::DoDirectiveWorker")

   if (!val || !cfg)
      // undefined inputs
      return -1;

   // Get the full line (w/o heading keyword)
   cfg->RetToken();
   char *rest = 0;
   val = cfg->GetToken(&rest);
   if (val) {
      // Build the line
      XrdOucString line;
      line.form("%s %s", val, rest);
      // Parse it now
      if (!strcmp(val, "master") || !strcmp(val, "node")) {
         // Init a master instance
         XrdProofWorker *pw = new XrdProofWorker(line.c_str());
         if (pw->fHost == "localhost" ||
             pw->Matches(fMgr->Host())) {
            // Replace the default line (the first with what found in the file)
            XrdProofWorker *fw = fWorkers.front();
            fw->Reset(line.c_str());
         }
         SafeDelete(pw);
      } else {
         // Build the worker object
         XrdProofdMultiStr mline(line.c_str());
         if (mline.IsValid()) {
            TRACE(DBG, "found multi-line with: "<<mline.N()<<" tokens");
            for (int i = 0; i < mline.N(); i++) {
               TRACE(HDBG, "found token: "<<mline.Get(i));
               fWorkers.push_back(new XrdProofWorker(mline.Get(i).c_str()));
            }
         } else {
            TRACE(DBG, "Found line: "<<line);
            fWorkers.push_back(new XrdProofWorker(line.c_str()));
         }
      }
   }
   return 0;
}

//__________________________________________________________________________
int XrdProofdNetMgr::Broadcast(int type, const char *msg,
                                XrdProofdResponse *r, bool notify)
{
   // Broadcast request to known potential sub-nodes.
   // Return 0 on success, -1 on error
   XPDLOC(NMGR, "NetMgr::Broadcast")

   int rc = 0;
   TRACE(REQ, "type: "<<type);

   // Loop over unique nodes
   std::list<XrdProofWorker *>::iterator iw = fNodes.begin();
   XrdProofWorker *w = 0;
   XrdClientMessage *xrsp = 0;
   while (iw != fNodes.end()) {
      if ((w = *iw) && w->fType != 'M') {
         // Do not send it to ourselves
         bool us = (((w->fHost.find("localhost") != STR_NPOS ||
                     XrdOucString(fMgr->Host()).find(w->fHost.c_str()) != STR_NPOS)) &&
                    (w->fPort == -1 || w->fPort == fMgr->Port())) ? 1 : 0;
         if (!us) {
            // Create 'url'
            XrdOucString u = fMgr->EffectiveUser();
            u += '@';
            u += w->fHost;
            if (w->fPort != -1) {
               u += ':';
               u += w->fPort;
            }
            // Type of server
            int srvtype = (w->fType != 'W') ? (kXR_int32) kXPD_Master
                                            : (kXR_int32) kXPD_Worker;
            TRACE(HDBG, "sending request to "<<u);
            // Send request
            if (!(xrsp = Send(u.c_str(), type, msg, srvtype, r, notify))) {
               TRACE(XERR, "problems sending request to "<<u);
            }
            // Cleanup answer
            SafeDelete(xrsp);
         }
      }
      // Next worker
      iw++;
   }

   // Done
   return rc;
}

//__________________________________________________________________________
XrdProofConn *XrdProofdNetMgr::GetProofConn(const char *url)
{
   // Get a XrdProofConn for url; create a new one if not available
   XPDLOC(NMGR, "NetMgr::GetProofConn")

   XrdSysMutexHelper mhp(fMutex);

   XrdProofConn *p = 0;
   if (fProofConnHash.Num() > 0) {
      if ((p = fProofConnHash.Find(url)) && (p->IsValid())) {
         // Valid connection exists
         TRACE(DBG, "found valid connection for "<<url);
         return p;
      }
      // If the connection is invalid connection clean it up
      SafeDelete(p);
      fProofConnHash.Del(url);
   }

   // If not found create a new one
   XrdOucString buf = " Manager connection from ";
   buf += fMgr->Host();
   buf += "|ord:000";
   char m = 'A'; // log as admin

   // We try only once
   int maxtry_save = -1;
   int timewait_save = -1;
   XrdProofConn::GetRetryParam(maxtry_save, timewait_save);
   XrdProofConn::SetRetryParam(1, 1);

   // Request Timeout
   EnvPutInt(NAME_REQUESTTIMEOUT, fRequestTO);

   if ((p = new XrdProofConn(url, m, -1, -1, 0, buf.c_str()))) {
      if (p->IsValid()) {
         // Cache it
         fProofConnHash.Rep(url, p, 0, Hash_keepdata);
      } else {
         SafeDelete(p);
      }
   }

   // Restore original retry parameters
   XrdProofConn::SetRetryParam(maxtry_save, timewait_save);

   // Done
   return p;
}

//__________________________________________________________________________
XrdClientMessage *XrdProofdNetMgr::Send(const char *url, int type,
                                         const char *msg, int srvtype,
                                         XrdProofdResponse *r, bool notify)
{
   // Broadcast request to known potential sub-nodes.
   // Return 0 on success, -1 on error
   XPDLOC(NMGR, "NetMgr::Send")

   XrdClientMessage *xrsp = 0;
   TRACE(REQ, "type: "<<type);

   if (!url || strlen(url) <= 0)
      return xrsp;

   // Atomic
   XrdSysMutexHelper mhp(fMutex);

   // Get a connection to the server
   XrdProofConn *conn = GetProofConn(url);

   // For requests we try 4 times
   int maxtry_save = -1;
   int timewait_save = -1;
   XrdProofConn::GetRetryParam(maxtry_save, timewait_save);
   XrdProofConn::SetRetryParam(4, timewait_save);

   bool ok = 1;
   if (conn && conn->IsValid()) {
      XrdOucString notifymsg("Send: ");
      // Prepare request
      XPClientRequest reqhdr;
      const void *buf = 0;
      char **vout = 0;
      memset(&reqhdr, 0, sizeof(reqhdr));
      conn->SetSID(reqhdr.header.streamid);
      reqhdr.header.requestid = kXP_admin;
      reqhdr.proof.int1 = type;
      switch (type) {
         case kROOTVersion:
            notifymsg += "change-of-ROOT version request to ";
            notifymsg += url;
            notifymsg += " msg: ";
            notifymsg += msg;
            reqhdr.header.dlen = (msg) ? strlen(msg) : 0;
            buf = (msg) ? (const void *)msg : buf;
            break;
         case kCleanupSessions:
            notifymsg += "cleanup request to ";
            notifymsg += url;
            notifymsg += " for user: ";
            notifymsg += msg;
            reqhdr.proof.int2 = (kXR_int32) srvtype;
            reqhdr.proof.sid = -1;
            reqhdr.header.dlen = (msg) ? strlen(msg) : 0;
            buf = (msg) ? (const void *)msg : buf;
            break;
         default:
            ok = 0;
            TRACE(XERR, "invalid request type "<<type);
            break;
      }

      // Notify the client that we are sending the request
      if (r && notify)
         r->Send(kXR_attn, kXPD_srvmsg, 0, (char *) notifymsg.c_str(), notifymsg.length());

      // Send over
      if (ok)
         xrsp = conn->SendReq(&reqhdr, buf, vout, "NetMgr::Send");

      // Print error msg, if any
      if (r && !xrsp && conn->GetLastErr()) {
         XrdOucString cmsg = url;
         cmsg += ": ";
         cmsg += conn->GetLastErr();
         r->Send(kXR_attn, kXPD_srvmsg, (char *) cmsg.c_str(), cmsg.length());
      }

   } else {
      TRACE(XERR, "could not open connection to "<<url);
      if (r) {
         XrdOucString cmsg = "failure attempting connection to ";
         cmsg += url;
         r->Send(kXR_attn, kXPD_srvmsg, (char *) cmsg.c_str(), cmsg.length());
      }
   }

   // Restore original retry parameters
   XrdProofConn::SetRetryParam(maxtry_save, timewait_save);

   // Done
   return xrsp;
}


//______________________________________________________________________________
int XrdProofdNetMgr::ReadBuffer(XrdProofdProtocol *p)
{
   // Process a readbuf request
   XPDLOC(NMGR, "NetMgr::ReadBuffer")

   int rc = 0;
   XPD_SETRESP(p, "ReadBuffer");

   XrdOucString emsg;

   // Find out the file name
   char *url = 0;
   char *file = 0;
   int dlen = p->Request()->header.dlen;
   if (dlen > 0 && p->Argp()->buff) {
      int flen = dlen;
      int ulen = 0;
      int offs = 0;
      char *c = (char *) strstr(p->Argp()->buff, ",");
      if (c) {
         ulen = (int) (c - p->Argp()->buff);
         url = new char[ulen+1];
         memcpy(url, p->Argp()->buff, ulen);
         url[ulen] = 0;
         offs = ulen + 1;
         flen -= offs;
      }
      file = new char[flen+1];
      memcpy(file, p->Argp()->buff+offs, flen);
      file[flen] = 0;
   } else {
      emsg = "file name not not found";
      TRACEP(p, XERR, emsg);
      response->Send(kXR_InvalidRequest, emsg.c_str());
      return 0;
   }

   // Unmarshall the data
   //
   kXR_int64 ofs = ntohll(p->Request()->readbuf.ofs);
   int len = ntohl(p->Request()->readbuf.len);
   TRACEP(p, REQ, "file: "<<file<<", ofs: "<<ofs<<", len: "<<len);

   // Check if local
   bool local = 0;
   int blen = dlen;
   XrdClientUrlInfo ui(file);
   if (ui.Host.length() > 0) {
      // Fully qualified name
      char *fqn = XrdNetDNS::getHostName(ui.Host.c_str());
      if (fqn && (strstr(fqn, "localhost") ||
                 !strcmp(fqn, "127.0.0.1") ||
                 !strcmp(fMgr->Host(),fqn))) {
         memcpy(file, ui.File.c_str(), ui.File.length());
         file[ui.File.length()] = 0;
         blen = ui.File.length();
         local = 1;
         TRACEP(p, DBG, "file is LOCAL");
      }
      SafeFree(fqn);
   }

   // Get the buffer
   int lout = len;
   char *buf = 0;
   char *filen = 0;
   char *pattern = 0;
   int grep = ntohl(p->Request()->readbuf.int1);
   if (grep > 0) {
      // 'grep' operation: len is the length of the 'pattern' to be grepped
      pattern = new char[len + 1];
      int j = blen - len;
      int i = 0;
      while (j < blen)
         pattern[i++] = file[j++];
      pattern[i] = 0;
      filen = strdup(file);
      filen[blen - len] = 0;
      TRACEP(p, DBG, "grep operation "<<grep<<", pattern:"<<pattern);
   }
   if (local) {
      if (grep > 0) {
         // Grep local file
         lout = blen; // initial length
         buf = ReadBufferLocal(filen, pattern, lout, grep);
      } else {
         // Read portion of local file
         buf = ReadBufferLocal(file, ofs, lout);
      }
   } else {
      // Read portion of remote file
      buf = ReadBufferRemote(url, file, ofs, lout, grep);
   }

   if (!buf) {
      if (lout > 0) {
         if (grep > 0) {
            if (TRACING(DBG)) {
               emsg.form("nothing found by 'grep' in %s, pattern: %s", filen, pattern);
               TRACEP(p, DBG, emsg);
            }
            response->Send();
            return 0;
         } else {
            emsg.form("could not read buffer from %s %s",
                     (local) ? "local file " : "remote file ", file);
            TRACEP(p, XERR, emsg);
            response->Send(kXR_InvalidRequest, emsg.c_str());
            return 0;
         }
      } else {
         // Just got an empty buffer
         if (TRACING(DBG)) {
            emsg = "nothing found in ";
            emsg += file;
            TRACEP(p, DBG, emsg);
         }
      }
   }

   // Send back to user
   response->Send(buf, lout);

   // Cleanup
   SafeFree(buf);
   SafeDelArray(file);
   SafeFree(filen);
   SafeDelArray(pattern);

   // Done
   return 0;
}

//______________________________________________________________________________
char *XrdProofdNetMgr::ReadBufferLocal(const char *file, kXR_int64 ofs, int &len)
{
   // Read a buffer of length 'len' at offset 'ofs' of local file 'file'; the
   // returned buffer must be freed by the caller.
   // Returns 0 in case of error.
   XPDLOC(NMGR, "NetMgr::ReadBufferLocal")

   XrdOucString emsg;
   TRACE(REQ, "file: "<<file<<", ofs: "<<ofs<<", len: "<<len);

   // Check input
   if (!file || strlen(file) <= 0) {
      TRACE(XERR, "file path undefined!");
      return (char *)0;
   }

   // Open the file in read mode
   int fd = open(file, O_RDONLY);
   if (fd < 0) {
      emsg = "could not open ";
      emsg += file;
      TRACE(XERR, emsg);
      return (char *)0;
   }

   // Size of the output
   struct stat st;
   if (fstat(fd, &st) != 0) {
      emsg = "could not get size of file with stat: errno: ";
      emsg += (int)errno;
      TRACE(XERR, emsg);
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
   off_t lst = (end >= ltot) ? ltot : ((end > fst) ? end  : ltot);
   TRACE(DBG, "file size: "<<ltot<<", read from: "<<fst<<" to "<<lst);

   // Number of bytes to be read
   len = lst - fst;

   // Output buffer
   char *buf = (char *)malloc(len + 1);
   if (!buf) {
      emsg = "could not allocate enough memory on the heap: errno: ";
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
      if (nr < 0) {
         TRACE(XERR, "error reading from file: errno: "<< errno);
         break;
      }

      // Update counters
      pos += nr;
      left -= nr;

   } while (nr > 0 && left > 0);

   // Termination
   buf[len] = 0;
   TRACE(HDBG, "read "<<nr<<" bytes: "<< buf);

   // Close file
   close(fd);

   // Done
   return buf;
}

//______________________________________________________________________________
char *XrdProofdNetMgr::ReadBufferLocal(const char *file,
                                         const char *pat, int &len, int opt)
{
   // Grep lines matching 'pat' form 'file'; the returned buffer (length in 'len')
   // must be freed by the caller.
   // Returns 0 in case of error.
   XPDLOC(NMGR, "NetMgr::ReadBufferLocal")

   XrdOucString emsg;
   TRACE(REQ, "file: "<<file<<", pat: "<<pat<<", len: "<<len);

   // Check input
   if (!file || strlen(file) <= 0) {
      TRACE(XERR, "file path undefined!");
      return (char *)0;
   }

   // Size of the output
   struct stat st;
   if (stat(file, &st) != 0) {
      emsg = "could not get size of file with stat: errno: ";
      emsg += (int)errno;
      TRACE(XERR, emsg);
      return (char *)0;
   }
   off_t ltot = st.st_size;

   // Open the file in read mode
   FILE *fp = fopen(file, "r");
   if (!fp) {
      emsg = "could not open ";
      emsg += file;
      TRACE(XERR, emsg);
      return (char *)0;
   }

   // Check pattern
   bool keepall = (pat && strlen(pat) > 0) ? 0 : 1; 

   // Fill option
   bool keep = 1;
   if (opt == 2) {
      // '-v' functionality
      keep = 0;
   } else if (opt != 1 ) {
      emsg = "unknown option: ";
      emsg += opt;
      TRACE(XERR, emsg);
      return (char *)0;
   }

   // Read line by line
   len = 0;
   char *buf = 0;
   char line[2048];
   int bufsiz = 0, left = 0, lines = 0;
   while ((ltot > 0) && fgets(line, sizeof(line), fp)) {
      int llen = strlen(line);
      ltot -= llen;
      // Filter out
      bool haspattern = (strstr(line, pat)) ? 1 : 0;
      if (!keepall && ((keep && !haspattern) || (!keep && haspattern)))
         // Skip
         continue;
      // Good line
      lines++;
      // (Re-)allocate the buffer
      if (!buf || (llen > left)) {
         int dsiz = 100 * ((int) ((len + llen) / lines) + 1);
         dsiz = (dsiz > llen) ? dsiz : llen;
         bufsiz += dsiz;
         buf = (char *)realloc(buf, bufsiz + 1);
         left += dsiz;
      }
      if (!buf) {
         emsg = "could not allocate enough memory on the heap: errno: ";
         emsg += (int)errno;
         XPDERR(emsg);
         fclose(fp);
         return (char *)0;
      }
      // Add line to the buffer
      memcpy(buf+len, line, llen);
      len += llen;
      left -= llen;
      if (TRACING(HDBG))
         fprintf(stderr, "line: %s", line);
   }

   // Check the result and terminate the buffer
   if (buf) {
      if (len > 0) {
         buf[len] = 0;
      } else {
         free(buf);
         buf = 0;
      }
   }

   // Close file
   fclose(fp);

   // Done
   return buf;
}

//______________________________________________________________________________
char *XrdProofdNetMgr::ReadBufferRemote(const char *url, const char *file,
                                          kXR_int64 ofs, int &len, int grep)
{
   // Send a read buffer request of length 'len' at offset 'ofs' for remote file
   // defined by 'url'; the returned buffer must be freed by the caller.
   // Returns 0 in case of error.
   XPDLOC(NMGR, "NetMgr::ReadBufferRemote")

   TRACE(REQ, "url: "<<(url ? url : "undef")<<
               ", file: "<<(file ? file : "undef")<<", ofs: "<<ofs<<
               ", len: "<<len<<", grep: "<<grep);

   // Check input
   if (!file || strlen(file) <= 0) {
      TRACE(XERR, "file undefined!");
      return (char *)0;
   }
   if (!url || strlen(url) <= 0) {
      // Use file as url
      url = file;
   }

   // We log in as the effective user to minimize the number of connections to the
   // other servers
   XrdClientUrlInfo u(url);
   u.User = fMgr->EffectiveUser();
   XrdProofConn *conn = GetProofConn(u.GetUrl().c_str());

   char *buf = 0;
   if (conn && conn->IsValid()) {
      // Prepare request
      XPClientRequest reqhdr;
      memset(&reqhdr, 0, sizeof(reqhdr));
      conn->SetSID(reqhdr.header.streamid);
      reqhdr.header.requestid = kXP_readbuf;
      reqhdr.readbuf.ofs = ofs;
      reqhdr.readbuf.len = len;
      reqhdr.readbuf.int1 = grep;
      reqhdr.header.dlen = strlen(file);
      const void *btmp = (const void *) file;
      char **vout = &buf;
      // Send over
      XrdClientMessage *xrsp =
         conn->SendReq(&reqhdr, btmp, vout, "NetMgr::ReadBufferRemote");

      // If positive answer
      if (xrsp && buf && (xrsp->DataLen() > 0)) {
         len = xrsp->DataLen();
      } else {
         if (xrsp && !(xrsp->IsError()))
            // The buffer was just empty: do not call it error
            len = 0;
         SafeFree(buf);
      }

      // Clean the message
      SafeDelete(xrsp);
   }

   // Done
   return buf;
}

//______________________________________________________________________________
char *XrdProofdNetMgr::ReadLogPaths(const char *url, const char *msg, int isess)
{
   // Get log paths from next tier; used in multi-master setups
   // Returns 0 in case of error.
   XPDLOC(NMGR, "NetMgr::ReadLogPaths")

   TRACE(REQ, "url: "<<(url ? url : "undef")<<
              ", msg: "<<(msg ? msg : "undef")<<", isess: "<<isess);

   // Check input
   if (!url || strlen(url) <= 0) {
      TRACE(XERR, "url undefined!");
      return (char *)0;
   }

   // We log in as the effective user to minimize the number of connections to the
   // other servers
   XrdClientUrlInfo u(url);
   u.User = fMgr->EffectiveUser();
   XrdProofConn *conn = GetProofConn(u.GetUrl().c_str());

   char *buf = 0;
   if (conn && conn->IsValid()) {
      // Prepare request
      XPClientRequest reqhdr;
      memset(&reqhdr, 0, sizeof(reqhdr));
      conn->SetSID(reqhdr.header.streamid);
      reqhdr.header.requestid = kXP_admin;
      reqhdr.proof.int1 = kQueryLogPaths;
      reqhdr.proof.int2 = isess;
      reqhdr.proof.sid = -1;
      reqhdr.header.dlen = strlen(msg);
      const void *btmp = (const void *) msg;
      char **vout = &buf;
      // Send over
      XrdClientMessage *xrsp =
         conn->SendReq(&reqhdr, btmp, vout, "NetMgr::ReadLogPaths");

      // If positive answer
      if (xrsp && buf && (xrsp->DataLen() > 0)) {
         int len = xrsp->DataLen();
         buf = (char *) realloc((void *)buf, len+1);
         if (buf)
            buf[len] = 0;
      } else {
         SafeFree(buf);
      }

      // Clean the message
      SafeDelete(xrsp);
   }

   // Done
   return buf;
}

//__________________________________________________________________________
void XrdProofdNetMgr::CreateDefaultPROOFcfg()
{
   // Fill-in fWorkers for a localhost based on the number of
   // workers fNumLocalWrks.
   XPDLOC(NMGR, "NetMgr::CreateDefaultPROOFcfg")

   TRACE(DBG, "enter: local workers: "<< fNumLocalWrks);

   XrdOucString mm;

   // Create 'localhost' lines for each worker
   int nwrk = fNumLocalWrks;
   if (nwrk > 0) {
      mm = "worker localhost port=";
      mm += fMgr->Port();
      while (nwrk--) {
         fWorkers.push_back(new XrdProofWorker(mm.c_str()));
         TRACE(DBG, "added line: " << mm);
      }
   }

   TRACE(DBG, "done: "<<fWorkers.size()-1<<" workers");

   // We are done
   return;
}

//__________________________________________________________________________
std::list<XrdProofWorker *> *XrdProofdNetMgr::GetActiveWorkers()
{
   // Return the list of workers after having made sure that the info is
   // up-to-date
   XPDLOC(NMGR, "NetMgr::GetActiveWorkers")

   XrdSysMutexHelper mhp(fMutex);

   if (fResourceType == kRTStatic && fPROOFcfg.fName.length() > 0) {
      // Check if there were any changes in the config file
      if (ReadPROOFcfg(1) != 0) {
         TRACE(XERR, "unable to read the configuration file");
         return (std::list<XrdProofWorker *> *)0;
      }
   }
   TRACE(DBG,  "returning list with "<<fWorkers.size()<<" entries");

   return &fWorkers;
}

//__________________________________________________________________________
std::list<XrdProofWorker *> *XrdProofdNetMgr::GetNodes()
{
   // Return the list of unique nodes after having made sure that the info is
   // up-to-date
   XPDLOC(NMGR, "NetMgr::GetNodes")

   XrdSysMutexHelper mhp(fMutex);

   if (fResourceType == kRTStatic && fPROOFcfg.fName.length() > 0) {
      // Check if there were any changes in the config file
      if (ReadPROOFcfg(1) != 0) {
         TRACE(XERR, "unable to read the configuration file");
         return (std::list<XrdProofWorker *> *)0;
      }
   }
   TRACE(DBG, "returning list with "<<fNodes.size()<<" entries");

   return &fNodes;
}

//__________________________________________________________________________
int XrdProofdNetMgr::ReadPROOFcfg(bool reset)
{
   // Read PROOF config file and load the information in fWorkers.
   // NB: 'master' information here is ignored, because it is passed
   //     via the 'xpd.workdir' and 'xpd.image' config directives
   XPDLOC(NMGR, "NetMgr::ReadPROOFcfg")

   TRACE(REQ, "saved time of last modification: " <<fPROOFcfg.fMtime);

   // Check inputs
   if (fPROOFcfg.fName.length() <= 0)
      return -1;

   // Get the modification time
   struct stat st;
   if (stat(fPROOFcfg.fName.c_str(), &st) != 0)
      return -1;
   TRACE(DBG, "time of last modification: " << st.st_mtime);

   // File should be loaded only once
   if (st.st_mtime <= fPROOFcfg.fMtime)
      return 0;

   // Save the modification time
   fPROOFcfg.fMtime = st.st_mtime;

   if (reset) {
      // Cleanup the worker list
      std::list<XrdProofWorker *>::iterator w = fWorkers.begin();
      while (w != fWorkers.end()) {
         delete *w;
         w = fWorkers.erase(w);
      }
   }

   // Open the defined path.
   FILE *fin = 0;
   if (!(fin = fopen(fPROOFcfg.fName.c_str(), "r")))
      return -1;

   if (reset) {
      // Create a default master line
      XrdOucString mm("master ",128);
      mm += fMgr->Host();
      fWorkers.push_back(new XrdProofWorker(mm.c_str()));
   }

   // Read now the directives
   int nw = 1;
   char lin[2048];
   while (fgets(lin,sizeof(lin),fin)) {
      // Skip empty lines
      int p = 0;
      while (lin[p++] == ' ') { ; } p--;
      if (lin[p] == '\0' || lin[p] == '\n')
         continue;

      // Skip comments
      if (lin[0] == '#')
         continue;

      // Remove trailing '\n';
      if (lin[strlen(lin)-1] == '\n')
         lin[strlen(lin)-1] = '\0';

      TRACE(DBG, "found line: " << lin);

      const char *pfx[2] = { "master", "node" };
      if (!strncmp(lin, pfx[0], strlen(pfx[0])) ||
          !strncmp(lin, pfx[1], strlen(pfx[1]))) {
         // Init a master instance
         XrdProofWorker *pw = new XrdProofWorker(lin);
         if (pw->fHost == "localhost" ||
             pw->Matches(fMgr->Host())) {
            // Replace the default line (the first with what found in the file)
            XrdProofWorker *fw = fWorkers.front();
            fw->Reset(lin);
         }
         SafeDelete(pw);
      } else {
         // Build the worker object
         fWorkers.push_back(new XrdProofWorker(lin));
         nw++;
      }
   }

   // Close files
   fclose(fin);

   // Find unique nodes
   if (reset)
      FindUniqueNodes();

   // We are done
   return ((nw == 0) ? -1 : 0);
}

//__________________________________________________________________________
int XrdProofdNetMgr::FindUniqueNodes()
{
   // Scan fWorkers for unique nodes (stored in fNodes).
   // Return the number of unque nodes.
   // NB: 'master' information here is ignored, because it is passed
   //     via the 'xpd.workdir' and 'xpd.image' config directives
   XPDLOC(NMGR, "NetMgr::FindUniqueNodes")

   TRACE(REQ, "# workers: " << fWorkers.size());

   // Cleanup the nodes list
   fNodes.clear();

   // Build the list of unique nodes (skip the master line);
   if (fWorkers.size() > 0) {
      std::list<XrdProofWorker *>::iterator w = fWorkers.begin();
      w++;
      for ( ; w != fWorkers.end(); w++) {
         bool add = 1;
         std::list<XrdProofWorker *>::iterator n;
         for (n = fNodes.begin() ; n != fNodes.end(); n++) {
            if ((*n)->Matches(*w)) {
               add = 0;
               break;
            }
         }
         if (add)
            fNodes.push_back(*w);
      }
   }
   TRACE(DBG, "found " << fNodes.size() <<" unique nodes");

   // We are done
   return fNodes.size();
}
