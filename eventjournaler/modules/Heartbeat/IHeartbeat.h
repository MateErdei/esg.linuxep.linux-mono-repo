// Copyright 2021 Sophos Limited. All rights reserved.
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
        virtual void deregisterId(const std::string& id) = 0;
        virtual void registerIds(std::vector<std::string> ids) = 0;
        virtual std::shared_ptr<HeartbeatPinger> getPingHandleForId(const std::string& id) = 0;
        virtual std::vector<std::string> getAllHeartbeatIds() = 0;
        virtual uint getNumDroppedEventsInLast24h() = 0;
        virtual std::map<std::string, bool> getMapOfIdsAgainstIsAlive() = 0;
    };
} // namespace SubscriberLib