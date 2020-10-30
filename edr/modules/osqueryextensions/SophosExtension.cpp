/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosExtension.h"
#include "Logger.h"
#include "SophosServerTable.h"

#include <functional>

SophosExtension::~SophosExtension()
{
    Stop();
}

void SophosExtension::Start(const std::string& socket, bool verbose)
{
    LOGINFO("Starting SophosExtension");


    if (m_stopped)
    {
        m_flags.socket = socket;
        m_flags.verbose = verbose;
        m_flags.interval = 3;
        m_flags.timeout = 3;
        m_extension = OsquerySDK::CreateExtension(m_flags, "SophosExtension", "1.0.0");

        m_extension->AddTablePlugin(std::make_unique<OsquerySDK::SophosServerTable>());
        LOGDEBUG("Extension Added");
        m_extension->Start();
        m_stopped = false;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { Run(); }));
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
        if (m_runnerThread)
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
    }
}

void SophosExtension::Run()
{
    LOGDEBUG("SophosExtension running");
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
        Start(m_flags.socket, m_flags.verbose);
    }
}
