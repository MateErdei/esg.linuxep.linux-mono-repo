// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <chrono>
#include <thread>

namespace unixsocket
{
    class IStoppableSleeper
    {
    public:
        using duration_t = std::chrono::nanoseconds;
        virtual ~IStoppableSleeper() = default;

        /**
         *
         * @param sleepTime
         * @return True if the sleep was stopped
         */
        virtual bool stoppableSleep(duration_t sleepTime)
        {
            std::this_thread::sleep_for(sleepTime);
            return false;
        }
    };
}