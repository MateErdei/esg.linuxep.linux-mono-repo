/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <modules/JournalerCommon/Event.h>
#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventQueueLib;

class MockEventQueue : public EventQueueLib::IEventQueue
{
public:
    MockEventQueue() = default;
    MOCK_METHOD1(push, bool(JournalerCommon::Event));
    MOCK_METHOD1(pop, std::optional<JournalerCommon::Event>(int));
};
