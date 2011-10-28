// @(#)root/net:$Id$
// Author: Marcelo Sousa   26/10/2011

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TUdpSocket
#define ROOT_TUdpSocket


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TUdpSocket                                                           //
//                                                                      //
// This class implements udp client sockets. A socket is an endpoint    //
// for communication between two machines.                              //
// The actual work is done via the TSystem class (either TUnixSystem,   //
// or TWinNTSystem). Currently there is no support for TWinNTSystem.    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif
#ifndef ROOT_TBits
#include "TBits.h"
#endif
#ifndef ROOT_TInetAddress
#include "TInetAddress.h"
#endif
#ifndef ROOT_MessageTypes
#include "MessageTypes.h"
#endif
#ifndef ROOT_TVirtualAuth
#include "TVirtualAuth.h"
#endif
#ifndef ROOT_TSecContext
#include "TSecContext.h"
#endif
#ifndef ROOT_TTimeStamp
#include "TTimeStamp.h"
#endif
#ifndef ROOT_TVirtualMutex
#include "TVirtualMutex.h"
#endif
#ifndef ROOT_TSocket
#include "TSocket.h"
#endif

class TUdpSocket : public TNamed {

friend class TServerSocket;

public:
   enum EStatusBits { kIsUnix = BIT(16),    // set if unix socket
                      kBrokenConn = BIT(17) // set if conn reset by peer or broken 
                    };
   enum EInterest { kRead = 1, kWrite = 2 };
   enum EServiceType { kSOCKD, kROOTD, kPROOFD };

protected:
   TInetAddress  fAddress;        // remote internet address and port #
   UInt_t        fBytesRecv;      // total bytes received over this socket
   UInt_t        fBytesSent;      // total bytes sent using this socket
   Int_t         fCompress;       // Compression level and algorithm
   TInetAddress  fLocalAddress;   // local internet address and port #
   Int_t         fRemoteProtocol; // protocol of remote daemon
   TSecContext  *fSecContext;     // after a successful Authenticate call
                                  // points to related security context
   TString       fService;        // name of service (matches remote port #)
   EServiceType  fServType;       // remote service type
   Int_t         fSocket;         // socket descriptor
   TString       fUrl;            // needs this for special authentication options
   TBits         fBitsInfo;       // bits array to mark TStreamerInfo classes already sent
   TList        *fUUIDs;          // list of TProcessIDs already sent through the socket

   TVirtualMutex *fLastUsageMtx;   // Protect last usage setting / reading
   TTimeStamp    fLastUsage;      // Time stamp of last usage

   static ULong64_t fgBytesRecv;  // total bytes received by all socket objects
   static ULong64_t fgBytesSent;  // total bytes sent by all socket objects

   TUdpSocket() : fAddress(), fBytesRecv(0), fBytesSent(0), fCompress(0),
                  fLocalAddress(), fRemoteProtocol(), fSecContext(0), fService(),
                  fServType(kSOCKD), fSocket(-1), fUrl(),
                  fBitsInfo(), fUUIDs(0), fLastUsageMtx(0), fLastUsage() { }

   void         SetDescriptor(Int_t desc) { fSocket = desc; }
   void         SendStreamerInfos(const TMessage &mess);
   Bool_t       RecvStreamerInfos(TMessage *mess);
   void         SendProcessIDs(const TMessage &mess);
   Bool_t       RecvProcessIDs(TMessage *mess);

private:
   TUdpSocket&   operator=(const TUdpSocket &);  // not implemented
   Option_t     *GetOption() const { return TObject::GetOption(); }

public:
   TUdpSocket(TInetAddress address, const char *service);
   TUdpSocket(TInetAddress address, Int_t port);
   TUdpSocket(const char *host, const char *service);
   TUdpSocket(const char *host, Int_t port);
   TUdpSocket(const char *sockpath);
   
   TUdpSocket(Int_t descriptor);
   TUdpSocket(Int_t descriptor, const char *sockpath);
   TUdpSocket(const TUdpSocket &s);
   
   virtual ~TUdpSocket() { Close(); }

   virtual void          Close(Option_t *opt="");
   virtual Int_t         GetDescriptor() const { return fSocket; }
   TInetAddress          GetInetAddress() const { return fAddress; }
   virtual TInetAddress  GetLocalInetAddress();
   Int_t                 GetPort() const { return fAddress.GetPort(); }
   const char           *GetService() const { return fService; }
   Int_t                 GetServType() const { return (Int_t)fServType; }
   virtual Int_t         GetLocalPort();
   UInt_t                GetBytesSent() const { return fBytesSent; }
   UInt_t                GetBytesRecv() const { return fBytesRecv; }
   Int_t                 GetCompressionAlgorithm() const;
   Int_t                 GetCompressionLevel() const;
   Int_t                 GetCompressionSettings() const;
   Int_t                 GetErrorCode() const;
   virtual Int_t         GetOption(ESockOptions opt, Int_t &val);
   Int_t                 GetRemoteProtocol() const { return fRemoteProtocol; }
   TSecContext          *GetSecContext() const { return fSecContext; }

   TTimeStamp            GetLastUsage() { R__LOCKGUARD2(fLastUsageMtx); return fLastUsage; }
   const char           *GetUrl() const { return fUrl; }

   virtual Bool_t        IsValid() const { return fSocket < 0 ? kFALSE : kTRUE; }
   virtual Int_t         Recv(TMessage *&mess);
   virtual Int_t         Recv(Int_t &status, Int_t &kind);
   virtual Int_t         Recv(char *mess, Int_t max);
   virtual Int_t         Recv(char *mess, Int_t max, Int_t &kind);
   virtual Int_t         RecvRaw(void *buffer, Int_t length, ESendRecvOptions opt = kDefault);
   virtual Int_t         Reconnect() { return -1; }
   virtual Int_t         Select(Int_t interest = kRead, Long_t timeout = -1);
   virtual Int_t         Send(const TMessage &mess);
   virtual Int_t         Send(Int_t kind);
   virtual Int_t         Send(Int_t status, Int_t kind);
   virtual Int_t         Send(const char *mess, Int_t kind = kMESS_STRING);
   virtual Int_t         SendObject(const TObject *obj, Int_t kind = kMESS_OBJECT);
   virtual Int_t         SendRaw(const void *buffer, Int_t length,
                                 ESendRecvOptions opt = kDefault);
   void                  SetCompressionAlgorithm(Int_t algorithm=0);
   void                  SetCompressionLevel(Int_t level=1);
   void                  SetCompressionSettings(Int_t settings=1);
   virtual Int_t         SetOption(ESockOptions opt, Int_t val);
   void                  SetRemoteProtocol(Int_t rproto) { fRemoteProtocol = rproto; }
   void                  SetSecContext(TSecContext *ctx) { fSecContext = ctx; }
   void                  SetService(const char *service) { fService = service; }
   void                  SetServType(Int_t st) { fServType = (EServiceType)st; }
   void                  SetUrl(const char *url) { fUrl = url; }

   void                  Touch() { R__LOCKGUARD2(fLastUsageMtx); fLastUsage.Set(); }
   
   static ULong64_t      GetSocketBytesSent();
   static ULong64_t      GetSocketBytesRecv();

   static void           NetError(const char *where, Int_t error);
   
   ClassDef(TUdpSocket,0)  //This class implements UDP client sockets
};

//______________________________________________________________________________
inline Int_t TUdpSocket::GetCompressionAlgorithm() const
{
   return (fCompress < 0) ? -1 : fCompress / 100;
}

//______________________________________________________________________________
inline Int_t TUdpSocket::GetCompressionLevel() const
{
   return (fCompress < 0) ? -1 : fCompress % 100;
}

//______________________________________________________________________________
inline Int_t TUdpSocket::GetCompressionSettings() const
{
   return (fCompress < 0) ? -1 : fCompress;
}

#endif
