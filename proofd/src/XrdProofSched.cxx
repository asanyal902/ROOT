// @(#)root/proofd:$Id$
// Author: G. Ganis  September 2007

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdProofSched                                                        //
//                                                                      //
// Authors: G. Ganis, CERN, 2007                                        //
//                                                                      //
// Interface for a PROOF scheduler.                                     //
// Alternative scheduler implementations should be provided as shared   //
// library containing an implementation of this class. The library      //
// must also provide a function to load an instance of this class       //
// with the following signature (see commented example below):          //
// extern "C" {                                                         //
//    XrdProofSched *XrdgetProofSched(const char *cfg,                  //
//                                    XrdProofdManager *mgr,            //
//                                    XrdProofGroupMgr *grpmgr,         //
//                                    XrdSysError *edest);              //
// }                                                                    //
// Here 'cfg' is the xrootd config file where directives to configure   //
// the scheduler are specified, 'mgr' is the instance of the cluster    //
// manager from where the scheduler can get info about the available    //
// workers and their status, 'grpmgr' is the instance of the group      //
// bringing the definition of the groups for this run, and 'edest' is   //
// instance of the error logger to be used.                             //
// The scheduler is identified by a name of max 16 chars.               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <list>

#include "XProofProtocol.h"
#include "XrdProofSched.h"
#include "XrdProofdManager.h"
#include "XrdProofWorker.h"
#include "XrdProofServProxy.h"
#include "XrdProofGroup.h"

#include "XrdOuc/XrdOucString.hh"
#include "XrdOuc/XrdOucStream.hh"
#ifdef OLDXRDOUC
#  include "XrdSysToOuc.h"
#  include "XrdOuc/XrdOucError.hh"
#else
#  include "XrdSys/XrdSysError.hh"
#endif

// Tracing
#include "XrdProofdTrace.h"
static const char *gTraceID = " ";
extern XrdOucTrace *XrdProofdTrace;
#define TRACEID gTraceID

//
// Example of scheduler loader for an implementation called XrdProofSchedDyn
//
// extern "C" {
// //______________________________________________________________________________
// XrdProofSched *XrdgetProofSched(const char *cfg, XrdProofdManager *mgr,
//                                 XrdProofGroupMgr *grpmgr, XrdSysError *edest)
// {
//   // This scheduler is meant to live in a shared library. The interface below is
//   // used by the server to obtain a copy of the scheduler object.
//
//   XrdProofSchedDyn *pss = new XrdProofSchedDyn(mgr, grpmgr, edest);
//   if (pss && pss->Config(cfg) == 0) {
//      return (XrdProofSched *) pss;
//   }
//   if (pss)
//      delete pss;
//   return (XrdProofSched *)0;
// }}

//__________________________________________________________________________
static bool XpdWrkComp(XrdProofWorker *&lhs, XrdProofWorker *&rhs)
{
   // COmpare two workers for sorting

   return ((lhs && rhs &&
            lhs->GetNActiveSessions() < rhs->GetNActiveSessions()) ? 1 : 0);
}

//______________________________________________________________________________
XrdProofSched::XrdProofSched(const char *name,
                             XrdProofdManager *mgr, XrdProofGroupMgr *grpmgr,
                             const char *cfn, XrdSysError *e)
{
   // Constructor

   fValid = 1;
   fMgr = mgr;
   fGrpMgr = grpmgr;
   fNextWrk = 1;
   fEDest = e;
   ResetParameters();

   memset(fName, 0, kXPSMXNMLEN);
   if (name)
      memcpy(fName, name, kXPSMXNMLEN-1);

   // Read config file, if required
   if (cfn && strlen(cfn) > 0)
      if (Config(cfn) != 0)
         fValid = 0;
}

//______________________________________________________________________________
void XrdProofSched::ResetParameters()
{
   // Reset values for the configurable parameters

   fMaxSessions = -1;
   fWorkerMax = -1;
   fWorkerSel = kSSORoundRobin;
   fOptWrksPerUnit = 2;
   fMinForQuery = 2;
   fNodesFraction = 0.5;
}

//_________________________________________________________________________________
int XrdProofSched::Config(const char *cfg)
{
   // Configure this instance using the content of file 'cfg'.
   // Return 0 on success, -1 in case of failure (file does not exists
   // or containing incoherent information).

   int rc = 0;

   // Nothing to do if no file
   if (!cfg || strlen(cfg) <= 0)
      return rc;

   XrdOucStream config(fEDest, getenv("XRDINSTANCE"));

   // Open and attach the config file
   int cfgFD = 0;
   if ((cfgFD = open(cfg, O_RDONLY, 0)) < 0) {
      XrdOucString msg("XrdProofSched::Config: error open config file: ");
      msg += fMaxSessions;
      TRACE(XERR, msg.c_str());
      return -1;
   }
   config.Attach(cfgFD);

   // Process items
   char *var = 0;
   char *val = 0;
   while ((var = config.GetMyFirstWord())) {
      if (!(strncmp("xpd.schedparam", var, 14))) {
         var += 14;
         // Get the parameters
         while ((val = config.GetToken()) && val[0]) {
            XrdOucString s(val);
            if (s.beginswith("wmx:")) {
               s.replace("wmx:","");
               fWorkerMax = strtol(s.c_str(), (char **)0, 10);
            } else if (s.beginswith("mxsess:")) {
               s.replace("mxsess:","");
               fMaxSessions = strtol(s.c_str(), (char **)0, 10);
            } else if (s.beginswith("selopt:")) {
               if (s.endswith("random"))
                  fWorkerSel = kSSORandom;
               else if (s.endswith("load"))
                  fWorkerSel = kSSOLoadBased;
               else
                  fWorkerSel = kSSORoundRobin;
            } else if (s.beginswith("fraction:")) {
               s.replace("fraction:","");
               fNodesFraction = strtod(s.c_str(), (char **)0);
            } else if (s.beginswith("optnwrks:")) {
               s.replace("optnwrks:","");
               fOptWrksPerUnit = strtol(s.c_str(), (char **)0, 10);
            } else if (s.beginswith("minforquery:")) {
               s.replace("minforquery:","");
               fMinForQuery = strtol(s.c_str(), (char **)0, 10);
            } else if (strncmp(val, "default", 7)) {
               // This line applies to another scheduler
               ResetParameters();
               break;
            }
         }
      } else if (!(strncmp("xpd.resource", var, 12))) {
         // For backward compatibility
         var += 12;
         // Get the scheduler name
         if (!(val = config.GetToken()) || !val[0])
            continue;
         if (strncmp(val, "static", 6) && strncmp(val, "default", 7))
            continue;
         // Get the values
         while ((val = config.GetToken()) && val[0]) {
            XrdOucString s(val);
            if (s.beginswith("wmx:")) {
               s.replace("wmx:","");
               fWorkerMax = strtol(s.c_str(), (char **)0, 10);
            } else if (s.beginswith("mxsess:")) {
               s.replace("mxsess:","");
               fMaxSessions = strtol(s.c_str(), (char **)0, 10);
            } else if (s.beginswith("selopt:")) {
               if (s.endswith("random"))
                  fWorkerSel = kSSORandom;
               else
                  fWorkerSel = kSSORoundRobin;
            }
         }
      }
   }

   // Notify
   XrdOucString msg("XrdProofSched::Config: maxsess: ") ; msg += fMaxSessions;
   msg += ", maxwrks: " ; msg += fWorkerMax;
   msg += ", selopt: " ; msg += fWorkerSel;
   TRACE(DBG, msg.c_str());

   // Done
   return rc;
}

//______________________________________________________________________________
int XrdProofSched::GetNumWorkers(XrdProofServProxy *xps)
{
   // Calculate the number of workers to be used given the state of the cluster

   // Go through the list of hosts and see how many CPUs are not used.
   int nFreeCPUs = 0;
   std::list<XrdProofWorker *> *wrks = fMgr->GetActiveWorkers();
   std::list<XrdProofWorker *>::iterator iter;
   for (iter = wrks->begin(); iter != wrks->end(); ++iter) {
      TRACE(DBG, "GetNumWorkers: "<< (*iter)->fImage<<
                 " : # act: "<<(*iter)->GetNActiveSessions());
      if ((*iter)->fType != 'M'
         && (*iter)->GetNActiveSessions() < fOptWrksPerUnit)
         nFreeCPUs++;
   }

   float priority = 1;
   if (xps->Group()) {
      std::list<XrdProofServProxy *> *sessions = fMgr->GetActiveSessions();
      std::list<XrdProofServProxy *>::iterator sesIter;
      float summedPriority = 0;
      for (sesIter = sessions->begin(); sesIter != sessions->end(); ++sesIter) {
         if ((*sesIter)->Group())
            summedPriority += (*sesIter)->Group()->Priority();
      }
      if (summedPriority > 0)
         priority = (xps->Group()->Priority() * sessions->size()) / summedPriority;

   }
   int nWrks = (int)(nFreeCPUs * fNodesFraction * priority) + fMinForQuery;
   nWrks = (nWrks >= (int) wrks->size()) ? wrks->size() - 1 : nWrks;
   TRACE(DBG,"GetNumWorkers: "<< nFreeCPUs<<" : "<< nWrks);

   return nWrks;
}

//__________________________________________________________________________
int XrdProofSched::GetWorkers(XrdProofServProxy *xps,
                              std::list<XrdProofWorker *> *wrks)
{
   // Get a list of workers that can be used by session 'xps'.
   int rc = 0;

   // The caller must provide a list where to store the result
   if (!wrks)
      return -1;

   if (!fMgr || !(fMgr->GetActiveWorkers()))
      return -1;

   // The current, full list
   std::list<XrdProofWorker *> *acws = fMgr->GetActiveWorkers();

   // Point to the master element
   XrdProofWorker *mst = acws->front();
   if (!mst)
      return -1;

   // The master first (stats are updated in XrdProofdProtocol::GetWorkers)
   wrks->push_back(mst);

   if (fWorkerSel == kSSOLoadBased) {
      // Dynamic scheduling: the scheduler will determine the #workers
      // to be used based on the current load and assign the least loaded ones

      // Sort the workers by the load
      XrdProofWorker::Sort(acws, XpdWrkComp);

      // Get the advised number
      int nw = GetNumWorkers(xps);

      std::list<XrdProofWorker *>::iterator nxWrk = acws->begin();
      while (nw--) {
         nxWrk++;
         // Add export version of the info
         // (stats are updated in XrdProofdProtocol::GetWorkers)
         wrks->push_back(*nxWrk);
      }
      // Done
      return 0;
   }

   if (fWorkerMax > 0 && fWorkerMax < (int) acws->size()) {

      // Now the workers
      if (fWorkerSel == kSSORandom) {
         // Random: the first time init the machine
         static bool rndmInit = 0;
         if (!rndmInit) {
            const char *randdev = "/dev/urandom";
            int fd;
            unsigned int seed;
            if ((fd = open(randdev, O_RDONLY)) != -1) {
               read(fd, &seed, sizeof(seed));
               srand(seed);
               close(fd);
               rndmInit = 1;
            }
         }
         // Selection
         int nwt = acws->size();
         std::vector<int> walloc(nwt, 0);
         std::vector<XrdProofWorker *> vwrk(nwt);

         // Fill the vector with cumulative number of actives
         int namx = -1;
         int i = 1;
         std::list<XrdProofWorker *>::iterator iwk = acws->begin();
         iwk++; // Skip master
         for ( ; iwk != acws->end(); iwk++) {
            vwrk[i] = *iwk;
            int na = (*iwk)->fActive;
            printf(" %d", na);
            walloc[i] = na + walloc[i-1];
            i++;
            namx = (na > namx) ? na : namx;
         }
         printf("\n");
         // Normalize
         for (i = 1; i < nwt; i++) {
            if (namx > 0)
               walloc[i] = namx*i - walloc[i] + i;
            else
               walloc[i] = i;
         }
         int natot = walloc[nwt - 1];

         int nw = fWorkerMax;
         while (nw--) {
            // Normalized number
            int maxAtt = 10000, natt = 0;
            int iw = -1;
            while ((iw < 1 || iw >= nwt) && natt < maxAtt) {
               int jw = rand() % natot;
               for (i = 0; i < nwt; i++) {
                  if (jw < walloc[i]) {
                     // re-normalize the weights for the higher index entries
                     int j = 0;
                     for (j = i; j < nwt; j++) {
                        if (walloc[j] > 0)
                           walloc[j]--;
                     }
                     natot--;
                     iw = i;
                     break;
                  }
               }
            }

            if (iw > -1) {
               // Add to the list (stats are updated in XrdProofdProtocol::GetWorkers)
               wrks->push_back(vwrk[iw]);
            } else {
               // Unable to generate the right number
               TRACE(XERR, "XrdProofSched::GetWorkers: random generation failed");
               rc = -1;
               break;
            }
         }

      } else {
         if (fNextWrk >= (int) acws->size())
            fNextWrk = 1;
         int iw = 0;
         std::list<XrdProofWorker *>::iterator nxWrk = acws->begin();
         int nw = fWorkerMax;
         while (nw--) {
            while (iw != fNextWrk) {
               nxWrk++;
               iw++;
            }
            // Add export version of the info
            // (stats are updated in XrdProofdProtocol::GetWorkers)
            wrks->push_back(*nxWrk);
            // Update next worker index
            fNextWrk++;
            if (fNextWrk >= (int) acws->size()) {
               fNextWrk = 1;
               iw = 0;
               nxWrk = acws->begin();
            }
         }
      }
   } else {
      // The full list
      std::list<XrdProofWorker *>::iterator iw = acws->begin();
      iw++;
      while (iw != acws->end()) {
         // Add to the list (stats are updated in XrdProofdProtocol::GetWorkers)
         wrks->push_back(*iw);
         iw++;
      }
   }

   // Make sure that something has been found
   if (acws->size() <= 1) {
      TRACE(XERR, "XrdProofSched::GetWorkers: no worker found: do nothing");
      rc = -1;
   }

   return rc;
}


//______________________________________________________________________________
int XrdProofSched::ExportInfo(XrdOucString &sbuf)
{
   // Fill sbuf with some info about our current status

   // Selection type
   const char *osel[] = { "all", "round-robin", "random", "load-based"};
   sbuf += "Selection: ";
   sbuf += osel[fWorkerSel+1];
   if (fWorkerSel > -1) {
      sbuf += ", max workers: ";
      sbuf += fWorkerMax; sbuf += " &";
   }

   // The full list
   std::list<XrdProofWorker *> *acws = fMgr->GetActiveWorkers();
   std::list<XrdProofWorker *>::iterator iw;
   for (iw = acws->begin(); iw != acws->end(); ++iw) {
      sbuf += (*iw)->fType;
      sbuf += ": "; sbuf += (*iw)->fHost;
      if ((*iw)->fPort > -1) {
         sbuf += ":"; sbuf += (*iw)->fPort;
      } else
         sbuf += "     ";
      sbuf += "  sessions: "; sbuf += (*iw)->fActive;
      sbuf += " &";
   }

   // Done
   return 0;
}
