/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OsqueryProcessImpl.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <modules/Proc/ProcUtilities.h>

#include <iterator>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


namespace
{
    constexpr int OUTPUT_BUFFER_LIMIT_KB = 1024;
} // namespace

namespace
{
    class OsqueryProcessFactory
    {
        OsqueryProcessFactory()
        {
            restoreCreator();
        }

    public:
        static OsqueryProcessFactory& instance()
        {
            static OsqueryProcessFactory osqueryProcessFactory;
            return osqueryProcessFactory;
        }
        void replaceCreator(std::function<Plugin::IOsqueryProcessPtr()> newCreator)
        {
            m_creator = newCreator;
        }
        void restoreCreator()
        {
            m_creator = []() { return Plugin::IOsqueryProcessPtr { new Plugin::OsqueryProcessImpl() }; };
        }
        Plugin::IOsqueryProcessPtr createOsquery()
        {
            return m_creator();
        }
        std::function<Plugin::IOsqueryProcessPtr()> m_creator;
    };

} // namespace
Plugin::IOsqueryProcessPtr Plugin::createOsqueryProcess()
{
    return OsqueryProcessFactory::instance().createOsquery();
}

namespace Plugin
{
    ScopedReplaceOsqueryProcessCreator::ScopedReplaceOsqueryProcessCreator(std::function<IOsqueryProcessPtr()> creator)
    {
        OsqueryProcessFactory::instance().replaceCreator(creator);
    }

    ScopedReplaceOsqueryProcessCreator::~ScopedReplaceOsqueryProcessCreator()
    {
        OsqueryProcessFactory::instance().restoreCreator();
    }

    void OsqueryProcessImpl::keepOsqueryRunning()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string osqueryPath = Plugin::osqueryPath();
        killAnyOtherOsquery();
        if (!fileSystem->isFile(osqueryPath))
        {
            std::string errorMessage = "Executable does not exist at: " + osqueryPath;
            throw Plugin::IOsqueryCannotBeExecuted(errorMessage);
        }
        LOGINFO("Run osquery process");
        std::string osquerySocket = Plugin::osquerySocket();

        if (fileSystem->exists(osquerySocket))
        {
            fileSystem->removeFileOrDirectory(osquerySocket);
        }

        std::vector<std::string> arguments = { "--config_path=" + Plugin::osqueryConfigFilePath(),
                                               "--flagfile=" + Plugin::osqueryFlagsFilePath() };

        prepareSystemBeforeStartingOSQuery();

        startProcess(osqueryPath, arguments);
        m_processMonitorPtr->waitUntilProcessEnds();
        LOGINFO("The osquery process finished");
        int exitcode = m_processMonitorPtr->exitCode();
        std::string output = m_processMonitorPtr->output();
        LOGDEBUG("The osquery process output and exit code retrieved");
        {
            std::lock_guard lock { m_processMonitorSharedResource };
            try
            {
                m_processMonitorPtr.reset();
            }
            catch (std::exception& ex)
            {
                LOGWARN("Exception on destroying processmonitor " << ex.what());
            }
        }
        // output should be empty. In case the output has only a few white spaces or new lines or even so few characters
        // that it would not give any help to identify the problem, it is better to avoid having an useless line with :
        // ERROR: <blank>
        // For this reason, applying a minimum threshold (arbitrary) of characters to allow it to be displayed as error
        if (output.size() > 5)
        {
            LOGERROR("The osquery process unexpected logs: " << output);
        }

        switch (exitcode)
        {
            case ECANCELED:
                throw Plugin::IOsqueryCrashed("crashed");
            case ENOEXEC:
                throw Plugin::IOsqueryCannotBeExecuted("Exec format error when executing " + osqueryPath);
            case EACCES:
                throw Plugin::IOsqueryCannotBeExecuted("Permission denied when executing " + osqueryPath);
            default:
                return;
        }
    }

    void OsqueryProcessImpl::requestStop()
    {
        std::lock_guard<std::mutex> lock { m_processMonitorSharedResource };
        if (m_processMonitorPtr)
        {
            m_processMonitorPtr->kill();
        }
    }

    void OsqueryProcessImpl::regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath)
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

    void OsqueryProcessImpl::regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath)
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

    bool OsqueryProcessImpl::checkIfServiceActive(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec("/usr/sbin/service", { serviceName, "stop" });

        return (process->output() == "active");
    }

    void OsqueryProcessImpl::stopSystemService(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec("/usr/sbin/service", { serviceName, "stop" });

        if(process->exitCode() == 0)
        {
            LOGINFO("Successfully stopped service: " << serviceName);
        }
        else
        {
            LOGWARN("Failed to stop service: " << serviceName);
        }
    }

    void OsqueryProcessImpl::prepareSystemBeforeStartingOSQuery()
    {
//        auto fileSystem = Common::FileSystem::fileSystem();
//
//        bool disableAuditD = true;
//
//        if (fileSystem->isFile(Plugin::edrConfigFilePath()))
//        {
//            LOGINFO("prepareSystemBeforeStartingOSQuery found file: " <<  Plugin::edrConfigFilePath());
//            try
//            {
//                boost::property_tree::ptree ptree;
//                boost::property_tree::read_ini(Plugin::edrConfigFilePath(), ptree);
//                disableAuditD = (ptree.get<std::string>("disable_autitd") == "1");
//                LOGINFO("prepareSystemBeforeStartingOSQuery success returned: " << disableAuditD);
//            }
//            catch (boost::property_tree::ptree_error& ex)
//            {
//                LOGINFO("prepareSystemBeforeStartingOSQuery failure returned: " << disableAuditD);
//            }
//        }
//        else
//        {
//            LOGINFO("Could not find EDR Plugin config file: " <<  Plugin::edrConfigFilePath());
//        }
//
//        LOGINFO("disable_autitd flag set to: " << disableAuditD );
//
//        if(disableAuditD)
//        {
//            std::string serviceName("auditd");
//
//            if (checkIfServiceActive(serviceName))
//            {
//                stopSystemService(serviceName);
//            }
//            else
//            {
//                LOGINFO("AuditD not found on system or is not active.");
//            }
//
//            // Check to make sure all platforms use the same service name for auditd
//
//        }

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath());
        regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());
    }


    void OsqueryProcessImpl::startProcess(const std::string& processPath, const std::vector<std::string>& arguments)
    {
        std::lock_guard lock { m_processMonitorSharedResource };
        m_processMonitorPtr = Common::Process::createProcess();
        m_processMonitorPtr->setOutputLimit(OUTPUT_BUFFER_LIMIT_KB);

        m_processMonitorPtr->setOutputTrimmedCallback(
            [](std::string overflowOutput) { LOGERROR("Non expected logs from osquery: " << overflowOutput); });

        m_processMonitorPtr->exec(processPath, arguments, {});
    }

    void OsqueryProcessImpl::killAnyOtherOsquery()
    {
        Proc::ensureNoExecWithThisCommIsRunning(Plugin::osqueryBinaryName(), Plugin::osqueryPath());
    }
} // namespace Plugin
