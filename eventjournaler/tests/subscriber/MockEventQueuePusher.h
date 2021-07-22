/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "modules/SubscriberLib/IEventHandler.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace SubscriberLib;

class MockEventQueuePusher : public SubscriberLib::IEventHandler
{
public:
    MockEventQueuePusher() = default;
    MOCK_METHOD1(handleEvent, void(JournalerCommon::Event));
};
