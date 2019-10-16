/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "AsyncSulDownloaderRunner.h"

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        using namespace UpdateScheduler;

        AsyncSulDownloaderRunner::AsyncSulDownloaderRunner(
            std::shared_ptr<SchedulerTaskQueue> taskQueue,
            const std::string& dirPath) :
            m_dirPath(dirPath),
            m_taskQueue(taskQueue),
            m_sulDownloaderRunner(),
            m_sulDownloaderExecHandle(),
            m_sulDownloaderRunnerStartTime(std::chrono::system_clock::now() - std::chrono::hours(24))
        {
        }

        void AsyncSulDownloaderRunner::triggerSulDownloader()
        {
            if (m_sulDownloaderRunner)
            {
                if (m_sulDownloaderExecHandle.valid())
                {
                    // synchronize and expect the current instance of suldownloader to finish.
                    m_sulDownloaderExecHandle.get();
                }
                // clear the current sulDownloader
                m_sulDownloaderRunner.reset();
            }

            m_sulDownloaderRunnerStartTime = std::chrono::system_clock::now();

            m_sulDownloaderRunner.reset(
                new SulDownloaderRunner(m_taskQueue, m_dirPath, "update_report.json", std::chrono::minutes(10)));
            m_sulDownloaderExecHandle = std::async(std::launch::async, [this]() { m_sulDownloaderRunner->run(); });
        }

        bool AsyncSulDownloaderRunner::isRunning()
        {
            if (!m_sulDownloaderRunner || !m_sulDownloaderExecHandle.valid())
            {
                return false;
            }

            std::future_status waitResult = m_sulDownloaderExecHandle.wait_for(std::chrono::seconds(0));
            return (waitResult != std::future_status::ready);
        }

        void AsyncSulDownloaderRunner::triggerAbort()
        {
            if (m_sulDownloaderRunner)
            {
                m_sulDownloaderRunner->abortWaitingForReport();
            }
        }

        bool AsyncSulDownloaderRunner::hasTimedOut()
        {
            std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

            std::chrono::duration<double> elapsed_seconds = m_sulDownloaderRunnerStartTime-currentTime;

            if(elapsed_seconds < std::chrono::seconds(600))
            {
                return false;
            }

            return true;
        }
    } // namespace runnerModule

} // namespace UpdateSchedulerImpl