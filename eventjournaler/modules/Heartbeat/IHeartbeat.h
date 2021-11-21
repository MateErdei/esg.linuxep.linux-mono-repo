/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <string>
#include <vector>
#include "HeartbeatPinger.h"

namespace Heartbeat
{
    class IHeartbeat
    {
    public:
        virtual ~IHeartbeat() = default;
        virtual void registerId(const std::string& id) = 0;
        virtual void deregisterId(const std::string& id) = 0;

        virtual HeartbeatPinger getPingHandleForId(const std::string& id) = 0;
        virtual void resetAll() = 0;
        virtual std::vector<std::string> getMissedHeartbeats() = 0;
    };
} // namespace SubscriberLib