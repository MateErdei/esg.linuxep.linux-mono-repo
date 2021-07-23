/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "modules/EventWriterWorkerLib/IEventQueuePopper.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventWriterLib;

class MockEventQueuePopper : public EventWriterLib::IEventQueuePopper
{
public:
    MockEventQueuePopper() = default;
    MOCK_METHOD1(getEvent, std::optional<JournalerCommon::Event>(int));
};
