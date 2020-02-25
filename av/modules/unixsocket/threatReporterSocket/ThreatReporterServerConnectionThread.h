/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>
#include <scan_messages/ThreatDetected.h>
#include "Common/Threads/NotifyPipe.h"

#include <cstdint>
#include <string>

namespace unixsocket
{
    class ThreatReporterServerConnectionThread
    {
    public:
        ThreatReporterServerConnectionThread(const ThreatReporterServerConnectionThread&) = delete;
        ThreatReporterServerConnectionThread& operator=(const ThreatReporterServerConnectionThread&) = delete;
        explicit ThreatReporterServerConnectionThread(int fd);
        void run();
        void notifyTerminate();
        bool finished();
        void start();

    private:
        bool m_finished;
        datatypes::AutoFd m_fd;
        Common::Threads::NotifyPipe m_terminationPipe;
    };
}


