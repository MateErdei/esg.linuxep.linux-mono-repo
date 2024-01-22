// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "CronSchedulerThread.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/UniformIntDistribution.h"
#include "Common/ZeroMQWrapper/IPoller.h"
#include "Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h"

#include <cassert>

namespace
{
    int minutes(UpdateSchedulerImpl::cronModule::CronSchedulerThread::DurationTime durationTime)
    {
        return std::chrono::duration_cast<std::chrono::minutes>(durationTime).count();
    }
} // namespace

namespace UpdateSchedulerImpl::cronModule
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
        m_actionOnInterrupt(ActionOnInterrupt::NOTHING),
        m_scheduledUpdateOffsetInMinutes(abs(scheduledUpdateOffsetInMinutes)),
        m_crossThreadState{ .m_periodTick = { repeatPeriod }, .m_updateOnStartUp = true, .m_changed = true },
        m_inThreadState{}
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
        LOGDEBUG("Setting update period to " << minutes(repeatPeriod) << " minutes");
        m_crossThreadState.m_periodTick = repeatPeriod;
        m_crossThreadState.m_changed = true;
    }

    void CronSchedulerThread::setUpdateOnStartUp(bool updateOnStartUp)
    {
        std::lock_guard<std::mutex> lock(m_sharedState);
        m_crossThreadState.m_updateOnStartUp = updateOnStartUp;
        m_crossThreadState.m_changed = true;
    }

    void CronSchedulerThread::run()
    {
        std::chrono::milliseconds timeToWait = m_firstTick;
        bool firstUpdate = true;

        Common::UtilityImpl::UniformIntDistribution distribution(0, m_scheduledUpdateOffsetInMinutes);

        updateInThreadState();
        announceThreadStarted();
        LOGINFO("Running with update period to " << minutes(m_inThreadState.m_periodTick) << " minutes");
        auto poller = Common::ZeroMQWrapper::createPoller();

        auto pipePollerEntry = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

        while (true)
        {
            updateInThreadState();

            using poll_result_t = Common::ZeroMQWrapper::IPoller::poll_result_t;
            poll_result_t poll_result;
            try
            {
                LOGDEBUG("wait for time to trigger update: " << minutes(timeToWait) << " minutes");
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
                if (firstUpdate && m_inThreadState.m_updateOnStartUp)
                {
                    bool triggerFirstUpdate = false;
                    auto fs = Common::FileSystem::fileSystem();
                    std::string lastStartTimeFile = Common::ApplicationConfiguration::applicationPathManager().getLastUpdateStartTimeMarkerPath();
                    try
                    {
                        if (fs->isFile(lastStartTimeFile))
                        {
                            std::string contents = fs->readFile(lastStartTimeFile);
                            auto [lastStartTime, errorMessage] = Common::UtilityImpl::StringUtils::stringToInt(contents);
                            if (!errorMessage.empty())
                            {
                                std::stringstream errMessage;
                                errMessage << "Failed to convert time in " << lastStartTimeFile << " with error:" << errorMessage;
                                throw std::runtime_error(errMessage.str());
                            }
                            Common::UtilityImpl::FormattedTime time;
                            u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();

                            //if the last update was triggered more than an hour ago trigger first update
                            if ((currentTime - lastStartTime) > 3600)
                            {
                                triggerFirstUpdate = true;
                            }
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        LOGWARN("Failed to check last update start time with error: " << ex.what());
                    }

                    if (triggerFirstUpdate)
                    {
                        LOGINFO("First update triggered");
                        m_schedulerQueue->push(SchedulerTask{SchedulerTask::TaskType::ScheduledUpdate, ""});
                    }
                    else
                    {
                        LOGINFO("First update will be not be triggered as last update was less than an hour ago");
                    }
                    firstUpdate = false;
                }
                else
                {
                    LOGDEBUG("Trigger new update"); // this one is the regular update does not need INFO level.
                    m_schedulerQueue->push(SchedulerTask{ SchedulerTask::TaskType::ScheduledUpdate, "" });
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
        return std::chrono::milliseconds(m_crossThreadState.m_periodTick);
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

    void CronSchedulerThread::join()
    {
        AbstractThread::join();
    }

    void CronSchedulerThread::updateInThreadState()
    {
        std::lock_guard<std::mutex> lock(m_sharedState);
        if (m_crossThreadState.m_changed)
        {
            m_crossThreadState.m_changed = false;
            m_inThreadState = m_crossThreadState;
        }
    }
} // namespace UpdateSchedulerImpl::cronModule