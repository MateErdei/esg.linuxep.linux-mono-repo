/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateScheduler.h"
#include "Logger.h"
#include "SchedulerTaskQueue.h"

namespace UpdateScheduler
{
    UpdateScheduler::UpdateScheduler(std::shared_ptr<SchedulerTaskQueue> queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService, std::shared_ptr<SchedulerPluginCallback> callback)
    : m_queueTask(queueTask), m_baseService(std::move(baseService)), m_callback(callback)
    {

    }

    void UpdateScheduler::mainLoop()
    {
        LOGINFO("Update Scheduler Starting");
        while(true)
        {
            SchedulerTask task = m_queueTask->pop();
            switch(task.taskType)
            {
                case SchedulerTask::TaskType::UpdateNow:
                    processUpdateNow();
                    break;
                case SchedulerTask::TaskType::ScheduledUpdate:
                    processScheduleUpdate();
                    break;
                case SchedulerTask::TaskType::Policy:
                    processPolicy(task.content);
                    break;
                case SchedulerTask::TaskType::Stop:
                    return;
                case SchedulerTask::TaskType::SulDownloaderFinished:
                    processSulDownloaderFinished(task.content);
                    break;
                case SchedulerTask::TaskType::SulDownloaderFailedToStart:
                    processSulDownloaderFailedToStart(task.content);
                    break;
                case SchedulerTask::TaskType::SulDownloaderTimedOut:
                    processSulDownloaderTimedOut();
                    break;
                case SchedulerTask::TaskType::SulDownloaderWasAborted:
                    processSulDownloaderWasAborted();
                    break;
                case SchedulerTask::TaskType::ShutdownReceived:
                    processShutdownReceived();
                    break;
                default:
                    break;
            }
        }
    }


    void UpdateScheduler::processPolicy(const std::string & policyXml)
    {

    }

    void UpdateScheduler::processUpdateNow()
    {

    }

    void UpdateScheduler::processScheduleUpdate()
    {

    }

    void UpdateScheduler::processShutdownReceived()
    {

    }

    void UpdateScheduler::processSulDownloaderFinished(const std::string& reportFileLocation)
    {

    }

    void UpdateScheduler::processSulDownloaderFailedToStart(const std::string& errorMessage)
    {

    }

    void UpdateScheduler::processSulDownloaderTimedOut()
    {

    }

    void UpdateScheduler::processSulDownloaderWasAborted()
    {

    }
}