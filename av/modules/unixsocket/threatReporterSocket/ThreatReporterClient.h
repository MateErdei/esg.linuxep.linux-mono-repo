// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "unixsocket/BaseClient.h"

#include "scan_messages/ThreatDetected.h"

#include "datatypes/AutoFd.h"

#include <string>

namespace unixsocket
{
    class ThreatReporterClientSocket : unixsocket::BaseClient
    {
    public:
        explicit ThreatReporterClientSocket(std::string  socket_path, const struct timespec& sleepTime={1,0});

        void sendThreatDetection(const scan_messages::ThreatDetected& detection);

        void connectWithRetries(const std::string& socketName) override;
    };
}
