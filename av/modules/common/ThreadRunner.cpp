/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreadRunner.h"

#include "Logger.h"

using namespace common;

ThreadRunner::ThreadRunner(Common::Threads::AbstractThread &thread, std::string name)
    : m_thread(thread), m_name(std::move(name))
{
    LOGINFO("Starting " << m_name);
    m_thread.start();
}

ThreadRunner::~ThreadRunner()
{
    killThreads();
}

void ThreadRunner::killThreads()
{
    LOGINFO("Stopping " << m_name);
    m_thread.requestStop();
    LOGINFO("Joining " << m_name);
    m_thread.join();
}