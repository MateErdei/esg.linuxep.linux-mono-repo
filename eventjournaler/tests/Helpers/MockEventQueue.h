// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/EventQueueLib/IEventQueue.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventQueueLib;

namespace
{
    class MockEventQueue : public EventQueueLib::IEventQueue
    {
    public:
        MockEventQueue() = default;
        MOCK_METHOD1(push, bool(JournalerCommon::Event));
        MOCK_METHOD1(pop, std::optional<JournalerCommon::Event>(int));
    };
}
