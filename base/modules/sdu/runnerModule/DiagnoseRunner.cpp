// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "DiagnoseRunner.h"

#include "Common/Process/IProcess.h"
#include "Common/WatchdogRequest/IWatchdogRequest.h"
#include "sdu/logger/Logger.h"

namespace RemoteDiagnoseImpl::runnerModule
{
    DiagnoseRunner::DiagnoseRunner(
        std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> schedulerTaskQueue,
        const std::string& directoryToWatch,
        const std::string& nameOfFileToWaitFor,
        std::chrono::seconds timeout) :
        m_listener(directoryToWatch, nameOfFileToWaitFor),
        m_directoryWatcher(Common::DirectoryWatcher::createDirectoryWatcher()),
        m_schedulerTaskQueue(std::move(schedulerTaskQueue)),
        m_timeout(timeout)
    {
        m_directoryWatcher->addListener(m_listener);
    }

    void DiagnoseRunner::run()
    {
        m_directoryWatcher->startWatch();
        Task schedulerTask;

        // Run Diagnose service, report error and return if it fails to start.
        std::tuple<int, std::string> startDiagnoseServiceResult = startDiagnoseService();
        if (std::get<0>(startDiagnoseServiceResult) != 0)
        {
            schedulerTask.taskType = Task::TaskType::DIAGNOSE_FAILED_TO_START;
            schedulerTask.Content = std::get<1>(startDiagnoseServiceResult);
            m_schedulerTaskQueue->push(schedulerTask);
            LOGINFO("DiagnoseService failed to start with error: " + std::get<1>(startDiagnoseServiceResult));
            return;
        }

        // Wait for the diagnose zip to be created or a timeout or abortWaitingForReport is called.
        std::string reportFileLocation = m_listener.waitForFile(m_timeout);

        // If the file failed to be created it means either an abortWaitingForReport was called or the wait timed
        // out and no file was found.
        if (reportFileLocation.empty())
        {
            if (m_listener.wasAborted())
            {
                schedulerTask.taskType = Task::TaskType::DIAGNOSE_MONITOR_DETACHED;
                LOGINFO("Diagnose Service not monitoring Diagnose execution.");
            }
            else
            {
                schedulerTask.taskType = Task::TaskType::DIAGNOSE_TIMED_OUT;
                LOGWARN("Diagnose Service timed out.");
            }

            m_schedulerTaskQueue->push(schedulerTask);
            return;
        }

        logIfDiagnoseServiceFailed();

        // If update result was successfully generated within timeout period then report file location and return.
        schedulerTask.taskType = Task::TaskType::DIAGNOSE_FINISHED;
        schedulerTask.Content = reportFileLocation;
        m_schedulerTaskQueue->push(schedulerTask);
        LOGSUPPORT("Diagnose Service finished.");
    }

    void DiagnoseRunner::logIfDiagnoseServiceFailed()
    {
        auto process = Common::Process::createProcess();
        process->exec("/bin/systemctl", { "is-failed", "sophos-spl-diagnose.service" });
        auto result = std::make_tuple(process->exitCode(), process->output());
        int exitCode = std::get<0>(result);

        if (exitCode == 0)
        {
            LOGERROR("Diagnose Service (sophos-spl-diagnose.service) failed.");
        }
    }

    std::tuple<int, std::string> DiagnoseRunner::startDiagnoseService()
    {
        try
        {
            auto request = Common::WatchdogRequest::factory().create();
            request->requestDiagnoseService();
        }
        catch (std::exception& ex)
        {
            return std::make_tuple(1, ex.what());
        }
        return std::make_tuple(0, std::string());
    }

    void DiagnoseRunner::abortWaitingForReport()
    {
        m_listener.abort();
    }
} // namespace RemoteDiagnoseImpl::runnerModule