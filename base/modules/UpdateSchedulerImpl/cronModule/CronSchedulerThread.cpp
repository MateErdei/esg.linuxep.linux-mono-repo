/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CronSchedulerThread.h"
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>
#include <cassert>

namespace UpdateSchedulerImpl
{
    namespace cronModule
    {

        using namespace UpdateScheduler;

        CronSchedulerThread::CronSchedulerThread(std::shared_ptr<SchedulerTaskQueue> schedulerQueue,
                                                 CronSchedulerThread::DurationTime firstTick,
                                                 CronSchedulerThread::DurationTime repeatPeriod)
                :
                m_sharedState()
                , m_schedulerQueue(schedulerQueue)
                , m_firstTick(firstTick)
                , m_periodTick(repeatPeriod)
                , m_actionOnInterrupt(ActionOnInterrupt::NOTHING)
        {

        }

        CronSchedulerThread::~CronSchedulerThread()
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            m_actionOnInterrupt = ActionOnInterrupt::STOP;
        }


        void CronSchedulerThread::requestStop()
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            m_actionOnInterrupt = ActionOnInterrupt::STOP;
            Common::Threads::AbstractThread::requestStop();
        }

        void CronSchedulerThread::reset()
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            m_actionOnInterrupt = ActionOnInterrupt::RESET;
            Common::Threads::AbstractThread::requestStop();
        }

        void CronSchedulerThread::setPeriodTime(CronSchedulerThread::DurationTime repeatPeriod)
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            m_periodTick = repeatPeriod;
        }

        void CronSchedulerThread::run()
        {
            std::chrono::milliseconds timeToWait = m_firstTick;
            announceThreadStarted();
            auto poller = Common::ZeroMQWrapper::createPoller();

            auto pipePollerEntry = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);


            while (true)
            {
                using poll_result_t = Common::ZeroMQWrapper::IPoller::poll_result_t;
                poll_result_t poll_result;
                try
                {
                    poll_result = poller->poll(timeToWait);
                }
                catch (Common::ZeroMQWrapperImpl::ZeroMQPollerException & ex)
                {
                    // the poller will not work anymore and the CronSchedulerThread must stop.
                    // This may happen on shutdown.
                    break;
                }

                timeToWait = getPeriodTick();

                if (poll_result.empty())
                {
                    // timeout means a new tick. Hence, create the sheduler update
                    m_schedulerQueue->push(SchedulerTask{SchedulerTask::TaskType::ScheduledUpdate, ""});
                }
                else
                {
                    // it is necessary to call stopRequested to clean the pipe buffer.
                    // and it is necessary to ensure the compiler will keep this.
                    if (!stopRequested())
                    {
                        assert(false);
                        std::terminate();
                    }
                    ActionOnInterrupt actionOnInterrupt = getActionOnInterruptAndReset();
                    // it was either a reset or a stop request
                    assert(actionOnInterrupt != ActionOnInterrupt::NOTHING);
                    if (actionOnInterrupt == ActionOnInterrupt::RESET)
                    {
                        // timeToWait is already up2date with the reset time
                        continue;
                    }
                    else
                    {
                        // skip from the while loop
                        return;
                    }
                }
            }
        }

        std::chrono::milliseconds CronSchedulerThread::getPeriodTick()
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            return std::chrono::milliseconds(m_periodTick);
        }

        CronSchedulerThread::ActionOnInterrupt CronSchedulerThread::getActionOnInterruptAndReset()
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            ActionOnInterrupt copyAction = m_actionOnInterrupt;
            m_actionOnInterrupt = ActionOnInterrupt::NOTHING;
            return copyAction;
        }

        void CronSchedulerThread::start()
        {
            AbstractThread::start();
        }

    }
}