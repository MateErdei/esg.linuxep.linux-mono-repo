/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "ApplicationPaths.h"
#include "IOsqueryProcess.h"
#include "Logger.h"
#include "TelemetryConsts.h"

#include <Common/FileSystem/IFileSystem.h>
#include <modules/Proc/ProcUtilities.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/Proc/ProcUtilities.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <cmath>

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
        std::shared_ptr<PluginCallback> callback,
        std::unique_ptr<livequery::IQueryProcessor> queryProcessor,
        std::unique_ptr<livequery::IResponseDispatcher> responseDispatcher) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_queryProcessor{std::move(queryProcessor)},
        m_responseDispatcher{std::move(responseDispatcher)},
        m_timesOsqueryProcessFailedToStart(0)
    {
    }

    void PluginAdapter::mainLoop()
    {
        LOGINFO("Entering the main loop");
        prepareSystemForPlugin();
        setUpOsqueryMonitor();

        std::unique_ptr<WaitUpTo> m_delayedRestart;
        while (true)
        {
            Task task = m_queueTask->pop();
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
                    LOGDEBUG("Process task OSQUERYPROCESSFINISHED");
                    m_timesOsqueryProcessFailedToStart = 0;
                    LOGDEBUG("osquery stopped. Scheduling its restart in 10 seconds.");
                    Common::Telemetry::TelemetryHelper::getInstance().increment(plugin::telemetryOsqueryRestarts, 1UL);
                    m_delayedRestart.reset( // NOLINT
                        new WaitUpTo(std::chrono::seconds(10), [this]() { this->m_queueTask->pushRestartOsquery(); }));
                    break;
                case Task::TaskType::Policy:
                    LOGWARN("No policy expected for EDR plugin");
                    break;
                case Task::TaskType::Query:
                    processQuery(task.m_content, task.m_correlationId);
                    break;
                case Task::TaskType::OSQUERYPROCESSFAILEDTOSTART:
                    static unsigned int baseDelay = 10;
                    static unsigned int growthBase = 2;
                    static unsigned int maxTime = 320;

                    auto delay =
                        static_cast<unsigned int>(baseDelay * ::pow(growthBase, m_timesOsqueryProcessFailedToStart));
                    if (delay < maxTime)
                    {
                        m_timesOsqueryProcessFailedToStart++;
                    }
                    LOGWARN("The osquery process failed to start. Scheduling a retry in " << delay << " seconds.");
                    Common::Telemetry::TelemetryHelper::getInstance().increment(plugin::telemetryOsqueryRestarts, 1UL);
                    m_delayedRestart.reset( // NOLINT
                        new WaitUpTo(
                            std::chrono::seconds(delay), [this]() { this->m_queueTask->pushRestartOsquery(); }));
            }
        }
    }

    void PluginAdapter::setUpOsqueryMonitor()
    {
        LOGDEBUG("Setup monitoring of osquery");
        std::shared_ptr<QueueTask> queue = m_queueTask;
        std::shared_ptr<Plugin::IOsqueryProcess> osqueryProcess { createOsqueryProcess() };
        m_osqueryProcess = osqueryProcess;
        m_monitor = std::async(std::launch::async, [queue, osqueryProcess]() {
            try
            {
                osqueryProcess->keepOsqueryRunning();
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

    void PluginAdapter::processQuery(const std::string &queryJson, const std::string &correlationId)
    {
        livequery::processQuery(*m_queryProcessor, *m_responseDispatcher, queryJson, correlationId);
    }


    void PluginAdapter::regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        if (fileSystem->isFile(osqueryConfigFilePath))
        {
            fileSystem->removeFile(osqueryConfigFilePath);
        }

        std::stringstream osqueryConfiguration;

        osqueryConfiguration << R"(
        {
            "options": {
                "schedule_splay_percent": 10
            },
            "schedule": {
                "process_events": {
                    "query": "select count(*) as process_events_count from process_events;",
                    "interval": 86400
                },
                "user_events": {
                    "query": "select count(*) as user_events_count from user_events;",
                    "interval": 86400
                },
                "selinux_events": {
                    "query": "select count(*) as selinux_events_count from selinux_events;",
                    "interval": 86400
                },
                "socket_events": {
                    "query": "select count(*) as socket_events_count from socket_events;",
                    "interval": 86400
                }
            }
        })";

        fileSystem->writeFile(osqueryConfigFilePath, osqueryConfiguration.str());

    }

    void PluginAdapter::regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath)
    {
        auto fileSystem = Common::FileSystem::fileSystem();

        if (fileSystem->isFile(osqueryFlagsFilePath))
        {
            fileSystem->removeFile(osqueryFlagsFilePath);
        }

        std::vector<std::string> flags { "--host_identifier=uuid",
                                         "--log_result_events=true",
                                         "--utc",
                                         "--disable_extensions=false",
                                         "--logger_stderr=false",
                                         "--logger_mode=420",
                                         "--logger_min_stderr=1",
                                         "--logger_min_status=1",
                                         "--disable_watchdog=false",
                                         "--watchdog_level=0",
                                         "--watchdog_memory_limit=250",
                                         "--watchdog_utilization_limit=30",
                                         "--watchdog_delay=60",
                                         "--enable_extensions_watchdog=false",
                                         "--disable_extensions=false",
                                         "--disable_audit=false",
                                         "--audit_persist=true",
                                         "--enable_syslog=true",
                                         "--audit_allow_config=true",
                                         "--audit_allow_process_events=true",
                                         "--audit_allow_fim_events=false",
                                         "--audit_allow_selinux_events=true",
                                         "--audit_allow_sockets=true",
                                         "--audit_allow_user_events=true",
                                         "--syslog_events_expiry=604800",
                                         "--events_expiry=604800",
                                         "--force=true",
                                         "--disable_enrollment=true",
                                         "--enable_killswitch=false",
                                         "--events_max=20000"};

        flags.push_back("--syslog_pipe_path=" + Plugin::syslogPipe()),
            flags.push_back("--pidfile=" + Plugin::osqueryPidFile());
        flags.push_back("--database_path=" + Plugin::osQueryDataBasePath());
        flags.push_back("--extensions_socket=" + Plugin::osquerySocket());
        flags.push_back("--logger_path=" + Plugin::osQueryLogPath());

        std::ostringstream flagsAsString;
        std::copy(flags.begin(), flags.end(), std::ostream_iterator<std::string>(flagsAsString, "\n"));

        fileSystem->writeFile(osqueryFlagsFilePath, flagsAsString.str());
    }


    bool PluginAdapter::checkIfServiceActive(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec("/bin/systemctl", { "is-active", serviceName });

        return (process->exitCode() == 0);
    }

    void PluginAdapter::stopSystemService(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec("/bin/systemctl", { "stop", serviceName });

        if(process->exitCode() == 0)
        {
            LOGINFO("Successfully stopped service: " << serviceName);
            process->exec("/bin/systemctl", { "disable", serviceName });
            if(process->exitCode() == 0)
            {
                LOGINFO("Successfully disabled service: " << serviceName);
            }
            else
            {
                LOGWARN("Failed to disable service: " << serviceName);
            }
        }
        else
        {
            LOGWARN("Failed to stop service: " << serviceName);
        }
    }

    void PluginAdapter::prepareSystemForPlugin()
    {
        auto fileSystem = Common::FileSystem::fileSystem();

        bool disableAuditD = true;

        if (fileSystem->isFile(Plugin::edrConfigFilePath()))
        {
            LOGINFO("prepareSystemBeforeStartingOSQuery found file: " <<  Plugin::edrConfigFilePath());
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(Plugin::edrConfigFilePath(), ptree);
                disableAuditD = (ptree.get<std::string>("disable_autitd") == "1");
                LOGINFO("prepareSystemBeforeStartingOSQuery success returned: " << disableAuditD);
            }
            catch (boost::property_tree::ptree_error& ex)
            {
                LOGINFO("prepareSystemBeforeStartingOSQuery failure returned: " << disableAuditD);
            }
        }
        else
        {
            LOGINFO("Could not find EDR Plugin config file: " <<  Plugin::edrConfigFilePath());
        }

        std::string serviceName("auditd");

        if(disableAuditD)
        {
            if (checkIfServiceActive(serviceName))
            {
                stopSystemService(serviceName);
            }
            else
            {
                LOGINFO("AuditD not found on system or is not active.");
            }

            // Check to make sure all platforms use the same service name for auditd
        }
        else
        {
            if(checkIfServiceActive(serviceName))
            {
                LOGINFO("EDR configuration set to not disable AuditD.");
            }
        }

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath());
        regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());
    }


} // namespace Plugin
