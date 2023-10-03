// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "EventQueueLib/IEventQueue.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventQueueLib;

namespace
{
    class MockEventQueue : public EventQueueLib::IEventQueue
    {
    public:
        MOCK_METHOD(bool, push, (JournalerCommon::Event), (override));
        MOCK_METHOD(std::optional<JournalerCommon::Event>, pop, (int), (override));
        MOCK_METHOD(void, stop, (), (override));
        MOCK_METHOD(void, restart, (), (override));
    };
}
