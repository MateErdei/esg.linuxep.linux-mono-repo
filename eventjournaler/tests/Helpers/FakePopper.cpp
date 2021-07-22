/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "FakePopper.h"
#include <optional>
#include <mutex>
#include "modules/WriterLib/IEventQueuePopper.h"

using namespace WriterLib;

FakePopper::FakePopper(Common::ZeroMQWrapper::data_t fakeData, int amountOfData)
{
    fake_eventQueue.reserve(amountOfData);
    m_block = true;
    for (int i = 0; i < amountOfData; i++)
    {
        fake_eventQueue.push_back(fakeData);
    }
}

std::optional<Common::ZeroMQWrapper::data_t> FakePopper::getEvent(int timeoutInMilliseconds)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait_for(lock, std::chrono::milliseconds(timeoutInMilliseconds), [this] { return !m_block; });

    if (m_block)
    {
        return std::nullopt;
    }

    Common::ZeroMQWrapper::data_t result = fake_eventQueue.back();
    fake_eventQueue.pop_back();
    return result;
}

void FakePopper::setBlock(bool block)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_block = block;
}

bool FakePopper::queueEmpty()
{
    return fake_eventQueue.empty();
}
