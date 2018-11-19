/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerProcessor.h"
#include "configModule/DownloadReportsAnalyser.h"
#include "Logger.h"
#include "configModule/UpdatePolicyTranslator.h"
#include "configModule/UpdateActionParser.h"
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/Process/IProcess.h>
#include <Common/OSUtilitiesImpl/SXLMachineID.h>
#include <thread>
#include <csignal>

namespace UpdateSchedulerImpl
{
    using SettingsHolder = UpdateSchedulerImpl::configModule::SettingsHolder;
    using ReportAndFiles = UpdateSchedulerImpl::configModule::ReportAndFiles;
    using UpdateStatus = UpdateSchedulerImpl::configModule::UpdateStatus;
    using namespace UpdateScheduler;
    std::string UpdateSchedulerProcessor::ALC_API("ALC");
    std::string UpdateSchedulerProcessor::VERSIONID("1");

    UpdateSchedulerProcessor::UpdateSchedulerProcessor(std::shared_ptr<SchedulerTaskQueue> queueTask,
                                                       std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                                                       std::shared_ptr<SchedulerPluginCallback> callback,
                                                       std::unique_ptr<ICronSchedulerThread> cronThread,
                                                       std::unique_ptr<IAsyncSulDownloaderRunner> sulDownloaderRunner)
            : m_queueTask(std::move(queueTask))
              , m_baseService(std::move(baseService))
              , m_callback(std::move(callback))
              , m_cronThread(std::move(cronThread))
              , m_sulDownloaderRunner(std::move(sulDownloaderRunner))
              , m_policyTranslator()
              , m_reportfilePath(
                    Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath())
              , m_configfilePath(
                    Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath())
              , m_formattedTime()
              , m_policyReceived(false)
              , m_pendingUpdate(false)
    {
        Common::OSUtilitiesImpl::SXLMachineID sxlMachineID;
        m_machineID = sxlMachineID.fetchMachineIdAndCreateIfNecessary();

    }

    void UpdateSchedulerProcessor::mainLoop()
    {
        LOGINFO("Update Scheduler Starting");

        enforceSulDownloaderFinished(600);

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
                    case SchedulerTask::TaskType::SulDownloaderMonitorDetached:
                        processSulDownloaderMonitorDetached();
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
        try
        {
            SettingsHolder settingsHolder = m_policyTranslator.translatePolicy(policyXml);

            if (!settingsHolder.updateCacheCertificatesContent.empty())
            {
                saveUpdateCacheCertificate(settingsHolder.updateCacheCertificatesContent);
            }

            UpdateSchedulerImpl::configModule::PolicyValidationException::validateOrThrow(settingsHolder);

            m_cronThread->setScheduledUpdate(settingsHolder.scheduledUpdate);
            if (settingsHolder.scheduledUpdate.getEnabled())
            {
                char buffer[20];
                std::tm scheduledTime = settingsHolder.scheduledUpdate.getScheduledTime();
                if (strftime(buffer,sizeof(buffer),"%A %H:%M", &scheduledTime))
                {
                    LOGINFO("Scheduling an update for " << buffer);
                }

                m_cronThread->setPeriodTime(std::chrono::minutes(1));

            }
            else
            {
                m_cronThread->setPeriodTime(settingsHolder.schedulerPeriod);
            }

            writeConfigurationData(settingsHolder.configurationData);

            if (!m_policyReceived)
            {
                // ensure that recent results 'if available' are processed.
                // When base is updated, it may stop this plugin. Hence, on start-up, it needs to double-check
                // there is no new results to be processed.
                std::string lastUpdate = processSulDownloaderFinished("report.json");

                // If scheduled updating is enabled, check if we have missed an update, if we have, ensure we
                // run an update on startup
                if (!lastUpdate.empty() && settingsHolder.scheduledUpdate.getEnabled())
                {
                    if (settingsHolder.scheduledUpdate.missedUpdate(lastUpdate))
                    {
                        LOGINFO("Missed a scheduled update. Updating in 5-10 minutes.");
                    }
                    else
                    {
                        m_cronThread->setUpdateOnStartUp(false);
                    }
                }
            }

            m_policyReceived = true;

            if (m_pendingUpdate)
            {
                m_pendingUpdate = false;
                processScheduleUpdate();
            }

        }
        catch ( UpdateSchedulerImpl::configModule::PolicyValidationException& ex)
        {
            LOGWARN("Invalid policy received: " << ex.what());
        }


    }

    void UpdateSchedulerProcessor::processUpdateNow(const std::string& actionXML)
    {
        if (!configModule::isUpdateNowAction(actionXML))
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
        std::string configPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();
        if (!Common::FileSystem::fileSystem()->isFile(configPath))
        {
            LOGWARN("No config.json file available. Requesting policy again");
            m_baseService->requestPolicies(UpdateSchedulerProcessor::ALC_API);
            m_pendingUpdate = true;
            return;
        }

        LOGSUPPORT("Triggering SulDownloader");
        m_sulDownloaderRunner->triggerSulDownloader();
    }

    void UpdateSchedulerProcessor::processShutdownReceived()
    {
        LOGINFO("Shutdown received");
        if (m_sulDownloaderRunner->isRunning())
        {
            LOGSUPPORT("Stop monitoring SulDownloader execution.");
            m_sulDownloaderRunner->triggerAbort();
        }
        m_cronThread->requestStop();
        m_queueTask->pushStop();
    }

    std::string UpdateSchedulerProcessor::processSulDownloaderFinished(const std::string& /*reportFileLocation*/)
    {

        auto iFileSystem = Common::FileSystem::fileSystem();
        bool remainingReportToProcess{false};
        if (iFileSystem->isFile(m_reportfilePath))
        {
            LOGINFO("SulDownloader Finished.");
            safeMoveDownloaderReportFile(m_reportfilePath);
            remainingReportToProcess = true;
        }

        LOGSUPPORT("Process reports to get events and status.");
        ReportAndFiles reportAndFiles = configModule::DownloadReportsAnalyser::processReports();
        // get last sync

        if (reportAndFiles.sortedFilePaths.empty())
        {
            LOGSUPPORT("No report to process");
            return std::string();
        }

        for (size_t i = 0; i < reportAndFiles.sortedFilePaths.size(); i++)
        {
            if (reportAndFiles.reportCollectionResult.IndicesOfSignificantReports[i] ==
                configModule::ReportCollectionResult::SignificantReportMark::RedundantReport)
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
                m_machineID,
                m_formattedTime
        );

        UpdateStatus copyStatus = reportAndFiles.reportCollectionResult.SchedulerStatus;
        // blank the timestamp that changes for every report.
        copyStatus.LastStartTime = "";
        copyStatus.LastFinishdTime = "";
        std::string statusWithoutTimeStamp = configModule::SerializeUpdateStatus(
                copyStatus,
                m_policyTranslator.revID(),
                VERSIONID,
                m_machineID,
                m_formattedTime
        );
        m_callback->setStatus(Common::PluginApi::StatusInfo{statusXML, statusWithoutTimeStamp, ALC_API});
        m_baseService->sendStatus(ALC_API, statusXML, statusWithoutTimeStamp);
        LOGSUPPORT("Send status to Central");
        if (reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult == 0)
        {
            return reportAndFiles.reportCollectionResult.SchedulerStatus.LastSyncTime;
        }
        return std::string();
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

    void UpdateSchedulerProcessor::processSulDownloaderMonitorDetached()
    {
        LOGWARN("Monitoring SulDownloader aborted");
        if ( !m_callback->shutdownReceived())
        {
            ensureSulDownloaderNotRunning();
        }
    }

    void UpdateSchedulerProcessor::saveUpdateCacheCertificate(const std::string& cacheCertificateContent)
    {
        std::string configFilePath = Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
        Common::FileSystem::fileSystem()->writeFile(configFilePath, cacheCertificateContent);
    }

    void UpdateSchedulerProcessor::writeConfigurationData(const SulDownloader::suldownloaderdata::ConfigurationData& configurationData)
    {
        std::string serializedConfigData = SulDownloader::suldownloaderdata::ConfigurationData::toJsonSettings(configurationData);
        Common::FileSystem::fileSystem()->writeFile(m_configfilePath, serializedConfigData);
    }

    void UpdateSchedulerProcessor::safeMoveDownloaderReportFile(const std::string& originalJsonFilePath) const
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
        enforceSulDownloaderFinished(1);
    }

    std::string UpdateSchedulerProcessor::getAppId()
    {
        return ALC_API;
    }

    void UpdateSchedulerProcessor::enforceSulDownloaderFinished(int numberOfSeconds2Wait)
    {
        std::string pathOfSulDownloader = Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
                "base/bin/SulDownloader");
        auto iFileSystem = Common::FileSystem::fileSystem();

        std::string pidOfCommand;
        for( std::string candidate: {"/bin/pidof", "/usr/sbin/pidof"})
        {
            if( iFileSystem->isExecutable(candidate))
            {
                pidOfCommand = candidate;
                break;
            }
        }

        if( pidOfCommand.empty())
        {
            LOGWARN("Can not verify if SulDownloader is running ");
            return;
        }


        int i = 0;
        while( true )
        {
            int pidOfSulDownloader;
            auto iProcess = Common::Process::createProcess();
            iProcess->exec(pidOfCommand, {pathOfSulDownloader});

            auto state = iProcess->wait(Common::Process::milli(5), 100);
            if (state != Common::Process::ProcessStatus::FINISHED)
            {
                LOGWARN("pidof(SulDownloader) Failed to exit after 500 ms");
                iProcess->kill();
            }

            int exitCode = iProcess->exitCode();
            if ( exitCode == 1)
            {
                // pidof returns 1 if no process with the given name is found.
                break;
            }
            else if (exitCode != 0)
            {
                LOGWARN("pidof(SulDownloader) returned "<<exitCode);
                break;
            }

            std::string outputPidOf = iProcess->output();
            LOGDEBUG("Pid of SulDownloader: '" << outputPidOf<<"'");
            try
            {
                pidOfSulDownloader = std::stoi(outputPidOf);
            }
            catch (std::exception & )
            {
                LOGWARN("Can not convert '"<<outputPidOf<<"' to int pid of SulDownloader");
                break; // Assume empty or non-convertable output means there is no SulDownloader running
            }

            if( i == 0)
            {
                LOGINFO("Waiting for SulDownloader (PID="<<pidOfSulDownloader<<") to finish.");
            }
            i++;
            // add new log every minute
            if( i%60 ==0 )
            {
                LOGINFO("SulDownloader (PID="<<pidOfSulDownloader<<") still running.");
            }

            if( i >= numberOfSeconds2Wait)
            {
                assert(pidOfSulDownloader > 0);
                LOGWARN("Forcing SulDownloader (PID="<<pidOfSulDownloader<<") to stop");
                ::kill(pidOfSulDownloader, SIGTERM);
                return;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            // handle receiving shutdown while waiting for sulDownloader to finish.
            if ( m_callback->shutdownReceived())
            {
                break;
            }

        }

        LOGINFO("No instance of SulDownloader running.");

    }
}