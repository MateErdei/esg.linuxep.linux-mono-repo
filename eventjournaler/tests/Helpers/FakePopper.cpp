// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "FakePopper.h"

#include <optional>
#include <mutex>
#include <thread>

using namespace EventQueueLib;

FakePopper::FakePopper(const JournalerCommon::Event& fakeData, int amountOfData, int perEventDelayMs)
    : perEventDelay_(perEventDelayMs)
{
    fake_eventQueue.reserve(amountOfData);
    for (int i = 0; i < amountOfData; i++)
    {
        fake_eventQueue.push_back(fakeData);
    }
}

std::optional<JournalerCommon::Event> FakePopper::pop(int timeoutInMilliseconds)
{
    (void)timeoutInMilliseconds;
    std::this_thread::sleep_for(std::chrono::milliseconds(perEventDelay_));

    if (fake_eventQueue.empty())
    {
        return std::nullopt;
    }

    JournalerCommon::Event result = std::move(fake_eventQueue.back());
    fake_eventQueue.pop_back();
    return result;
}
