/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakePopper.h"
#include <optional>
#include <mutex>
#include <thread>
#include "modules/EventWriterWorkerLib/IEventQueuePopper.h"

using namespace EventWriterLib;

FakePopper::FakePopper(JournalerCommon::Event fakeData, int amountOfData)
{
    fake_eventQueue.reserve(amountOfData);
    for (int i = 0; i < amountOfData; i++)
    {
        fake_eventQueue.push_back(fakeData);
    }
}

std::optional<JournalerCommon::Event> FakePopper::getEvent(int timeoutInMilliseconds)
{
    (void)timeoutInMilliseconds;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (fake_eventQueue.empty())
    {
        return std::nullopt;
    }

    JournalerCommon::Event result = fake_eventQueue.back();
    fake_eventQueue.pop_back();
    return result;
}
