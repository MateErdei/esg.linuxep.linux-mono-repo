// Copyright 2021-2023 Sophos Limited. All rights reserved.
#pragma once

#include "JournalerCommon/Event.h"

#include <optional>

namespace EventQueueLib
{
    class IEventQueuePopper
    {
    public:
        virtual ~IEventQueuePopper() = default;
        IEventQueuePopper() = default;
        IEventQueuePopper(IEventQueuePopper&) = delete;

        /**
         * Removes and returns data from the queue
         *
         * If queue is stopped will immediately return
         *
         * @param timeout to wait for queue to be populated
         * @return data if queue has any, nullopt otherwise
         */
        virtual std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) = 0;

        /**
         * Stop the queue, waking up and poppers, and not allowing them to wait
         */
        virtual void stop() = 0;

        /**
         * Restart the queue, allowing pop() to work as designed
         */
        virtual void restart() = 0;

    };
}
