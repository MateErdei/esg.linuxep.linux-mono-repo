/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IDataType.h>

namespace WriterLib
{
    class IEventQueuePopper
    {
    public:
        virtual ~IEventQueuePopper() = default;
        IEventQueuePopper() = default;

        /**
         * Gets the next event from the Event Queue
         * @param timeout to wait for queue to be populated in milliseconds, defaults to 1000
         * @return data if queue has any, nullptr otherwise
         */
         virtual std::optional<Common::ZeroMQWrapper::data_t> getEvent(int timeoutInMilliseconds = 1000) = 0;
    };
} // namespace EventQueuePopper