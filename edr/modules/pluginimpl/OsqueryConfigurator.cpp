// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "OsqueryConfigurator.h"

#include "EdrCommon/ApplicationPaths.h"
#include "Logger.h"
#include "PluginUtils.h"
#include "EdrCommon/TelemetryConsts.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/FileUtils.h"

#include <thread>
#include <iterator>

namespace Plugin
{
    OsqueryConfigurator::OsqueryConfigurator()
    {
    }

    void OsqueryConfigurator::regenerateOsqueryOptionsConfigFile(const std::string& osqueryConfigFilePath, unsigned long expiry)
    {
        LOGINFO("Creating osquery options config file");
        auto fileSystem = Common::FileSystem::fileSystem();
        if (fileSystem->isFile(osqueryConfigFilePath))
        {
            fileSystem->removeFile(osqueryConfigFilePath);
        }

        std::stringstream osqueryConfiguration;

        osqueryConfiguration << R"(
        {
            "options": {
                "events_expiry": @@expiry@@,
                "syslog_events_expiry": @@expiry@@
            }
        })";

        std::string finalConfigString =
            Common::UtilityImpl::StringUtils::replaceAll(
                osqueryConfiguration.str(), "@@expiry@@", std::to_string(expiry));

        fileSystem->writeFile(osqueryConfigFilePath, finalConfigString);
    }

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
                "flush_process_events": {
                    "query": "select count(*) as process_events_count from process_events;",
                    "interval": 3600
                },
                "flush_user_events": {
                    "query": "select count(*) as user_events_count from user_events;",
                    "interval": 3600
                },
                "flush_selinux_events": {
                    "query": "select count(*) as selinux_events_count from selinux_events;",
                    "interval": 3600
                },
                "flush_syslog_events": {
                    "query": "select count(*) as syslog_events_count from syslog_events;",
                    "interval": 3600
                }
            }
        })";

        fileSystem->writeFile(osqueryConfigFilePath, osqueryConfiguration.str());
    }

    void OsqueryConfigurator::regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath,
                                                         bool enableAuditEventCollection,
                                                         bool enableScheduledQueries,
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
                                         "--enable_extensions_watchdog=true",
                                         "--disable_extensions=false",
                                         "--audit_persist=true",
                                         "--enable_syslog=true",
                                         "--audit_allow_config=true",
                                         "--audit_allow_process_events=true",
                                         "--audit_allow_fim_events=false",
                                         "--audit_allow_selinux_events=true",
                                         "--audit_allow_apparmor_events=true",
                                         "--audit_allow_sockets=false", // socket events can be extremely expensive
                                         "--audit_allow_user_events=true",
                                         "--syslog_events_expiry=604800",
                                         "--events_expiry=604800",
                                         "--force=true",
                                         "--disable_enrollment=true",
                                         "--enable_killswitch=false",
                                         "--verbose",
                                         "--config_refresh=3600"};

        std::string eventsMaxValue = std::to_string(PluginUtils::getEventsMaxFromConfig());
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.set(plugin::telemetryEventsMax, eventsMaxValue);
        flags.push_back("--events_max=" + eventsMaxValue);

        std::vector<std::string> flagsFromConfig = PluginUtils::getWatchdogFlagsFromConfig();
        for (auto const& flag :flagsFromConfig)
        {
            flags.emplace_back(flag);
        }

        auto discoveryQueryInterval = PluginUtils::getDiscoveryQueryFlagFromConfig();
        if (!discoveryQueryInterval.empty())
        {
            flags.emplace_back(discoveryQueryInterval);
        }


        if (enableScheduledQueries)
        {
            LOGDEBUG("Adding XDR flags to osquery flags file.");
            flags.emplace_back("--extensions_timeout=10");
            // There is a bug in osquery https://github.com/osquery/osquery/issues/7256 where it does not wait for
            // all extensions in the list to register, just the first.
            // flags.emplace_back("--extensions_require=SophosLoggerPlugin,SophosExtension");
            flags.emplace_back("--extensions_require=SophosLoggerPlugin");
            flags.emplace_back("--logger_plugin=SophosLoggerPlugin");

            std::stringstream scheduleEpochSS;
            scheduleEpochSS << "--schedule_epoch=" << scheduleEpoch;
            LOGDEBUG("Using osquery schedule_epoch flag as: " << scheduleEpochSS.str());
            flags.push_back(scheduleEpochSS.str());
        }
        else
        {
            flags.emplace_back("--extensions_require=SophosExtension");
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
        m_disableAuditDInPluginConfig = retrieveDisableAuditDFlagFromSettingsFile();
        bool disableAuditDataGathering = shouldAuditDataCollectionBeEnabled();

        SystemConfigurator::setupOSForAudit(m_disableAuditDInPluginConfig);

        regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath(), disableAuditDataGathering, xdrEnabled, scheduleEpoch);
        regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());
    }

    bool OsqueryConfigurator::retrieveDisableAuditDFlagFromSettingsFile() const
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        bool disableAuditD = true;
        std::string configpath = Plugin::edrConfigFilePath();
        if (fileSystem->isFile(configpath))
        {
            try
            {
                disableAuditD = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("disable_auditd");
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

    bool OsqueryConfigurator::shouldAuditDataCollectionBeEnabled() const
    {
        if (m_disableAuditDInPluginConfig)
        {
            LOGDEBUG("Plugin should gather data from audit subsystem netlink");
            return true;
        }
        else
        {
            LOGDEBUG("Plugin should not gather data from audit subsystem netlink");
            return false;
        }
    }

    void OsqueryConfigurator::addTlsServerCertsOsqueryFlag(std::vector<std::string>& flags)
    {
        const std::vector<Path> caPaths = {
            "/etc/ssl/certs/ca-certificates.crt",
            "/etc/pki/tls/certs/ca-bundle.crt",
            "/etc/ssl/ca-bundle.pem"};

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

} // namespace Plugin
