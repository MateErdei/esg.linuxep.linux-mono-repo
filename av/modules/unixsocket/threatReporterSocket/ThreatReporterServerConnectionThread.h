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

namespace unixsocket
{
    class ThreatReporterServerConnectionThread : public Common::Threads::AbstractThread
    {
    public:
        ThreatReporterServerConnectionThread(const ThreatReporterServerConnectionThread&) = delete;
        ThreatReporterServerConnectionThread& operator=(const ThreatReporterServerConnectionThread&) = delete;
        explicit ThreatReporterServerConnectionThread(int fd, std::shared_ptr<IMessageCallback> callback);
        void run() override;

    private:
        datatypes::AutoFd m_fd;
        std::shared_ptr<IMessageCallback> m_callback;
    };
}


