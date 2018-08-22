/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <UpdateScheduler/ICronSchedulerThread.h>
#include <Common/Threads/AbstractThread.h>
#include <memory>

namespace UpdateSchedulerImpl
{

    // uses private inheritance to abstractThread because it wants to have a different meaning for requestStop and also to be able to
    // override the run method.
    class CronSchedulerThread
            : public virtual ICronSchedulerThread, private Common::Threads::AbstractThread
    {
    public:
        using DurationTime = ICronSchedulerThread::DurationTime;
        CronSchedulerThread( std::shared_ptr<SchedulerTaskQueue> schedulerQueue,  DurationTime firstTick, DurationTime repeatPeriod );
        ~CronSchedulerThread();

        void start() override;

        void requestStop() override;

        void reset() override;

        void setPeriodTime(DurationTime repeatPeriod);

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




