// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "scan_messages/ThreatDetected.h"
#include "unixsocket/BaseClient.h"

#include <string>

namespace unixsocket
{
    class SafeStoreClientSocket : unixsocket::BaseClient
    {
    public:
        explicit SafeStoreClientSocket(std::string socket_path, const struct timespec& sleepTime = { 1, 0 });

        void sendQuarantineRequest(const scan_messages::ThreatDetected& detection);
    };
} // namespace unixsocket
