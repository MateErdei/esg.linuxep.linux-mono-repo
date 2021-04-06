/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>
#include <scan_messages/ThreatDetected.h>
#include "Common/Threads/NotifyPipe.h"
#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>
#include <unixsocket/IMessageCallback.h>
#include "unixsocket/BaseServerConnectionThread.h"

namespace unixsocket
{
    class ThreatReporterServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ThreatReporterServerConnectionThread(const ThreatReporterServerConnectionThread&) = delete;
        ThreatReporterServerConnectionThread& operator=(const ThreatReporterServerConnectionThread&) = delete;
        explicit ThreatReporterServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<IMessageCallback> threatReportCallback,
            std::shared_ptr<IMessageCallback> threatEventPublisherCallback);
        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<IMessageCallback> m_threatReportCallback;
        std::shared_ptr<IMessageCallback> m_threatEventPublisherCallback;
    };
}


