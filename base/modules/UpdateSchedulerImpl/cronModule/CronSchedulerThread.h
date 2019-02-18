/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Threads/AbstractThread.h>
#include <UpdateScheduler/ICronSchedulerThread.h>
#include <UpdateScheduler/ScheduledUpdate.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

#include <memory>

namespace UpdateSchedulerImpl
{
    namespace cronModule
    {
        // uses private inheritance to abstractThread because it wants to have a different meaning for requestStop and
        // also to be able to override the run method.
        class CronSchedulerThread : public virtual UpdateScheduler::ICronSchedulerThread,
                                    private Common::Threads::AbstractThread
        {
        public:
            using DurationTime = UpdateScheduler::ICronSchedulerThread::DurationTime;

            CronSchedulerThread(
                std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> schedulerQueue,
                DurationTime firstTick,
                DurationTime repeatPeriod,
                int scheduledUpdateOffsetInMinutes = 8);

            ~CronSchedulerThread();

            void start() override;

            void requestStop() override;

            void reset() override;

            void setPeriodTime(DurationTime repeatPeriod) override;

            void setScheduledUpdate(UpdateScheduler::ScheduledUpdate scheduledUpdate) override;

            void setUpdateOnStartUp(bool updateOnStartUp) override;

        private:
            void run() override;

            enum class ActionOnInterrupt
            {
                NOTHING,
                RESET,
                STOP
            };

            std::chrono::milliseconds getPeriodTick();

            ActionOnInterrupt getActionOnInterruptAndReset();

            std::mutex m_sharedState;
            std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_schedulerQueue;
            DurationTime m_firstTick;
            DurationTime m_periodTick;
            ActionOnInterrupt m_actionOnInterrupt;
            UpdateScheduler::ScheduledUpdate m_scheduledUpdate;
            int m_scheduledUpdateOffsetInMinutes;
            bool m_updateOnStartUp;
        };
    } // namespace cronModule

} // namespace UpdateSchedulerImpl
