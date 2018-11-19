/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SchedulerTaskQueue.h"
#include "ScheduledUpdate.h"
#include <chrono>

namespace UpdateScheduler
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

        virtual void setScheduledUpdate(ScheduledUpdate scheduledUpdate) = 0;

        virtual void setUpdateOnStartUp(bool updateOnStartUp) = 0;
    };
}