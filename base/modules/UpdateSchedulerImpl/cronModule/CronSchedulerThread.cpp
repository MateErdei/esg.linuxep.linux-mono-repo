/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CronSchedulerThread.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/UniformIntDistribution.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>
#include "Logger.h"

#include <cassert>

namespace UpdateSchedulerImpl
{
    namespace cronModule
    {
        using namespace UpdateScheduler;

        CronSchedulerThread::CronSchedulerThread(
            std::shared_ptr<SchedulerTaskQueue> schedulerQueue,
            CronSchedulerThread::DurationTime firstTick,
            CronSchedulerThread::DurationTime repeatPeriod,
            int scheduledUpdateOffsetInMinutes) :
            m_sharedState(),
            m_schedulerQueue(schedulerQueue),
            m_firstTick(firstTick),
            m_periodTick(repeatPeriod),
            m_actionOnInterrupt(ActionOnInterrupt::NOTHING),
            m_scheduledUpdate(),
            m_scheduledUpdateOffsetInMinutes(abs(scheduledUpdateOffsetInMinutes)),
            m_updateOnStartUp(true)
        {
        }

        CronSchedulerThread::~CronSchedulerThread()
        {
            // destructor must ensure that the thread is not running anymore or
            // seg fault may occur.
            requestStop();
            join();
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
            LOGINFO("Period time set to: " << repeatPeriod.count());
        }

        void CronSchedulerThread::setScheduledUpdate(ScheduledUpdate scheduledUpdate)
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            m_scheduledUpdate = scheduledUpdate;
        }

        void CronSchedulerThread::setUpdateOnStartUp(bool updateOnStartUp)
        {
            std::lock_guard<std::mutex> lock(m_sharedState);
            m_updateOnStartUp = updateOnStartUp;
        }

        void CronSchedulerThread::run()
        {

            std::chrono::milliseconds timeToWait = m_firstTick;
            bool firstUpdate = true;

            Common::UtilityImpl::UniformIntDistribution distribution(
                -m_scheduledUpdateOffsetInMinutes, m_scheduledUpdateOffsetInMinutes);
            int scheduledUpdateOffsetInMinutes = distribution.next();

            announceThreadStarted();
            LOGINFO("running with period: " << m_periodTick.count() << " ms");
            auto poller = Common::ZeroMQWrapper::createPoller();

            auto pipePollerEntry = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            while (true)
            {
                using poll_result_t = Common::ZeroMQWrapper::IPoller::poll_result_t;
                poll_result_t poll_result;
                try
                {
                    LOGDEBUG("wait for time to trigger update: " << std::chrono::duration_cast<std::chrono::minutes>(timeToWait).count() << " minutes");
                    poll_result = poller->poll(timeToWait);
                }
                catch (Common::ZeroMQWrapperImpl::ZeroMQPollerException& ex)
                {
                    LOGWARN("poll exception: " << ex.what());
                    // the poller will not work anymore and the CronSchedulerThread must stop.
                    // This may happen on shutdown.
                    break;
                }

                timeToWait = getPeriodTick();

                if (poll_result.empty())
                {
                    LOGDEBUG("Checking if time to update");
                    // First update after m_firstTick seconds
                    if (firstUpdate && m_updateOnStartUp)
                    {
                        LOGINFO("First update triggered");
                        m_schedulerQueue->push(SchedulerTask{ SchedulerTask::TaskType::ScheduledUpdate, "" });
                        firstUpdate = false;

                        if (m_scheduledUpdate.getEnabled())
                        {
                            resetScheduledUpdate();
                            // Wait for 15 minutes so we do not update more than once
                            timeToWait = std::chrono::minutes(15);
                        }
                    }

                    // timeout means a new tick. Hence, queue an update if scheduled updating is not enabled
                    else if (!m_scheduledUpdate.getEnabled())
                    {
                        LOGINFO("Trigger new update");
                        m_schedulerQueue->push(SchedulerTask{ SchedulerTask::TaskType::ScheduledUpdate, "" });
                    }

                    // scheduled updating is enabled. Check if it is time to update
                    else
                    {
                        if (m_scheduledUpdate.timeToUpdate(scheduledUpdateOffsetInMinutes))
                        {
                            LOGINFO("Trigger new scheduled update");
                            m_schedulerQueue->push(SchedulerTask{ SchedulerTask::TaskType::ScheduledUpdate, "" });
                            resetScheduledUpdate();

                            scheduledUpdateOffsetInMinutes = distribution.next();
                            // Wait for 15 minutes so we do not update more than once
                            timeToWait = std::chrono::minutes(15);
                        }
                    }
                }
                else
                {
                    LOGINFO("interrupted");
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
                        LOGINFO("interrupted and will continue to run");
                        continue;
                    }
                    else
                    {
                        // skip from the while loop
                        LOGINFO("interrupted and stopped");
                        return;
                    }
                }
            }
            LOGINFO("stopped");
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

        void CronSchedulerThread::start() { AbstractThread::start(); }

        void CronSchedulerThread::resetScheduledUpdate()
        {
            m_scheduledUpdate.resetScheduledUpdateTimes();

            std::string nextUpdate = Common::UtilityImpl::TimeUtils::fromTime(m_scheduledUpdate.getScheduledTime());
            LOGINFO("Next Update scheduled to: " << nextUpdate);

        }

    } // namespace cronModule
} // namespace UpdateSchedulerImpl