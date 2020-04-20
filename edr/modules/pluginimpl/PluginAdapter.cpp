/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "ApplicationPaths.h"
#include "IOsqueryProcess.h"
#include "Logger.h"
#include "TelemetryConsts.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/Proc/ProcUtilities.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/PluginApi/NoPolicyAvailableException.h>

#include <cmath>
#include <unistd.h>
#include <fstream>

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
#include "Telemetry.h"

#include <livequery/ResponseDispatcher.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_parallelQueryProcessor{queryrunner::createQueryRunner(Plugin::osquerySocket())},
        m_timesOsqueryProcessFailedToStart(0),
        m_osqueryConfigurator()
    {
    }

    void PluginAdapter::mainLoop()
    {
        try
        {
            m_baseService->requestPolicies("ALC");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: " << "ALC");
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

        std::string alcPolicy = waitForTheFirstALCPolicy(*m_queueTask, std::chrono::seconds(5), 5);
        LOGSUPPORT("Processing ALC Policy");
        processALCPolicy(alcPolicy, true);
        LOGSUPPORT("Cleanup Old Osquery Files");
        cleanUpOldOsqueryFiles();
        LOGSUPPORT("Start Osquery");
        setUpOsqueryMonitor();

        std::unique_ptr<WaitUpTo> m_delayedRestart;
        while (true)
        {
            Task task;
            if (!m_queueTask->pop(task, QUEUE_TIMEOUT))
            {
                LOGDEBUG("No activity for " << QUEUE_TIMEOUT << " seconds. Checking files");
                cleanUpOldOsqueryFiles();
                if (pluginMemoryAboveThreshold())
                {
                    LOGINFO("Plugin stopping, memory usage exceeded: " << MAX_PLUGIN_MEM_BYTES / 1000 << "kB");
                    stopOsquery();
                    return;
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
                        LOGDEBUG("Process task Policy");
                        processALCPolicy(task.m_content, false);
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

    void PluginAdapter::cleanUpOldOsqueryFiles()
    {
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
                    setUpOsqueryMonitor();
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
        m_osqueryConfigurator.prepareSystemForPlugin();
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
        osqueryStarted.wait_started();
    }

    void PluginAdapter::stopOsquery()
    {
        try
        {
            while (m_osqueryProcess && m_monitor.valid())
            {
                LOGINFO("Issue request to stop to osquery.");
                m_osqueryProcess->requestStop();

                if (m_monitor.wait_for(std::chrono::seconds(2)) == std::future_status::timeout)
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
        m_osqueryConfigurator.loadALCPolicy(policy);
        bool current_enabled = m_osqueryConfigurator.enableAuditDataCollection();
        if (firstTime)
        {
            m_collectAuditEnabled = current_enabled;
            return;
        }

        std::string option{current_enabled?"true":"false"};
        if (current_enabled != m_collectAuditEnabled)
        {
            m_collectAuditEnabled = current_enabled;
            LOGINFO(
                "Option to enable audit collection changed to " << option << ". Scheduling osquery STOP");
            m_restartNoDelay = true;
            stopOsquery();
        }
        else
        {
            LOGDEBUG("Option to enable audit collection remains "<< option);
        }
    }

    std::string PluginAdapter::waitForTheFirstALCPolicy(
        QueueTask& queueTask,
        std::chrono::seconds timeoutInS,
        int maxTasksThreshold)
    {
        std::vector<Plugin::Task> nonPolicyTasks;
        std::string policyContent;
        for (int i = 0; i < maxTasksThreshold; i++)
        {
            Plugin::Task task;
            if (!queueTask.pop(task, timeoutInS.count()))
            {
                LOGINFO("Policy has not been sent to the plugin");
                break;
            }
            if (task.m_taskType == Plugin::Task::TaskType::Policy)
            {
                policyContent = task.m_content;
                LOGINFO("First Policy received.");
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
        LOGDEBUG("Return from WaitForTheFirstALCPolicy ");
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
} // namespace Plugin
