/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosExtension.h"
#include "Logger.h"
#include "SophosServerTable.h"
#include "SophosAVDetectionTable.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <functional>

SophosExtension::~SophosExtension()
{
    // cppcheck-suppress virtualCallInConstructor
    Stop();
}

void SophosExtension::Start(const std::string& socket, bool verbose, std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGINFO("Starting SophosExtension");


    if (m_stopped)
    {
        m_flags.socket = socket;
        m_flags.verbose = verbose;
        m_flags.interval = 3;
        m_flags.timeout = 3;
        m_extension = OsquerySDK::CreateExtension(m_flags, "SophosExtension", "1.0.0");

        LOGDEBUG("Adding Sophos Server Table");
        m_extension->AddTablePlugin(std::make_unique<OsquerySDK::SophosServerTable>());
        LOGDEBUG("Adding Sophos Detections Table");
        m_extension->AddTablePlugin(std::make_unique<OsquerySDK::SophosAVDetectionTable>());
        LOGDEBUG("Extension Added");
        m_extension->Start();


        m_stopped = false;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this, extensionFinished] { Run(extensionFinished); }));
        LOGDEBUG("Sophos Extension running in thread");
    }
}

void SophosExtension::Stop()
{
    if (!m_stopped)
    {
        LOGINFO("Stopping SophosExtension");
        m_stopped = true;
        m_extension->Stop();
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        LOGINFO("SophosExtension::Stopped");
    }
}

void SophosExtension::Run(std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGINFO("SophosExtension running");
    m_extension->Wait();
    if (!m_stopped)
    {
        const auto healthCheckMessage = m_extension->GetHealthCheckFailureMessage();
        if (!healthCheckMessage.empty())
        {
            LOGWARN(healthCheckMessage);
        }

        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.increment("sophos-extension-restarts", 1L);
        LOGWARN("Service extension stopped unexpectedly. Calling reset.");
        extensionFinished->store(true);
    }
}
