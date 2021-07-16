/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <queue>
#include <condition_variable>
#include "Common/ZeroMQWrapper/IReadable.h"

namespace EventQueueLib
{
    class IEventQueue
    {
    public:
        virtual ~IEventQueue() = default;
        IEventQueue() = default;
        virtual bool push(Common::ZeroMQWrapper::data_t event) = 0;
        virtual std::optional<Common::ZeroMQWrapper::data_t> pop(int timeoutInMilliseconds) = 0;
    };
}
