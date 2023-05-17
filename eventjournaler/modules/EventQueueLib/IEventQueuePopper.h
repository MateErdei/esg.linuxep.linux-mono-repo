// Copyright 2021-2023 Sophos Limited. All rights reserved.
#pragma once

#include <modules/JournalerCommon/Event.h>

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
         * @param timeout to wait for queue to be populated
         * @return data if queue has any, nullopt otherwise
         */
        virtual std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) = 0;
    };
}
