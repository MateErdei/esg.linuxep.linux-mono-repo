// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "UpdateSchedulerProcessor.h"
#include "UpdateSchedulerUtils.h"

#include "configModule/DownloadReportsAnalyser.h"
#include "configModule/UpdateActionParser.h"
#include "runnerModule/AsyncSulDownloaderRunner.h"
#include "stateMachinesModule/EventStateMachine.h"
#include "stateMachinesModule/StateMachineProcessor.h"

#include "UpdateScheduler/SchedulerTaskQueue.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FlagUtils/FlagUtils.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IPidLockFileUtils.h"
#include "Common/OSUtilitiesImpl/SXLMachineID.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/NoPolicyAvailableException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UpdateUtilities/InstalledFeatures.h"
#include "SulDownloader/suldownloaderdata/ConfigurationDataUtil.h"
#include "SulDownloader/suldownloaderdata/UpdateSupplementDecider.h"

// StringUtils is required for debug builds!
#include "Common/UtilityImpl/StringUtils.h"

#include <json.hpp>

#include <chrono>
#include <thread>

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
} // namespace

namespace UpdateSchedulerImpl
{
    std::vector<std::string> readInstalledFeatures()
    {
        try
        {
            return Common::UpdateUtilities::readInstalledFeaturesJsonFile();
        }
        catch (const Common::FileSystem::IFileSystemException& fileSystemException)
        {
            LOGERROR("The installed features list could not be read from disk: " << fileSystemException.what());
        }
        catch (const nlohmann::detail::exception& jsonException)
        {
            LOGERROR("The installed features list could not be deserialised for reading from disk: " << jsonException.what());
        }
        return {};
    }

    bool isSuldownloaderRunning()
    {
        auto sulLockFilePath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();
        try
        {
            LOGDEBUG("Trying to take suldownloader lock " << sulLockFilePath);
            Common::FileSystem::acquireLockFile(sulLockFilePath);
            LOGDEBUG("Took lock " << sulLockFilePath << ", assuming suldownloader not running");
            return false;
        }
        catch (const std::system_error& ex)
        {
            LOGDEBUG("Could not acquire suldownloader lock, assuming suldownloader is running: " << ex.what());
        }
        return true;
    }

    using SettingsHolderSettingsHolder = UpdateSchedulerImpl::configModule::SettingsHolder;
    using ReportAndFiles = UpdateSchedulerImpl::configModule::ReportAndFiles;
    using UpdateStatus = UpdateSchedulerImpl::configModule::UpdateStatus;
    using namespace UpdateScheduler;

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
        m_flagsPolicyProcessed(false),
        m_forceUpdate(false),
        m_forcePausedUpdate(false),
        m_featuresInPolicy(),
        m_featuresCurrentlyInstalled(readInstalledFeatures())
    {
        Common::OSUtilitiesImpl::SXLMachineID sxlMachineID;
        try
        {
            m_machineID = sxlMachineID.getMachineID();
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            m_machineID = sxlMachineID.generateMachineID();
            LOGERROR("Failed to read SXL Machine ID: " << e.what());
        }
    }

    void UpdateSchedulerProcessor::mainLoop()
    {
        LOGINFO("Update Scheduler Starting");
        m_callback->setRunning(true);
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

        try
        {
            m_baseService->requestPolicies(UpdateSchedulerProcessor::FLAGS_API);
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: " << UpdateSchedulerProcessor::FLAGS_API);
            // Ignore no Policy Available errors
        }
        catch (const Common::PluginApi::ApiException& apiException)
        {
            std::string errorMsg(apiException.what());
            assert(errorMsg.find(Common::PluginApi::NoPolicyAvailableException::NoPolicyAvailable) == std::string::npos);
            LOGERROR("Unexpected error when requesting policy: " << apiException.what());
        }

        while (true)
        {
            SchedulerTask task;
            bool hasTask = false;
            try
            {
                hasTask = false;
                hasTask = m_queueTask->pop(task, QUEUE_TIMEOUT);
                if (hasTask)
                {
                    switch (task.taskType) {
                        case SchedulerTask::TaskType::UpdateNow:
                            processUpdateNow(task.content);
                            break;
                        case SchedulerTask::TaskType::ScheduledUpdate:
                            processScheduleUpdate();
                            break;
                        case SchedulerTask::TaskType::Policy:
                            LOGDEBUG("Process task POLICY: " << task.appId);

                            if (task.appId == UpdateSchedulerProcessor::ALC_API)
                            {
                                processALCPolicy(task.content);
                            }
                            else if (task.appId == UpdateSchedulerProcessor::FLAGS_API)
                            {
                                processFlags(task.content);
                            }
                            else
                            {
                                LOGWARN("Received " << task.appId << " policy unexpectedly");
                            }
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
            }
            catch (const Common::Exceptions::IException& ex)
            {
                if (hasTask)
                {
                    LOGERROR("Unexpected error: " << ex.what() << " while processing " << static_cast<int>(task.taskType));
                }
                else
                {
                    LOGERROR("Unexpected error: " << ex.what() << " while attempting to get task");
                }
                log_exception(ex);
            }
            catch (const std::exception& ex)
            {
                if (hasTask)
                {
                    LOGERROR("Unexpected error: " << ex.what() << " while processing " << static_cast<int>(task.taskType));
                }
                else
                {
                    LOGERROR("Unexpected error: " << ex.what() << " while attempting to get task");
                }
                log_exception(ex);
            }
        }
    }

    void UpdateSchedulerProcessor::processALCPolicy(const std::string& policyXml)
    {
        LOGINFO("New ALC policy received");

        if (!m_flagsPolicyProcessed || !Common::FileSystem::fileSystem()->exists(Common::ApplicationConfiguration::applicationPathManager().getMcsFlagsFilePath()))
        {
            LOGDEBUG(Common::ApplicationConfiguration::applicationPathManager().getMcsFlagsFilePath() << " does not exist, performing a short wait for Central flags");

            // Waiting for flags policy to attempt to get the latest central flags
            std::string flagsPolicy = waitForPolicy(*m_queueTask, 5, UpdateSchedulerProcessor::FLAGS_API);
            processFlags(flagsPolicy);
        }

        try
        {
            auto updateSettings = m_policyTranslator.translatePolicy(policyXml);

            if (!updateSettings.updateCacheCertificatesContent.empty())
            {
                saveUpdateCacheCertificate(updateSettings.updateCacheCertificatesContent);
            }

            // Check that the policy period is within expected range and set default if not
            long updatePeriod = updateSettings.schedulerPeriod.count();
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
                m_cronThread->setPeriodTime(updateSettings.schedulerPeriod);
            }

            if (!Common::FileSystem::fileSystem()->isFile(m_configfilePath))
            {
                // Update immediately if we haven't previously had a policy
                m_pendingUpdate = true;
            }
            std::string token = UpdateSchedulerUtils::getJWToken();
            if (token.empty())
            {
                LOGWARN("No jwt token field found in mcs.config");
            }
            updateSettings.configurationData.setJWToken(token);
            updateSettings.configurationData.setDeviceId(UpdateSchedulerUtils::getDeviceId());
            updateSettings.configurationData.setTenantId(UpdateSchedulerUtils::getTenantId());
            updateSettings.configurationData.setDoForcedPausedUpdate(m_forcePausedUpdate);
            updateSettings.configurationData.setDoForcedUpdate(m_forceUpdate);
            writeConfigurationData(updateSettings.configurationData);
            weeklySchedule_ = updateSettings.weeklySchedule;
            m_featuresInPolicy = updateSettings.configurationData.getFeatures();
            m_subscriptionRigidNamesInPolicy.clear();
            m_subscriptionRigidNamesInPolicy.push_back(updateSettings.configurationData.getPrimarySubscription().rigidName());

            for (auto& productSubscription : updateSettings.configurationData.getProductsSubscription())
            {
                m_subscriptionRigidNamesInPolicy.push_back(productSubscription.rigidName());
            }

            if (weeklySchedule_.enabled)
            {
                char buffer[20];
                std::tm scheduledTime{};
                scheduledTime.tm_wday = weeklySchedule_.weekDay;
                scheduledTime.tm_hour = weeklySchedule_.hour;
                scheduledTime.tm_min = weeklySchedule_.minute;
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

            auto previousConfigurationData = UpdateSchedulerUtils::getPreviousConfigurationData();

            if (previousConfigurationData.has_value() &&
                    (SulDownloader::suldownloaderdata::ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(
                        updateSettings.configurationData, previousConfigurationData.value()) ||
                    SulDownloader::suldownloaderdata::ConfigurationDataUtil::checkIfShouldForceUpdate(
                                    updateSettings.configurationData, previousConfigurationData.value())))
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
            LOGWARN("Invalid ALC policy received: " << ex.what());
        }
    }

    void UpdateSchedulerProcessor::processFlags(const std::string& flagsContent)
    {
        m_flagsPolicyProcessed = true;
        bool changed = false;
        LOGINFO("Processing Flags");
        LOGDEBUG("Flags: " << flagsContent);
        if (flagsContent.empty())
        {
            return;
        }

        bool currentFlag = m_forceUpdate;
        m_forceUpdate = Common::FlagUtils::isFlagSet(UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, flagsContent);
        LOGDEBUG("Received " << UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG << " flag value: " << m_forceUpdate);

        if (currentFlag == m_forceUpdate)
        {
            LOGDEBUG(UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG << " flag value still: " << m_forceUpdate);
        }
        else
        {
            changed = true;
        }
        currentFlag = m_forcePausedUpdate;
        m_forcePausedUpdate = Common::FlagUtils::isFlagSet(UpdateSchedulerUtils::FORCE_PAUSED_UPDATE_ENABLED_FLAG, flagsContent);
        LOGDEBUG("Received " << UpdateSchedulerUtils::FORCE_PAUSED_UPDATE_ENABLED_FLAG << " flag value: " << m_forcePausedUpdate);

        if (currentFlag == m_forcePausedUpdate)
        {
            LOGDEBUG(UpdateSchedulerUtils::FORCE_PAUSED_UPDATE_ENABLED_FLAG << " flag value still: " << m_forcePausedUpdate);
        }
        else
        {
            changed = true;
        }

        auto config = UpdateSchedulerUtils::getCurrentConfigurationData();
        if (config.has_value() && changed)
        {
            LOGINFO("Writing new flag into to config");
            auto currentConfigData = config.value();
            currentConfigData.setDoForcedUpdate(m_forceUpdate);
            currentConfigData.setDoForcedPausedUpdate(m_forcePausedUpdate);
            writeConfigurationData(currentConfigData);
        }
    }

    std::string UpdateSchedulerProcessor::waitForPolicy(
        SchedulerTaskQueue& queueTask, int maxTasksThreshold, const std::string& policyAppId)
    {
        std::vector<SchedulerTask> nonPolicyTasks;
        std::string policyContent;
        for (int i = 0; i < maxTasksThreshold; i++)
        {
            SchedulerTask task;
            if (!queueTask.pop(task, QUEUE_TIMEOUT))
            {
                LOGINFO(policyAppId << " policy has not been sent to the plugin");
                break;
            }
            if (task.taskType == SchedulerTask::TaskType::Policy && task.appId == policyAppId)
            {
                policyContent = task.content;
                LOGINFO("First " << policyAppId << " policy received.");
                break;
            }
            LOGDEBUG("Keep task: " <<static_cast<int>(task.taskType));
            nonPolicyTasks.emplace_back(task);
            if (task.taskType == SchedulerTask::TaskType::Stop)
            {
                LOGINFO("Abort waiting for the first policy as Stop signal received.");
                throw DetectRequestToStop("");
            }
        }
        LOGDEBUG("Return from waitForPolicy ");

        for (auto it = nonPolicyTasks.rbegin(); it != nonPolicyTasks.rend(); ++it)
        {
            queueTask.pushFront(*it);
        }
        return policyContent;
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
        processScheduleUpdate(true);
    }

    void UpdateSchedulerProcessor::processScheduleUpdate(bool UpdateNow)
    {
        if (m_sulDownloaderRunner->isRunning() && !m_sulDownloaderRunner->hasTimedOut())
        {
            LOGINFO("An active instance of SulDownloader is already running, continuing with current instance.");
            return;
        }

        auto* fileSystem = Common::FileSystem::fileSystem();
        if (!fileSystem->isFile(m_configfilePath))
        {
            LOGWARN("No update_config.json file available. Requesting policy again");
            m_baseService->requestPolicies(UpdateSchedulerProcessor::ALC_API);
            m_pendingUpdate = true;
            return;
        }

        // Update config with latest JWT
        auto [configurationData, configUpdated] = UpdateSchedulerUtils::getUpdateConfigWithLatestJWT();
        if (configUpdated)
        {
            writeConfigurationData(configurationData);
        }

        bool updateProducts = true;
        std::string supplementOnlyMarkerFilePath =
            Common::FileSystem::join(Common::FileSystem::dirName(m_configfilePath), "supplement_only.marker");

        // Check if we should do a supplement-only update or not
        time_t lastProductUpdateCheck = configModule::DownloadReportsAnalyser::getLastProductUpdateCheck();
        SulDownloader::suldownloaderdata::UpdateSupplementDecider decider(weeklySchedule_);
        updateProducts = decider.updateProducts(lastProductUpdateCheck,UpdateNow);


        if (!updateProducts)
        {
            // Force product update if a requested feature is not installed
            for (const auto& feature : configurationData.getFeatures())
            {
                if (std::find(m_featuresCurrentlyInstalled.begin(), m_featuresCurrentlyInstalled.end(), feature) ==
                    m_featuresCurrentlyInstalled.end())
                {
                    LOGINFO("Feature " << feature << " not yet installed, forcing product update");
                    updateProducts = true;
                    break;
                }
            }
        }

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

        if (detectedUpgradeWithBrokenLiveResponse())
        {
            UpdateScheduler::SchedulerTask task;
            task.taskType = UpdateScheduler::SchedulerTask::TaskType::ScheduledUpdate;
            m_queueTask->push(UpdateScheduler::SchedulerTask{ task });
            return {};
        }

        if (processLatestReport)
        {
            LOGINFO("Re-processing latest report to generate current status message information.");
        }
        else if (iFileSystem->isFile(m_reportfilePath))
        {
            LOGINFO("SulDownloader Finished.");
            safeMoveDownloaderReportFile(m_reportfilePath);
            UpdateSchedulerUtils::cleanUpMarkerFile();

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

        std::string lastInstallTime(reportAndFiles.reportCollectionResult.SchedulerStatus.LastInstallStartedTime);

        if(lastInstallTime.empty())
        {
            // last install time come from the start time in the update report if an upgrade has happened.
            // only need to set to LastStartTime if cannot get LastInstallStartedTime
            lastInstallTime = reportAndFiles.reportCollectionResult.SchedulerStatus.LastStartTime;
        }

        lastInstallTime = Common::UtilityImpl::TimeUtils::toEpochTime(lastInstallTime);
        int lastUpdateResult = reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult;
        stateMachinesModule::StateMachineProcessor stateMachineProcessor(lastInstallTime);
        stateMachineProcessor.updateStateMachines(lastUpdateResult);

        //  if the state machine says we can send.
        //  Note we will only send an event if the overall status changes (SUCCESS to FAILED for example) based
        //  on the state machine logic, or once every 24 hours / updates.
        if ( stateMachineProcessor.getStateMachineData().canSendEvent())
        {
            std::string eventXml = serializeUpdateEvent(
                reportAndFiles.reportCollectionResult.SchedulerEvent, m_policyTranslator, m_formattedTime);
            LOGINFO("Sending event to Central");

            m_baseService->sendEvent(ALC_API, eventXml);
        }

        // The installed feature list is saved by Suldownloader on successful update.
        // Read it here so that we can produce the status.
        if (reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult == 0)
        {
            LOGDEBUG("Reading feature codes from installed feature codes file");
            m_featuresCurrentlyInstalled = readInstalledFeatures();
        }

        std::string statusXML = SerializeUpdateStatus(
            reportAndFiles.reportCollectionResult.SchedulerStatus,
            m_policyTranslator.revID(),
            VERSIONID,
            m_machineID,
            m_formattedTime,
            m_subscriptionRigidNamesInPolicy,
            m_featuresCurrentlyInstalled,
            stateMachineProcessor.getStateMachineData());

        UpdateStatus copyStatus = reportAndFiles.reportCollectionResult.SchedulerStatus;
        // blank the timestamp that changes for every report.
        copyStatus.LastStartTime = "";
        copyStatus.LastFinishdTime = "";
        std::string statusWithoutTimeStamp = configModule::SerializeUpdateStatus(
            copyStatus, m_policyTranslator.revID(), VERSIONID, m_machineID, m_formattedTime, m_subscriptionRigidNamesInPolicy, m_featuresCurrentlyInstalled, stateMachineProcessor.getStateMachineData());
        m_callback->setStateMachine(stateMachineProcessor.getStateMachineData());
        m_callback->setStatus(Common::PluginApi::StatusInfo{ statusXML, statusWithoutTimeStamp, ALC_API });
        m_baseService->sendStatus(ALC_API, statusXML, statusWithoutTimeStamp);
        LOGINFO("Sending status to Central");

        if (reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult == 0)
        {
            Common::Telemetry::TelemetryHelper::getInstance().set("latest-update-succeeded", true);
            Common::Telemetry::TelemetryHelper::getInstance().set(
                "successful-update-time", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
            Common::Telemetry::TelemetryHelper::getInstance().set("sdds-mechanism", UpdateSchedulerUtils::getSDDSMechanism(true));

            // on successful update copy the current update configuration to previous update configuration
            // the previous configuration file will be used on the next policy change and by suldownloader
            // to force an update when subscriptions and features change.
            try
            {
                std::string tempPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
                std::string content = Common::FileSystem::fileSystem()->readFile(m_configfilePath);
                Common::FileSystem::fileSystem()->writeFileAtomically(m_previousConfigFilePath, content, tempPath);

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
        const Common::Policy::UpdateSettings& configurationData)
    {
        std::string tempPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string serializedConfigData =
            SulDownloader::suldownloaderdata::ConfigurationData::toJsonSettings(configurationData);
        LOGDEBUG("Writing to update_config.json:");
        LOGDEBUG(serializedConfigData);
        Common::FileSystem::fileSystem()->writeFileAtomically(m_configfilePath, serializedConfigData, tempPath);
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

    void UpdateSchedulerProcessor::waitForSulDownloaderToFinish(int numberOfSecondsToWait)
    {
        std::string pathFromMarkerFile = UpdateSchedulerUtils::readMarkerFile();
        std::string pathOfSulDownloader;
        // if the marker file is empty or has a slash it in Suldownloader is running from the default location and not from thinstaller
        if (pathFromMarkerFile.empty())
        {
            pathOfSulDownloader = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderPath();
        }
        else
        {
            pathOfSulDownloader = "installer/bin/SulDownloader";
        }

        int i = 0;
        while (true)
        {
            if (!isSuldownloaderRunning())
            {
                UpdateSchedulerUtils::cleanUpMarkerFile();
                break;
            }

            if (i == 0)
            {
                LOGINFO("Waiting for SulDownloader to finish.");
            }
            i++;
            // add new log every minute
            if (i % 60 == 0)
            {
                LOGINFO("SulDownloader still running.");
            }

            if (i >= numberOfSecondsToWait)
            {
                LOGWARN("SulDownloader still running after wait period");
                UpdateSchedulerUtils::cleanUpMarkerFile();
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