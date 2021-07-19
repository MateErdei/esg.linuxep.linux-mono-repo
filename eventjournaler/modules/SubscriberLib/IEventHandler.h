/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IDataType.h>

namespace SubscriberLib
{
    class IEventQueuePusher
    {
    public:
        virtual ~IEventQueuePusher() = default;
        IEventQueuePusher() = default;

        /**
         * Pushes data on to the Event Queue
         * @param data to push
         */
        virtual void push(Common::ZeroMQWrapper::data_t event) = 0;
    };
} // namespace EventQueuePusher
