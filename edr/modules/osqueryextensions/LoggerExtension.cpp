/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LoggerExtension.h"

#include "Logger.h"

#include <functional>

#include <SophosLoggerPlugin.h>

LoggerExtension::LoggerExtension(
    const std::string& intermediaryPath,
    const std::string& datafeedPath,
    const std::string& osqueryXDRConfigFilePath,
    const std::string& pluginVarDir,
    unsigned int dataLimit,
    unsigned int periodInSeconds,
    std::function<void(void)> dataExceededCallback) :
    m_resultsSender(
        intermediaryPath,
        datafeedPath,
        osqueryXDRConfigFilePath,
        pluginVarDir,
        dataLimit,
        periodInSeconds,
        dataExceededCallback)
{
    m_flags.interval = 3;
    m_flags.timeout = 3;
}

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

    // store these locally in case the run thread crashes and we want to call this start again from within Run.
    m_maxBatchBytes = maxBatchBytes;
    m_maxBatchSeconds = maxBatchSeconds;

    if (m_stopped)
    {
        m_flags.socket = socket;
        m_flags.verbose = verbose;
        m_extension = OsquerySDK::CreateExtension(m_flags, "SophosLoggerPlugin", "1.1.0.1");
        m_extension->AddLoggerPlugin(
            std::make_unique<SophosLoggerPlugin>(m_resultsSender, m_maxBatchBytes, m_maxBatchSeconds));
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
        LOGDEBUG("LoggerExtension::Stopping");
        m_stopped = true;
        m_extension->Stop();
        if (m_runnerThread)
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        LOGINFO("LoggerExtension::Stopped");
    }
}

void LoggerExtension::Run()
{
    LOGINFO("LoggerExtension::Run");
    m_extension->Wait();
    if (!m_stopped)
    {
        const auto healthCheckMessage = m_extension->GetHealthCheckFailureMessage();
        if (!healthCheckMessage.empty())
        {
            LOGWARN(healthCheckMessage);
        }

        LOGWARN(L"Service extension stopped unexpectedly. Calling reset.");
        Stop();
    }
}

void LoggerExtension::setDataLimit(unsigned int limitBytes)
{
    m_resultsSender.setDataLimit(limitBytes);
}

void LoggerExtension::setDataPeriod(unsigned int periodSeconds)
{
    m_resultsSender.setDataPeriod(periodSeconds);
}

bool LoggerExtension::checkDataPeriodHasElapsed()
{
    return m_resultsSender.checkDataPeriodElapsed();
}
