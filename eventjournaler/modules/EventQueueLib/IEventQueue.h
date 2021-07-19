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

        /**
         * Pushes data on to the queue
         * @param data to push
         * @return true, if push successful, false otherwise (caused by full queue)
         */
        virtual bool push(Common::ZeroMQWrapper::data_t event) = 0;

        /**
         * Removes and returns data from the queue
         * @param timeout to wait for queue to be populated
         * @return data if queue has any, nullopt otherwise
         */
        virtual std::optional<Common::ZeroMQWrapper::data_t> pop(int timeoutInMilliseconds) = 0;
    };
}
