// Copyright 2004-2023 Sophos Limited. All rights reserved.

#include "AbstractThread.h"
namespace Common
{
    namespace Threads
    {
        AbstractThread::AbstractThread() :
            m_threadStarted(), m_ensureThreadStarted(), m_notifyPipe(), m_thread(), m_threadStartedFlag(false)
        {
        }

        AbstractThread::~AbstractThread()
        {
            requestStop();
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        void AbstractThread::start()
        {
            std::unique_lock<std::mutex> lock(m_threadStarted);
            m_thread = std::thread(&AbstractThread::run, this);
            m_ensureThreadStarted.wait(lock, [this]() { return m_threadStartedFlag; });
        }

        void AbstractThread::requestStop() { m_notifyPipe.notify(); }

        void AbstractThread::join()
        {
            if (m_thread.joinable())
            {
                m_thread.join();
            }
            m_threadStartedFlag = false;
        }

        bool AbstractThread::stopRequested() { return m_notifyPipe.notified(); }

        void AbstractThread::announceThreadStarted()
        {
            // unblock start (waiting on m_ensureThreadStarted
            // this make subsequent calls to hasFinished safe from race-condition.
            std::unique_lock<std::mutex> templock(m_threadStarted);
            m_threadStartedFlag = true;
            m_ensureThreadStarted.notify_all();
        }

        bool AbstractThread::joinable()
        {
            return m_thread.joinable();
        }

    } // namespace Threads
} // namespace Common
