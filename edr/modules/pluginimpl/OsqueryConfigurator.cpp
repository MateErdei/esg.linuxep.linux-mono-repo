/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OsqueryConfigurator.h"

#include "ApplicationPaths.h"
#include "Logger.h"
#include "PluginUtils.h"
#include "TelemetryConsts.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/FileUtils.h>

#include <thread>
#include <iterator>

namespace Plugin
{
    void OsqueryConfigurator::regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath)
    {
        LOGINFO("Creating osquery root scheduled pack");
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
                },
                "syslog_events": {
                    "query": "select count(*) as syslog_events_count from syslog_events;",
                    "interval": 86400
                }
            }
        })";

        fileSystem->writeFile(osqueryConfigFilePath, osqueryConfiguration.str());
    }

    void OsqueryConfigurator::regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath,
                                                         bool enableAuditEventCollection, bool xdrEnabled,
                                                         time_t scheduleEpoch)
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
                                         "--enable_killswitch=false"};

        std::string eventsMaxValue = "100000";
        std::pair<std::string,std::string> eventsMax = Common::UtilityImpl::FileUtils::extractValueFromFile(Plugin::edrConfigFilePath(), "events_max");
        if (!eventsMax.first.empty())
        {

            if (eventsMax.first.find_first_not_of("1234567890") == std::string::npos)
            {
                LOGINFO("Setting events_max to " << eventsMax.first << " as per value in " << Plugin::edrConfigFilePath());
                eventsMaxValue = eventsMax.first;
            }
            else
            {
                LOGWARN("events_max value in '" << Plugin::edrConfigFilePath() << "' not an integer, so using default of " << eventsMaxValue);
            }

        }
        else
        {
            if (Common::UtilityImpl::StringUtils::startswith(eventsMax.second, "No such node"))
            {
                LOGDEBUG("No events_max value specified in " << Plugin::edrConfigFilePath() << " so using default of " << eventsMaxValue);
            }
            else
            {
                LOGWARN("Failed to retrieve events_max value from " << Plugin::edrConfigFilePath() << " with error: " << eventsMax.second);
            }
        }
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.set(plugin::telemetryEventsMax, eventsMaxValue);
        flags.push_back("--events_max=" + eventsMaxValue);

        if (xdrEnabled)
        {
            LOGDEBUG("Adding XDR flags to osquery flags file.");
            flags.emplace_back("--extensions_timeout=10");
            flags.emplace_back("--extensions_require=SophosLoggerPlugin");
            flags.emplace_back("--logger_plugin=SophosLoggerPlugin");

            std::stringstream scheduleEpochSS;
            scheduleEpochSS << "--schedule_epoch=" << scheduleEpoch;
            LOGDEBUG("Using osquery schedule_epoch flag as: " << scheduleEpochSS.str());
            flags.push_back(scheduleEpochSS.str());
        }

        bool networkTables;
        try
        {
            networkTables = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(PluginUtils::NETWORK_TABLES_AVAILABLE);
        }
        catch (const std::runtime_error& ex)
        {
            LOGWARN("Unable to retrieve network table setting from config due to: " << ex.what());
            networkTables = false;
        }

        if (!networkTables)
        {
            LOGDEBUG("Adding disable tables flag to osquery flags file.");
            flags.emplace_back("--disable_tables=curl,curl_certificate");
        }

        flags.push_back("--syslog_pipe_path=" + Plugin::syslogPipe());
        flags.push_back("--pidfile=" + Plugin::osqueryPidFile());
        flags.push_back("--extensions_socket=" + Plugin::osquerySocket());
        flags.push_back("--database_path=" + Plugin::osQueryDataBasePath());
        flags.push_back("--logger_path=" + Plugin::osQueryLogDirectoryPath());
        flags.push_back("--extensions_autoload=" + Plugin::osQueryExtensionsPath());


        std::string disableAuditFlagValue = enableAuditEventCollection ? "false" : "true";
        flags.push_back("--disable_audit=" + disableAuditFlagValue);
        addTlsServerCertsOsqueryFlag(flags);

        flags.push_back("--augeas_lenses=" + Plugin::osQueryLensesPath());

        std::ostringstream flagsAsString;
        std::copy(flags.begin(), flags.end(), std::ostream_iterator<std::string>(flagsAsString, "\n"));

        fileSystem->writeFile(osqueryFlagsFilePath, flagsAsString.str());
    }

    void OsqueryConfigurator::prepareSystemForPlugin(bool xdrEnabled, time_t scheduleEpoch)
    {
        bool disableAuditD = retrieveDisableAuditFlagFromSettingsFile();

        bool disableAuditDataGathering = enableAuditDataCollection();

        SystemConfigurator::setupOSForAudit(disableAuditD);

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath(), disableAuditDataGathering, xdrEnabled, scheduleEpoch);
        regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());

        if (xdrEnabled)
        {
            enableQueryPack(Plugin::osqueryXDRConfigFilePath());
        }
        else
        {
            disableQueryPack(Plugin::osqueryXDRConfigFilePath());
        }
    }

    bool OsqueryConfigurator::retrieveDisableAuditFlagFromSettingsFile() const
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        bool disableAuditD = true;
        std::string configpath = Plugin::edrConfigFilePath();
        if (fileSystem->isFile(configpath))
        {
            try
            {
                disableAuditD = !Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("disable_auditd");
            }
            catch (std::runtime_error& ex)
            {
                LOGINFO("Using default value for disable_audit flag");
                Plugin::PluginUtils::setGivenFlagFromSettingsFile("disable_auditd", disableAuditD);
            }
        }
        else
        {
            LOGWARN("Could not find EDR Plugin config file: " << configpath
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
        bool enableAuditDataCollection = retrieveDisableAuditFlagFromSettingsFile() && !MTRBoundEnabled();

        if (enableAuditDataCollection)
        {
            LOGINFO("Plugin configured to gather data from auditd netlink");
        }
        else
        {
            LOGINFO("Plugin configured not to gather data from auditd netlink");
        }
        return enableAuditDataCollection;
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

    void OsqueryConfigurator::addTlsServerCertsOsqueryFlag(std::vector<std::string>& flags)
    {
        const std::vector<Path> caPaths = { "/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt"};

        for (const auto& caPath : caPaths)
        {
            if (Common::FileSystem::fileSystem()->isFile(caPath))
            {
                flags.push_back("--tls_server_certs=" + caPath);
                return;
            }
        }
        LOGWARN("CA path not found");
    }

    void OsqueryConfigurator::enableQueryPack(const std::string& queryPackFilePath)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string disabledPath = queryPackFilePath + ".DISABLED";
        if (fs->exists(disabledPath))
        {
            try
            {
                fs->moveFile(disabledPath, queryPackFilePath);
                LOGDEBUG("Enabled query pack conf file");
            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to enabled query pack conf file: " << ex.what());
            }
        }
    }

    void OsqueryConfigurator::disableQueryPack(const std::string& queryPackFilePath)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->exists(queryPackFilePath))
        {
            try
            {
                fs->moveFile(queryPackFilePath, queryPackFilePath + ".DISABLED");
                LOGDEBUG("Disabled query pack conf file");
            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to disable query pack conf file: " << ex.what());
            }
        }
    }

} // namespace Plugin
