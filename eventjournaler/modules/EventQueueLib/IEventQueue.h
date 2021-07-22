/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "Common/ZeroMQWrapper/IReadable.h"

#include <modules/JournalerCommon/Event.h>

#include <condition_variable>
#include <queue>

namespace EventQueueLib
{
    class IEventQueue
    {
    public:
        virtual ~IEventQueue() = default;
        IEventQueue() = default;
        virtual bool push(JournalerCommon::Event event) = 0;
        virtual std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) = 0;
    };
}
