///////////////////////////////////////////////////////////
//
// Copyright (C) 2004-2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "AbstractThread.h"
namespace Common
{
    namespace Threads
    {

        AbstractThread::AbstractThread():
            m_threadStarted(),
            m_ensureThreadStarted(),
            m_thread(),
            m_notifyPipe()
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
            m_thread = std::thread(&AbstractThread::run,this);
            m_ensureThreadStarted.wait(lock);
        }

        void AbstractThread::requestStop()
        {
            m_notifyPipe.notify();
        }

        void AbstractThread::join()
        {
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        bool AbstractThread::stopRequested()
        {
            return m_notifyPipe.notified();
        }

        void AbstractThread::announceThreadStarted()
        {
            // unblock start (waiting on m_ensureThreadStarted
            // this make subsequent calls to hasFinished safe from race-condition.
            std::unique_lock<std::mutex> templock(m_threadStarted);
            m_ensureThreadStarted.notify_all();
        }
    }
}
