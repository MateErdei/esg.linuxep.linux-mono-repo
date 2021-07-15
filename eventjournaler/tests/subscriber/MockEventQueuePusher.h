/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "modules/SubscriberLib/IEventQueuePusher.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace SubscriberLib;

class MockEventQueuePusher : public SubscriberLib::IEventQueuePusher
{
public:
    MockEventQueuePusher() = default;
    MOCK_METHOD1(push, void(Common::ZeroMQWrapper::data_t));
};
