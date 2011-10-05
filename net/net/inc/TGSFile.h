// @(#)root/net:$Id$
// Author: Marcelo Sousa   23/08/2011

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGSFile
#define ROOT_TGSFile

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGSFile                                                              //
//                                                                      //
// A TGSFile is a normal TWebFile but it reads data from the            //
// Google Storage server. As a derived TWebFile class TGSFile it is     //
// a read only file. The HTTP requests are generated by THTTPMessage    //
// objects with the auth_prefix set as GOOG1. The user id and secret    //
// pass required to sign the requests are passed through the            //
// environment variables GT_ACCESS_ID and GT_ACCESS_KEY.                //
// For more information check:                                          //
//   http://code.google.com/apis/storage/docs/getting-started.html      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TWebFile
#include "TWebFile.h"
#endif

#ifndef ROOT_TUrl
#include "TUrl.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

class TGSFile: public TWebFile {

private:
   TGSFile();
  
protected:
   TString fAuthPrefix;  //Authentication prefix for Google Storage
   TString fAccessId;    //User id 
   TString fAccessKey;   //Secret key
   TUrl    fServer;      //Server url
   TString fBucket;      //Bucket name
      
   Int_t  GetHead();
   Bool_t ReadBuffer10(char *buf, Int_t len);

public:
   TGSFile(const char *url, Option_t *opt="");
   virtual ~TGSFile() { }

   Bool_t  ReadBuffer(char *buf, Int_t len);
   TString GetAuthPrefix() const { return fAuthPrefix; }
   TString GetAccessId() const { return fAccessId; }
   TString GetAccessKey() const { return fAccessKey; }
   TUrl    GetUrl() const { return fServer; }
   TString GetBucket() const { return fBucket; }

   ClassDef(TGSFile, 0)  // Read a ROOT file from the Google Storage cloud
};

#endif
