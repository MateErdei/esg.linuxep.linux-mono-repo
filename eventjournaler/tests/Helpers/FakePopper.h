// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/EventQueueLib/IEventQueuePopper.h"

#include <vector>

class FakePopper : public EventQueueLib::IEventQueuePopper
{
public:
    explicit FakePopper(const JournalerCommon::Event& fakeData, int amountOfData = 1);
    std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) override;

private:
    std::vector<JournalerCommon::Event> fake_eventQueue;
};
