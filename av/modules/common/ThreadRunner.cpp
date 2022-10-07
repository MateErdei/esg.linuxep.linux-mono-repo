// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Logger.h"
#include "ThreadRunner.h"


using namespace common;

ThreadRunner::ThreadRunner(std::shared_ptr<common::AbstractThreadPluginInterface> thread, std::string name, bool startNow)
    : m_thread(std::move(thread)), m_name(std::move(name))
{
    if (!m_thread)
    {
        throw std::runtime_error("Cannot create ThreadRunner as thread is null");
    }

    if (startNow)
    {
        startThread();
    }
}

ThreadRunner::~ThreadRunner()
{
    stopThread();
}



void ThreadRunner::requestStopIfNotStopped()
{
    if (m_started)
    {
        stopThread();
    }
}

void ThreadRunner::startIfNotStarted()
{
    if (!m_started)
    {
        startThread();
    }
}


void ThreadRunner::stopThread()
{
    LOGINFO("Stopping " << m_name);
    m_thread->tryStop();
    LOGINFO("Joining " << m_name);
    m_thread->join();
    m_started = false;
    m_thread->clearTerminationPipe(); // Ensure a restart won't terminate immediately
}

void ThreadRunner::startThread()
{
    LOGINFO("Starting " << m_name);
    m_thread->restart(); // Ensures termination pipe is cleared before start
    m_started = true;
}
