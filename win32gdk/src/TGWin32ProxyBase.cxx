// @(#)root/win32gdk:$Name:  $:$Id: TGWin32ProxyBase.cxx,v 1.5 2003/08/23 14:51:25 brun Exp $
// Author: Valeriy Onuchin  08/08/2003

/*************************************************************************
 * Copyright (C) 1995-2001, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "Windows4Root.h"
#include <windows.h>

#include "TGWin32ProxyBase.h"
#include "TRefCnt.h"
#include "TList.h"
#include "TGWin32.h"

////////////////////////////////////////////////////////////////////////////////
class TGWin32CallBackObject: public TObject
{
public:
   TGWin32CallBack   fCallBack;  // callback function (called by GUI thread)
   void              *fParam;    // arguments passed to/from callback function

   TGWin32CallBackObject(TGWin32CallBack cb,void *p):fCallBack(cb),fParam(p) {}
   ~TGWin32CallBackObject() { if (fParam) delete fParam; }
};

////////////////////////////////////////////////////////////////////////////////
class TGWin32ProxyBasePrivate {
public:

   HANDLE               fEvent;   // event used for syncronization
   TGWin32ProxyBasePrivate();
   ~TGWin32ProxyBasePrivate();
};

//______________________________________________________________________________
TGWin32ProxyBasePrivate::TGWin32ProxyBasePrivate()
{
   // ctor

   fEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

//______________________________________________________________________________
TGWin32ProxyBasePrivate::~TGWin32ProxyBasePrivate()
{
   // dtor

   if (fEvent) ::CloseHandle(fEvent);
   fEvent = 0;
}

ULong_t TGWin32ProxyBase::fgPostMessageId = 0;
ULong_t TGWin32ProxyBase::fgMainThreadId = 0;

////////////////////////////////////////////////////////////////////////////////
//______________________________________________________________________________
TGWin32ProxyBase::TGWin32ProxyBase()
{
   // ctor

   fCallBack = 0;
   fParam = 0;
   fListOfCallBacks = new TList();
   fBatchLimit = 100;
   fId = ::GetCurrentThreadId();
   fPimpl = new TGWin32ProxyBasePrivate();

   if (!fgPostMessageId) fgPostMessageId = ::RegisterWindowMessage("TGWin32ProxyBase::Post");
}

//______________________________________________________________________________
TGWin32ProxyBase::~TGWin32ProxyBase()
{
   // dtor

   fListOfCallBacks->Delete();
   delete fListOfCallBacks;
   fListOfCallBacks = 0;

   delete fPimpl;
}

//______________________________________________________________________________
void TGWin32ProxyBase::Lock()
{
   // enter critical section

   TGWin32::Lock();
}

//______________________________________________________________________________
void TGWin32ProxyBase::Unlock()
{
   // leave critical section

   TGWin32::Unlock();
}

//______________________________________________________________________________
Double_t TGWin32ProxyBase::GetMilliSeconds()
{
   // returns elapsed time in milliseconds with microseconds precision

   static LARGE_INTEGER freq;
   static Bool_t first = kTRUE;
   LARGE_INTEGER count;
   static Double_t overhead = 0;

   if (first) {
      LARGE_INTEGER count0;
      ::QueryPerformanceFrequency(&freq);
      ::QueryPerformanceCounter(&count0);
      if (1) {
         Double_t dummy;
         dummy = ((Double_t)count0.QuadPart - overhead)*1000./((Double_t)freq.QuadPart);
      }
      ::QueryPerformanceCounter(&count);
      overhead = (Double_t)count.QuadPart - (Double_t)count0.QuadPart;
      first = kFALSE;
   }

   ::QueryPerformanceCounter(&count);
   return ((Double_t)count.QuadPart - overhead)*1000./((Double_t)freq.QuadPart);
}

//______________________________________________________________________________
void TGWin32ProxyBase::SetMainThreadId(ULong_t id)
{
   //

   if (!fgMainThreadId) fgMainThreadId = id;
}

//______________________________________________________________________________
void TGWin32ProxyBase::ExecuteCallBack(Bool_t sync)
{
   // Executes all batched callbacks and the latest callback
   // This method is executed by GUI thread

   // process batched callbacks
   if (fListOfCallBacks && fListOfCallBacks->GetSize()) {
      TIter next(fListOfCallBacks);
      TGWin32CallBackObject *obj;

      while ((obj = (TGWin32CallBackObject*)next())) {
         obj->fCallBack(obj->fParam);  // execute callback
      }
   }
   if (sync) {
      if (fCallBack) fCallBack(fParam);
      ::SetEvent(fPimpl->fEvent);
   }
}

//______________________________________________________________________________
Bool_t TGWin32ProxyBase::ForwardCallBack(Bool_t sync)
{
   // if sync is kTRUE:
   //    - post message to main thread.
   //    - execute callbacks from fListOfCallBacks
   //    - wait for response
   // else
   //    -  add callback to fListOfCallBacks
   //
   // returns kTRUE if callback execution is delayed (batched)

   Int_t wait = 0;
   if (!fgMainThreadId) return kFALSE;

   Bool_t batch = !sync &&  (fListOfCallBacks->GetSize()<fBatchLimit);

   if (batch) {
      fListOfCallBacks->Add(new TGWin32CallBackObject(fCallBack,fParam));
      return kTRUE;
   }

   while (::PostThreadMessage(fgMainThreadId,fgPostMessageId,(WPARAM)this,0L)==0) {
      // wait because there is chance that message queue does not exist yet
      ::SleepEx(50,1);
      if (wait++>5) return kFALSE; // failed to post
   }

   ::WaitForSingleObject(fPimpl->fEvent,INFINITE);
   ::ResetEvent(fPimpl->fEvent);

   fListOfCallBacks->Delete();
   return kFALSE;
}

//______________________________________________________________________________
void TGWin32ProxyBase::SendExitMessage()
{
   // send exit message to GUI thread

   Int_t wait = 0;
   while (::PostThreadMessage(fgMainThreadId, fgPostMessageId, 0, 0L)==0) {
      // wait because there is chance that message queue does not exist yet
      ::SleepEx(50,1);
      if (wait++>5) return;
   }
}
