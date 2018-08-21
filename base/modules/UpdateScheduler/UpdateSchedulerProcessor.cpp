/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerProcessor.h"
#include "Logger.h"
#include "SchedulerTaskQueue.h"
#include "UpdatePolicyTranslator.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>


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

        m_cronThread->start();

        // ensure that recent results 'if available' are processed.
        // When base is updated, it may stop this plugin. Hence, on start-up, it needs to double-check
        // there is no new results to be processed.
        m_queueTask->push(SchedulerTask{SchedulerTask::TaskType::SulDownloaderFinished, "report.json"});

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

        m_cronThread->setPeriodTime(settingsHolder.schedulerPeriod);

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
        if (m_sulDownloaderRunner->isRunning())
        {
            m_sulDownloaderRunner->triggerAbort();
        }
        m_queueTask->pushStop();
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
        std::string configFilePath = Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
        Common::FileSystem::fileSystem()->writeFile(configFilePath, cacheCertificateContent);
    }

    void UpdateSchedulerProcessor::writeConfigurationData(const SulDownloader::ConfigurationData& configurationData)
    {
        std::string serializedConfigData = SulDownloader::ConfigurationData::toJsonSettings(configurationData);
        std::string configFilePath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();
        Common::FileSystem::fileSystem()->writeFile(configFilePath, serializedConfigData);
    }
}