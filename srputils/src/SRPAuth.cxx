// @(#)root/srputils:$Name:  $:$Id: SRPAuth.cxx,v 1.9 2003/10/07 14:03:03 rdm Exp $
// Author: Fons Rademakers   15/02/2000

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


#include <stdio.h>
extern "C" {
#include <t_pwd.h>
#include <t_client.h>
}

#include "TSocket.h"
#include "TAuthenticate.h"
#include "THostAuth.h"
#include "TError.h"
#include "TSystem.h"
#include "TEnv.h"
#include "rpderr.h"


Int_t SRPAuthenticate(TAuthenticate *, const char *user, const char *passwd,
                      const char *remote, TString &, Int_t);

class SRPAuthInit {
public:
   SRPAuthInit() { TAuthenticate::SetSecureAuthHook(&SRPAuthenticate); }
};
static SRPAuthInit srpauth_init;


TSocket *sock = 0;
THostAuth *HostAuth = 0;
Int_t  gRSAKey = 0;
//______________________________________________________________________________
Int_t SRPAuthenticate(TAuthenticate *auth, const char *user, const char *passwd,
                      const char *remote, TString &det, Int_t version)
{
   // Authenticate to remote rootd/proofd server using the SRP (secure remote
   // password) protocol. Returns 0 if authentication failed, 1 if
   // authentication succeeded and 2 if SRP is not available and standard
   // authentication should be tried. Called via TAuthenticate class.

   Int_t  result = 0;
   char  *usr = 0;
   char  *psswd = 0;
   Int_t  stat, kind;

   // From the calling TAuthenticate
   sock = auth->GetSocket();
   HostAuth = auth->GetHostAuth();

   // send user name
   if (user && user[0])
      usr = StrDup(user);
   else
      usr = TAuthenticate::PromptUser(remote); // Should never get here ...

   Int_t ReUse= 1, Prompt= 0;
   TString Details;

   if (version > 1) {

     // Check ReUse
     ReUse  = TAuthenticate::GetAuthReUse();
     Prompt = TAuthenticate::GetPromptUser();

     // Build auth details
     Details = Form("pt:%d ru:%d us:%s",Prompt,ReUse,usr);

     // Create Options string
     char *Options= new char[strlen(usr)+20];
     int Opt = ReUse * kAUTH_REUSE_MSK;
     sprintf(Options,"%d %d %s",Opt,strlen(usr),usr);


     // Now we are ready to send a request to the rootd/proofd
     // daemons to check if we have already a valid security context
     // and eventually to start a negotiation to get one ...
     kind = kROOTD_SRPUSER;
     stat = ReUse;
     int rc = 0;
     if ((rc = TAuthenticate::AuthExists(auth,
               (Int_t)TAuthenticate::kSRP,Details,Options,&kind,&stat)) == 1) {
       // A valid authentication exists: we are done ...
       if (Options) delete[] Options;
       return 1;
     }
     if (rc == -2) {
       if (Options) delete[] Options;
       return rc;
     }

   } else {

     sock->Send(usr, kROOTD_SRPUSER);
     sock->Recv(stat, kind);

     // stat == 2 when no SRP support compiled in remote rootd
     if (kind == kROOTD_SRPUSER && stat == 0)
        return 2;

     if (kind == kROOTD_ERR) {
        if (gDebug>0) TAuthenticate::AuthError("SRPAuthenticate", stat);
        if (stat == kErrConnectionRefused) return -2;
        return 0;
     }

   }

   struct t_num     n, g, s, B, *A;
   struct t_client *tc;
   char    hexbuf[MAXHEXPARAMLEN];
   UChar_t buf1[MAXPARAMLEN], buf2[MAXPARAMLEN], buf3[MAXSALTLEN];

   // receive n from server
   sock->Recv(hexbuf, MAXHEXPARAMLEN, kind);
   if (kind != kROOTD_SRPN) {
      if (gDebug>0) ::Error("SRPAuthenticate", "expected kROOTD_SRPN message");
      goto out;
   }
   n.data = buf1;
   n.len  = t_fromb64((char*)n.data, hexbuf);

   // receive g from server
   sock->Recv(hexbuf, MAXHEXPARAMLEN, kind);
   if (kind != kROOTD_SRPG) {
      if (gDebug>0) ::Error("SRPAuthenticate", "expected kROOTD_SRPG message");
      goto out;
   }
   g.data = buf2;
   g.len  = t_fromb64((char*)g.data, hexbuf);

   // receive salt from server
   sock->Recv(hexbuf, MAXHEXPARAMLEN, kind);
   if (kind != kROOTD_SRPSALT) {
      if (gDebug>0) ::Error("SRPAuthenticate", "expected kROOTD_SRPSALT message");
      goto out;
   }
   s.data = buf3;
   s.len  = t_fromb64((char*)s.data, hexbuf);

   tc = t_clientopen(usr, &n, &g, &s);

   A = t_clientgenexp(tc);

   // send A to server
   sock->Send(t_tob64(hexbuf, (char*)A->data, A->len), kROOTD_SRPA);

   if (passwd && passwd[0])
      psswd = StrDup(passwd);
   else {
      psswd = TAuthenticate::PromptPasswd(Form("%s@%s SRP password: ",user,remote));
      if (!psswd)
         if (gDebug>0) ::Error("SRPAuthenticate", "password not set");
   }

   t_clientpasswd(tc, psswd);

   // receive B from server
   sock->Recv(hexbuf, MAXHEXPARAMLEN, kind);
   if (kind != kROOTD_SRPB) {
      if (gDebug>0) ::Error("SRPAuthenticate", "expected kROOTD_SRPB message");
      goto out;
   }
   B.data = buf1;
   B.len  = t_fromb64((char*)B.data, hexbuf);

   t_clientgetkey(tc, &B);

   // send response to server
   sock->Send(t_tohex(hexbuf, (char*)t_clientresponse(tc), RESPONSE_LEN),
              kROOTD_SRPRESPONSE);

   t_clientclose(tc);

   if (version > 1) {

     // Save passwd for later use ...
     TAuthenticate::SetGlobalUser(user);
     TAuthenticate::SetGlobalPasswd(psswd);
     TAuthenticate::SetGlobalPwHash(kFALSE);
     TAuthenticate::SetGlobalSRPPwd(kTRUE);


     // Receive result of the overall process
     sock->Recv(stat, kind);
     if (kind == kROOTD_ERR) {
       if (gDebug>0) TAuthenticate::AuthError("SRPAuthenticate", stat);
       goto out;
     }

     if (ReUse == 1) {

       if (kind != kROOTD_RSAKEY)
       Warning("SRPAuthenticate", "problems recvn RSA key flag: got message %d, flag: %d",kind,gRSAKey);
       gRSAKey = 1;

       // Send the key securely
       TAuthenticate::SendRSAPublicKey(sock);

       // Receive result of the overall process
       sock->Recv(stat, kind);
       if (kind == kROOTD_ERR)
          if (gDebug>0) TAuthenticate::AuthError("SRPAuthenticate", stat);
     }

     if (kind == kROOTD_SRPUSER && stat == 1)
        result = 1;

     if (kind == kROOTD_SRPUSER && stat > 0) {
       char *rfrm= new char[stat+1];
       sock->Recv(rfrm,stat+1, kind);  // receive user,offset) info
       // Parse answer
       char *lUser = new char[stat];
       int OffSet = -1;
       sscanf(rfrm,"%s %d",lUser,&OffSet);

       // Decode Token
       char *Token = 0;
       if (ReUse == 1 && OffSet > -1) {
         if (TAuthenticate::SecureRecv(sock,gRSAKey,&Token) == -1) {
           Warning("SRPAuthenticate","Problems secure-receiving Token - may result in corrupted Token");
         }
       } else {
         Token = StrDup("");
       }

       // Create and save AuthDetails object
       TAuthenticate::SaveAuthDetails(auth,(Int_t)TAuthenticate::kSRP,OffSet,ReUse,Details,lUser,gRSAKey,Token);
       det = Details;
       if (Token) delete[] Token;

       // Receive result from remote Login process
       sock->Recv(stat, kind);
       if (stat==1 && kind==kROOTD_AUTH) {
         if (gDebug>0) Info("SRPAuthenticate","Remotely authenticated as %s (OffSet:%d)",lUser,OffSet);
         result = 1;
       }

     } else {
       if (kind != kROOTD_ERR )
         if (gDebug>0) Warning("SRPAuthenticate", "problems recvn (user,offset) length (%d:%d)",kind,stat);
       TAuthenticate::AuthError("SRPAuthenticate", stat);
     }

   } else {

     // Old protocol

     // Receive result of the overall process
     sock->Recv(stat, kind);

     if (kind == kROOTD_ERR)
       if (gDebug > 0) TAuthenticate::AuthError("SRPAuthenticate", stat);

     if (kind == kROOTD_AUTH && stat == 1)
        result = 1;
   }
out:
   delete [] usr;
   delete [] psswd;

   return result;
}
