// @(#)root/proof:$Name:  $:$Id: TProofMgr.cxx,v 1.1 2005/12/10 16:51:57 rdm Exp $
// Author: G. Ganis, Nov 2005

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProofMgr                                                            //
//                                                                      //
// The PROOF manager interacts with the PROOF server coordinator to     //
// create or destroy a PROOF session, attach to or detach from          //
// existing one, and to monitor any client activity on the cluster.     //
// At most one manager instance per server is allowed.                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TList.h"
#include "TProof.h"
#include "TProofMgr.h"


ClassImp(TProofMgr)

//______________________________________________________________________________
TProofMgr::TProofMgr(const char *url, Int_t, const char *alias)
         : TVirtualProofMgr(url)
{
   // Create a PROOF manager for the standard (old) environment.

   fServType = kProofd;

   // Correct URL protocol
   if (!strcmp(fUrl.GetProtocol(), TUrl("a").GetProtocol()))
      fUrl.SetProtocol("proof");

   // Check and save the host FQDN ...
   TInetAddress addr = gSystem->GetHostByName(fUrl.GetHost());
   if (addr.IsValid()) {
      fUrl.SetHost(addr.GetHostName());
      if (!strcmp(fUrl.GetProtocol(),"UnNamedHost"))
         fUrl.SetHost(addr.GetHostAddress());
   }

   SetName(fUrl.GetUrl());
   if (alias)
      SetAlias(alias);
   else
      SetAlias(fUrl.GetHost());
}

//______________________________________________________________________________
TVirtualProof *TProofMgr::AttachSession(Int_t id, Bool_t)
{
   // Dummy version provided for completeness. Just returns a pointer to
   // existing session 'id' (as shown by TProof::QuerySessions) or 0 if 'id' is
   // not valid.

   TVirtualProofDesc *d = GetProofDesc(id);
   if (d) {
      if (d->GetProof())
         // Nothing to do if already in contact with proofserv
         return d->GetProof();
   }

   Info("AttachSession","invalid proofserv id (%d)", id);
   return 0;
}

//______________________________________________________________________________
void TProofMgr::DetachSession(Int_t id, Option_t *opt)
{
   // Detach session with 'id' from its proofserv. The 'id' is the number
   // shown by QuerySessions.

   TVirtualProofDesc *d = GetProofDesc(id);
   if (d) {
      if (d->GetProof())
         d->GetProof()->Detach(opt);
      fSessions->Remove(d);
      delete d;
   }

   return;
}

//______________________________________________________________________________
TList *TProofMgr::QuerySessions(Option_t *opt)
{
   // Get list of sessions accessible to this manager.

   if (opt && !strncasecmp(opt,"L",1))
      // Just return the existing list
      return fSessions;

   // Create list if not existing
   if (!fSessions) {
      fSessions = new TList();
      fSessions->SetOwner();
   }

   // Fill-in entries from the official list
   if (gROOT->GetListOfProofs()) {
      // Loop over
      TIter nxp(gROOT->GetListOfProofs());
      TVirtualProof *p = 0;
      Int_t ns = 0;
      while ((p = (TVirtualProof *)nxp())) {
         // Only those belonginh to this server
         if (MatchUrl(p->GetUrl())) {
            if (!(fSessions->FindObject(p->GetSessionTag()))) {
               TVirtualProofDesc *d =
                   new TVirtualProofDesc(p->GetName(), p->GetTitle(), p->GetUrl(),
                                         ++ns, p->GetSessionID(), p->IsIdle(), p);
               fSessions->Add(d);
            }
         }
      }
   }

   // Drop entries not existing any longer
   if (fSessions->GetSize() > 0) {
      TIter nxd(fSessions);
      TVirtualProofDesc *d = 0;
      while ((d = (TVirtualProofDesc *)nxd())) {
         if (d->GetProof()) {
            if (!(gROOT->GetListOfProofs()->FindObject(d->GetProof()))) {
               fSessions->Remove(d);
               SafeDelete(d);
            } else {
               if (opt && !strncasecmp(opt,"S",1))
                  d->Print("");
            }
         }
      }
   }

   // We are done
   return fSessions;
}
