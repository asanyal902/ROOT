// @(#)root/net:$Id$
// Author: Fons Rademakers   17/01/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TWebFile                                                             //
//                                                                      //
// A TWebFile is like a normal TFile except that it reads its data      //
// via a standard apache web server. A TWebFile is a read-only file.    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TWebFile.h"
#include "TROOT.h"
#include "TSocket.h"
#include "Bytes.h"
#include "TError.h"
#include "TSystem.h"
#include <errno.h>
#include <stdlib.h>

#ifdef WIN32
#define EADDRINUSE  10048
#define EISCONN     10056
#endif

static const char *gUserAgent = "User-Agent: ROOT-TWebFile/1.1";

TUrl TWebFile::fgProxy;


// Internal class used to manage the socket that may stay open between
// calls when HTTP/1.1 protocol is used
class TWebSocket {
private:
   TWebFile *fWebFile;  // associated web file
public:
   TWebSocket(TWebFile *f);
   ~TWebSocket();
   void ReOpen();
};

TWebSocket::TWebSocket(TWebFile *f)
{
   // Open web file socket.

   fWebFile = f;
   if (!f->fSocket)
      ReOpen();
}

TWebSocket::~TWebSocket()
{
   // Close socket in case not HTTP/1.1 protocol.

   if (!fWebFile->fHTTP11) {
      delete fWebFile->fSocket;
      fWebFile->fSocket = 0;
   }
}

void TWebSocket::ReOpen()
{
   // Re-open web file socket.

   if (fWebFile->fSocket)
      delete fWebFile->fSocket;

   TUrl connurl;
   if (fWebFile->fProxy.IsValid())
      connurl = fWebFile->fProxy;
   else
      connurl = fWebFile->fUrl;

   for (Int_t i = 0; i < 5; i++) {
      fWebFile->fSocket = new TSocket(connurl.GetHost(), connurl.GetPort());
      if (!fWebFile->fSocket->IsValid()) {
         delete fWebFile->fSocket;
         fWebFile->fSocket = 0;
         if (gSystem->GetErrno() == EADDRINUSE || gSystem->GetErrno() == EISCONN) {
            gSystem->Sleep(i*10);
         } else {
            ::Error("TWebSocket::ReOpen", "cannot connect to remote host %s (errno=%d)",
                    fWebFile->fUrl.GetHost(), gSystem->GetErrno());
            return;
         }
      } else
         return;
   }
}


ClassImp(TWebFile)

//______________________________________________________________________________
TWebFile::TWebFile(const char *url, Option_t *opt) : TFile(url, "WEB")
{
   // Create a Web file object. A web file is the same as a read-only
   // TFile except that it is being read via a HTTP server. The url
   // argument must be of the form: http://host.dom.ain/file.root.
   // The opt can be "NOPROXY", to bypass any set "http_proxy" shell
   // variable. The proxy can be specified as (in sh, or equivalent csh):
   //   export http_proxy=http://pcsalo.cern.ch:3128
   // The proxy can also be specified via the static method TWebFile::SetProxy().
   // If the file specified in the URL does not exist or is not accessible
   // the kZombie bit will be set in the TWebFile object. Use IsZombie()
   // to see if the file is accessible. The preferred interface to this
   // constructor is via TFile::Open().

   TString option = opt;
   fNoProxy = kFALSE;
   if (option.Contains("NOPROXY", TString::kIgnoreCase))
      fNoProxy = kTRUE;
   CheckProxy();

   Init(kFALSE);
}

//______________________________________________________________________________
TWebFile::TWebFile(TUrl url, Option_t *opt) : TFile(url.GetUrl(), "WEB")
{
   // Create a Web file object. A web file is the same as a read-only
   // TFile except that it is being read via a HTTP server. Make sure url
   // is a valid TUrl object.
   // The opt can be "NOPROXY", to bypass any set "http_proxy" shell
   // variable. The proxy can be specified as (in sh, or equivalent csh):
   //   export http_proxy=http://pcsalo.cern.ch:3128
   // The proxy can also be specified via the static method TWebFile::SetProxy().
   // If the file specified in the URL does not exist or is not accessible
   // the kZombie bit will be set in the TWebFile object. Use IsZombie()
   // to see if the file is accessible.

   TString option = opt;
   fNoProxy = kFALSE;
   if (option.Contains("NOPROXY", TString::kIgnoreCase))
      fNoProxy = kTRUE;
   CheckProxy();

   Init(kFALSE);
}

//______________________________________________________________________________
TWebFile::~TWebFile()
{
   // Cleanup.

   delete fSocket;
}

//______________________________________________________________________________
void TWebFile::Init(Bool_t)
{
   // Initialize a TWebFile object.

   char buf[4];
   int  err;

   fSocket = 0;

   if ((err = GetHead()) < 0) {
      if (err == -2)
         Error("TWebFile", "%s does not exist", fUrl.GetUrl());
      MakeZombie();
      gDirectory = gROOT;
      return;
   }

   if (fIsRootFile) {
      Seek(0);
      if (ReadBuffer(buf, 4)) {
         MakeZombie();
         gDirectory = gROOT;
         return;
      }

      if (strncmp(buf, "root", 4) && strncmp(buf, "PK", 2)) {  // PK is zip file
         Error("TWebFile", "%s is not a ROOT file", fUrl.GetUrl());
         MakeZombie();
         gDirectory = gROOT;
         return;
      }
   }

   TFile::Init(kFALSE);
   fD = -2;   // so TFile::IsOpen() will return true when in TFile::~TFile
}

//______________________________________________________________________________
void TWebFile::CheckProxy()
{
   // Check if shell var "http_proxy" has been set and should be used.

   if (fNoProxy)
      return;

   if (fgProxy.IsValid()) {
      fProxy = fgProxy;
      return;
   }

   TString proxy = gSystem->Getenv("http_proxy");
   if (proxy != "") {
      TUrl p(proxy);
      if (strcmp(p.GetProtocol(), "http")) {
         Error("CheckProxy", "protocol must be HTTP in proxy URL %s",
               proxy.Data());
         return;
      }
      fProxy = p;
   }
}

//______________________________________________________________________________
Bool_t TWebFile::IsOpen() const
{
   // A TWebFile that has been correctly constructed is always considered open.

   return IsZombie() ? kFALSE : kTRUE;
}

//______________________________________________________________________________
Int_t TWebFile::ReOpen(Option_t *mode)
{
   // Reopen a file with a different access mode, like from READ to
   // UPDATE or from NEW, CREATE, RECREATE, UPDATE to READ. Thus the
   // mode argument can be either "READ" or "UPDATE". The method returns
   // 0 in case the mode was successfully modified, 1 in case the mode
   // did not change (was already as requested or wrong input arguments)
   // and -1 in case of failure, in which case the file cannot be used
   // anymore. A TWebFile cannot be reopened in update mode.

   TString opt = mode;
   opt.ToUpper();

   if (opt != "READ" && opt != "UPDATE")
      Error("ReOpen", "mode must be either READ or UPDATE, not %s", opt.Data());

   if (opt == "UPDATE")
      Error("ReOpen", "update mode not allowed for a TWebFile");

   return 1;
}

//______________________________________________________________________________
Bool_t TWebFile::ReadBuffer(char *buf, Int_t len)
{
   // Read specified byte range from remote file via HTTP daemon. This
   // routine connects to the remote host, sends the request and returns
   // the buffer. Returns kTRUE in case of error.

   Int_t st;
   if ((st = ReadBufferViaCache(buf, len))) {
      if (st == 2)
         return kTRUE;
      return kFALSE;
   }

   if (!fHasModRoot)
      return ReadBuffer10(buf, len);

   // Give full URL so Apache's virtual hosts solution works.
   // Use protocol 0.9 for efficiency, we are not interested in the 1.0 headers.
   TString msg = "GET ";
   msg += fUrl.GetProtocol();
   msg += "://";
   msg += fUrl.GetHost();
   msg += ":";
   msg += fUrl.GetPort();
   msg += "/";
   msg += fUrl.GetFile();
   msg += "?";
   msg += fOffset;
   msg += ":";
   msg += len;
   msg += "\r\n";

   if (GetFromWeb(buf, len, msg) == -1)
      return kTRUE;

   fOffset += len;

   return kFALSE;
}

//______________________________________________________________________________
Bool_t TWebFile::ReadBuffer10(char *buf, Int_t len)
{
   // Read specified byte range from remote file via HTTP 1.0 daemon (without
   // mod-root installed). This routine connects to the remote host, sends the
   // request and returns the buffer. Returns kTRUE in case of error.

   // Give full URL so Apache's virtual hosts solution works.
   TString msg = "GET ";
   msg += fUrl.GetProtocol();
   msg += "://";
   msg += fUrl.GetHost();
   msg += ":";
   msg += fUrl.GetPort();
   msg += "/";
   msg += fUrl.GetFile();
   if (fHTTP11)
      msg += " HTTP/1.1";
   else
      msg += " HTTP/1.0";
   msg += "\r\n";
   if (fHTTP11) {
      msg += "Host: ";
      msg += fUrl.GetHost();
      msg += "\r\n";
   }
   msg += gUserAgent;
   msg += "\r\n";
   msg += "Range: bytes=";
   msg += fOffset;
   msg += "-";
   msg += fOffset+len-1;
   msg += "\r\n\r\n";

   Int_t n;
   while ((n = GetFromWeb10(buf, len, msg)) == -2) { }
   if (n == -1)
      return kTRUE;

   fOffset += len;

   return kFALSE;
}

//______________________________________________________________________________
Bool_t TWebFile::ReadBuffers(char *buf,  Long64_t *pos, Int_t *len, Int_t nbuf)
{
   // Read specified byte ranges from remote file via HTTP daemon.
   // Reads the nbuf blocks described in arrays pos and len,
   // where pos[i] is the seek position of block i of length len[i].
   // Note that for nbuf=1, this call is equivalent to TFile::ReafBuffer
   // This function is overloaded by TNetFile, TWebFile, etc.
   // Returns kTRUE in case of failure.

   if (!fHasModRoot)
      return ReadBuffers10(buf, pos, len, nbuf);

   // Give full URL so Apache's virtual hosts solution works.
   // Use protocol 0.9 for efficiency, we are not interested in the 1.0 headers.
   TString msgh = "GET ";
   msgh += fUrl.GetProtocol();
   msgh += "://";
   msgh += fUrl.GetHost();
   msgh += ":";
   msgh += fUrl.GetPort();
   msgh += "/";
   msgh += fUrl.GetFile();
   msgh += "?";

   TString msg = msgh;

   Int_t k = 0, n = 0;
   for (Int_t i = 0; i < nbuf; i++) {
      if (n) msg += ",";
      msg += pos[i] + fArchiveOffset;
      msg += ":";
      msg += len[i];
      n   += len[i];
      if (msg.Length() > 8000) {
         msg += "\r\n";
         if (GetFromWeb(&buf[k], n, msg) == -1)
            return kTRUE;
         msg = msgh;
         k += n;
         n = 0;
      }
   }

   msg += "\r\n";

   if (GetFromWeb(&buf[k], n, msg) == -1)
      return kTRUE;

   return kFALSE;
}

//______________________________________________________________________________
Bool_t TWebFile::ReadBuffers10(char *buf,  Long64_t *pos, Int_t *len, Int_t nbuf)
{
   // Read specified byte ranges from remote file via HTTP 1.0 daemon (without
   // mod-root installed). Read the nbuf blocks described in arrays pos and len,
   // where pos[i] is the seek position of block i of length len[i].
   // Note that for nbuf=1, this call is equivalent to TFile::ReafBuffer
   // This function is overloaded by TNetFile, TWebFile, etc.
   // Returns kTRUE in case of failure.

   // Give full URL so Apache's virtual hosts solution works.
   TString msgh = "GET ";
   msgh += fUrl.GetProtocol();
   msgh += "://";
   msgh += fUrl.GetHost();
   msgh += ":";
   msgh += fUrl.GetPort();
   msgh += "/";
   msgh += fUrl.GetFile();
   if (fHTTP11)
      msgh += " HTTP/1.1";
   else
      msgh += " HTTP/1.0";
   msgh += "\r\n";
   if (fHTTP11) {
      msgh += "Host: ";
      msgh += fUrl.GetHost();
      msgh += "\r\n";
   }
   msgh += gUserAgent;
   msgh += "\r\n";
   msgh += "Range: bytes=";

   TString msg = msgh;

   Int_t k = 0, n = 0, r;
   for (Int_t i = 0; i < nbuf; i++) {
      if (n) msg += ",";
      msg += pos[i] + fArchiveOffset;
      msg += "-";
      msg += pos[i] + fArchiveOffset + len[i] - 1;
      n   += len[i];
      if (msg.Length() > 8000) {
         msg += "\r\n\r\n";
         while ((r = GetFromWeb10(&buf[k], n, msg)) == -2) { }
         if (r == -1)
            return kTRUE;
         msg = msgh;
         k += n;
         n = 0;
      }
   }

   msg += "\r\n\r\n";

   while ((r = GetFromWeb10(&buf[k], n, msg)) == -2) { }
   if (r == -1)
      return kTRUE;

   return kFALSE;
}

//______________________________________________________________________________
Int_t TWebFile::GetFromWeb(char *buf, Int_t len, const TString &msg)
{
   // Read request from web server. Returns -1 in case of error,
   // 0 in case of success.

   if (!len) return 0;

   TUrl connurl;
   if (fProxy.IsValid())
      connurl = fProxy;
   else
      connurl = fUrl;

   TSocket s(connurl.GetHost(), connurl.GetPort());
   if (!s.IsValid()) {
      Error("GetFromWeb", "cannot connect to remote host %s", fUrl.GetHost());
      return -1;
   }

   if (s.SendRaw(msg.Data(), msg.Length()) == -1) {
      Error("GetFromWeb", "error sending command to remote host %s", fUrl.GetHost());
      return -1;
   }

   if (s.RecvRaw(buf, len) == -1) {
      Error("GetFromWeb", "error receiving data from remote host %s", fUrl.GetHost());
      return -1;
   }

   fBytesRead += len;
   SetFileBytesRead(GetFileBytesRead() + len);

   return 0;
}

//______________________________________________________________________________
Int_t TWebFile::GetFromWeb10(char *buf, Int_t len, const TString &msg)
{
   // Read multiple byte range request from web server.
   // Uses HTTP 1.0 daemon wihtout mod-root.
   // Returns -2 in case of HTTP/1.1 and connection has been closed and call
   // has to be retried, -1 in case of error, 0 in case of success.

   if (!len) return 0;

   // open fSocket and close it when going out of scope
   TWebSocket ws(this);

   if (!fSocket || !fSocket->IsValid()) {
      Error("GetFromWeb10", "cannot connect to remote host %s", fUrl.GetHost());
      return -1;
   }

   if (fSocket->SendRaw(msg.Data(), msg.Length()) == -1) {
      Error("GetFromWeb10", "error sending command to remote host %s", fUrl.GetHost());
      return -1;
   }

   char line[1024];
   Int_t n, ret = 0, nranges = 0, ltot = 0;
   TString boundary, boundaryEnd;
   Long64_t first = -1, last = -1, tot;

   while ((n = GetLine(fSocket, line, 1024)) >= 0) {
      if (n == 0) {
         if (ret < 0)
            return ret;

         if (first >= 0) {
            Int_t ll = Int_t(last - first) + 1;
            if (fSocket->RecvRaw(&buf[ltot], ll) == -1) {
               Error("GetFromWeb10", "error receiving data from remote host %s", fUrl.GetHost());
               return -1;
            }
            ltot += ll;
            fBytesRead += ll;
            SetFileBytesRead(GetFileBytesRead() + ll);

            first = -1;

            if (boundary == "")
               break;  // not a multipart response
         }

         continue;
      }

      if (gDebug > 0)
         Info("GetFromWeb10", "header: %s", line);

      if (boundaryEnd == line) {
         if (gDebug > 0)
            Info("GetFromWeb10", "got all headers");
         break;
      }
      if (boundary == line) {
         nranges++;
         if (gDebug > 0)
            Info("GetFromWeb10", "get new multipart byte range (%d)", nranges);
      }

      TString res = line;
      if (res.BeginsWith("HTTP/1.")) {
         TString scode = res(9, 3);
         Int_t code = scode.Atoi();
         if (code != 206) {
            ret = -1;
            TString mess = res(13, 1000);
            Error("GetFromWeb10", "%s: %s (%d)", fUrl.GetUrl(), mess.Data(), code);
         }
      }
      if (res.BeginsWith("Content-Type: multipart")) {
         boundary = "--" + res(res.Index("boundary=")+9, 1000);
         boundaryEnd = boundary + "--";
      }
      if (res.BeginsWith("Content-range:")) {
#ifdef R__WIN32
         sscanf(res.Data(), "Content-range: bytes %I64d-%I64d/%I64d", &first, &last, &tot);
#else
         sscanf(res.Data(), "Content-range: bytes %lld-%lld/%lld", &first, &last, &tot);
#endif
      }
      if (res.BeginsWith("Content-Range:")) {
#ifdef R__WIN32
         sscanf(res.Data(), "Content-Range: bytes %I64d-%I64d/%I64d", &first, &last, &tot);
#else
         sscanf(res.Data(), "Content-Range: bytes %lld-%lld/%lld", &first, &last, &tot);
#endif
      }
   }

   if (n == -1 && fHTTP11) {
      if (gDebug > 0)
         Info("GetFromWeb10", "HTTP/1.1 socket closed, reopen");
      ws.ReOpen();
      return -2;
   }

   if (ltot != len) {
      Error("GetFromWeb10", "error receiving expected amount of data (got %d, expected %d) from remote host %s",
            ltot, len, fUrl.GetHost());
      return -1;
   }

   return 0;
}

//______________________________________________________________________________
void TWebFile::Seek(Long64_t offset, ERelativeTo pos)
{
   // Set position from where to start reading.

   switch (pos) {
   case kBeg:
      fOffset = offset + fArchiveOffset;
      break;
   case kCur:
      fOffset += offset;
      break;
   case kEnd:
      // this option is not used currently in the ROOT code
      if (fArchiveOffset)
         Error("Seek", "seeking from end in archive is not (yet) supported");
      fOffset = fEND - offset;  // is fEND really EOF or logical EOF?
      break;
   }
}

//______________________________________________________________________________
Long64_t TWebFile::GetSize() const
{
   // Return maximum file size.

   if (!fHasModRoot || fSize >= 0)
      return fSize;

   Long64_t size;
   char     asize[64];

   TString msg = "GET ";
   msg += fUrl.GetProtocol();
   msg += "://";
   msg += fUrl.GetHost();
   msg += ":";
   msg += fUrl.GetPort();
   msg += "/";
   msg += fUrl.GetFile();
   msg += "?";
   msg += -1;
   msg += "\r\n";

   if (const_cast<TWebFile*>(this)->GetFromWeb(asize, 64, msg) == -1)
      return kMaxInt;

#ifndef R__WIN32
   size = atoll(asize);
#else
   size = _atoi64(asize);
#endif

   fSize = size;

   return size;
}

//______________________________________________________________________________
Int_t TWebFile::GetHead()
{
   // Get the HTTP header. Depending on the return code we can see if
   // the file exists and if the server uses mod_root.
   // Returns -1 in case of an error, -2 in case the file does not exists,
   // 0 in case of success.

   fSize       = -1;
   fHasModRoot = kFALSE;
   fHTTP11     = kFALSE;

   // Give full URL so Apache's virtual hosts solution works.
   TString msg = "HEAD ";
   msg += fUrl.GetProtocol();
   msg += "://";
   msg += fUrl.GetHost();
   msg += ":";
   msg += fUrl.GetPort();
   msg += "/";
   msg += fUrl.GetFile();
   msg += " HTTP/1.0";
   msg += "\r\n";
   msg += gUserAgent;
   msg += "\r\n\r\n";

   TUrl connurl;
   if (fProxy.IsValid())
      connurl = fProxy;
   else
      connurl = fUrl;

   TSocket *s = 0;
   for (Int_t i = 0; i < 5; i++) {
      s = new TSocket(connurl.GetHost(), connurl.GetPort());
      if (!s->IsValid()) {
         delete s;
         if (gSystem->GetErrno() == EADDRINUSE || gSystem->GetErrno() == EISCONN) {
            s = 0;
            gSystem->Sleep(i*10);
         } else {
            Error("GetHead", "cannot connect to remote host %s (errno=%d)", fUrl.GetHost(),
                  gSystem->GetErrno());
            return -1;
         }
      } else
         break;
   }
   if (!s)
      return -1;

   if (s->SendRaw(msg.Data(), msg.Length()) == -1) {
      Error("GetHead", "error sending command to remote host %s", fUrl.GetHost());
      delete s;
      return -1;
   }

   char line[1024];
   Int_t n, ret = 0;

   while ((n = GetLine(s, line, 1024)) >= 0) {
      if (n == 0) {
         if (gDebug > 0)
            Info("GetHead", "got all headers");
         delete s;
         return ret;
      }

      if (gDebug > 0)
         Info("GetHead", "header: %s", line);

      TString res = line;
      if (res.BeginsWith("HTTP/1.")) {
         if (res.BeginsWith("HTTP/1.1"))
            fHTTP11 = kTRUE;
         TString scode = res(9, 3);
         Int_t code = scode.Atoi();
         if (code == 500)
            fHasModRoot = kTRUE;
         else if (code == 404)
            ret = -2;   // file does not exist
         else if (code > 200) {
            ret = -1;
            TString mess = res(13, 1000);
            Error("GetHead", "%s: %s (%d)", fUrl.GetUrl(), mess.Data(), code);
         }
      }
      if (res.BeginsWith("Content-Length:")) {
         TString slen = res(16, 1000);
         fSize = slen.Atoll();
      }
   }
   delete s;

   return ret;
}

//______________________________________________________________________________
Int_t TWebFile::GetLine(TSocket *s, char *line, Int_t size)
{
   // Read a line from the socket. Reads at most one less than the number of
   // characters specified by size. Reading stops when a newline character
   // is found, The newline (\n) and cr (\r), if any, are removed.
   // Returns -1 in case of error, or the number of characters read (>= 0)
   // otherwise.

   char c;
   Int_t err, n = 0;
   while ((err = s->RecvRaw(&c, 1)) >= 0) {
      if (n == size-1 || c == '\n' || err == 0) {
         if (line[n-1] == '\r')
            n--;
         break;
      }
      line[n++] = c;
   }
   line[n] = '\0';
   if (err < 0) {
      if (!fHTTP11 || gDebug > 0)
         Error("GetLine", "error receiving data from remote host %s", fUrl.GetHost());
      return -1;
   }
   return n;
}

//______________________________________________________________________________
void TWebFile::SetProxy(const char *proxy)
{
   // Static method setting global proxy URL.

   if (proxy && *proxy) {
      TUrl p(proxy);
      if (strcmp(p.GetProtocol(), "http")) {
         :: Error("TWebFile::SetProxy", "protocol must be HTTP in proxy URL %s",
                  proxy);
         return;
      }
      fgProxy = p;
   }
}

//______________________________________________________________________________
const char *TWebFile::GetProxy()
{
   // Static method returning the global proxy URL.

   if (fgProxy.IsValid())
      return fgProxy.GetUrl();
   return "";
}
