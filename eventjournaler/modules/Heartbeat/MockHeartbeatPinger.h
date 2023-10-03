// Copyright 2021 Sophos Limited. All rights reserved.
#pragma once
#include "HeartbeatPinger.h"
#include <map>
#include <memory>
#include <set>
#include <gmock/gmock.h>

namespace Heartbeat
{
    class MockHeartbeatPinger : public HeartbeatPinger
    {
    public:
        MOCK_METHOD0(ping, void());
        MOCK_METHOD0(pushDroppedEvent, void());
        MOCK_METHOD1(setDroppedEventsMax, void(uint newMax));
    };

} // namespace Heartbeat