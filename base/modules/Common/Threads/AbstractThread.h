///////////////////////////////////////////////////////////
//
// Copyright (C) 2004-2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#ifndef EVEREST_BASE_COMMON_THREADS_ABSTRACTTHREAD_H
#define EVEREST_BASE_COMMON_THREADS_ABSTRACTTHREAD_H
#include <condition_variable>
#include <mutex>
#include <thread>
#include "NotifyPipe.h"
namespace Common
{
    namespace Threads
    {
        class AbstractThread
        {
        public:
            /**
             * Construct the abstract thread. It is to be derived from and provide an implementation of run.
             * This class is intended to be used in places where thread is needed.
             * Developers using this class must be aware of its requirement, specially in relation to how to
             * implement the ::run method.
             */
            AbstractThread();
            virtual ~AbstractThread();
            /**
             * Start the thread and wait it to be 'really started'.
             * After returning from start, the abstractthread ensures that ::run method has started.
             * After a thread is created, it can be in either initializing, running, finished.
             * Some come may be simplified by ensuring that a thread is either running or finished.
             * The start method ensure that, but to do so, it is required that the implementation of ::run method calls
             * ::announceThreadStarted at some point.
             */
            void start();
            /**
             * Notify the thread that it should stop 'as soon as possible'. The ::run method will have access to this
             * information by querying the ::stopRequested method.
             */
            void requestStop();

            void join();

        protected:
            /**
             * Check if the thread was requested to stop. To be used in implementation of ::run.
             *
             * @return true if ::requestStop was ever executed otherwise false.
             */
            bool stopRequested();
            /**
             * Inform the AbstractThread that the ::run method has started and allow ::start to return from its 'blocking mode'.
             * Implementations of ::run MUST call this function early after the start of ::run or the parent thread will be in deadlock in
             * the ::start method.
             */
            void announceThreadStarted();
            /**
             * The function that will be running in the separate thread.
             * Implementation of the ::run should be in the following pattern:
             * @code
             * void MyThread::run() override
             * {
             *   announceThreadStarted();
             *   while ( !stopRequested())
             *   {
             *     // do my thread specific task.
             *     // call sleep if necessary if in busy loop.
             *   }
             * }
             * @endcode
             */
            virtual void run() = 0;


            NotifyPipe m_notifyPipe;
        private:
            std::mutex m_threadStarted;
            std::condition_variable m_ensureThreadStarted;
            std::thread m_thread;


        };
    }
}


#endif //EVEREST_BASE_COMMON_THREADS_ABSTRACTTHREAD_H
