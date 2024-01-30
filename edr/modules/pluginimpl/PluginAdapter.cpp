// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"

#include "EdrCommon/ApplicationPaths.h"
#include "IOsqueryProcess.h"
#include "LiveQueryPolicyParser.h"
#include "Logger.h"
#include "PluginUtils.h"
#include "EdrCommon/TelemetryConsts.h"
#include "EdrCommon/EdrConstants.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FlagUtils/FlagUtils.h"
#include "Common/PluginApi/NoPolicyAvailableException.h"
#include "Common/ProcUtilImpl/ProcUtilities.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/ZeroMQWrapper/IIPCException.h"

#include <cmath>
#include <fstream>
#include <unistd.h>

// helper class that allow to schedule a task.
// but it also has some capability of interrupting the scheduler at any point
// which is used to respond on shutdown request.
class WaitUpTo
{
    std::chrono::milliseconds m_ms;
    std::function<void()> m_onFinish;
    std::promise<void> m_promise;
    std::future<void> m_anotherThread;
    std::atomic<bool> m_finished;

public:
    WaitUpTo(std::chrono::milliseconds ms, std::function<void(void)> onFinish) :
        m_ms(ms),
        m_onFinish(std::move(onFinish)),
        m_promise {},
        m_anotherThread {},
        m_finished { false }
    {
        m_anotherThread = std::async(std::launch::async, [this]() {
            if (m_promise.get_future().wait_for(m_ms) == std::future_status::timeout)
            {
                m_finished = true;
                m_onFinish();
            }
        });
    }
    void cancel()
    {
        if (!m_finished.exchange(true, std::memory_order::memory_order_seq_cst))
        {
            m_promise.set_value();
        }
    }
    ~WaitUpTo()
    {
        cancel();
    }
};

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
            m_queueTask(std::move(queueTask)),
            m_baseService(std::move(baseService)),
            m_callback(std::move(callback)),
            m_parallelQueryProcessor{queryrunner::createQueryRunner(Plugin::osquerySocket(), Plugin::livequeryExecutable())},
            m_dataLimit(EdrCommon::DEFAULT_XDR_DATA_LIMIT_BYTES),
            m_loggerExtensionPtr(
                    std::make_shared<LoggerExtension>(
                            Plugin::osqueryXDRResultSenderIntermediaryFilePath(),
                            Plugin::osqueryXDROutputDatafeedFilePath(),
                            Plugin::osqueryXDRConfigFilePath(),
                            Plugin::osqueryMTRConfigFilePath(),
                            Plugin::osqueryCustomConfigFilePath(),
                            Plugin::varDir(),
                            EdrCommon::DEFAULT_XDR_DATA_LIMIT_BYTES,
                            EdrCommon::DEFAULT_XDR_PERIOD_SECONDS,
                            [this] { dataFeedExceededCallback(); },
                            EdrCommon::DEFAULT_MAX_BATCH_TIME_SECONDS,
                            EdrCommon::DEFAULT_MAX_BATCH_SIZE_BYTES)
            ),
            m_scheduleEpoch(Plugin::varDir(),
                        "xdrScheduleEpoch",
                        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
            m_timesOsqueryProcessFailedToStart(0),
            m_osqueryConfigurator()
    {
        updateExtensions();
        Common::Telemetry::TelemetryHelper::getInstance().registerResetCallback(EdrCommon::TELEMETRY_CALLBACK_COOKIE, [this](Common::Telemetry::TelemetryHelper& telemetry){ telemetryResetCallback(telemetry); });
    }

    void PluginAdapter::updateExtensions()
    {
        if (m_extensionAndStateMap.find("SophosExtension") == m_extensionAndStateMap.end())
        {
            m_extensionAndStateMap["SophosExtension"] = std::make_pair<std::shared_ptr<IServiceExtension>, std::shared_ptr<std::atomic_bool>>(
                std::make_shared<SophosExtension>(), std::make_shared<std::atomic_bool>(false));
        }
        if (m_enableScheduledQueries && m_extensionAndStateMap.find("LoggerExtension") == m_extensionAndStateMap.end())
        {
            LOGDEBUG("Adding LoggerExtension to list of extensions");
            m_extensionAndStateMap["LoggerExtension"] = std::make_pair<std::shared_ptr<IServiceExtension>, std::shared_ptr<std::atomic_bool>>(
                m_loggerExtensionPtr, std::make_shared<std::atomic_bool>(false));
        }
        else
        {
            if (!m_enableScheduledQueries && m_extensionAndStateMap.find("LoggerExtension") != m_extensionAndStateMap.end())
            {
                LOGINFO("Removing LoggerExtension from list of extensions");
                m_extensionAndStateMap["LoggerExtension"].first->Stop(SVC_EXT_STOP_TIMEOUT);
                m_extensionAndStateMap.erase("LoggerExtension");
            }
        }
    }

    void PluginAdapter::abortQueriesAndClearQueue()
    {
        m_parallelQueryProcessor.abortQueries();
        m_callback->setOsqueryShouldBeRunning(false);
        m_queueTask->clearQueue();
    }
    
    int PluginAdapter::mainLoop()
    {
        int ret = 0;
        m_callback->setRunning(true);
        
        try
        {
            m_baseService->requestPolicies("LiveQuery");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: LiveQuery");
            // Ignore no Policy Available errors
        }
        catch (const Common::ZeroMQWrapper::IIPCException& exception)
        {
            LOGERROR("Failed to request LiveQuery policy with error: "<< exception.what());
        }

        try
        {
            m_baseService->requestPolicies("FLAGS");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: FLAGS");
            // Ignore no Policy Available errors
        }
        catch (const Common::ZeroMQWrapper::IIPCException& exception)
        {
            LOGERROR("Failed to request FLAGS policy with error: "<< exception.what());
        }

        try
        {
            ret = innerMainLoop();
        }
        catch (DetectRequestToStop& ex)
        {
            LOGINFO("Early request to stop found.");
            abortQueriesAndClearQueue();
            ret = EdrCommon::RESTART_EXIT_CODE;
        }
        catch (FileDescriptorLimitReached& ex)
        {
            LOGWARN(ex.what());
            abortQueriesAndClearQueue();
            ret = EdrCommon::E_FD_LIMIT_REACHED;
        }
        return ret;
    }

    int PluginAdapter::innerMainLoop()
    {
        int ret = EdrCommon::RESTART_EXIT_CODE;
        LOGINFO("Entering the main loop");
        m_callback->initialiseTelemetry();
        ensureMCSCanReadOldResponses();
        std::string liveQueryPolicy = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(5), 5, "LiveQuery");
        std::string flagsPolicy = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(5), 5, "FLAGS");

        processFlags(flagsPolicy, true);
        processLiveQueryPolicy(liveQueryPolicy, true);
        // Send Status On Startup
        sendLiveQueryStatus();
        cleanUpOldOsqueryFiles();
        LOGSUPPORT("Start Osquery");
        setUpOsqueryMonitor();

        std::unique_ptr<WaitUpTo> m_delayedRestart;

        auto lastCleanUpTime = std::chrono::steady_clock::now();
        auto lastMemoryCheckTime = std::chrono::steady_clock::now();
        auto memoryCheckPeriod = std::chrono::minutes (5);
        auto cleanupPeriod = std::chrono::minutes(10);
        int extensionReconnectTries = 0;

        OsqueryDataManager osqueryDataManager;
        std::shared_ptr<OsqueryDataRetentionCheckState> osqueryDataRetentionCheckState =
            std::make_shared<OsqueryDataRetentionCheckState>();

        LOGINFO("Completed initialisation of EDR");
        while (true)
        {
            //Check extensions are still running and restart osquery if any have stopped unexpectedly
            bool anyStoppedExtensions = false;
            for (auto& [extensionName, extensionAndState] : m_extensionAndStateMap)
            {
                if (extensionAndState.second->load())
                {
                    LOGDEBUG(extensionName << " is not running");
                    anyStoppedExtensions = true;
                    // Need to call stop to clear up, even though it's now not running, Stop() has not been called yet.
                    extensionAndState.first->Stop(SVC_EXT_STOP_TIMEOUT);
                    break;
                }
            }

            if (anyStoppedExtensions)
            {
                // Don't stop osquery straight away because it is likely that osquery was stopped by its WD and will
                // start again soon, so we can restart the stopped extensions.
                std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
                if (m_osqueryProcess->isRunning())
                {
                    LOGDEBUG("OSQuery WD process is still running but one or more extensions have stopped");
                    auto secondsOsqueryHasBeenUp = std::abs(now - m_osqueryStartTime);
                    // Make sure osquery WD has been up and running for long enough before we try any reconnects
                    if (secondsOsqueryHasBeenUp >= 10)
                    {
                        if (extensionReconnectTries++ < 10)
                        {
                            LOGINFO("Attempting to reconnect extensions to osquery after an unexpected extension exit");
                            registerAndStartExtensionsPlugin();
                        }
                        else
                        {
                            LOGINFO("Extension reconnection attempts exceeded, restarting OSQuery and extensions after an unexpected extension exit");
                            stopOsqueryAndExtensions();
                        }
                    }
                }
                else
                {
                    // Fall back to old behaviour of tearing everything down if osquery WD is not running.
                    LOGINFO("Restarting OSQuery and extensions after unexpected extension exit");
                    stopOsqueryAndExtensions();
                }
            }
            else
            {
                extensionReconnectTries = 0;
            }

            // Check if we're running in XDR mode and if we are and the data limit period has elapsed then
            // make sure that the query pack is either still enabled or becomes enabled.
            // enableQueryPack is safe to call even when the query pack is already enabled.
            if (m_enableScheduledQueries)
            {
                if ( m_loggerExtensionPtr->checkDataPeriodHasElapsed())
                {
                    if (Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks(m_queryPacksInPolicy, m_loggerExtensionPtr->getDataLimitReached()))
                    {
                        m_callback->setOsqueryShouldBeRunning(false);
                        m_queueTask->pushOsqueryRestart("Restarting osquery to apply changes after re-enabling query packs following a data limit rollover");
                    }
                    sendLiveQueryStatus();
                }

                time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
                if (hasScheduleEpochEnded(now))
                {
                    LOGINFO("Previous schedule_epoch: " << m_scheduleEpoch.getValue() << ", has ended. Starting new schedule_epoch: " << now);
                    m_scheduleEpoch.setValueAndForceStore(now);
                    // osquery will automatically be restarted but set this to make sure there is no delay.
                    m_callback->setOsqueryShouldBeRunning(false);
                    m_queueTask->pushOsqueryRestart("Restarting osquery due schedule_epoch updating");
                    m_expectedOsqueryRestart = true;
                }
            }

            if (!osqueryDataRetentionCheckState->running)
            {
                osqueryDataManager.asyncCheckAndReconfigureDataRetention(osqueryDataRetentionCheckState);
                if (osqueryDataRetentionCheckState->numberOfRetries == 0)
                {
                    LOGINFO("Failed to reconfigure osquery.");
                    osqueryDataRetentionCheckState->numberOfRetries = 5;
                    stopOsqueryAndExtensions();
                    osqueryDataManager.purgeDatabase();
                    m_restartNoDelay = true;
                }
            }

            auto timeNow = std::chrono::steady_clock::now();
            Task task;

            if (timeNow > (lastMemoryCheckTime + memoryCheckPeriod))
            {
                lastMemoryCheckTime = timeNow;
                if (pluginMemoryAboveThreshold())
                {
                    LOGINFO("Plugin stopping, memory usage exceeded: " << EdrCommon::MAX_PLUGIN_MEM_BYTES / 1000 << "kB");
                    Common::Telemetry::TelemetryHelper::getInstance().increment(
                        plugin::telemetryEdrRestartsMemory, 1UL);
                    // Push stop task onto the task queue, to ensure shutdown cleanly, and ensures a
                    // constant flow of tasks being put onto the queue does not prevent shutdown.
                    m_queueTask->pushStop();
                    return EdrCommon::E_MEMORY_LIMIT_REACHED;
                }
            }

            if (!m_queueTask->pop(task, QUEUE_TIMEOUT))
            {

                // only attempt cleanup after the 10 minute period has elapsed
                if (timeNow > (lastCleanUpTime + cleanupPeriod))
                {
                    lastCleanUpTime = timeNow;
                    LOGDEBUG("Cleanup time elapsed , checking files");
                    cleanUpOldOsqueryFiles();
                }
            }
            else
            {
                switch (task.m_taskType)
                {
                    case Task::TaskType::STOP:
                        LOGDEBUG("Process task STOP");
                        osqueryDataRetentionCheckState->enabled = false;
                        stopOsqueryAndExtensions();
                        return ret;
                    case Task::TaskType::START_OSQUERY:
                        LOGDEBUG("Process task START_OSQUERY");
                        LOGINFO("Restarting osquery");
                        setUpOsqueryMonitor();
                        break;
                    case Task::TaskType::QUEUE_OSQUERY_RESTART:
                        LOGDEBUG("Process task QUEUE_OSQUERY_RESTART");
                        LOGINFO("Restarting osquery, reason: " << task.m_content);
                        m_restartNoDelay = true;
                        m_expectedOsqueryRestart = true;
                        stopOsqueryAndExtensions();
                        break;
                    case Task::TaskType::OSQUERY_PROCESS_FINISHED:
                    {
                        m_callback->setOsqueryRunning(false);
                        LOGDEBUG("Process task OSQUERY_PROCESS_FINISHED");
                        m_timesOsqueryProcessFailedToStart = 0;
                        if (!m_expectedOsqueryRestart)
                        {
                            LOGDEBUG("Increment telemetry " << plugin::telemetryOsqueryRestarts);
                            Common::Telemetry::TelemetryHelper::getInstance().increment(
                                plugin::telemetryOsqueryRestarts, 1UL);
                        }

                        int64_t delay = m_restartNoDelay ? 0 : 10;
                        m_restartNoDelay = false;
                        m_expectedOsqueryRestart = false;
                        LOGINFO("osquery stopped. Scheduling its restart in " << delay <<" seconds.");
                        m_delayedRestart.reset(
                            new WaitUpTo(
                                std::chrono::seconds(delay), [this]() { this->m_queueTask->pushStartOsquery(); }));
                        break;
                    }
                    case Task::TaskType::POLICY:
                        LOGDEBUG("Process task POLICY: " << task.m_appId);
                        processPolicy(task.m_content, task.m_appId);
                        break;
                    case Task::TaskType::QUERY:
                        LOGDEBUG("Process task QUERY");
                        processQuery(task.m_content, task.m_correlationId);
                        break;
                    case Task::TaskType::OSQUERY_PROCESS_FAILED_TO_START:
                        m_callback->setOsqueryRunning(false);
                        LOGDEBUG("Process task OSQUERY_PROCESS_FAILED_TO_START");
                        static unsigned int baseDelay = 10;
                        static unsigned int growthBase = 2;
                        static unsigned int maxTime = 320;

                        auto delay = static_cast<unsigned int>(
                            baseDelay * ::pow(growthBase, m_timesOsqueryProcessFailedToStart));
                        if (delay < maxTime)
                        {
                            m_timesOsqueryProcessFailedToStart++;
                        }
                        LOGWARN("The osquery process failed to start. Scheduling a retry in " << delay << " seconds.");
                        Common::Telemetry::TelemetryHelper::getInstance().increment(
                            plugin::telemetryOsqueryRestarts, 1UL);
                        m_delayedRestart.reset(
                            new WaitUpTo(
                                std::chrono::seconds(delay), [this]() { this->m_queueTask->pushStartOsquery(); }));
                }
            }

            int fdCount = Proc::getNumberOfOwnFileDescriptors();
            LOGDEBUG("Number of File Descriptors EDR has: " << fdCount);
            if (fdCount > EdrCommon::MAX_PLUGIN_FD_COUNT)
            {
                Common::Telemetry::TelemetryHelper::getInstance().increment(
                        plugin::telemetryEdrRestartsFD, 1UL);
                throw FileDescriptorLimitReached("Restarting due to having too many file descriptors");
            }
        }
        return ret;
    }

    void PluginAdapter::ensureMCSCanReadOldResponses()
    {
        auto* ifileSystem = Common::FileSystem::fileSystem();
        auto ifilePermissions =  Common::FileSystem::filePermissions();
        try
        {
            std::string responsePath = Plugin::livequeryResponsePath();
            if (ifileSystem->isDirectory(responsePath))
            {
                std::vector<std::string> paths = ifileSystem->listFiles(responsePath);

                if (!paths.empty())
                {
                    LOGINFO("Response files exist that pre-date this instance of EDR. Ensuring MCS has ownership");

                    for (const auto& filepath : paths)
                    {
                        LOGDEBUG("Setting ownership of " << filepath << " to same as MCS");
                        ifilePermissions->chown(filepath, "sophos-spl-local", "sophos-spl-group");
                    }
                }
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Pre-existing response files cannot be chowned due to exception: " << e.what());
        }
    }

    void PluginAdapter::cleanUpOldOsqueryFiles()
    {
        LOGSUPPORT("Cleanup Old Osquery Files");
        m_DataManager.cleanUpOsqueryLogs();
    }


    void PluginAdapter::setUpOsqueryMonitor()
    {
        LOGINFO("Prepare system for running osquery");
        m_osqueryConfigurator.prepareSystemForPlugin(m_enableScheduledQueries, m_scheduleEpoch.getValue());
        stopOsqueryAndExtensions();
        LOGDEBUG("Setup monitoring of osquery");
        std::shared_ptr<QueueTask> queue = m_queueTask;
        std::shared_ptr<Plugin::IOsqueryProcess> osqueryProcess { createOsqueryProcess() };
        m_osqueryProcess = osqueryProcess;
        OsqueryStarted osqueryStarted;
        m_osqueryStartTime = Common::UtilityImpl::TimeUtils::getCurrTime();
        m_callback->setOsqueryShouldBeRunning(true);
        auto callback = m_callback;
        m_monitor = std::async(std::launch::async, [queue, osqueryProcess, &osqueryStarted, callback]() {
            try
            {
                callback->setOsqueryRunning(true);
                osqueryProcess->keepOsqueryRunning(osqueryStarted);
            }
            catch (Plugin::IOsqueryCrashed&)
            {
                LOGWARN("osquery stopped unexpectedly");
            }
            catch (const Plugin::IOsqueryCannotBeExecuted& exception)
            {
                callback->setOsqueryRunning(false);
                LOGERROR("Unable to execute osquery: " << exception.what());
                queue->pushOsqueryProcessDelayRestart();
                return;
            }
            catch (std::exception& ex)
            {
                LOGERROR("Monitor process failed: " << ex.what());
            }
            callback->setOsqueryRunning(false);
            queue->pushOsqueryProcessFinished();
        });
        // block here till osquery new instance is started.
        for(auto& extensionAndState : m_extensionAndStateMap)
        {
            extensionAndState.second.first->Stop(SVC_EXT_STOP_TIMEOUT);
        }
        osqueryStarted.wait_started();


        registerAndStartExtensionsPlugin();

        LOGINFO("Plugin preparation complete");
    }

    void PluginAdapter::registerAndStartExtensionsPlugin()
    {
        auto fs = Common::FileSystem::fileSystem();
        if (!fs->waitForFile(Plugin::osquerySocket(), 10000))
        {
            m_callback->setOsqueryRunning(false);
            LOGERROR("OSQuery socket does not exist after waiting 10 seconds. Restarting osquery");
            stopOsqueryAndExtensions();
            m_restartNoDelay = true;
            return;
        }
        if (m_enableScheduledQueries)
        {
            m_loggerExtensionPtr->reloadTags();
        }

        updateExtensions();
        for (const auto& extensionAndRunningStatus : m_extensionAndStateMap)
        {
            auto extensionPair = extensionAndRunningStatus.second;
            extensionPair.second->store(false);
            try
            {
                extensionPair.first->Start(Plugin::osquerySocket(),
                                        false,
                                           extensionPair.second);
            }
            catch (const std::exception& ex)
            {
                extensionPair.second->store(true);
                LOGERROR("Failed to start extension, extension.Start threw: " << ex.what());
            }
        }
    }

    void PluginAdapter::stopOsqueryAndExtensions()
    {
        try
        {
            // Call stop on all extensions, this is ok to call whether running or not.
            std::vector<std::thread> shutdownThreads;
            for (auto& extension : m_extensionAndStateMap)
            {
                shutdownThreads.emplace_back(&IServiceExtension::Stop, extension.second.first, SVC_EXT_STOP_TIMEOUT);
            }
            while (m_osqueryProcess && m_monitor.valid())
            {
                LOGINFO("Issue request to stop to osquery.");

                m_osqueryProcess->requestStop();

                if (m_monitor.wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
                {
                    LOGWARN("Timeout while waiting for osquery monitoring to finish.");
                    continue;
                }
                m_monitor.get(); // ensure that IOsqueryProcess::keepOsqueryRunning has finished.
            }
            for (auto& shutdownThread : shutdownThreads)
            {
                if (shutdownThread.joinable())
                {
                    shutdownThread.join();
                }
            }
        }
        catch (std::exception& ex)
        {
            LOGWARN("Unexpected exception: " << ex.what());
        }
    }

    PluginAdapter::~PluginAdapter()
    {
        try
        {
            // Repeat stop to be sure that it is done.
            stop();
        }
        catch (const Common::Exceptions::IException& ex)
        {
            LOGFATAL("Failed to stop plugin in destructor! " << ex.what_with_location());
        }
        catch (const std::exception& ex)
        {
            LOGFATAL("Failed to stop plugin in destructor! " << ex.what());
        }
    }

    void PluginAdapter::stop()
    {
        // Will be called multiple times.
        clearExtensions();
        try
        {
            if (m_monitor.valid())
            {
                stopOsqueryAndExtensions(); // in case it reach this point without the request to stop being issued.
            }
        }
        catch (std::exception& ex)
        {
            std::cerr << "Plugin adapter exception: " << ex.what() << std::endl;
        }
        // safe to clean up.
        m_queueTask->clearQueue();
        Common::Telemetry::TelemetryHelper::getInstance().unregisterResetCallback(EdrCommon::TELEMETRY_CALLBACK_COOKIE);

    }

    void PluginAdapter::processQuery(const std::string& queryJson, const std::string& correlationId)
    {
        m_parallelQueryProcessor.addJob(queryJson, correlationId);
    }

    void PluginAdapter::processPolicy(const std::string& policyXml, const std::string& appId)
    {
        try
        {
            if (m_currentPolicies.at(appId) == policyXml)
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
            return;
        }

        m_currentPolicies.insert_or_assign(appId, policyXml);
        if (appId == "FLAGS")
        {
            processFlags(policyXml, false);
        }
        else if (appId == "LiveQuery")
        {
            processLiveQueryPolicy(policyXml, false);
        }
        else
        {
            LOGWARN("Received " << appId << " policy unexpectedly");
        }
    }

    void PluginAdapter::processFlags(const std::string& flagsContent, bool firstTime)
    {
        LOGSUPPORT("Flags: " << flagsContent);
        if (flagsContent.empty())
        {
            return;
        }

        bool flagsHaveChanged = false;
        bool networkTablesAvailable = Common::FlagUtils::isFlagSet(PluginUtils::NETWORK_TABLES_FLAG, flagsContent);

        PluginUtils::updatePluginConfWithFlag(PluginUtils::NETWORK_TABLES_AVAILABLE,
            networkTablesAvailable, flagsHaveChanged);

        if (flagsHaveChanged)
        {
            if (!firstTime)
            {
                m_callback->setOsqueryShouldBeRunning(false);
                m_queueTask->pushOsqueryRestart(
                    "Network table flag has changed. Restarting osquery to apply changes");
            }
        }

    }


    OsqueryConfigurator& PluginAdapter::osqueryConfigurator()
    {
        return m_osqueryConfigurator;
    }

    bool PluginAdapter::pluginMemoryAboveThreshold()
    {
        try
        {
            std::ifstream statStream {"/proc/self/statm", std::fstream::in};
            unsigned long vsize;
            unsigned long rssPages;
            statStream >> vsize >> rssPages;
            unsigned long rssBytes = rssPages * getpagesize();
            LOGDEBUG("Plugin memory usage: " << rssBytes/1024 << "kB");
            return rssBytes > EdrCommon::MAX_PLUGIN_MEM_BYTES;
        }
        catch (std::exception& exception)
        {
            LOGERROR("Could not read plugin memory usage");
        }
        return false;
    }

    void PluginAdapter::dataFeedExceededCallback()
    {
        LOGWARN("Datafeed limit has been hit. Disabling scheduled queries");
        PluginUtils::disableAllQueryPacks();
        sendLiveQueryStatus();
        std::string key = std::string(plugin::telemetryScheduledQueries)+ "." + plugin::telemetryUploadLimitHitQueries;
        Common::Telemetry::TelemetryHelper::getInstance().set(key, true);
        // Thread safe osquery and extensions stop call, osquery and all extensions will be restarted automatically
        m_callback->setOsqueryShouldBeRunning(false);
        m_queueTask->pushOsqueryRestart("XDR data limit exceeded");

    }

    void PluginAdapter::telemetryResetCallback(Common::Telemetry::TelemetryHelper& telemetry)
    {
        LOGDEBUG("Telemetry has been reset");

        for (const auto& queryName : m_loggerExtensionPtr->getFoldableQueries())
        {
            telemetry.addValueToSet(plugin::telemetryFoldableQueries, queryName);
        }
    }

    std::string PluginAdapter::serializeLiveQueryStatus(bool dailyDataLimitExceeded)
    {

//  EXAMPLE STATUS
//<?xml version="1.0" encoding="utf-8" standalone="yes"?>
//<status type='LiveQuery'>
//    <CompRes policyType='56' Res='NoRef' RevID='{{revId}}'/>
//    <scheduled>
//        <dailyDataLimitExceeded>{{true|false}}</dailyDataLimitExceeded>
//    </scheduled>
//</status>

        std::stringstream status;
        status << R"sophos(<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<status type='LiveQuery'>
    <CompRes policyType='56' Res=')sophos";
        status << m_liveQueryStatus;
        status << R"sophos(' RevID=')sophos";
        status << m_liveQueryRevId;
        status << R"sophos('/>
    <scheduled>
        <dailyDataLimitExceeded>)sophos";
        if (dailyDataLimitExceeded)
        {
            status << "true";
        }
        else
        {
            status << "false";
        }
        status << R"sophos(</dailyDataLimitExceeded>
    </scheduled>
</status>)sophos";

        return status.str();
    }

    void PluginAdapter::applyLiveQueryPolicy(
        std::optional<Common::XmlUtilities::AttributesMap> policyAttributesMap,
        bool firstTime)
    {
        std::optional<std::string> customQueries;
        std::vector<Json::Value> foldingRules;
        bool osqueryRestartNeeded = false;

        m_dataLimit = getDataLimit(policyAttributesMap);
        m_loggerExtensionPtr->setDataLimit(m_dataLimit);

        m_liveQueryRevId = getRevId(policyAttributesMap);
        bool previousEnableScheduledQueries = m_enableScheduledQueries;
        m_enableScheduledQueries = getScheduledQueriesEnabledInPolicy(policyAttributesMap);
        if (m_enableScheduledQueries)
        {
            LOGINFO("Scheduled queries are enabled in policy");
        }
        else
        {
            LOGINFO("Scheduled queries are disabled in policy");
        }
        PluginUtils::updatePluginConfWithFlag(PluginUtils::MODE_IDENTIFIER, m_enableScheduledQueries, osqueryRestartNeeded);

        // force osquery restart if xdr value has been updated.
        osqueryRestartNeeded = (m_enableScheduledQueries != previousEnableScheduledQueries) || osqueryRestartNeeded;

        if (!m_enableScheduledQueries && osqueryRestartNeeded)
        {
            // xdr mode has been disabled clearing jrl markers
            PluginUtils::clearAllJRLMarkers();
        }
        m_queryPacksInPolicy = getEnabledQueryPacksInPolicy(policyAttributesMap);

        if (std::find(m_queryPacksInPolicy.begin(), m_queryPacksInPolicy.end(), "MTR") != m_queryPacksInPolicy.end())
        {
            m_loggerExtensionPtr->setMTRLicense(true);
        }
        else
        {
            m_loggerExtensionPtr->setMTRLicense(false);
        }

        //sets osqueryRestartNeeded to true if enabled query packs have changed
        osqueryRestartNeeded = PluginUtils::handleDisablingAndEnablingScheduledQueryPacks(m_queryPacksInPolicy,m_loggerExtensionPtr->getDataLimitReached()) || osqueryRestartNeeded;

        bool changeCustomQueries = false;
        changeCustomQueries = getCustomQueries(policyAttributesMap, customQueries);
        if (PluginUtils::haveCustomQueriesChanged(customQueries) && changeCustomQueries)
        {
            PluginUtils::enableCustomQueries(customQueries, osqueryRestartNeeded, m_loggerExtensionPtr->getDataLimitReached());
        }

        bool changeFoldingRules = false;
        changeFoldingRules = getFoldingRules(policyAttributesMap, foldingRules);
        if (m_loggerExtensionPtr->compareFoldingRules(foldingRules) && changeFoldingRules)
        {
            auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

            m_loggerExtensionPtr->setFoldingRules(foldingRules);
            osqueryRestartNeeded = true;
            LOGDEBUG("LiveQuery Policy folding rules have changed");
            for (const auto& queryName : m_loggerExtensionPtr->getFoldableQueries())
            {
                LOGDEBUG("Folding rule query: " << queryName);
                telemetry.addValueToSet(plugin::telemetryFoldableQueries, queryName);
            }
        }

        m_liveQueryStatus = "Same";

        if (osqueryRestartNeeded && !firstTime)
        {
            m_callback->setOsqueryShouldBeRunning(false);
            m_queueTask->pushOsqueryRestart("LiveQuery policy has changed. Restarting osquery to apply changes");
        }
    }

    void PluginAdapter::processLiveQueryPolicy(const std::string& liveQueryPolicy, bool firstTime)
    {
        LOGINFO("Processing LiveQuery Policy");
        if (!liveQueryPolicy.empty())
        {
            std::optional<Common::XmlUtilities::AttributesMap> attributesMap;
            try
            {
                attributesMap = Common::XmlUtilities::parseXml(liveQueryPolicy);
            }
            catch (std::exception& e)
            {
                LOGERROR("Failed to read LiveQuery Policy: " << e.what());
                m_liveQueryStatus = "Failure";
                return;
            }
            applyLiveQueryPolicy(attributesMap, firstTime);
            m_liveQueryStatus = "Same";
            return;
        }
        else
        {
            m_liveQueryStatus = "NoRef";
        }
        m_liveQueryRevId = "";
    }

    void PluginAdapter::sendLiveQueryStatus()
    {
        bool dailyDataLimitExceeded = m_loggerExtensionPtr->getDataLimitReached();
        std::string statusXml = serializeLiveQueryStatus(dailyDataLimitExceeded);
        LOGINFO("Sending LiveQuery Status");
        m_baseService->sendStatus("LiveQuery", statusXml, statusXml);
    }

    bool PluginAdapter::hasScheduleEpochEnded(time_t now)
    {
        time_t scheduleEpochValue = m_scheduleEpoch.getValue();
        if (scheduleEpochValue > now + ONE_DAY_IN_SECONDS)
        {
            LOGINFO( "Schedule Epoch time: " << scheduleEpochValue << " is in the future, resetting to current time.");
            return true;
        }
        return (now - SCHEDULE_EPOCH_DURATION) > scheduleEpochValue;
    }


    std::string PluginAdapter::waitForTheFirstPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS,
                                                     int maxTasksThreshold,
                                                     const std::string& policyAppId)
    {
        std::vector<Plugin::Task> nonPolicyTasks;
        std::string policyContent;
        for (int i = 0; i < maxTasksThreshold; i++)
        {
            Plugin::Task task;
            if (!queueTask.pop(task, timeoutInS.count()))
            {
                LOGINFO(policyAppId << " policy has not been sent to the plugin");
                break;
            }
            if (task.m_taskType == Plugin::Task::TaskType::POLICY && task.m_appId == policyAppId)
            {
                policyContent = task.m_content;
                LOGINFO("First " << policyAppId << " policy received.");
                break;
            }
            LOGSUPPORT("Keep task: " << static_cast<int>(task.m_taskType));
            nonPolicyTasks.push_back(task);
            if (task.m_taskType == Plugin::Task::TaskType::STOP)
            {
                LOGINFO("Abort waiting for the first policy as Stop signal received.");
                throw DetectRequestToStop("");
            }
        }
        LOGDEBUG("Return from waitForTheFirstPolicy ");
        for (const auto& task : nonPolicyTasks)
        {
            queueTask.push(task);
        }
        return policyContent;
    }

    void PluginAdapter::clearExtensions()
    {
        LOGDEBUG("Clearing Extensions");

        for (auto& runningStatus : m_extensionAndStateMap)
        {
            runningStatus.second.first->Stop(SVC_EXT_STOP_TIMEOUT);
        }
        m_extensionAndStateMap.clear();
        LOGDEBUG("Cleared Extensions");
    }
} // namespace Plugin
