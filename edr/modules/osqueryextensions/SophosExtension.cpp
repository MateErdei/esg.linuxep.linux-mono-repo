// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "SophosExtension.h"
#include "Logger.h"
#include "SophosServerTable.h"
#include "SophosAVDetectionTable.h"
#include "GrepTable.h"
#include "HexToIntTable.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <functional>

SophosExtension::~SophosExtension()
{
    // cppcheck-suppress virtualCallInConstructor
    Stop(SVC_EXT_STOP_TIMEOUT);
}

void SophosExtension::Start(const std::string& socket, bool verbose, std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGINFO("Starting SophosExtension");

    if (m_stopped)
    {
        auto oldExtension = accessExtension();
        if (oldExtension)
        {
            LOGDEBUG("Stopping old SophosExtension");
            oldExtension->Stop();
            waitForExtension(oldExtension);
            oldExtension.reset();
            resetExtension();
        }

        m_flags.socket = socket;
        m_flags.verbose = verbose;
        m_flags.interval = 3;
        m_flags.timeout = 3;

        LOGDEBUG("Creating SophosExtension");
        auto newExtension = resetExtension(
                OsquerySDK::CreateExtension(m_flags, "SophosExtension", "1.0.0")
        );

        LOGDEBUG("Adding Sophos Server Table");
        newExtension->AddTablePlugin(std::make_unique<OsquerySDK::SophosServerTable>());
        LOGDEBUG("Adding Sophos Detections Table");
        newExtension->AddTablePlugin(std::make_unique<OsquerySDK::SophosAVDetectionTable>());
        LOGDEBUG("Adding Grep Table");
        newExtension->AddTablePlugin(std::make_unique<OsquerySDK::GrepTable>());
        LOGDEBUG("Adding HexToInt Table");
        newExtension->AddTablePlugin(std::make_unique<OsquerySDK::HexToIntTable>());
        LOGDEBUG("Extension Added");
        newExtension->Start();
        m_stopped = false;
        m_runnerThread = std::make_unique<boost::thread>(boost::thread([this, extensionFinished] { Run(extensionFinished); }));
        LOGDEBUG("Sophos Extension running in thread");
    }
    else
    {
        LOGDEBUG("SophosExtension Start() called when extension not stopped");
    }
}

void SophosExtension::Stop(long timeoutSeconds)
{
    if (!m_stopped && !stopping())
    {
        LOGINFO("Stopping SophosExtension");
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
        LOGINFO("SophosExtension::Stopped");
    }
}

void SophosExtension::Run(const std::shared_ptr<std::atomic_bool>& extensionFinished)
{
    LOGINFO("SophosExtension running");
    auto extension = accessExtension();
    if (extension == nullptr)
    {
        LOGERROR("Tried to run before initialising SophosExtension");
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
            LOGWARN("Service extension stopped unexpectedly due to failed osquery health check: " << healthCheckMessage);
        }
        else
        {
            LOGWARN("Service extension stopped unexpectedly");
        }
        extensionFinished->store(true);
    }
}

int SophosExtension::GetExitCode()
{
    auto locked = m_extension.lock();
    if (locked->get())
    {
        return locked->get()->GetReturnCode();
    }
    return -1;
}

SophosExtension::ExtensionTypePtr SophosExtension::accessExtension()
{
    auto locked = m_extension.lock();
    return *locked;
}

SophosExtension::ExtensionTypePtr SophosExtension::resetExtension(SophosExtension::ExtensionTypePtr extension)
{
    auto locked = m_extension.lock();
    *locked = std::move(extension);
    return *locked;
}

void SophosExtension::resetExtension()
{
    auto locked = m_extension.lock();
    locked->reset();
}

void SophosExtension::waitForExtension(ExtensionTypePtr& extension)
{
    std::lock_guard locked(waitForExtensionLock_);
    if (extension)
    {
        extension->Wait();
    }
}
