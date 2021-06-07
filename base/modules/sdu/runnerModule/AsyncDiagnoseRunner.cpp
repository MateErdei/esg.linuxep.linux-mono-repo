/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "AsyncDiagnoseRunner.h"

namespace RemoteDiagnoseImpl
{
    namespace runnerModule
    {

        AsyncDiagnoseRunner::AsyncDiagnoseRunner(
            std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> taskQueue,
            const std::string& dirPath) :
            m_dirPath(dirPath),
            m_taskQueue(taskQueue),
            m_diagnoseRunner(),
            m_diagnoseExecHandle(),
            m_diagnoseRunnerStartTime(std::chrono::system_clock::now() - std::chrono::hours(24))
        {
        }

        void AsyncDiagnoseRunner::triggerDiagnose()
        {
            if (m_diagnoseRunner)
            {
                if (m_diagnoseExecHandle.valid())
                {
                    // synchronize and expect the current instance of diagnose to finish.
                    m_diagnoseExecHandle.get();
                }
                // clear the current diagnose
                m_diagnoseRunner.reset();
            }

            m_diagnoseRunnerStartTime = std::chrono::system_clock::now();

            m_diagnoseRunner.reset(
                new DiagnoseRunner(m_taskQueue, m_dirPath, "sspl.tar.gz", std::chrono::minutes(10)));
            m_diagnoseExecHandle = std::async(std::launch::async, [this]() { m_diagnoseRunner->run(); });
        }

        bool AsyncDiagnoseRunner::isRunning()
        {
            if (!m_diagnoseRunner || !m_diagnoseExecHandle.valid())
            {
                return false;
            }

            std::future_status waitResult = m_diagnoseExecHandle.wait_for(std::chrono::seconds(0));
            return (waitResult != std::future_status::ready);
        }

        void AsyncDiagnoseRunner::triggerAbort()
        {
            if (m_diagnoseRunner)
            {
                m_diagnoseRunner->abortWaitingForReport();
            }
        }

        bool AsyncDiagnoseRunner::hasTimedOut()
        {
            std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

            std::chrono::duration<double> elapsed_seconds = m_diagnoseRunnerStartTime-currentTime;

            if(elapsed_seconds < std::chrono::seconds(600))
            {
                return false;
            }

            return true;
        }
    } // namespace runnerModule

} // namespace RemoteDiagnoseImpl