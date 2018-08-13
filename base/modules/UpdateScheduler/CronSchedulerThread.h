/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "SchedulerTaskQueue.h"
#include <Common/Threads/AbstractThread.h>
#include <memory>

namespace UpdateScheduler
{

    // uses private inheritance to abstractThread because it wants to have a different meaning for requestStop and also to be able to
    // override the run method.
    class CronSchedulerThread : private Common::Threads::AbstractThread
    {
    public:
        using DurationTime = std::chrono::duration<int64_t, std::milli>;
        CronSchedulerThread( std::shared_ptr<SchedulerTaskQueue> schedulerQueue,  DurationTime firstTick, DurationTime repeatPeriod );
        ~CronSchedulerThread();

        // inherits the start from the Common::Threads::AbstractThread
        using Common::Threads::AbstractThread::start;

        void requestStop();
        void reset();
        void resetTime( DurationTime repeatPeriod);

    private:
        void run() override ;
        enum  class ActionOnInterrupt{NOTHING, RESET, STOP};

        std::chrono::milliseconds getPeriodTick() ;
        ActionOnInterrupt getActionOnInterruptAndReset();

        std::mutex m_sharedState;
        std::shared_ptr<SchedulerTaskQueue> m_schedulerQueue;
        DurationTime m_firstTick;
        DurationTime m_periodTick;
        ActionOnInterrupt m_actionOnInterrupt;
    };
}




