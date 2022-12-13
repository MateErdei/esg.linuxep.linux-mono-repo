// Copyright 2022 Sophos Limited. All rights reserved.

#include "ProcessForceExitTimer.h"

#include "Logger.h"

using namespace sspl::sophosthreatdetectorimpl;

ProcessForceExitTimer::ProcessForceExitTimer(std::chrono::seconds timeout, datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
: m_timeout(timeout)
, m_systemCallWrapper(systemCallWrapper)
{

}

ProcessForceExitTimer::~ProcessForceExitTimer()
{
    tryStop();
    join();
}

void ProcessForceExitTimer::tryStop()
{
    {
        std::lock_guard lock(m_mutex);
        m_stopRequested = true;
    }

    m_cond.notify_one();
}

void ProcessForceExitTimer::setExitCode(int exitCode)
{
    std::lock_guard lock(m_mutex);
    m_exitCode = exitCode;
}

void ProcessForceExitTimer::run()
{
    announceThreadStarted();

    std::unique_lock<std::mutex> lck(m_mutex);

    // Waits for period or for stop to be requested
    if (!m_cond.wait_for(lck, m_timeout, [this](){ return m_stopRequested.load(); }))
    {
        LOGWARN("Timed out waiting for graceful shutdown - forcing exit with return code " << m_exitCode);
        m_systemCallWrapper->_exit(m_exitCode);
    }
}