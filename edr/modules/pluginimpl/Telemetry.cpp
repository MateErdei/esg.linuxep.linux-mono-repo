// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ApplicationPaths.h"
#include "Logger.h"
#include "TelemetryConsts.h"
#include "Telemetry.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include <nlohmann/json.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <time.h>
namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace plugin
{
    std::optional<std::string> getVersion()
    {
        try
        {
            Path versionIniFilepath = Plugin::getVersionIniFilePath();
            boost::property_tree::ptree pTree;
            boost::property_tree::read_ini(versionIniFilepath, pTree);
            return pTree.get<std::string>(PRODUCT_VERSION_STR);
        }
        catch (std::exception& ex)
        {
            LOGERROR("Telemetry cannot find the plugin version");
            return std::nullopt;
        }
    }

    std::optional<unsigned long> getOsqueryDatabaseSize()
    {
        try
        {
           auto fs = Common::FileSystem::fileSystem();
           auto files = fs->listFiles(Plugin::osQueryDataBasePath());
           unsigned long size = 0;
           for (auto& file : files)
            {
               size += fs->fileSize(file);
            }
           return size;
        }
        catch (std::exception& ex)
        {
            LOGERROR("Telemetry cannot get size of osquery database files");
            return std::nullopt;
        }
    }

    void readOsqueryInfoFiles()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::vector<std::string> files = fs->listFiles(Plugin::osQueryLogDirectoryPath());
        for (auto file: files)
        {
            if (Common::UtilityImpl::StringUtils::isSubstring(file,"osqueryd.INFO"))
            {
                std::time_t created = fs->lastModifiedTime(file);
                std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
                double difference = difftime(created,now);
                if (difference < 86400)
                {
                    LOGDEBUG("Reading contents of " << file << " for overflow indicators");
                    std::vector<std::string> lines = fs->readLines(file);
                    for (auto& line:lines)
                    {
                        processOsqueryLogLineForEventsMaxTelemetry(line);
                    }
                }


            }
        }
    }

    void processOsqueryLogLineForEventsMaxTelemetry(std::string& logLine)
    {

        if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "Expiring events for subscriber:"))
        {
            auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
            std::string contents = telemetry.serialise();
            nlohmann::json array = nlohmann::json::parse(contents);
            std::string key;
            //example error log: Expiring events for subscriber: syslog_events (overflowed limit 100000)

            std::vector<std::string> sections = Common::UtilityImpl::StringUtils::splitString(logLine,"Expiring events for subscriber:");
            std::vector<std::string> unstrippedQueryName = Common::UtilityImpl::StringUtils::splitString(sections[1],"(");
            std::string tableName = Common::UtilityImpl::StringUtils::replaceAll(unstrippedQueryName[0], " ", "");

            if (tableName == "process_events")
            {
                key = plugin::telemetryProcessEventsMaxHit;
            }
            else if (tableName == "selinux_events")
            {
                key = plugin::telemetrySelinuxEventsMaxHit;
            }
            else if (tableName == "syslog_events")
            {
                key = plugin::telemetrySyslogEventsMaxHit;
            }
            else if (tableName == "user_events")
            {
                key = plugin::telemetryUserEventsMaxHit;
            }
            else if (tableName == "socket_events")
            {
                key = plugin::telemetrySocketEventsMaxHit;
            }
            else
            {
                LOGERROR("Table name " << tableName << " not recognised, cannot set event_max telemetry for it");

            }

            telemetry.set(key, true);

            LOGDEBUG("Setting true for telemetry key: " << key);

        }

    }

} // namespace plugin
