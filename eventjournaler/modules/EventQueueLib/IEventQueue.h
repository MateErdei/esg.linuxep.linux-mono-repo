// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IEventQueuePopper.h"

namespace EventQueueLib
{
    class IEventQueue : public IEventQueuePopper
    {
    public:
        /**
         * Pushes data on to the queue
         * @param data to push
         * @return true, if push successful, false otherwise (caused by full queue)
         */
        virtual bool push(JournalerCommon::Event event) = 0;
    };
}
