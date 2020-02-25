/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OsqueryConfigurator.h"
#include "ApplicationPaths.h"
#include "Logger.h"
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Process/IProcess.h>

#include <thread>

namespace Plugin{
    void OsqueryConfigurator::regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath)
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

    void OsqueryConfigurator::regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath, bool enableAuditEventCollection)
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
                                         "--events_max=20000" };

        flags.push_back("--syslog_pipe_path=" + Plugin::syslogPipe());
        flags.push_back("--pidfile=" + Plugin::osqueryPidFile());
        flags.push_back("--database_path=" + Plugin::osQueryDataBasePath());
        flags.push_back("--extensions_socket=" + Plugin::osquerySocket());
        flags.push_back("--logger_path=" + Plugin::osQueryLogPath());

        std::string disableAuditFlagValue = enableAuditEventCollection ? "false":"true";
        flags.push_back("--disable_audit=" + disableAuditFlagValue);

        std::ostringstream flagsAsString;
        std::copy(flags.begin(), flags.end(), std::ostream_iterator<std::string>(flagsAsString, "\n"));

        fileSystem->writeFile(osqueryFlagsFilePath, flagsAsString.str());
    }

    bool OsqueryConfigurator::checkIfServiceActive(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec(Plugin::systemctlPath(), { "is-active", serviceName });

        return (process->exitCode() == 0);
    }

    void OsqueryConfigurator::stopAndDisableSystemService(const std::string& serviceName)
    {
        auto process = Common::Process::createProcess();
        process->exec(Plugin::systemctlPath(), { "stop", serviceName });

        if (process->exitCode() == 4)
        {
            // handle error: unit auditd.service may be requested by dependency only
            // try to stop the service again using service command
            process->exec(Plugin::servicePath(), { serviceName, "stop" });
        }

        if (process->exitCode() == 0)
        {
            LOGINFO("Successfully stopped service: " << serviceName);
            process->exec(Plugin::systemctlPath(), { "disable", serviceName });
            if (process->exitCode() == 0)
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

    void OsqueryConfigurator::prepareSystemForPlugin()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        bool disableAuditD = true;

        if (fileSystem->isFile(Plugin::edrConfigFilePath()))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(Plugin::edrConfigFilePath(), ptree);
                disableAuditD = (ptree.get<std::string>("disable_auditd") == "1");
            }
            catch (boost::property_tree::ptree_error& ex)
            {
                LOGWARN("Failed to read disable_auditd configuration from config file, using default value");
            }
        }
        else
        {
            LOGWARN(
                    "Could not find EDR Plugin config file: " << Plugin::edrConfigFilePath()
                                                              << ", using disable_auditd default value");
        }

        std::string auditdServiceName("auditd");
        if (disableAuditD)
        {
            LOGINFO("EDR configuration set to disable AuditD");
            if (checkIfServiceActive(auditdServiceName))
            {
                try
                {
                    stopAndDisableSystemService(auditdServiceName);
                }
                catch (std::exception& ex)
                {
                    LOGERROR("Failed to stop and disable auditd");
                }
            }
            else
            {
                LOGINFO("AuditD not found on system or is not active.");
            }
            breakLinkBetweenJournaldAndAuditSubsystem();
        }
        else
        {
            LOGINFO("EDR configuration set to not disable AuditD");
            if (checkIfServiceActive(auditdServiceName))
            {
                LOGWARN("AuditD is running, it will not be possible to obtain event data.");
            }
        }

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath(), disableAuditD);
        regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());
    }

    std::tuple<int, std::string> OsqueryConfigurator::runSystemCtlCommand(const std::string& command, const std::string& target)
    {
        auto process = Common::Process::createProcess();
        process->exec(Plugin::systemctlPath(), { command, target });
        return std::make_tuple(process->exitCode(), process->output());
    }

    bool OsqueryConfigurator::checkIfJournaldLinkedToAuditSubsystem()
    {
        return std::get<1>(runSystemCtlCommand("is-enabled", "systemd-journald-audit.socket")) != "masked";
    }

    void OsqueryConfigurator::maskJournald()
    {
        LOGINFO("Masking journald audit socket");
        auto [maskExitCode, maskOutput] = runSystemCtlCommand("mask", "systemd-journald-audit.socket");
        if (maskExitCode != 0)
        {
            std::stringstream ss;
            ss << "Masking systemd-journald-audit.socket failed, exit code: " << maskExitCode << ", output: " << maskOutput;
            throw std::runtime_error(ss.str());
        }

        int timesToTry = 5;
        int msToWait = 200;
        for (int i = 0; i < timesToTry; ++i)
        {
            if (!checkIfJournaldLinkedToAuditSubsystem())
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(msToWait));
        }

        std::stringstream ss;
        ss << "systemd-journald-audit.socket was not masked after " << timesToTry * msToWait << "ms";
        throw std::runtime_error(ss.str());
    }

    void OsqueryConfigurator::breakLinkBetweenJournaldAndAuditSubsystem()
    {
        if (checkIfJournaldLinkedToAuditSubsystem())
        {
            try
            {
                maskJournald();

                // Restart systemd-journald service
                auto [stopJournaldExitCode, stopJournaldOutput] = runSystemCtlCommand("stop", "systemd-journald");
                if (stopJournaldExitCode != 0)
                {
                    LOGERROR("Failed to stop systemd-journald, exit code: " << stopJournaldExitCode << ", output: " << stopJournaldOutput);
                }

                auto [startJournaldExitCode, startJournaldOutput] = runSystemCtlCommand("start", "systemd-journald");
                if (startJournaldExitCode != 0)
                {
                    LOGERROR("Failed to start systemd-journald, exit code: " << startJournaldExitCode << ", output: " << startJournaldOutput);
                }
            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to mask journald audit socket: " << ex.what());
            }
        }
        else
        {
            LOGDEBUG("Journald audit socket is already masked");
        }
    }

}
