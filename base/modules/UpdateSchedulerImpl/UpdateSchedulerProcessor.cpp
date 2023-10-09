/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerProcessor.h"

#include "Logger.h"

#include "configModule/DownloadReportsAnalyser.h"
#include "configModule/UpdateActionParser.h"
#include "configModule/UpdatePolicyTranslator.h"
#include "stateMachinesModule/DownloadStateMachine.h"
#include "../../tests/Common/Helpers/FakeTimeUtils.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/OSUtilitiesImpl/SXLMachineID.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/NoPolicyAvailableException.h>
#include <Common/Process/IProcess.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <SulDownloader/suldownloaderdata/ConfigurationDataUtil.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <SulDownloader/suldownloaderdata/UpdateSupplementDecider.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <UpdateSchedulerImpl/runnerModule/AsyncSulDownloaderRunner.h>
#include <UpdateSchedulerImpl/stateMachinesModule/EventStateMachine.h>
#include <UpdateSchedulerImpl/stateMachinesModule/InstallStateMachine.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineProcessor.h>

#include <chrono>
#include <csignal>
#include <json.hpp>
#include <thread>
#include <iomanip>

using namespace std::chrono;

namespace
{
    // FIXME: remove after LINUXDAR-1942
    bool detectedUpgradeWithBrokenLiveResponse()
    {      
        auto* fs = Common::FileSystem::fileSystem();
        std::string fileMarkOfUpgrade{"/opt/sophos-spl/tmp/.upgradeToNewWarehouse"}; 
        if (fs->exists(fileMarkOfUpgrade))
        {
            try{
                fs->removeFile(fileMarkOfUpgrade); 
            }
            catch( std::exception & ex)
            {
                LOGWARN("Failed to remove file: " << fileMarkOfUpgrade << ". Reason: " << ex.what()); 
            }
            
            LOGINFO("Upgrade to new warehouse structure detected. Triggering a new out-of-sync update");
            return true;
        }
        return false;
    }

    /**
     * Persists a list of feature codes to disk in JSON format. Used by Update Scheduler so that it can correctly
     * generate ALC status feature code list on an update failure or when first started.
     * @param Reference to std::vector<std::string> which holds list of feature codes, e.g. CORE
     */
    void writeInstalledFeaturesJsonFile(std::vector<std::string>& features)
    {
        try
        {
            nlohmann::json jsonFeatures(features);
            auto fileSystem = Common::FileSystem::fileSystem();
            fileSystem->writeFile(
                Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(),
                jsonFeatures.dump());
        }
        catch (nlohmann::json::parse_error& jsonException)
        {
            LOGERROR("The installed features list could not be serialised for persisting to disk: " << jsonException.what());
        }
        catch (Common::FileSystem::IFileSystemException& fileSystemException)
        {
            LOGERROR("The installed features list could not be written to disk: " << fileSystemException.what());
        }
    }

    /**
     * Returns the list of features that are currently installed, if there is no file or the file cannot be parsed
     * then this returns an empty list.
     * @return std::vector<std::string> of feature codes, e.g. CORE
     */
    std::vector<std::string> readInstalledFeaturesJsonFile()
    {
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            if (!fileSystem->exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
            {
                return {};
            }
            std::string featureCodes = fileSystem->readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath());
            nlohmann::json jsonFeatures = nlohmann::json::parse(featureCodes);
            return jsonFeatures.get<std::vector<std::string>>();
        }
        catch (Common::FileSystem::IFileSystemException& fileSystemException)
        {
            LOGERROR("The installed features list could not be read from disk: " << fileSystemException.what());
        }
        catch (nlohmann::json::parse_error& jsonException)
        {
            LOGERROR("The installed features list could not be deserialised for reading from disk: " << jsonException.what());
        }
        return {};
    }
} // namespace

namespace UpdateSchedulerImpl
{
    using SettingsHolder = UpdateSchedulerImpl::configModule::SettingsHolder;
    using ReportAndFiles = UpdateSchedulerImpl::configModule::ReportAndFiles;
    using UpdateStatus = UpdateSchedulerImpl::configModule::UpdateStatus;
    using namespace UpdateScheduler;
    std::string UpdateSchedulerProcessor::ALC_API("ALC");
    std::string UpdateSchedulerProcessor::VERSIONID("1");

    UpdateSchedulerProcessor::UpdateSchedulerProcessor(
        std::shared_ptr<SchedulerTaskQueue> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<SchedulerPluginCallback> callback,
        std::unique_ptr<ICronSchedulerThread> cronThread,
        std::unique_ptr<IAsyncSulDownloaderRunner> sulDownloaderRunner) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_cronThread(std::move(cronThread)),
        m_sulDownloaderRunner(std::move(sulDownloaderRunner)),
        m_policyTranslator(),
        m_updateVarPath(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath()),
        m_reportfilePath(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath()),
        m_configfilePath(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath()),
        m_previousConfigFilePath(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderPreviousConfigFilePath()),
        m_formattedTime(),
        m_policyReceived(false),
        m_pendingUpdate(false),
        m_featuresInPolicy(),
        m_featuresCurrentlyInstalled(readInstalledFeaturesJsonFile())
    {
        Common::OSUtilitiesImpl::SXLMachineID sxlMachineID;
        try
        {
            m_machineID = sxlMachineID.getMachineID();
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            m_machineID = sxlMachineID.generateMachineID();
            LOGERROR(e.what());
        }
    }

    void UpdateSchedulerProcessor::mainLoop()
    {
        LOGINFO("Update Scheduler Starting");

        waitForSulDownloaderToFinish(600);

        m_cronThread->start();

        // Request policy on startup
        try
        {
            m_baseService->requestPolicies(UpdateSchedulerProcessor::ALC_API);
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: " << UpdateSchedulerProcessor::ALC_API);
            // Ignore no Policy Available errors
        }
        catch (const Common::PluginApi::ApiException& apiException)
        {
            std::string errorMsg(apiException.what());
            assert(
                errorMsg.find(Common::PluginApi::NoPolicyAvailableException::NoPolicyAvailable) == std::string::npos);
            LOGERROR("Unexpected error when requesting policy: " << apiException.what());
        }

        while (true)
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

            // Check that the policy period is within expected range and set default if not
            long updatePeriod = settingsHolder.schedulerPeriod.count();
            constexpr long year = 365 * 24 * 60;
            if (updatePeriod < 5 || updatePeriod > year)
            {
                LOGWARN(
                    "Invalid scheduled update period given: "
                    << updatePeriod
                    << ". It must be between 5 minutes and a year. Leaving update settings as previous");
            }
            else
            {
                m_cronThread->setPeriodTime(settingsHolder.schedulerPeriod);
            }

            if (!Common::FileSystem::fileSystem()->isFile(m_configfilePath))
            {
                // Update immediately if we haven't previously had a policy
                m_pendingUpdate = true;
            }

            writeConfigurationData(settingsHolder.configurationData);
            m_scheduledUpdateConfig = settingsHolder.weeklySchedule;
            m_featuresInPolicy = settingsHolder.configurationData.getFeatures();

            if (m_scheduledUpdateConfig.enabled)
            {
                char buffer[20];
                std::tm scheduledTime{};
                scheduledTime.tm_wday = m_scheduledUpdateConfig.weekDay;
                scheduledTime.tm_hour = m_scheduledUpdateConfig.hour;
                scheduledTime.tm_min = m_scheduledUpdateConfig.minute;
                if (strftime(buffer, sizeof(buffer), "%A %H:%M", &scheduledTime))
                {
                    LOGINFO("Scheduling product updates for " << buffer);
                    LOGINFO("Scheduling data updates every "<< updatePeriod << " minutes");
                }
            }
            else
            {
                LOGINFO("Scheduling updates every "<< updatePeriod << " minutes");
            }

            std::optional<SulDownloader::suldownloaderdata::ConfigurationData> previousConfigurationData =
                getPreviousConfigurationData();

            if (previousConfigurationData.has_value() &&
                    (SulDownloader::suldownloaderdata::ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(
                        settingsHolder.configurationData, previousConfigurationData.value()) ||
                    SulDownloader::suldownloaderdata::ConfigurationDataUtil::checkIfShouldForceUpdate(
                                    settingsHolder.configurationData, previousConfigurationData.value())))
            {
                LOGINFO("Detected product configuration change, triggering update.");
                m_pendingUpdate = true;
            }

            if (!m_policyReceived)
            {
                m_policyReceived = true;

                // ensure that recent results 'if available' are processed.
                // When base is updated, it may stop this plugin. Hence, on start-up, it needs to double-check
                // there is no new results to be processed.
                std::string lastUpdate = processSulDownloaderFinished("update_report.json");

            }

            if (m_pendingUpdate)
            {
                m_pendingUpdate = false;
                processScheduleUpdate();
            }
            else
            {
                // get the previous report and send status if one exists and call processSulDownloaderFinished.
                // to force sending new status if any back to central.
                // if a update_report.json file exists, an update is in progress, so do not force re-processing the
                // previous report.

                std::vector<std::string> filesInReportDirectory =
                    Common::FileSystem::fileSystem()->listFiles(m_updateVarPath);
                std::string startPattern("update_report");
                std::string endPattern(".json");
                std::string unprocessedReport("update_report.json");
                bool newReportFound = false;
                bool oldReportFound = false;

                for (auto& file : filesInReportDirectory)
                {
                    std::string fileName = Common::FileSystem::basename(file);

                    // make sure file name begins with 'update_report' and ends with .'json'
                    if (fileName == unprocessedReport)
                    {
                        newReportFound = true;
                        break;
                    }
                    else if (
                        fileName.find(startPattern) == 0 &&
                        fileName.find(endPattern) == (fileName.length() - endPattern.length()))
                    {
                        oldReportFound = true;
                    }
                }

                if (!newReportFound && oldReportFound)
                {
                    processSulDownloaderFinished("", true);
                }
            }
        }
        catch (UpdateSchedulerImpl::configModule::PolicyValidationException& ex)
        {
            LOGWARN("Invalid policy received: " << ex.what());
        }
    }

    void UpdateSchedulerProcessor::processUpdateNow(const std::string& actionXML)
    {
        if (!configModule::isUpdateNowAction(actionXML))
        {
            LOGWARN("Unexpected action xml received: " << actionXML);
            return;
        }

        LOGSUPPORT("Received Update Now action. ");
        m_cronThread->reset();
        processScheduleUpdate();
    }

    void UpdateSchedulerProcessor::processScheduleUpdate()
    {
        if (m_sulDownloaderRunner->isRunning() && !m_sulDownloaderRunner->hasTimedOut())
        {
            LOGINFO("An active instance of SulDownloader is already running, continuing with current instance.");
            return;
        }

        std::string configPath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();
        auto* fileSystem = Common::FileSystem::fileSystem();
        if (!fileSystem->isFile(configPath))
        {
            LOGWARN("No update_config.json file available. Requesting policy again");
            m_baseService->requestPolicies(UpdateSchedulerProcessor::ALC_API);
            m_pendingUpdate = true;
            return;
        }

        // Check if we should do a supplement-only update or not
        std::string supplementOnlyMarkerFilePath =
            Common::FileSystem::join(
                Common::FileSystem::dirName(configPath),
                "supplement_only.marker"
            );
        time_t lastProductUpdateCheck = configModule::DownloadReportsAnalyser::getLastProductUpdateCheck();
        SulDownloader::suldownloaderdata::UpdateSupplementDecider decider(m_scheduledUpdateConfig);
        bool updateProducts = decider.updateProducts(lastProductUpdateCheck);
        if (updateProducts)
        {
            LOGSUPPORT("Triggering product update check");
            fileSystem->removeFile(supplementOnlyMarkerFilePath, true);
        }
        else
        {
            // supplement only update
            LOGSUPPORT("Triggering supplement-only update check");
            fileSystem->writeFile(supplementOnlyMarkerFilePath, "");
        }

        LOGSUPPORT("Triggering SulDownloader");
        LOGINFO("Attempting to update from warehouse");
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

    std::string UpdateSchedulerProcessor::processSulDownloaderFinished(
        const std::string& /*reportFileLocation*/,
        const bool processLatestReport)
    {
        auto* iFileSystem = Common::FileSystem::fileSystem();
        bool remainingReportToProcess{ false };

        if (detectedUpgradeWithBrokenLiveResponse())
        {
            UpdateScheduler::SchedulerTask task;
            task.taskType = UpdateScheduler::SchedulerTask::TaskType::ScheduledUpdate;
            m_queueTask->push(UpdateScheduler::SchedulerTask{ task });
            return std::string();
        }

        if (processLatestReport)
        {
            LOGINFO("Re-processing latest report to generate current status message information.");
        }
        else if (iFileSystem->isFile(m_reportfilePath))
        {
            LOGINFO("SulDownloader Finished.");
            safeMoveDownloaderReportFile(m_reportfilePath);
            remainingReportToProcess = true;
        }

        LOGSUPPORT("Process reports to get events and status.");
        ReportAndFiles reportAndFiles = configModule::DownloadReportsAnalyser::processReports();

        if (reportAndFiles.sortedFilePaths.empty())
        {
            LOGSUPPORT("No report to process");
            return std::string();
        }

        // removed previous processed reports files, only need to store process reports files for the this run (last/
        // current run).
        iFileSystem->removeFilesInDirectory(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderProcessedReportPath());

        for (size_t i = 0; i < reportAndFiles.sortedFilePaths.size(); i++)
        {
            if (reportAndFiles.reportCollectionResult.IndicesOfSignificantReports[i] ==
                    configModule::ReportCollectionResult::SignificantReportMark::RedundantReport &&
                !processLatestReport)
            {
                LOGSUPPORT("Remove File: " << reportAndFiles.sortedFilePaths[i]);
                iFileSystem->removeFile(reportAndFiles.sortedFilePaths[i]);
            }
            else
            {
                std::string baseName = Common::FileSystem::basename(reportAndFiles.sortedFilePaths[i]);
                std::string destinationPath = Common::FileSystem::join(
                    Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderProcessedReportPath(),
                    baseName);

                try
                {
                    // create a marker file in the processed_report path with the same name as the process report file.
                    // The file will be used on the next update to determine which reports have already been processed.
                    iFileSystem->writeFile(destinationPath, "");
                }
                catch (Common::FileSystem::IFileSystemException& ex)
                {
                    LOGWARN(
                        "Failed to mark '" << reportAndFiles.sortedFilePaths[i] << "' at '" << destinationPath << "'");
                }
            }
        }

        // only send events if the event is relevant and has not been sent yet.
        if (reportAndFiles.reportCollectionResult.SchedulerEvent.IsRelevantToSend && remainingReportToProcess)
        {
            std::string eventXml = serializeUpdateEvent(
                reportAndFiles.reportCollectionResult.SchedulerEvent, m_policyTranslator, m_formattedTime);
            LOGINFO("Sending event to Central");

            m_baseService->sendEvent(ALC_API, eventXml);
        }

        // If the update succeeded then persist the currently installed feature codes to disk, so that on a failure
        // these features can be reported in the ALC status.
        if (reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult == 0)
        {
            m_featuresCurrentlyInstalled = m_featuresInPolicy;
            LOGDEBUG("Writing currently installed feature codes json to disk");
            writeInstalledFeaturesJsonFile(m_featuresCurrentlyInstalled);
        }

        std::string lastInstallTime(reportAndFiles.reportCollectionResult.SchedulerStatus.LastInstallStartedTime);

        if(lastInstallTime.empty())
        {
            // last install time come from the start time in the update report if an upgrade has happened.
            // only need to set to LastStartTime if cannot get LastInstallStartedTime
            lastInstallTime = reportAndFiles.reportCollectionResult.SchedulerStatus.LastStartTime;
        }

        lastInstallTime = Common::UtilityImpl::TimeUtils::toEpochTime(lastInstallTime);

        stateMachinesModule::StateMachineProcessor stateMachineProcessor(lastInstallTime);
        stateMachineProcessor.updateStateMachines(reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult);

        std::string statusXML = SerializeUpdateStatus(
            reportAndFiles.reportCollectionResult.SchedulerStatus,
            m_policyTranslator.revID(),
            VERSIONID,
            m_machineID,
            m_formattedTime,
            m_featuresCurrentlyInstalled,
            stateMachineProcessor.getStateMachineData());

        UpdateStatus copyStatus = reportAndFiles.reportCollectionResult.SchedulerStatus;
        // blank the timestamp that changes for every report.
        copyStatus.LastStartTime = "";
        copyStatus.LastFinishdTime = "";
        std::string statusWithoutTimeStamp = configModule::SerializeUpdateStatus(
            copyStatus, m_policyTranslator.revID(), VERSIONID, m_machineID, m_formattedTime, m_featuresCurrentlyInstalled, stateMachineProcessor.getStateMachineData());
        m_callback->setStatus(Common::PluginApi::StatusInfo{ statusXML, statusWithoutTimeStamp, ALC_API });
        m_baseService->sendStatus(ALC_API, statusXML, statusWithoutTimeStamp);
        LOGINFO("Sending status to Central");

        if (reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult == 0)
        {
            Common::Telemetry::TelemetryHelper::getInstance().set("latest-update-succeeded", true);
            Common::Telemetry::TelemetryHelper::getInstance().set(
                "successful-update-time", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

            // on successful update copy the current update configuration to previous update configuration
            // the previous configuration file will be used on the next policy change and by suldownloader
            // to force an update when subscriptions and features change.
            try
            {
                iFileSystem->copyFile(m_configfilePath, m_previousConfigFilePath);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN(
                    "Failed to create previous configuration file at : " << m_previousConfigFilePath << ", with error, "
                                                                         << ex.what());
            }

            // In order to work out if we should do a supplement-only update we need to record the
            // last successful product update
            if (!reportAndFiles.reportCollectionResult.SchedulerStatus.LastUpdateWasSupplementOnly)
            {
                // Record we did a product update
                SulDownloader::suldownloaderdata::UpdateSupplementDecider::recordSuccessfulProductUpdate();
            }

            return reportAndFiles.reportCollectionResult.SchedulerStatus.LastSyncTime;
        }

        Common::Telemetry::TelemetryHelper::getInstance().set("latest-update-succeeded", false);
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-update-count", 1UL);
        return std::string();
    }

    void UpdateSchedulerProcessor::processSulDownloaderFailedToStart(const std::string& errorMessage)
    {
        LOGERROR("SulDownloader failed to Start with the following error: " << errorMessage);
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-downloader-count", 1UL);
    }

    void UpdateSchedulerProcessor::processSulDownloaderTimedOut()
    {
        LOGERROR("SulDownloader failed to complete its job in 10 minutes");
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-downloader-count", 1UL);

        waitForSulDownloaderToFinish();
    }

    void UpdateSchedulerProcessor::processSulDownloaderMonitorDetached()
    {
        LOGWARN("Monitoring SulDownloader aborted");
        if (!m_callback->shutdownReceived())
        {
            waitForSulDownloaderToFinish();
        }
    }

    void UpdateSchedulerProcessor::saveUpdateCacheCertificate(const std::string& cacheCertificateContent)
    {
        std::string updatecacheCertPath =
            Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
        Common::FileSystem::fileSystem()->writeFile(updatecacheCertPath, cacheCertificateContent);
    }

    void UpdateSchedulerProcessor::writeConfigurationData(
        const SulDownloader::suldownloaderdata::ConfigurationData& configurationData)
    {
        std::string serializedConfigData =
            SulDownloader::suldownloaderdata::ConfigurationData::toJsonSettings(configurationData);
        Common::FileSystem::fileSystem()->writeFile(m_configfilePath, serializedConfigData);
    }

    std::optional<SulDownloader::suldownloaderdata::ConfigurationData> UpdateSchedulerProcessor::
        getPreviousConfigurationData()
    {
        Path previousConfigFilePath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath(),
            Common::ApplicationConfiguration::applicationPathManager().getPreviousUpdateConfigFileName());

        std::string previousConfigSettings;
        SulDownloader::suldownloaderdata::ConfigurationData previousConfigurationData;

        if (Common::FileSystem::fileSystem()->isFile(previousConfigFilePath))
        {
            LOGDEBUG("Previous update configuration file found.");
            try
            {
                previousConfigSettings = Common::FileSystem::fileSystem()->readFile(previousConfigFilePath);

                previousConfigurationData =
                    SulDownloader::suldownloaderdata::ConfigurationData::fromJsonSettings(previousConfigSettings);
                return std::optional<SulDownloader::suldownloaderdata::ConfigurationData>{ previousConfigurationData };
            }
            catch (SulDownloader::suldownloaderdata::SulDownloaderException& ex)
            {
                LOGWARN("Failed to load previous configuration settings from : " << previousConfigFilePath);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to read previous configuration file : " << previousConfigFilePath);
            }
        }

        return std::nullopt;
    }

    void UpdateSchedulerProcessor::safeMoveDownloaderReportFile(const std::string& originalJsonFilePath) const
    {
        auto* iFileSystem = Common::FileSystem::fileSystem();
        std::string dirPath = Common::FileSystem::dirName(originalJsonFilePath);
        std::string fileName = Common::FileSystem::basename(originalJsonFilePath);
        assert(Common::UtilityImpl::StringUtils::endswith(fileName, ".json"));
        std::string noExtension = fileName.substr(0, fileName.size() - 5);
        assert(noExtension == "update_report");
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

    void UpdateSchedulerProcessor::waitForSulDownloaderToFinish() { waitForSulDownloaderToFinish(1); }

    std::string UpdateSchedulerProcessor::getAppId() { return ALC_API; }

    void UpdateSchedulerProcessor::waitForSulDownloaderToFinish(int numberOfSeconds2Wait)
    {
        std::string pathOfSulDownloader = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().sophosInstall(), "base/bin/SulDownloader");
        auto* iFileSystem = Common::FileSystem::fileSystem();

        std::string pidOfCommand;
        for (std::string candidate : { "/bin/pidof", "/usr/sbin/pidof" })
        {
            if (iFileSystem->isExecutable(candidate))
            {
                pidOfCommand = candidate;
                break;
            }
        }

        if (pidOfCommand.empty())
        {
            LOGWARN("Can not verify if SulDownloader is running ");
            return;
        }

        int i = 0;
        while (true)
        {
            int pidOfSulDownloader;
            auto iProcess = Common::Process::createProcess();
            iProcess->exec(pidOfCommand, { pathOfSulDownloader });

            auto state = iProcess->wait(Common::Process::milli(10), 1000);
            if (state != Common::Process::ProcessStatus::FINISHED)
            {
                LOGWARN("pidof(SulDownloader) Failed to exit after 10s");
                iProcess->kill();
            }

            int exitCode = iProcess->exitCode();
            if (exitCode == 1)
            {
                // pidof returns 1 if no process with the given name is found.
                break;
            }
            else if (exitCode != 0 && exitCode != SIGTERM)
            {
                LOGWARN("pidof(SulDownloader) returned " << exitCode);
                break;
            }

            std::string outputPidOf = iProcess->output();
            LOGDEBUG("Pid of SulDownloader: '" << outputPidOf << "'");
            try
            {
                pidOfSulDownloader = std::stoi(outputPidOf);
            }
            catch (std::exception&)
            {
                LOGWARN("Can not convert '" << outputPidOf << "' to int pid of SulDownloader");
                break; // Assume empty or non-convertable output means there is no SulDownloader running
            }

            if (i == 0)
            {
                LOGINFO("Waiting for SulDownloader (PID=" << pidOfSulDownloader << ") to finish.");
            }
            i++;
            // add new log every minute
            if (i % 60 == 0)
            {
                LOGINFO("SulDownloader (PID=" << pidOfSulDownloader << ") still running.");
            }

            if (i >= numberOfSeconds2Wait)
            {
                assert(pidOfSulDownloader > 0);
                LOGWARN("SulDownloader (PID=" << pidOfSulDownloader << ") still running after wait period");
                return;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            // handle receiving shutdown while waiting for sulDownloader to finish.
            if (m_callback->shutdownReceived())
            {
                break;
            }
        }

        LOGINFO("No instance of SulDownloader running.");
    }
} // namespace UpdateSchedulerImpl