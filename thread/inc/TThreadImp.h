// @(#)root/thread:$Name$:$Id$
// Author: Victor Perev   10/08/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TThreadImp
#define ROOT_TThreadImp


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadImp                                                           //
//                                                                      //
// This class implements threads. A thread is an execution environment  //
// much lighter than a process. A single process can have multiple      //
// threads. The actual work is done via the TThreadImp class (either    //
// TThreadPosix, TThreadSolaris or TThreadNT).                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_TThread
#include "TThread.h"
#endif
class TThreadImp : public TObject {
public:

        TThreadImp();
       ~TThreadImp();

virtual Int_t Join(Int_t jid, void **ret)=0;
virtual Int_t SelfId()=0;
virtual Int_t Run(TThread *th)=0;

virtual Int_t Kill(TThread *th)=0;
virtual Int_t SetCancelOff()=0;
virtual Int_t SetCancelOn()=0; 		
virtual Int_t SetCancelAsynchronous()=0;
virtual Int_t SetCancelDeferred()=0; 	
virtual Int_t CancelPoint()=0;
virtual Int_t CleanUpPush(void **main, void *free,void *arg)=0;
virtual Int_t CleanUpPop(void **main, Int_t exe)=0;
virtual Int_t CleanUp(void **main)=0;

virtual Int_t Exit(void *ret)=0;

virtual Int_t Sleep  (ULong_t secs, ULong_t nanos = 0)=0;
virtual Int_t GetTime(ULong_t *absSec, ULong_t *absNanoSec)=0;

ClassDef(TThreadImp,0)  // ThreadImp class

};
#endif



