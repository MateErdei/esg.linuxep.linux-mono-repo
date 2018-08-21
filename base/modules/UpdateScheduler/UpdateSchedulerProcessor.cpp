/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerProcessor.h"
#include "Logger.h"
#include "SchedulerTaskQueue.h"
#include "UpdatePolicyTranslator.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>


namespace UpdateScheduler
{
    UpdateSchedulerProcessor::UpdateSchedulerProcessor(std::shared_ptr<SchedulerTaskQueue> queueTask,
                                                       std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                                                       std::shared_ptr<SchedulerPluginCallback> callback,
                                                       std::unique_ptr<ICronSchedulerThread> cronThread,
                                                       std::unique_ptr<IAsyncSulDownloaderRunner> sulDownloaderRunner)
            : m_queueTask(queueTask)
              , m_baseService(std::move(baseService))
              , m_callback(callback)
              , m_cronThread(std::move(cronThread))
              , m_sulDownloaderRunner(std::move(sulDownloaderRunner))
              , m_policyTranslator()
    {

    }

    void UpdateSchedulerProcessor::mainLoop()
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


    void UpdateSchedulerProcessor::processPolicy(const std::string& policyXml)
    {
        SettingsHolder settingsHolder = m_policyTranslator.translatePolicy(policyXml);
        if (!settingsHolder.updateCacheCertificatesContent.empty())
        {
            saveUpdateCacheCertificate(settingsHolder.updateCacheCertificatesContent);
        }

        //FIXME handle frequency

        writeConfigurationData(settingsHolder.configurationData);
    }

    void UpdateSchedulerProcessor::processUpdateNow()
    {
        m_cronThread->reset();
        processScheduleUpdate();
    }

    void UpdateSchedulerProcessor::processScheduleUpdate()
    {


    }

    void UpdateSchedulerProcessor::processShutdownReceived()
    {

    }

    void UpdateSchedulerProcessor::processSulDownloaderFinished(const std::string& reportFileLocation)
    {

    }

    void UpdateSchedulerProcessor::processSulDownloaderFailedToStart(const std::string& errorMessage)
    {

    }

    void UpdateSchedulerProcessor::processSulDownloaderTimedOut()
    {

    }

    void UpdateSchedulerProcessor::processSulDownloaderWasAborted()
    {

    }

    void UpdateSchedulerProcessor::saveUpdateCacheCertificate(const std::string& cacheCertificateContent)
    {

    }

    void UpdateSchedulerProcessor::writeConfigurationData(const SulDownloader::ConfigurationData&)
    {

    }
}