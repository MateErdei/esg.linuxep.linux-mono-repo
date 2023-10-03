// Copyright 2021 Sophos Limited. All rights reserved.
#pragma once

#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/IDataType.h"

#include "JournalerCommon/Event.h"

namespace SubscriberLib
{
    class IEventHandler
    {
    public:
        virtual ~IEventHandler() = default;
        IEventHandler() = default;
        /**
         * Pushes data on to the Event Queue
         * @param data to push
         */
        virtual void handleEvent(JournalerCommon::Event event) = 0;
    };
} // namespace SubscriberLib
