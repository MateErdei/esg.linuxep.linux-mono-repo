/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerProcessor.h"
#include "DownloadReportsAnalyser.h"
#include "Logger.h"
#include "SchedulerTaskQueue.h"
#include "UpdatePolicyTranslator.h"
#include "UpdateActionParser.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/Process/IProcess.h>


namespace UpdateScheduler
{
    std::string UpdateSchedulerProcessor::ALC_API("ALC");
    std::string UpdateSchedulerProcessor::VERSIONID("1");

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
              , m_reportfilePath(
                    Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath())
              , m_configfilePath(
                    Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath())
              , m_formattedTime()
              , m_policyReceived(false)
    {

    }

    void UpdateSchedulerProcessor::mainLoop()
    {
        LOGINFO("Update Scheduler Starting");

        m_cronThread->start();


        while(true)
        {
            try
            {
                SchedulerTask task = m_queueTask->pop();
                switch (task.taskType)
                {
                    case SchedulerTask::TaskType::UpdateNow:
                        processUpdateNow(task.content);
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
            catch (std::exception& ex)
            {
                LOGERROR("Unexpected error: " << ex.what());
            }

        }
    }


    void UpdateSchedulerProcessor::processPolicy(const std::string& policyXml)
    {
        LOGINFO("New policy received");
        SettingsHolder settingsHolder = m_policyTranslator.translatePolicy(policyXml);
        if (!settingsHolder.updateCacheCertificatesContent.empty())
        {
            saveUpdateCacheCertificate(settingsHolder.updateCacheCertificatesContent);
        }

        m_cronThread->setPeriodTime(settingsHolder.schedulerPeriod);

        writeConfigurationData(settingsHolder.configurationData);

        if (!m_policyReceived)
        {
            // ensure that recent results 'if available' are processed.
            // When base is updated, it may stop this plugin. Hence, on start-up, it needs to double-check
            // there is no new results to be processed.
            processSulDownloaderFinished("report.json");

        }
        m_policyReceived = true;

    }

    void UpdateSchedulerProcessor::processUpdateNow(const std::string& actionXML)
    {
        if (!isUpdateNowAction(actionXML))
        {
            LOGWARN("Unexpected action xml received: " << actionXML);
            return;;
        }

        LOGSUPPORT("Received Update Now action. ");
        m_cronThread->reset();
        processScheduleUpdate();
    }

    void UpdateSchedulerProcessor::processScheduleUpdate()
    {
        if (m_sulDownloaderRunner->isRunning())
        {
            LOGWARN("An active instance of SulDownloader was found. Aborting it.");
            m_sulDownloaderRunner->triggerAbort();
            ensureSulDownloaderNotRunning();

        }
        LOGSUPPORT("Triggering SulDownloader");
        m_sulDownloaderRunner->triggerSulDownloader();
    }

    void UpdateSchedulerProcessor::processShutdownReceived()
    {
        LOGINFO("Shutdown received");
        if (m_sulDownloaderRunner->isRunning())
        {
            LOGSUPPORT("Abort SulDownloader");
            m_sulDownloaderRunner->triggerAbort();
        }
        m_cronThread->requestStop();
        m_queueTask->pushStop();
    }

    void UpdateSchedulerProcessor::processSulDownloaderFinished(const std::string& /*reportFileLocation*/)
    {

        auto iFileSystem = Common::FileSystem::fileSystem();
        bool remainingReportToProcess{false};
        if (iFileSystem->isFile(m_reportfilePath))
        {
            LOGINFO("SulDownloader Finished.");
            safeMoveDownloaderConfigFile(m_reportfilePath);
            remainingReportToProcess = true;
        }

        LOGSUPPORT("Process reports to get events and status.");
        ReportAndFiles reportAndFiles = DownloadReportsAnalyser::processReports();

        if (reportAndFiles.sortedFilePaths.empty())
        {
            LOGSUPPORT("No report to process");
            return;
        }

        for (size_t i = 0; i < reportAndFiles.sortedFilePaths.size(); i++)
        {
            if (reportAndFiles.reportCollectionResult.IndicesOfSignificantReports[i] ==
                ReportCollectionResult::SignificantReportMark::RedundantReport)
            {
                LOGSUPPORT("Remove File: " << reportAndFiles.sortedFilePaths[i]);
                iFileSystem->removeFile(reportAndFiles.sortedFilePaths[i]);
            }
        }

        // only send events if the event is relevant and has not been sent yet.
        if (reportAndFiles.reportCollectionResult.SchedulerEvent.IsRelevantToSend && remainingReportToProcess)
        {
            std::string eventXml = serializeUpdateEvent(
                    reportAndFiles.reportCollectionResult.SchedulerEvent,
                    m_policyTranslator,
                    m_formattedTime
            );
            LOGSUPPORT("Sending event to Central");
            m_baseService->sendEvent(ALC_API, eventXml);
        }

        std::string statusXML = SerializeUpdateStatus(
                reportAndFiles.reportCollectionResult.SchedulerStatus,
                m_policyTranslator.revID(),
                VERSIONID,
                m_formattedTime
        );

        UpdateStatus copyStatus = reportAndFiles.reportCollectionResult.SchedulerStatus;
        // blank the timestamp that changes for every report.
        copyStatus.LastStartTime = "";
        copyStatus.LastFinishdTime = "";
        std::string statusWithoutTimeStamp = SerializeUpdateStatus(
                copyStatus,
                m_policyTranslator.revID(),
                VERSIONID,
                m_formattedTime
        );
        m_callback->setStatus(Common::PluginApi::StatusInfo{statusXML, statusWithoutTimeStamp, ALC_API});
        m_baseService->sendStatus(ALC_API, statusXML, statusWithoutTimeStamp);
        LOGSUPPORT("Send status to Central");
    }

    void UpdateSchedulerProcessor::processSulDownloaderFailedToStart(const std::string& errorMessage)
    {
        LOGERROR("SulDownloader failed to Start with the following error: " << errorMessage);
    }

    void UpdateSchedulerProcessor::processSulDownloaderTimedOut()
    {
        LOGERROR("SulDownloader failed to complete its job in 10 minutes");
        ensureSulDownloaderNotRunning();
    }

    void UpdateSchedulerProcessor::processSulDownloaderWasAborted()
    {
        LOGWARN("SulDownloader process was aborted");
        ensureSulDownloaderNotRunning();
    }

    void UpdateSchedulerProcessor::saveUpdateCacheCertificate(const std::string& cacheCertificateContent)
    {
        std::string configFilePath = Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
        Common::FileSystem::fileSystem()->writeFile(configFilePath, cacheCertificateContent);
    }

    void UpdateSchedulerProcessor::writeConfigurationData(const SulDownloader::ConfigurationData& configurationData)
    {
        std::string serializedConfigData = SulDownloader::ConfigurationData::toJsonSettings(configurationData);
        Common::FileSystem::fileSystem()->writeFile(m_configfilePath, serializedConfigData);
    }

    void UpdateSchedulerProcessor::safeMoveDownloaderConfigFile(const std::string& originalJsonFilePath) const
    {
        auto iFileSystem = Common::FileSystem::fileSystem();
        std::string dirPath = Common::FileSystem::dirName(originalJsonFilePath);
        std::string fileName = Common::FileSystem::basename(originalJsonFilePath);
        assert(Common::UtilityImpl::StringUtils::endswith(fileName, ".json"));
        std::string noExtension = fileName.substr(0, fileName.size() - 5);
        assert(noExtension == "report");
        std::string timeStamp = m_formattedTime.currentTime();
        std::string targetPathName = Common::FileSystem::join(dirPath, noExtension + timeStamp);
        for (int i = 0;; i++)
        {
            std::string currName = targetPathName;
            if (i != 0)
            {
                currName += "_" + std::to_string(i);
            }
            currName += ".json";
            if (!iFileSystem->isFile(currName))
            {
                targetPathName = currName;
                break;
            }
        }
        LOGSUPPORT("Rename report to " << targetPathName);
        iFileSystem->moveFile(originalJsonFilePath, targetPathName);
    }

    void UpdateSchedulerProcessor::ensureSulDownloaderNotRunning()
    {
        auto process = Common::Process::createProcess();
        process->exec("/usr/bin/killall", {"SulDownloader"});
        std::string result = process->output();
        LOGSUPPORT("Killall suldownloader produced message: " << result);
    }
}