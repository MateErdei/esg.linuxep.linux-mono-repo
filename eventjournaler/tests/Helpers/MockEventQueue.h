/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventQueueLib;

class MockEventQueue : public EventQueueLib::IEventQueue
{
public:
    MockEventQueue() = default;
    MOCK_METHOD1(push, bool(Common::ZeroMQWrapper::data_t));
    MOCK_METHOD1(pop, std::optional<Common::ZeroMQWrapper::data_t>(int));
};
