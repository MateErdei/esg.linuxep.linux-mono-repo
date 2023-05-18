// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/EventQueueLib//IEventQueuePopper.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventWriterLib;

class MockEventQueuePopper : public EventQueueLib::IEventQueuePopper
{
public:
    MOCK_METHOD(std::optional<JournalerCommon::Event>, pop, (int), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, restart, (), (override));
};
