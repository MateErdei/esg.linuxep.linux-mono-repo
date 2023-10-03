// Copyright 2021 Sophos Limited. All rights reserved.
#pragma once

#include "IHeartbeat.h"
#include <map>
#include <gmock/gmock.h>

namespace Heartbeat
{
    class MockHeartbeat : public IHeartbeat
    {
    public:
        MOCK_METHOD1(deregisterId, void(const std::string& id));
        MOCK_METHOD1(registerIds, void(std::vector<std::string> ids));

        MOCK_METHOD1(getPingHandleForId, std::shared_ptr<HeartbeatPinger>(const std::string& id));
        MOCK_METHOD0(getAllHeartbeatIds, std::vector<std::string>());
        MOCK_METHOD0(getNumDroppedEventsInLast24h, uint());
        MOCK_METHOD0(getMapOfIdsAgainstIsAlive, std::map<std::string, bool>());
    };
} // namespace Heartbeat