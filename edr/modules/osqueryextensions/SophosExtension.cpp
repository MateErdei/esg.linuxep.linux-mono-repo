/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosExtension.h"
#include "Logger.h"
#include "SophosServerTable.h"
#include "SophosAVDetectionTable.h"

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
    if (!m_stopped && !m_stopping)
    {
        LOGINFO("Stopping SophosExtension");
        m_stopping = true;
        m_extension->Stop();
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        m_stopped = true;
        m_stopping = false;
        LOGINFO("SophosExtension::Stopped");
    }
}

void SophosExtension::Run(std::shared_ptr<std::atomic_bool> extensionFinished)
{
    LOGINFO("SophosExtension running");

    // Only run the extension if not in a stopping state, to prevent race condition, if stopping while starting
    if (!m_stopping)
    {
        m_extension->Wait();
    }

    if (!m_stopped && !m_stopping)
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

int SophosExtension::GetExitCode()
{
   return m_extension->GetReturnCode();
}
