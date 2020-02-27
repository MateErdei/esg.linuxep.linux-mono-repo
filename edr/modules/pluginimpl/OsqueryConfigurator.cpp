/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OsqueryConfigurator.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <thread>

namespace Plugin
{
    void OsqueryConfigurator::regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath)
    {
        LOGINFO("Creating osquery scheduled pack");
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

    void OsqueryConfigurator::regenerateOSQueryFlagsFile(
        const std::string& osqueryFlagsFilePath,
        bool enableAuditEventCollection)
    {
        LOGINFO("Creating osquery flags file");
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

        std::string disableAuditFlagValue = enableAuditEventCollection ? "false" : "true";
        flags.push_back("--disable_audit=" + disableAuditFlagValue);

        std::ostringstream flagsAsString;
        std::copy(flags.begin(), flags.end(), std::ostream_iterator<std::string>(flagsAsString, "\n"));

        fileSystem->writeFile(osqueryFlagsFilePath, flagsAsString.str());
    }

    void OsqueryConfigurator::prepareSystemForPlugin()
    {
        bool disableAuditD = enableAuditDataCollection();
        if (disableAuditD)
        {
            LOGINFO("Configuring Osquery and the System to collect audit data");
        }
        else
        {
            LOGINFO("Configuring Osquery and the System to not collect audit data");
        }

        SystemConfigurator::setupOSForAudit(disableAuditD);

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath(), disableAuditD);
        regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());
    }

    bool OsqueryConfigurator::retrieveDisableAuditFlagFromSettingsFile() const
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        bool disableAuditD = true;
        std::string configpath = Plugin::edrConfigFilePath();
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
        return disableAuditD;
    }

    bool OsqueryConfigurator::ALCContainsMTRFeature(const std::string& alcPolicyXMl)
    {
        using namespace Common::XmlUtilities;
        if (alcPolicyXMl.empty())
        {
            return false;
        }
        try
        {
            AttributesMap attributesMap = parseXml(alcPolicyXMl);
            const std::string expectedMTRFeaturePath { "AUConfigurations/Features/Feature#MDR" };
            Attributes attributes = attributesMap.lookup(expectedMTRFeaturePath);
            return !attributes.empty();
        }
        catch (XmlUtilitiesException& ex)
        {
            std::string reason = ex.what();
            LOGWARN("Failed to parse ALC policy: " << reason);
        }
        return false;
    }

    bool OsqueryConfigurator::enableAuditDataCollection() const
    {
        return disableSystemAuditDAndTakeOwnershipOfNetlink() and !MTRBoundEnabled();
    }

    void OsqueryConfigurator::loadALCPolicy(const std::string& alcPolicy)
    {
        m_mtrboundEnabled = ALCContainsMTRFeature(alcPolicy);
        if (m_mtrboundEnabled)
        {
            LOGINFO("Detected MTR is enabled");
        }
        else
        {
            LOGINFO("No MTR Detected");
        }
    }

    bool OsqueryConfigurator::MTRBoundEnabled() const
    {
        return m_mtrboundEnabled;
    }

    bool OsqueryConfigurator::disableSystemAuditDAndTakeOwnershipOfNetlink() const
    {
        if (retrieveDisableAuditFlagFromSettingsFile())
        {
            LOGINFO("plugins.conf configured to collect audit data from osquery");
            return true;
        }
        LOGINFO("plugins.conf configured to not collect audit data from osquery");
        return false;
    }

} // namespace Plugin
