/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IDataType.h>

namespace SubscriberLib
{
    class IEventQueuePopper
    {
    public:
        virtual ~IEventQueuePopper() = default;
        IEventQueuePopper() = default;
        virtual std::optional<Common::ZeroMQWrapper::data_t> getEvent(int timeoutInMilliseconds) = 0;
    };
} // namespace EventQueuePopper
