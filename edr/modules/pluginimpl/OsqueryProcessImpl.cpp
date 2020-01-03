/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OsqueryProcessImpl.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <modules/Proc/ProcUtilities.h>

#include <cassert>
#include <iterator>
#include <unistd.h>

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
            // TODO make sure that when this throws we catch else where.
            fileSystem->removeFileOrDirectory(osquerySocket);
        }

        const std::vector<std::string>& arguments = { "--config_path=" + Plugin::osqueryConfigFilePath(),
                                                      "--flagfile=" + Plugin::osqueryFlagsFilePath() };

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath());
        regenerateOSQueryConfigFile(Plugin::osqueryConfigFilePath());
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

    void OsqueryProcessImpl::regenerateOSQueryConfigFile(const std::string& osqueryConfigFilePath)
    {
        char rawHostname[1024];
        gethostname(rawHostname, 1024);
        std::string hostname(rawHostname);

        auto fileSystem = Common::FileSystem::fileSystem();

        if (fileSystem->isFile(osqueryConfigFilePath))
        {
            fileSystem->removeFile(osqueryConfigFilePath);
        }

        std::stringstream osqueryConfiguration;

        osqueryConfiguration << R"(
        {
            "options": {
                "host_identifier": ")" << hostname << R"(",
                "schedule_splay_percent": 10
            },
            "schedule": {
                "process_events": {
                    "query": "select count(*) from process_events;",
                    "interval": 86400
                },
                "user_events": {
                    "query": "select count(*) from user_events;",
                    "interval": 86400
                },
                "selinux_events": {
                    "query": "select count(*) from selinux_events;",
                    "interval": 86400
                },
                "socket_events": {
                    "query": "select count(*) from socket_events;",
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
                                         "--logger_stderr=true",
                                         "--logger_mode=420",
                                         "--logger_min_stderr=0",
                                         "--logger_min_status=0",
                                         "--disable_watchdog=false",
                                         "--watchdog_level=0",
                                         "--watchdog_memory_limit=250",
                                         "--watchdog_utilization_limit=30",
                                         "--watchdog_delay=60",
                                         "--enable_extensions_watchdog=false",
                                         "--disable_audit=false",
                                         "--audit_allow_config=true",
                                         "--audit_allow_process_events=true",
                                         "--audit_allow_fim_events=false",
                                         "--audit_allow_selinux_events=true",
                                         "--audit_allow_sockets=true",
                                         "--audit_allow_user_events=true",
                                         "--events_expiry=86400" };

        flags.push_back("--pidfile=" + Plugin::osqueryPidFile());
        flags.push_back("--database_path=" + Plugin::osQueryDataBasePath());
        flags.push_back("--extensions_socket=" + Plugin::osquerySocket());
        flags.push_back("--logger_path=" + Plugin::osQueryLogPath());

        std::ostringstream flagsAsString;
        std::copy(flags.begin(), flags.end(), std::ostream_iterator<std::string>(flagsAsString, "\n"));

        fileSystem->writeFile(osqueryFlagsFilePath, flagsAsString.str());
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
