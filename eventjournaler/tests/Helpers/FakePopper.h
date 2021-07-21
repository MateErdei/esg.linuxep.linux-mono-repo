/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <optional>
#include <mutex>
#include <condition_variable>
#include "modules/WriterLib/IEventQueuePopper.h"

using namespace WriterLib;

class FakePopper : public WriterLib::IEventQueuePopper
{
public:
    FakePopper(Common::ZeroMQWrapper::data_t fakeData, int amountOfData = 1);
    std::optional<Common::ZeroMQWrapper::data_t> getEvent(int timeoutInMilliseconds) override;
    void setBlock(bool block);

private:
    std::vector<Common::ZeroMQWrapper::data_t> fake_eventQueue;
    bool m_block;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};
