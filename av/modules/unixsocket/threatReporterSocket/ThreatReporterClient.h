/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ThreatDetected.h"

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"

#include <string>

namespace unixsocket
{
    class ThreatReporterClientSocket
    {
    public:
        ThreatReporterClientSocket& operator=(const ThreatReporterClientSocket&) = delete;
        ThreatReporterClientSocket(const ThreatReporterClientSocket&) = delete;
        explicit ThreatReporterClientSocket(std::string  socket_path, const struct timespec& sleepTime={1,0});
        ~ThreatReporterClientSocket() = default;

        void sendThreatDetection(const scan_messages::ThreatDetected& detection);
    private:
        int attemptConnect();
        void connect();

        datatypes::AutoFd m_socket_fd;
        std::string m_socketPath;
        const timespec &m_sleepTime;

    };
}
