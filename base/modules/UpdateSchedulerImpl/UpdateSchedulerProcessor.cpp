// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "UpdateSchedulerProcessor.h"

#include "UpdateSchedulerUtils.h"
#include "UpdateSupplementDecider.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IPidLockFileUtils.h"
#include "Common/FlagUtils/FlagUtils.h"
#include "Common/OSUtilitiesImpl/SXLMachineID.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/NoPolicyAvailableException.h"
#include "Common/Policy/MCSPolicy.h"
#include "Common/Policy/SerialiseUpdateSettings.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UpdateUtilities/ConfigurationDataUtil.h"
#include "Common/UpdateUtilities/InstalledFeatures.h"
#include "UpdateScheduler/SchedulerTaskQueue.h"
#include "common/Logger.h"
#include "configModule/DownloadReportsAnalyser.h"
#include "configModule/UpdateActionParser.h"
#include "runnerModule/AsyncSulDownloaderRunner.h"
#include "stateMachinesModule/EventStateMachine.h"
#include "stateMachinesModule/StateMachineProcessor.h"

// StringUtils is required for debug builds!
#include "UpdateSchedulerTelemetryConsts.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <chrono>
#include <json.hpp>
#include <thread>

static constexpr int SULDOWNLOADER_TIMEOUT = 1800;
using namespace std::chrono;

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
            LOGERROR(
                "The installed features list could not be deserialised for reading from disk: "
                << jsonException.what());
        }
        return {};
    }

    bool isSuldownloaderRunning()
    {
        auto sulLockFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();
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
        mcsPolicyProcessed_(false),
        m_forceUpdate(false),
        m_forcePausedUpdate(false),
        m_useSDDS3DeltaV2(false),
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
        waitForSulDownloaderToFinish(SULDOWNLOADER_TIMEOUT);
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
            assert(
                errorMsg.find(Common::PluginApi::NoPolicyAvailableException::NoPolicyAvailable) == std::string::npos);
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
                    switch (task.taskType)
                    {
                        case SchedulerTask::TaskType::UpdateNow:
                            processUpdateNow(task.content);
                            break;
                        case SchedulerTask::TaskType::ScheduledUpdate:
                            processScheduleUpdate();
                            break;
                        case SchedulerTask::TaskType::Policy:
                            processPolicy(task.content, task.appId);
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
                    LOGWARN(
                        "Unexpected error: " << ex.what() << " while processing " << static_cast<int>(task.taskType));
                }
                else
                {
                    LOGWARN("Unexpected error: " << ex.what() << " while attempting to get task");
                }
                log_exception(ex);
            }
            catch (const std::exception& ex)
            {
                if (hasTask)
                {
                    LOGERROR(
                        "Unexpected error: " << ex.what() << " while processing " << static_cast<int>(task.taskType));
                }
                else
                {
                    LOGERROR("Unexpected error: " << ex.what() << " while attempting to get task");
                }
                log_exception(ex);
            }
        }
    }

    void UpdateSchedulerProcessor::processPolicy(const std::string& policyXml, const std::string& appId)
    {
        try
        {
            // If the update config file is missing we still need to reprocess a duplicate policy in order to regenerate
            // it
            if (m_currentPolicies.at(appId) == policyXml && Common::FileSystem::fileSystem()->isFile(m_configfilePath))
            {
                LOGDEBUG("Policy with app id " << appId << " unchanged, will not be processed");
                return;
            }
        }
        catch (const std::out_of_range&)
        {
            LOGDEBUG("Recieved first policy with app id " << appId);
        }

        if (policyXml.empty())
        {
            LOGERROR("Received empty policy for " << appId);
            // Can't parse empty policy
            return;
        }

        LOGDEBUG("Process task POLICY: " << appId);

        if (appId == UpdateSchedulerProcessor::ALC_API)
        {
            processALCPolicy(policyXml);
        }
        else if (appId == UpdateSchedulerProcessor::FLAGS_API)
        {
            processFlags(policyXml);
        }
        else if (appId == UpdateSchedulerProcessor::MCS_API)
        {
            processMCSPolicy(policyXml);
        }
        else
        {
            LOGWARN("Received " << appId << " policy unexpectedly");
            return;
        }
        m_currentPolicies.insert_or_assign(appId, policyXml);
    }

    void UpdateSchedulerProcessor::processALCPolicy(const std::string& policyXml)
    {
        LOGINFO("New ALC policy received");

        if (!m_flagsPolicyProcessed ||
            !Common::FileSystem::fileSystem()->exists(
                Common::ApplicationConfiguration::applicationPathManager().getMcsFlagsFilePath()))
        {
            LOGDEBUG(
                Common::ApplicationConfiguration::applicationPathManager().getMcsFlagsFilePath()
                << " does not exist, performing a short wait for Central flags");

            // Waiting for flags policy to attempt to get the latest central flags
            std::string flagsPolicy = waitForPolicy(*m_queueTask, 5, UpdateSchedulerProcessor::FLAGS_API);
            processFlags(flagsPolicy);
        }

        if (!mcsPolicyProcessed_)
        {
            // Waiting for MCS policy to attempt to get the latest central MCS policy
            std::string mcsPolicy = waitForPolicy(*m_queueTask, 5, UpdateSchedulerProcessor::MCS_API);
            processMCSPolicy(mcsPolicy);

        }

        try
        {
            auto updateSettings = m_policyTranslator.translatePolicy(policyXml);
            updateSettings.configurationData.setLocalMessageRelayHosts(messageRelay_);
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
            updateSettings.configurationData.setUseSdds3DeltaV2(m_useSDDS3DeltaV2);
            writeConfigurationData(updateSettings.configurationData);
            weeklySchedule_ = updateSettings.weeklySchedule;
            m_subscriptionRigidNamesInPolicy.clear();
            m_subscriptionRigidNamesInPolicy.push_back(
                updateSettings.configurationData.getPrimarySubscription().rigidName());

            for (auto& productSubscription : updateSettings.configurationData.getProductsSubscription())
            {
                m_subscriptionRigidNamesInPolicy.push_back(productSubscription.rigidName());
            }

            Common::Telemetry::TelemetryHelper::getInstance().set(
                Telemetry::scheduledUpdatingEnabled, weeklySchedule_.enabled);
            if (weeklySchedule_.enabled)
            {
                char updateDay[20];
                char updateTime[20];
                std::tm scheduledTime{};
                scheduledTime.tm_wday = weeklySchedule_.weekDay;
                scheduledTime.tm_hour = weeklySchedule_.hour;
                scheduledTime.tm_min = weeklySchedule_.minute;
                if (strftime(updateDay, sizeof(updateDay), "%A", &scheduledTime) &&
                    strftime(updateTime, sizeof(updateTime), "%H:%M", &scheduledTime))
                {
                    Common::Telemetry::TelemetryHelper::getInstance().set(
                        Telemetry::scheduledUpdatingDay, std::string(updateDay));
                    Common::Telemetry::TelemetryHelper::getInstance().set(
                        Telemetry::scheduledUpdatingTime, std::string(updateTime));
                    LOGINFO("Scheduling product updates for " << updateDay << " at " << updateTime);
                    LOGINFO("Scheduling data updates every " << updatePeriod << " minutes");
                }
            }
            else
            {
                LOGINFO("Scheduling updates every " << updatePeriod << " minutes");
            }

            auto previousConfigurationData = UpdateSchedulerUtils::getPreviousConfigurationData();

            if (previousConfigurationData.has_value() &&
                (Common::UpdateUtilities::ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(
                     updateSettings.configurationData, previousConfigurationData.value()) ||
                 Common::UpdateUtilities::ConfigurationDataUtil::checkIfShouldForceUpdate(
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
                processSulDownloaderFinished("update_report.json");
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
        m_forcePausedUpdate =
            Common::FlagUtils::isFlagSet(UpdateSchedulerUtils::FORCE_PAUSED_UPDATE_ENABLED_FLAG, flagsContent);
        LOGDEBUG(
            "Received " << UpdateSchedulerUtils::FORCE_PAUSED_UPDATE_ENABLED_FLAG
                        << " flag value: " << m_forcePausedUpdate);

        if (currentFlag == m_forcePausedUpdate)
        {
            LOGDEBUG(
                UpdateSchedulerUtils::FORCE_PAUSED_UPDATE_ENABLED_FLAG << " flag value still: " << m_forcePausedUpdate);
        }
        else
        {
            changed = true;
        }

        currentFlag = m_useSDDS3DeltaV2;
        m_useSDDS3DeltaV2 =
            Common::FlagUtils::isFlagSet(UpdateSchedulerUtils::SDDS3_DELTA_V2_ENABLED_FLAG, flagsContent);
        LOGDEBUG(
            "Received " << UpdateSchedulerUtils::SDDS3_DELTA_V2_ENABLED_FLAG << " flag value: " << m_useSDDS3DeltaV2);

        if (currentFlag == m_useSDDS3DeltaV2)
        {
            LOGDEBUG(UpdateSchedulerUtils::SDDS3_DELTA_V2_ENABLED_FLAG << " flag value still: " << m_useSDDS3DeltaV2);
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
            currentConfigData.setUseSdds3DeltaV2(m_useSDDS3DeltaV2);
            writeConfigurationData(currentConfigData);
        }
    }

    void UpdateSchedulerProcessor::processMCSPolicy(const std::string& policyXml)
    {

        mcsPolicyProcessed_ = true;

        LOGINFO("Processing MCSPolicy");
        LOGDEBUG("MCSPolicy: " << policyXml);
        if (policyXml.empty())
        {
            return;
        }

        Common::Policy::MCSPolicy mcsPolicy(policyXml);

        messageRelay_ = mcsPolicy.getMessageRelaysHostNames();

        if (Common::FileSystem::fileSystem()->exists(m_configfilePath))
        {
            auto config = UpdateSchedulerUtils::getCurrentConfigurationData();
            if (config.has_value()) {
                auto currentConfigData = config.value();
                currentConfigData.setLocalMessageRelayHosts(mcsPolicy.getMessageRelaysHostNames());
                writeConfigurationData(currentConfigData);
            }
        }
    }

    std::string UpdateSchedulerProcessor::waitForPolicy(
        SchedulerTaskQueue& queueTask,
        int maxTasksThreshold,
        const std::string& policyAppId)
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
            LOGDEBUG("Keep task: " << static_cast<int>(task.taskType));
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
        UpdateSupplementDecider decider(weeklySchedule_);
        updateProducts = decider.updateProducts(lastProductUpdateCheck, UpdateNow);

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

    void UpdateSchedulerProcessor::processSulDownloaderFinished(
        const std::string& /*reportFileLocation*/,
        const bool processLatestReport)
    {
        auto* iFileSystem = Common::FileSystem::fileSystem();

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
            return;
        }

        // removed previous processed reports files, only need to store process reports files for this run (last/
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


        lastInstallTime = Common::UtilityImpl::TimeUtils::toEpochTime(lastInstallTime);
        int lastUpdateResult = reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult;
        stateMachinesModule::StateMachineProcessor stateMachineProcessor(lastInstallTime);
        stateMachineProcessor.updateStateMachines(lastUpdateResult);

        //  if the state machine says we can send.
        //  Note we will only send an event if the overall status changes (SUCCESS to FAILED for example) based
        //  on the state machine logic, or once every 24 hours / updates.
        if (stateMachineProcessor.getStateMachineData().canSendEvent())
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
            m_subscriptionRigidNamesInPolicy,
            m_featuresCurrentlyInstalled,
            stateMachineProcessor.getStateMachineData());

        m_callback->setStateMachine(stateMachineProcessor.getStateMachineData());
        m_callback->setStatus(Common::PluginApi::StatusInfo{ statusXML, statusXML, ALC_API });
        m_baseService->sendStatus(ALC_API, statusXML, statusXML);
        LOGINFO("Sending status to Central");

        for (auto& product : reportAndFiles.reportCollectionResult.SchedulerStatus.Products)
        {
            if (product.RigidName == "ServerProtectionLinux-Base-component")
            {
                Common::Telemetry::TelemetryHelper::getInstance().set(
                    Telemetry::suiteVersion, product.DownloadedVersion);
            }
        }

        if (reportAndFiles.reportCollectionResult.SchedulerStatus.LastResult == 0)
        {
            Common::Telemetry::TelemetryHelper::getInstance().set(Telemetry::latestUpdateSucceeded, true);
            Common::Telemetry::TelemetryHelper::getInstance().set(
                Telemetry::successfulUpdateTime,
                duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
            Common::Telemetry::TelemetryHelper::getInstance().set(
                Telemetry::sddsMechanism, UpdateSchedulerUtils::getSDDSMechanism(true));

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
                UpdateSupplementDecider::recordSuccessfulProductUpdate();
            }

            return;
        }

        Common::Telemetry::TelemetryHelper::getInstance().set(Telemetry::latestUpdateSucceeded, false);
        Common::Telemetry::TelemetryHelper::getInstance().increment(Telemetry::failedUpdateCount, 1UL);
    }

    void UpdateSchedulerProcessor::processSulDownloaderFailedToStart(const std::string& errorMessage)
    {
        LOGERROR("SulDownloader failed to Start with the following error: " << errorMessage);
        Common::Telemetry::TelemetryHelper::getInstance().increment(Telemetry::failedDownloaderCount, 1UL);
    }

    void UpdateSchedulerProcessor::processSulDownloaderTimedOut()
    {
        int timeout_minutes = SULDOWNLOADER_TIMEOUT / 60;
        LOGERROR("SulDownloader failed to complete its job in " << timeout_minutes << " minutes" );
        Common::Telemetry::TelemetryHelper::getInstance().increment(Telemetry::failedDownloaderCount, 1UL);

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

    void UpdateSchedulerProcessor::writeConfigurationData(const Common::Policy::UpdateSettings& updateSettings)
    {
        std::string tempPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string serializedUpdateSettings = Common::Policy::SerialiseUpdateSettings::toJsonSettings(updateSettings);
        LOGDEBUG("Writing to update_config.json:");
        LOGDEBUG(serializedUpdateSettings);
        Common::FileSystem::fileSystem()->writeFileAtomically(m_configfilePath, serializedUpdateSettings, tempPath);
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

    void UpdateSchedulerProcessor::waitForSulDownloaderToFinish()
    {
        waitForSulDownloaderToFinish(1);
    }

    std::string UpdateSchedulerProcessor::getAppId()
    {
        return ALC_API;
    }

    void UpdateSchedulerProcessor::waitForSulDownloaderToFinish(int numberOfSecondsToWait)
    {
        std::string pathFromMarkerFile = UpdateSchedulerUtils::readMarkerFile();
        std::string pathOfSulDownloader;
        // if the marker file is empty or has a slash it in Suldownloader is running from the default location and not
        // from thinstaller
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