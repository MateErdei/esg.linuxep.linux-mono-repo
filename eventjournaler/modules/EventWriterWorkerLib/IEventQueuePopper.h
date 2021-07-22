/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IDataType.h>
#include <JournalerCommon/Event.h>
#include <optional>

namespace EventWriterLib
{
    class IEventQueuePopper
    {
    public:
        virtual ~IEventQueuePopper() = default;
        IEventQueuePopper() = default;

        /**
         * Gets the next event from the Event Queue
         * @param timeout to wait for queue to be populated
         * @return data if queue has any, nullptr otherwise
         */
        virtual std::optional<JournalerCommon::Event> getEvent(int timeoutInMilliseconds) = 0;
    };
} // namespace EventQueuePopper
