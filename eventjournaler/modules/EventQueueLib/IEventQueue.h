/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "Common/ZeroMQWrapper/IReadable.h"

#include <modules/JournalerCommon/Event.h>

#include <condition_variable>
#include <optional>
#include <queue>

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
        virtual bool push(JournalerCommon::Event event) = 0;

        /**
         * Removes and returns data from the queue
         * @param timeout to wait for queue to be populated
         * @return data if queue has any, nullopt otherwise
         */
        virtual std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) = 0;
    };
}
