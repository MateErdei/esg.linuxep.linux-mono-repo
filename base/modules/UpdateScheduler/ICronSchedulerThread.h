/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SchedulerTaskQueue.h"
#include <chrono>

namespace UpdateSchedulerImpl
{

    class ICronSchedulerThread
    {
    public:
        using DurationTime = std::chrono::milliseconds;

        virtual ~ICronSchedulerThread() = default;

        virtual void start() = 0;

        virtual void requestStop() = 0;

        virtual void reset() = 0;

        virtual void setPeriodTime(DurationTime repeatPeriod) = 0;
    };
}