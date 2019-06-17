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
            m_sulDownloaderExecHandle()
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
    } // namespace runnerModule

} // namespace UpdateSchedulerImpl