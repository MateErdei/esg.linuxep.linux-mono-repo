// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "LoggerExtension.h"
#include "Logger.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/FileSystem/IFileSystemException.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/LoggerPlugin/SophosLoggerPlugin.h"
#else
#include "LoggerPlugin/SophosLoggerPlugin.h"
#endif

#include <cassert>
#include <functional>
#include <utility>



LoggerExtension::LoggerExtension(
        const std::string& intermediaryPath,
        const std::string& datafeedPath,
        const std::string& osqueryXDRConfigFilePath,
        const std::string& osqueryMTRConfigFilePath,
        const std::string& osqueryCustomConfigFilePath,
        const std::string& pluginVarDir,
        unsigned long long int dataLimit,
        unsigned int periodInSeconds,
        std::function<void(void)> dataExceededCallback,
        unsigned int maxBatchSeconds,
        uintmax_t maxBatchBytes) :
    m_resultsSender(
        intermediaryPath,
        datafeedPath,
        osqueryXDRConfigFilePath,
        osqueryMTRConfigFilePath,
        osqueryCustomConfigFilePath,
        pluginVarDir,
        dataLimit,
        periodInSeconds,
        std::move(dataExceededCallback)),
    m_maxBatchBytes(maxBatchBytes),
    m_maxBatchSeconds(maxBatchSeconds)
{
    m_flags.interval = 3;
    m_flags.timeout = 3;
}

LoggerExtension::~LoggerExtension()
{
    // cppcheck-suppress virtualCallInConstructor
    Stop(SVC_EXT_STOP_TIMEOUT);
}

void LoggerExtension::Start(
    const std::string& socket,
    bool verbose,
    std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGINFO("Starting LoggerExtension");
    if (m_stopped)
    {
        auto oldExtension = accessExtension();
        if (oldExtension)
        {
            LOGDEBUG("Stopping old LoggerExtension");
            oldExtension->Stop();
            waitForExtension(oldExtension);
            oldExtension.reset();
            resetExtension();
        }

        m_flags.socket = socket;
        m_flags.verbose = verbose;

        LOGDEBUG("Creating LoggerExtension");
        auto newExtension = resetExtension(
                OsquerySDK::CreateExtension(m_flags, "SophosLoggerPlugin", "1.1.0.1")
        );

        newExtension->AddLoggerPlugin(
            std::make_unique<SophosLoggerPlugin>(m_resultsSender, m_foldingRules, m_batchTimer, m_maxBatchBytes, std::chrono::seconds(m_maxBatchSeconds)));
        LOGDEBUG("Logger Plugin Added");
        newExtension->Start();
        m_stopped = false;
        m_runnerThread = std::make_unique<boost::thread>(boost::thread([this, extensionFinished] { Run(extensionFinished); }));
        LOGDEBUG("Logger Plugin running in thread");
    }
    else
    {
        LOGDEBUG("LoggerExtension Start() called when extension not stopped");
    }
}

void LoggerExtension::Stop(long timeoutSeconds)
{
    if (!m_stopped && !stopping())
    {
        LOGINFO("Stopping LoggerExtension");
        stopping(true);

        auto extension = accessExtension();
        if (extension)
        {
            extension->Stop();
        }
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->timed_join(boost::posix_time::seconds(timeoutSeconds));
            m_runnerThread.reset();
        }
        waitForExtension(extension);
        extension.reset(); // reset our local variable
        resetExtension(); // reset the instance variable
        m_stopped = true;
        stopping(false);
        LOGINFO("LoggerExtension::Stopped");
    }
}

void LoggerExtension::reloadTags()
{
    try
    {
        m_resultsSender.loadScheduledQueryTags();
    }
    catch (const Common::FileSystem::IFileSystemException& e)
    {
        LOGWARN("Failed to load scheduled query tags on start up of Logger extension. Error: " << e.what_with_location());
    }
    catch (const std::runtime_error& e)
    {
        LOGWARN("Failed to load scheduled query tags on start up of Logger extension. Error: " <<  e.what());
    }
}

void LoggerExtension::Run(const std::shared_ptr<std::atomic_bool>& extensionFinished)
{
    assert(extensionFinished);
    try
    {
        LOGINFO("LoggerExtension running");
        auto extension = accessExtension();
        if (extension == nullptr)
        {
            LOGERROR("Tried to run before initialising LoggerExtension");
            return;
        }
        // Only run the extension if not in a stopping state, to prevent race condition, if stopping while starting
        if (!stopping())
        {
            waitForExtension(extension);
        }

        if (!m_stopped && !stopping())
        {
            const auto healthCheckMessage = extension->GetHealthCheckFailureMessage();
            if (!healthCheckMessage.empty())
            {
                // A health check failure message indicates that osquery failed to reply to the extension in good time.
                LOGWARN("Logger extension stopped unexpectedly due to failed osquery health check: " << healthCheckMessage);
            }
            else
            {
                LOGWARN("Logger extension stopped unexpectedly");
            }
            extensionFinished->store(true);
        }
    }
    catch (const Common::Exceptions::IException& ex)
    {
        LOGFATAL("LoggerException failed with IException: " << ex.what_with_location());
        extensionFinished->store(true);
    }
    catch (const std::exception& ex)
    {
        LOGFATAL("LoggerException failed with exception: " << ex.what());
        extensionFinished->store(true);
    }
}

void LoggerExtension::setDataLimit(unsigned long long int limitBytes)
{
    LOGDEBUG("Setting Data Limit to " << limitBytes);
    m_resultsSender.setDataLimit(limitBytes);
}

void LoggerExtension::setMTRLicense(bool hasMTRLicense)
{
    m_resultsSender.setMtrLicense(hasMTRLicense);
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

unsigned long long int LoggerExtension::getDataLimit()
{
    return m_resultsSender.getDataLimit();
}

void LoggerExtension::setFoldingRules(const std::vector<Json::Value>& foldingRules)
{
    m_foldingRules = foldingRules;
}

std::vector<Json::Value> LoggerExtension::getCurrentFoldingRules()
{
    return m_foldingRules;
}

bool LoggerExtension::compareFoldingRules(const std::vector<Json::Value>& newFoldingRules)
{
    return m_foldingRules != newFoldingRules;
}

std::vector<std::string> LoggerExtension::getFoldableQueries() const
{
    std::vector<std::string> queries;

    try
    {
        for (const auto& foldingRule : m_foldingRules)
        {
            queries.push_back(foldingRule["query_name"].asString());
        }
    }
    catch (const std::exception& exception)
    {
        LOGWARN("Failed to extract query name from folding rules. Error: " << exception.what());
    }

    return queries;
}

int LoggerExtension::GetExitCode()
{
    auto locked = m_extension.lock();
    if (locked->get())
    {
        return locked->get()->GetReturnCode();
    }
    return -1;
}

LoggerExtension::ExtensionTypePtr LoggerExtension::accessExtension()
{
    auto locked = m_extension.lock();
    return *locked;
}

LoggerExtension::ExtensionTypePtr LoggerExtension::resetExtension(LoggerExtension::ExtensionTypePtr extension)
{
    auto locked = m_extension.lock();
    *locked = std::move(extension);
    return *locked;
}

void LoggerExtension::resetExtension()
{
    auto locked = m_extension.lock();
    locked->reset();
}

void LoggerExtension::waitForExtension(ExtensionTypePtr& extension)
{
    std::lock_guard locked(waitForExtensionLock_);
    if (extension)
    {
        extension->Wait();
    }
}
