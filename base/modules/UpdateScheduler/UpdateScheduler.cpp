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
                case SchedulerTask::TaskType::ScheduledUpdate:
                case SchedulerTask::TaskType::Policy:
                case SchedulerTask::TaskType::Stop:
                case SchedulerTask::TaskType::SulDownloaderFinished:
                case SchedulerTask::TaskType::SulDownloaderFailedToStart:
                case SchedulerTask::TaskType::SulDownloaderTimedOut:
                case SchedulerTask::TaskType::SulDownloaderWasAborted:
                    return;
            }
        }
    }


    void UpdateScheduler::processPolicy(const std::string & policyXml)
    {

    }
}