// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once
#ifdef SPL_BAZEL
#include "EventQueueLib//IEventQueuePopper.h"
#else
#include "modules/EventQueueLib//IEventQueuePopper.h"
#endif

#include <gmock/gmock.h>

using namespace ::testing;

class MockEventQueuePopper : public EventQueueLib::IEventQueuePopper
{
public:
    MOCK_METHOD(std::optional<JournalerCommon::Event>, pop, (int), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, restart, (), (override));
};
