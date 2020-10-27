/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LoggerExtension.h"
#include "Logger.h"
#include <SophosLoggerPlugin.h>

#include <functional>

LoggerExtension::~LoggerExtension()
{
    Stop();
}

void LoggerExtension::Start(
    const std::string& socket,
    bool verbose,
    uintmax_t maxBatchBytes,
    unsigned int maxBatchSeconds)
{
    LOGINFO("LoggerExtension::Start");

    if (m_stopped)
    {
        m_flags.socket = socket;
        m_flags.verbose = verbose;
        m_extension = OsquerySDK::CreateExtension(m_flags, "SophosLoggerPlugin", "1.1.0.1");
        m_extension->AddLoggerPlugin(
            std::make_unique<SophosLoggerPlugin>(m_resultsSender, maxBatchBytes, maxBatchSeconds));
        LOGDEBUG("Logger Plugin Added");
        m_extension->Start();
        m_stopped = false;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { Run(); }));
        LOGDEBUG("Run Logger Plugin thread");
    }
}

void LoggerExtension::Stop()
{
    if (!m_stopped)
    {
        LOGINFO("LoggerExtension::Stop");
        m_stopped = true;
        m_extension->Stop();
        if (m_runnerThread)
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
    }
}

void LoggerExtension::Run()
{
    LOGINFO("LoggerExtension::Run");
    m_extension->Wait();
    if (!m_stopped)
    {
        //Handle crash
    }
}
