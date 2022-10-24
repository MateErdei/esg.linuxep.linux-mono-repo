// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "scan_messages/ThreatDetected.h"
#include "scan_messages/QuarantineResponse.h"
#include "unixsocket/BaseClient.h"

#include <string>

namespace unixsocket
{
    class SafeStoreClient : public unixsocket::BaseClient
    {
    public:
        explicit SafeStoreClient(std::string socket_path, const duration_t& sleepTime = DEFAULT_SLEEP_TIME, IStoppableSleeperSharedPtr sleeper={});

        void sendQuarantineRequest(const scan_messages::ThreatDetected& detection);
        scan_messages::QuarantineResult waitForResponse();
    };
} // namespace unixsocket
