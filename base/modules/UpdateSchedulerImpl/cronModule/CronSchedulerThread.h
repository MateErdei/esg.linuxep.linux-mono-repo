/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Threads/AbstractThread.h>
#include <UpdateScheduler/ICronSchedulerThread.h>
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
                int scheduledUpdateOffsetInMinutes = 8,
                DurationTime onDelayUpdateWaitTime = std::chrono::minutes(15));

            ~CronSchedulerThread() override;

            void start() override;

            // cppcheck-suppress virtualCallInConstructor
            void requestStop() override;

            void reset() override;

            void setPeriodTime(DurationTime repeatPeriod) override;

            void setUpdateOnStartUp(bool updateOnStartUp) override;
            void join(); // for test
        private:
            void run() override;
            enum class ActionOnInterrupt
            {
                NOTHING,
                RESET,
                STOP
            };

            struct CrossThreadState
            {
                DurationTime m_periodTick;
                bool m_updateOnStartUp;
                bool m_changed;
            };

            std::chrono::milliseconds getPeriodTick();

            ActionOnInterrupt getActionOnInterruptAndReset();

            std::mutex m_sharedState;
            std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_schedulerQueue;
            DurationTime m_firstTick;
            DurationTime m_onDelayUpdateWaitTime;
            ActionOnInterrupt m_actionOnInterrupt;
            int m_scheduledUpdateOffsetInMinutes;

            void updateInThreadState();
            CrossThreadState m_crossThreadState;
            CrossThreadState m_inThreadState;
        };
    } // namespace cronModule

} // namespace UpdateSchedulerImpl
