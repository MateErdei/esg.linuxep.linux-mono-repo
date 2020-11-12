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
        std::function<void(void)> dataExceededCallback,
        unsigned int maxBatchSeconds,
        uintmax_t maxBatchBytes) :
    m_resultsSender(
        intermediaryPath,
        datafeedPath,
        osqueryXDRConfigFilePath,
        pluginVarDir,
        dataLimit,
        periodInSeconds,
        dataExceededCallback),
    m_maxBatchBytes(maxBatchBytes),
    m_maxBatchSeconds(maxBatchSeconds)
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
    std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGINFO("Starting LoggerExtension");
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
        m_runnerThread = std::make_unique<std::thread>(std::thread([this, extensionFinished] { Run(extensionFinished); }));
        LOGDEBUG("Logger Plugin running in thread");
    }
}

void LoggerExtension::Stop()
{
    LOGINFO("Stopping LoggerExtension");
    if (!m_stopped)
    {
        m_stopped = true;
        m_extension->Stop();
        LOGINFO("LoggerExtension ExtensionInterface Stop Called");
        if (m_runnerThread && m_runnerThread->joinable())
        {
            LOGINFO("Joining LoggerExtension RunnerThread");
            m_runnerThread->join();
            LOGINFO("LoggerExtension Joined RunnerThread");
            m_runnerThread.reset();
        }
        LOGINFO("LoggerExtension::Stopped");
    }
}

void LoggerExtension::Run(std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGDEBUG("LoggerExtension running");
    m_extension->Wait();
    if (!m_stopped)
    {
        const auto healthCheckMessage = m_extension->GetHealthCheckFailureMessage();
        if (!healthCheckMessage.empty())
        {
            LOGWARN(healthCheckMessage);
        }

        LOGWARN("Service extension stopped unexpectedly. Calling reset.");
        extensionFinished->store(true);
    }
}

void LoggerExtension::setDataLimit(unsigned int limitBytes)
{
    LOGDEBUG("Setting Data Limit to " << limitBytes);
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

bool LoggerExtension::getDataLimitReached()
{
    return m_resultsSender.getDataLimitReached();
}
