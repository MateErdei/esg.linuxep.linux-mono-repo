/******************************************************************************************************

Copyright 2018-2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "ApplicationPaths.h"
#include "IOsqueryProcess.h"
#include "Logger.h"
#include "TelemetryConsts.h"
#include "PluginUtils.h"
#include "LiveQueryPolicyParser.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/Proc/ProcUtilities.h>
#include <Common/PluginApi/NoPolicyAvailableException.h>

#include <cmath>
#include <unistd.h>
#include <fstream>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

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
    // XDR consts
    static const int DEFAULT_MAX_BATCH_SIZE_BYTES = 2000000; // 2MB
    static const int DEFAULT_MAX_BATCH_TIME_SECONDS = 15;
    static const int DEFAULT_XDR_DATA_LIMIT_BYTES = 250000000; // 250MB
    static const int DEFAULT_XDR_PERIOD_SECONDS = 86400; // 1 day

    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
            m_queueTask(std::move(queueTask)),
            m_baseService(std::move(baseService)),
            m_callback(std::move(callback)),
            m_parallelQueryProcessor{queryrunner::createQueryRunner(Plugin::osquerySocket(), Plugin::livequeryExecutable())},
            m_dataLimit(DEFAULT_XDR_DATA_LIMIT_BYTES),
            m_loggerExtensionPtr(
                    std::make_shared<LoggerExtension>(
                            Plugin::osqueryXDRResultSenderIntermediaryFilePath(),
                            Plugin::osqueryXDROutputDatafeedFilePath(),
                            Plugin::osqueryXDRConfigFilePath(),
                            Plugin::osqueryMTRConfigFilePath(),
                            Plugin::osqueryCustomConfigFilePath(),
                            Plugin::varDir(),
                            DEFAULT_XDR_DATA_LIMIT_BYTES,
                            DEFAULT_XDR_PERIOD_SECONDS,
                            [this] { dataFeedExceededCallback(); },
                            DEFAULT_MAX_BATCH_TIME_SECONDS,
                            DEFAULT_MAX_BATCH_SIZE_BYTES)
            ),
            m_scheduleEpoch(Plugin::varDir(),
                        "xdrScheduleEpoch",
                        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
            m_timesOsqueryProcessFailedToStart(0),
            m_osqueryConfigurator()
    {
        std::vector<std::shared_ptr<IServiceExtension>> osqueryExtensions = { m_loggerExtensionPtr, std::make_shared<SophosExtension>()};
        for(const auto& extension : osqueryExtensions)
        {
            auto extensionRunningStatus = std::pair<std::shared_ptr<IServiceExtension>, std::shared_ptr<std::atomic_bool>>
                    (extension, std::make_shared<std::atomic_bool>(false));
            m_extensionAndStateList.push_back(extensionRunningStatus);
        }

    }

    void PluginAdapter::mainLoop()
    {
        try
        {
            m_baseService->requestPolicies("ALC");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: ALC");
            // Ignore no Policy Available errors
        }

        try
        {
            m_baseService->requestPolicies("LiveQuery");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: LiveQuery");
            // Ignore no Policy Available errors
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

        try
        {
            innerMainLoop();
        }
        catch (DetectRequestToStop& ex)
        {
            LOGINFO("Early request to stop found.");
        }
    }

    void PluginAdapter::innerMainLoop()
    {
        LOGINFO("Entering the main loop");
        m_callback->initialiseTelemetry();
        ensureMCSCanReadOldResponses();
        std::string alcPolicy = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(5), 5, "ALC");
        std::string liveQueryPolicy = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(1), 5, "LiveQuery");

        processALCPolicy(alcPolicy, true);
        processLiveQueryPolicy(liveQueryPolicy);
//      Send Status On Startup
        sendLiveQueryStatus();
        cleanUpOldOsqueryFiles();
        loadXdrFlags();
        LOGSUPPORT("Start Osquery");
        setUpOsqueryMonitor();

        std::unique_ptr<WaitUpTo> m_delayedRestart;

        auto lastCleanUpTime = std::chrono::steady_clock::now();
        auto cleanupPeriod = std::chrono::minutes(10);

        auto lastTimeQueriedMtr = std::chrono::steady_clock::now();
        auto mtrConfigQueryPeriod = std::chrono::minutes(1);

        while (true)
        {
            // Check if we're running in XDR mode and if we are and the data limit period has elapsed then
            // make sure that the query pack is either still enabled or becomes enabled.
            // enableQueryPack is safe to call even when the query pack is already enabled.
            if (m_isXDR )
            {
                if ( m_loggerExtensionPtr->checkDataPeriodHasElapsed())
                {
                    m_osqueryConfigurator.enableQueryPack(Plugin::osqueryXDRConfigFilePath());
                    sendLiveQueryStatus();
                }

                //Check extensions are still running and restart osquery if any have stopped unexpectedly
                bool anyStoppedExtensions = false;
                for (auto runningStatus : m_extensionAndStateList)
                {
                    if(runningStatus.second->load())
                    {
                        anyStoppedExtensions = true;
                        break;
                    }
                }

                if (anyStoppedExtensions)
                {
                    LOGINFO("Restarting OSQuery after unexpected extension exit");
                    stopOsquery();
                    m_restartNoDelay = true;
                }

                time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
                if (hasScheduleEpochEnded(now))
                {
                    LOGINFO("Previous schedule_epoch: " << m_scheduleEpoch.getValue() << ", has ended. Starting new schedule_epoch: " << now);
                    m_scheduleEpoch.setValueAndForceStore(now);
                    // osquery will automatically be restarted but set this to make sure there is no delay.
                    m_restartNoDelay = true;
                    stopOsquery();
                }
            }

            Task task;
            if (!m_queueTask->pop(task, QUEUE_TIMEOUT))
            {
                auto timeNow = std::chrono::steady_clock::now();

                // only attempt MTR config query every 1 min
                if (timeNow > (lastTimeQueriedMtr + mtrConfigQueryPeriod))
                {
                    lastTimeQueriedMtr = timeNow;
                    if (m_osqueryConfigurator.checkIfReconfigurationRequired())
                    {
                        m_restartNoDelay = true;
                        stopOsquery();
                    }
                }

                // only attempt cleanup after the 10 minute period has elapsed
                if (timeNow > (lastCleanUpTime + cleanupPeriod))
                {
                    lastCleanUpTime = timeNow;
                    LOGDEBUG("Cleanup time elapsed , checking files");
                    cleanUpOldOsqueryFiles();
                    if (pluginMemoryAboveThreshold())
                    {
                        LOGINFO("Plugin stopping, memory usage exceeded: " << MAX_PLUGIN_MEM_BYTES / 1000 << "kB");
                        stopOsquery();
                        return;
                    }
                }
            }
            else
            {
                switch (task.m_taskType)
                {
                    case Task::TaskType::STOP:
                        LOGDEBUG("Process task STOP");
                        stopOsquery();
                        return;
                    case Task::TaskType::RESTARTOSQUERY:
                        LOGDEBUG("Process task RESTARTOSQUERY");
                        LOGINFO("Restarting osquery");
                        setUpOsqueryMonitor();
                        break;
                    case Task::TaskType::OSQUERYPROCESSFINISHED:
                    {
                        LOGDEBUG("Process task OSQUERYPROCESSFINISHED");
                        m_timesOsqueryProcessFailedToStart = 0;
                        Common::Telemetry::TelemetryHelper::getInstance().increment(
                            plugin::telemetryOsqueryRestarts, 1UL);

                        int64_t delay = m_restartNoDelay ? 0 : 10;
                        m_restartNoDelay = false;
                        LOGINFO("osquery stopped. Scheduling its restart in " << delay <<" seconds.");
                        m_delayedRestart.reset( // NOLINT
                            new WaitUpTo(
                                std::chrono::seconds(delay), [this]() { this->m_queueTask->pushRestartOsquery(); }));
                        break;
                    }
                    case Task::TaskType::Policy:
                        LOGDEBUG("Process task Policy: " << task.m_appId);
                        if (task.m_appId == "FLAGS")
                        {
                            processFlags(task.m_content);
                        }
                        else if (task.m_appId == "ALC")
                        {
                            processALCPolicy(task.m_content, false);
                        }
                        else if (task.m_appId == "LiveQuery")
                        {
                            processLiveQueryPolicy(task.m_content);
                        }
                        else{
                            LOGWARN("Received " << task.m_appId << " policy unexpectedly");
                        }
                        break;
                    case Task::TaskType::Query:
                        LOGDEBUG("Process task Query");
                        processQuery(task.m_content, task.m_correlationId);
                        break;
                    case Task::TaskType::OSQUERYPROCESSFAILEDTOSTART:
                        LOGDEBUG("Process task OsqueryProcessFailedToStart");
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
                        m_delayedRestart.reset( // NOLINT
                            new WaitUpTo(
                                std::chrono::seconds(delay), [this]() { this->m_queueTask->pushRestartOsquery(); }));
                }
            }
        }
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
        databasePurge();
        m_DataManager.cleanUpOsqueryLogs();
    }

    void PluginAdapter::databasePurge()
    {
        auto* ifileSystem = Common::FileSystem::fileSystem();
        try
        {
            LOGSUPPORT("Checking osquery database size");
            std::string databasePath = Plugin::osQueryDataBasePath();
            if (ifileSystem->isDirectory(databasePath))
            {
                std::vector<std::string> paths = ifileSystem->listFiles(databasePath);

                if (paths.size() > MAX_THRESHOLD)
                {
                    LOGINFO("Purging Database");
                    stopOsquery();

                    for (const auto& filepath : paths)
                    {
                        ifileSystem->removeFile(filepath);
                    }

                    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
                    telemetry.increment(plugin::telemetryOSQueryDatabasePurges, 1L);

                    LOGDEBUG("Purging Done");

                    // osquery will automatically be restarted, make sure there is no delay.
                    m_restartNoDelay = true;
                }
            }
            else
            {
                LOGSUPPORT("Osquery database does not exist");
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Database cannot be purged due to exception: " << e.what());
        }
    }

    void PluginAdapter::setUpOsqueryMonitor()
    {
        LOGINFO("Prepare system for running osquery");
        m_osqueryConfigurator.prepareSystemForPlugin(m_isXDR, m_scheduleEpoch.getValue());
        stopOsquery();
        LOGDEBUG("Setup monitoring of osquery");
        std::shared_ptr<QueueTask> queue = m_queueTask;
        std::shared_ptr<Plugin::IOsqueryProcess> osqueryProcess { createOsqueryProcess() };
        m_osqueryProcess = osqueryProcess;
        OsqueryStarted osqueryStarted;
        m_monitor = std::async(std::launch::async, [queue, osqueryProcess, &osqueryStarted]() {
            try
            {
                osqueryProcess->keepOsqueryRunning(osqueryStarted);
            }
            catch (Plugin::IOsqueryCrashed&)
            {
                LOGWARN("osquery stopped unexpectedly");
            }
            catch (const Plugin::IOsqueryCannotBeExecuted& exception)
            {
                LOGERROR("Unable to execute osquery: " << exception.what());
                queue->pushOsqueryProcessDelayRestart();
                return;
            }
            catch (std::exception& ex)
            {
                LOGERROR("Monitor process failed: " << ex.what());
            }
            queue->pushOsqueryProcessFinished();
        });
        // block here till osquery new instance is started.
        for(auto extensionAndState : m_extensionAndStateList)
        {
            extensionAndState.first->Stop();
        }
        osqueryStarted.wait_started();

        if (m_isXDR)
        {
            registerAndStartExtensionsPlugin();
        }
    }

    void PluginAdapter::registerAndStartExtensionsPlugin()
    {
        auto fs = Common::FileSystem::fileSystem();
        if (!fs->waitForFile(Plugin::osquerySocket(), 10000))
        {
            LOGERROR("OSQuery socket does not exist after waiting 10 seconds. Restarting osquery");
            stopOsquery();
            m_restartNoDelay = true;
            return;
        }
        m_loggerExtensionPtr->reloadTags();
        for( auto extensionAndRunningStatus :   m_extensionAndStateList)
        {
            auto extension = extensionAndRunningStatus.first;
            extensionAndRunningStatus.second->store(false);
            try
            {
                extension->Start(Plugin::osquerySocket(),
                                        false,
                                        extensionAndRunningStatus.second);
            }
            catch (const std::exception& ex)
            {
                LOGERROR("Failed to start extension, extension.Start threw: " << ex.what());
            }
        }
    }

    void PluginAdapter::stopOsquery()
    {
        try
        {
            // Call stop on all extensions, this is ok to call whether running or not.
            for(auto extension : m_extensionAndStateList)
            {
                extension.first->Stop();
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
            if (m_monitor.valid())
            {
                stopOsquery(); // in case it reach this point without the request to stop being issued.
            }
        }
        catch (std::exception& ex)
        {
            std::cerr << "Plugin adapter exception: " << ex.what() << std::endl;
        }
        // safe to clean up.
    }

    void PluginAdapter::processQuery(const std::string& queryJson, const std::string& correlationId)
    {
        m_parallelQueryProcessor.addJob(queryJson, correlationId);
    }

    void PluginAdapter::processALCPolicy(const std::string& policy, bool firstTime)
    {
        LOGSUPPORT("Processing ALC Policy");
        m_osqueryConfigurator.loadALCPolicy(policy);
        bool enableAuditDataCollection = m_osqueryConfigurator.enableAuditDataCollection();
        if (firstTime)
        {
            m_collectAuditEnabled = enableAuditDataCollection;
            return;
        }

        std::string option{ enableAuditDataCollection ?"true":"false"};
        if (enableAuditDataCollection != m_collectAuditEnabled)
        {
            m_collectAuditEnabled = enableAuditDataCollection;
            LOGINFO(
                "Option to enable audit collection changed to " << option << ". Scheduling osquery STOP");
            stopOsquery();
            m_restartNoDelay = true;
        }
        else
        {
            LOGDEBUG("Option to enable audit collection remains "<< option);
        }
    }

    unsigned int PluginAdapter::getDataLimit(const std::string &liveQueryPolicy)
    {
        Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(liveQueryPolicy);
        const std::string dataLimitPath{"policy/configuration/scheduled/dailyDataLimit"};
        Common::XmlUtilities::Attributes attributes = attributesMap.lookup(dataLimitPath);

        std::string policyDataLimitAsString = attributes.contents();
        int policyDataLimitAsInt = std::stoi(policyDataLimitAsString);
        LOGDEBUG("Using dailyDataLimit from LiveQuery Policy: " << policyDataLimitAsInt);
        return  policyDataLimitAsInt;
    }

    std::string PluginAdapter::getRevId(const std::string& liveQueryPolicy) {
        Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(liveQueryPolicy);
        const std::string dataLimitPath{"policy"};
        Common::XmlUtilities::Attributes attributes = attributesMap.lookup(dataLimitPath);

        std::string revId = attributes.value("RevID", "");
        if (revId != "") {
            LOGDEBUG("Got RevID: " << revId << " from LiveQuery Policy");
        } else {
            throw FailedToParseLiveQueryPolicy("Didn't find RevID");
        }
        return revId;
    }

    void PluginAdapter::processFlags(const std::string& flagsContent)
    {
        LOGSUPPORT("Flags: " << flagsContent);
        bool flagsHaveChanged = false;
        bool isXDR = Plugin::PluginUtils::isFlagSet(PluginUtils::XDR_FLAG, flagsContent);
        bool networkTablesAvailable = Plugin::PluginUtils::isFlagSet(PluginUtils::NETWORK_TABLES_FLAG, flagsContent);

        if (isXDR)
        {
            LOGSUPPORT("Flags running mode in policy is XDR");
        }
        else
        {
            LOGSUPPORT("Flags running mode in policy is EDR");
        }

        try
        {
            bool oldRunningMode = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(PluginUtils::MODE_IDENTIFIER);
            if (isXDR != oldRunningMode)
            {
                LOGINFO("Updating XDR flag settings");
                Plugin::PluginUtils::setGivenFlagFromSettingsFile(PluginUtils::MODE_IDENTIFIER, isXDR);
                flagsHaveChanged = true;
            }
        }
        catch (const std::runtime_error& ex)
        {
            LOGINFO("Setting XDR flag settings");
            Plugin::PluginUtils::setGivenFlagFromSettingsFile(PluginUtils::MODE_IDENTIFIER, isXDR);
            flagsHaveChanged = true;
        }

        try
        {
            bool oldNetworkTablesSetting = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(PluginUtils::NETWORK_TABLES_AVAILABLE);
            if (networkTablesAvailable != oldNetworkTablesSetting)
            {
                LOGINFO("Updating network tables flag settings");
                Plugin::PluginUtils::setGivenFlagFromSettingsFile(PluginUtils::NETWORK_TABLES_AVAILABLE, networkTablesAvailable);
                flagsHaveChanged = true;
            }
        }
        catch (const std::runtime_error& ex)
        {
            LOGINFO("Setting network tables flag settings");
            Plugin::PluginUtils::setGivenFlagFromSettingsFile(PluginUtils::NETWORK_TABLES_AVAILABLE, networkTablesAvailable);
            flagsHaveChanged = true;
        }

        if (flagsHaveChanged)
        {
            LOGINFO("Flags have changed so restarting osquery");
            m_isXDR = isXDR;
            if (m_isXDR)
            {
                LOGINFO("Flags running mode is XDR");
            }
            else
            {
                LOGINFO("Flags running mode is EDR");
            }
            stopOsquery();
            m_restartNoDelay = true;
        }
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
            if (task.m_taskType == Plugin::Task::TaskType::Policy && task.m_appId == policyAppId)
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
        for (auto task : nonPolicyTasks)
        {
            queueTask.push(task);
        }
        return policyContent;
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
            return rssBytes > MAX_PLUGIN_MEM_BYTES;
        }
        catch (std::exception& exception)
        {
            LOGERROR("Could not read plugin memory usage");
        }
        return false;
    }

    void PluginAdapter::loadXdrFlags()
    {
        try
        {
            m_isXDR = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(PluginUtils::MODE_IDENTIFIER);
        }
        catch (const std::runtime_error& ex)
        {
            LOGWARN("Running mode not set in plugin.conf file. Using the default running mode EDR");
        }

        if (m_isXDR)
        {
            LOGINFO("Flags running mode is XDR");
        }
        else
        {
            LOGINFO("Flags running mode is EDR");
        }
    }

    void PluginAdapter::dataFeedExceededCallback()
    {
        LOGWARN("Datafeed limit has been hit. Disabling scheduled queries");

        sendLiveQueryStatus();
        stopOsquery();
        m_osqueryConfigurator.disableQueryPack(Plugin::osqueryXDRConfigFilePath());
        // osquery will automatically be restarted but set this to make sure there is no delay.
        m_restartNoDelay = true;
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

    bool PluginAdapter::haveCustomQueriesChanged(const std::optional<std::string>& customQueries)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::optional<std::string> oldCustomQueries;

        if (fs->exists(osqueryCustomConfigFilePath()))
        {
            oldCustomQueries = fs->readFile(osqueryCustomConfigFilePath());
        }

        return customQueries != oldCustomQueries;
    }

    void PluginAdapter::processLiveQueryPolicy(const std::string& liveQueryPolicy)
    {
        if (!liveQueryPolicy.empty())
        {
            std::optional<std::string> customQueries;
            std::vector<Json::Value> foldingRules;
            try
            {
                m_dataLimit = getDataLimit(liveQueryPolicy);
                m_loggerExtensionPtr->setDataLimit(m_dataLimit);
                m_liveQueryRevId = getRevId(liveQueryPolicy);
                customQueries = getCustomQueries(liveQueryPolicy);
                foldingRules = getFoldingRules(liveQueryPolicy);
                m_liveQueryStatus = "Same";
            }
            catch (std::exception& e)
            {
                LOGERROR("Failed to read LiveQuery Policy: " << e.what());
                m_liveQueryStatus = "Failure";
            }

            try
            {
                if (m_liveQueryStatus != "Failure" && haveCustomQueriesChanged(customQueries))
                {
                    auto fs = Common::FileSystem::fileSystem();

                    if (fs->exists(osqueryCustomConfigFilePath()))
                    {
                        fs->removeFileOrDirectory(osqueryCustomConfigFilePath());
                    }
                    if (customQueries.has_value())
                    {
                        fs->writeFile(osqueryCustomConfigFilePath(), customQueries.value());
                    }
                    stopOsquery();
                    m_restartNoDelay = true;
                }
            }
            catch (Common::FileSystem::IFileSystemException &e)
            {
                LOGWARN("Filesystem Exception While removing/writing custom query file: " << e.what());
            }

            if (m_liveQueryStatus != "Failure" && !foldingRules.empty())
            {
                LOGDEBUG("Processing LiveQuery Policy folding rules");
                for (const auto& r : foldingRules)
                {
                    LOGDEBUG(r);
                }

                // TODO: configure logger extension
            }

            return;

        }
        else
        {
            m_liveQueryStatus = "NoRef";
        }
        m_liveQueryRevId = "";
    }

    // update status

    void PluginAdapter::sendLiveQueryStatus()
    {
        bool dailyDataLimitExceeded = m_loggerExtensionPtr->getDataLimitReached();
        std::string statusXml = serializeLiveQueryStatus(dailyDataLimitExceeded);
        LOGINFO("Sending LiveQuery Status");
        m_baseService->sendStatus("LiveQuery", statusXml, statusXml);
    }

    bool PluginAdapter::hasScheduleEpochEnded(time_t now)
    {
        return (now - SCHEDULE_EPOCH_DURATION) > m_scheduleEpoch.getValue();
    }


} // namespace Plugin