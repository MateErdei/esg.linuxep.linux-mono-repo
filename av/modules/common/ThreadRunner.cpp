/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "ThreadRunner.h"


using namespace common;

ThreadRunner::ThreadRunner(std::shared_ptr<common::AbstractThreadPluginInterface> thread, std::string name, bool startNow)
    : m_thread(std::move(thread)), m_name(std::move(name))
{
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
    if (m_thread)
    {
        LOGINFO("Stopping " << m_name);
        m_thread->tryStop();
        LOGINFO("Joining " << m_name);
        m_thread->join();
    }
    else
    {
        LOGERROR("Failed to stop thread for " << m_name);
    }
    m_started = false;
}

void ThreadRunner::startThread()
{
    if (m_thread)
    {
        LOGINFO("Starting " << m_name);
        m_thread->start();
        m_started = true;
    }
    else
    {
        LOGERROR("Failed to start thread for " << m_name);
        m_started = false;
    }
}
