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
        explicit ThreatReporterClientSocket(const std::string& socket_path);
        ~ThreatReporterClientSocket() = default;

        void sendThreatDetection(scan_messages::ThreatDetected detection);
    private:
        datatypes::AutoFd m_socket_fd;

    };
}
