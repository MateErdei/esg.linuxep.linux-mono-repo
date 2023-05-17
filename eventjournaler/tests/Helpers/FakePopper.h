// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/EventWriterWorkerLib/IEventQueuePopper.h"

#include <vector>


using namespace EventWriterLib;

class FakePopper : public EventWriterLib::IEventQueuePopper
{
public:
    explicit FakePopper(JournalerCommon::Event fakeData, int amountOfData = 1);
    std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) override;

private:
    std::vector<JournalerCommon::Event> fake_eventQueue;
};
