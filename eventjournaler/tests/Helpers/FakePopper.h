// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "EventQueueLib/IEventQueuePopper.h"

#include <vector>

class FakePopper : public EventQueueLib::IEventQueuePopper
{
public:
    explicit FakePopper(const JournalerCommon::Event& fakeData, int amountOfData = 1, int perEventDelayMs = 500);
    std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) override;

    void stop() override { active_ = false; }

    void restart() override { active_ = true; }

    bool active_{ true };
    int perEventDelay_;

private:
    std::vector<JournalerCommon::Event> fake_eventQueue;
};
