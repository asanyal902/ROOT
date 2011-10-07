// @(#)root/thread:$Id$
// Author: Anar Manafov   20/09/2011

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TThreadPool
#define ROOT_TThreadPool

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadPool                                                          //
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// ROOT
#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_TMutex
#include "TMutex.h"
#endif
#ifndef ROOT_TCondition
#include "TCondition.h"
#endif
// STD
#include <queue>
#include <vector>
#include <iostream>
#include <sstream>


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TNonCopyable                                                         //
// Class which makes child to be non-copyable object.                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
class TNonCopyable {
protected:
   TNonCopyable()
   {}
   ~TNonCopyable()
   {}
private:
   TNonCopyable(const TNonCopyable&);
   const TNonCopyable& operator=(const TNonCopyable&);
};

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadPoolTaskImp                                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
template <class _T, class _P>
class TThreadPoolTaskImp {
public:
   bool run(_P &_param) {
      _T *pThis = reinterpret_cast<_T *>(this);
      return pThis->runTask(_param);
   }
};

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadPoolTask                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
template <class _T, class _P>
class TThreadPoolTask {
public:
   typedef TThreadPoolTaskImp<_T, _P> task_t;

public:
   TThreadPoolTask(task_t &_task, _P &_param):
      fTask(_task),
      fTaskParam(_param) {
   }
   bool run() {
      return fTask.run(fTaskParam);
   }

private:
   task_t &fTask;
   _P fTaskParam;
};

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadPool                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
template <class _T, class _P>
class TThreadPool : public TNonCopyable {
    
   typedef TThreadPoolTask<_T, _P> task_t;
   typedef std::queue<task_t*> taskqueue_t;
   typedef std::vector<TThread*> threads_array_t;

public:
   TThreadPool(size_t _threadsCount, bool _needDbg = false):
      fstopped(false),
      fbSilent(!_needDbg) {
      fThreadNeeded = new TCondition(&fmutex);
      fThreadAvailable = new TCondition(&fmutex);

      for (size_t i = 0; i < _threadsCount; ++i) {
         TThread *pThread = new TThread(&TThreadPool::Executor, this);
         fThreads.push_back(pThread);
         pThread->Run();
      }

      fThreadJoinHelper =  new TThread(&TThreadPool::JoinHelper, this);
   }

   ~TThreadPool() {
      Stop();
      // deleting threads
      threads_array_t::const_iterator iter = fThreads.begin();
      threads_array_t::const_iterator iter_end = fThreads.end();
      for (; iter != iter_end; ++iter)
         delete(*iter);

      delete fThreadJoinHelper;

      delete fThreadNeeded;
      delete fThreadAvailable;
   }

   void PushTask(typename TThreadPoolTask<_T, _P>::task_t &_task, _P _param) {
      {
         DbgLog("Main thread. Try to push a task");

         TLockGuard lock(&fmutex);
         task_t *task = new task_t(_task, _param);
         fTasks.push(task);
         ++fTasksCount;

         DbgLog("Main thread. the task is pushed");
      }
      TLockGuard lock(&fmutex);
      fThreadNeeded->Broadcast();
   }

   void Stop(bool processRemainingJobs = false) {
      // prevent more jobs from being added to the queue
      if (fstopped)
         return;

      if (processRemainingJobs) {
         TLockGuard lock(&fmutex);
         // wait for queue to drain
         while (!fTasks.empty() && !fstopped) {
            DbgLog("Main thread is waiting");
            fThreadAvailable->Wait();
            DbgLog("Main thread is DONE waiting");
         }
      }
      // tell all threads to stop
      {
         TLockGuard lock(&fmutex);
         fstopped = true;
         fThreadNeeded->Broadcast();
         DbgLog("Main threads requests to STOP");
      }

      // Waiting for all threads to complete
      fThreadJoinHelper->Run();
      fThreadJoinHelper->Join();
   }

   size_t TasksCount() const {
      return fTasksCount;
   }

   size_t SuccessfulTasks() const {
      return fsuccessfulTasks;
   }

private:
   static void* Executor(void *_arg) {
      TThreadPool *pThis = reinterpret_cast<TThreadPool*>(_arg);

      while (!pThis->fstopped) {
         task_t *task(NULL);

         std::stringstream ss;
         ss
               << ">>>> Check for tasks."
               << " Number of Tasks: " << pThis->fTasks.size();
         pThis->DbgLog(ss.str());

         // There is a task, let's take it
         {
            TLockGuard lock(&pThis->fmutex);
            // Find a task to perform
            if (pThis->fTasks.empty() && !pThis->fstopped) {
               pThis->DbgLog("waiting for a task");

               // No tasks, we wait for a task to come
               pThis->fThreadNeeded->Wait();

               pThis->DbgLog("done waiting for tasks");
            }

            {
               if (!pThis->fTasks.empty()) {
                  task = pThis->fTasks.front();
                  pThis->fTasks.pop();

                  pThis->DbgLog("get the task");
               }
               pThis->DbgLog("done Check <<<<");
            }
         }

         // Execute the task
         if (task) {
            pThis->DbgLog("Run the task");

            if (task->run()) {
               TLockGuard lock(&pThis->fmutex);
               ++pThis->fsuccessfulTasks;
            }
            delete task;
            task = NULL;

            pThis->DbgLog("Done Running the task");
         }
         // Task is done, report that the thread is free
         TLockGuard lock(&pThis->fmutex);
         pThis->fThreadAvailable->Broadcast();
      }

      pThis->DbgLog("**** DONE ***");
      return NULL;
   }

   static void *JoinHelper(void *_arg) {
      TThreadPool *pThis = reinterpret_cast<TThreadPool*>(_arg);
      threads_array_t::const_iterator iter = pThis->fThreads.begin();
      threads_array_t::const_iterator iter_end = pThis->fThreads.end();
      for (; iter != iter_end; ++iter)
         (*iter)->Join();

      return NULL;
   }

   static bool IsThreadActive(TThread *_pThread) {
      // so far we consider only kRunningState as activity
      return (_pThread->GetState() == TThread::kRunningState);
   }

   void DbgLog(const std::string &_msg) {
      if (fbSilent)
         return;
      TLockGuard lock(&fdbgOutputMutex);
      std::cout << "[" << TThread::SelfId() << "] " << _msg << std::endl;
   }

private:
   taskqueue_t fTasks;
   TMutex fmutex;
   TCondition *fThreadNeeded;
   TCondition *fThreadAvailable;
   threads_array_t fThreads;
   TThread *fThreadJoinHelper;
   volatile bool fstopped;
   size_t fsuccessfulTasks;
   size_t fTasksCount;
   TMutex fdbgOutputMutex;
   bool fbSilent; // No DBG messages
};

#endif
