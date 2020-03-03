/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryJsonToMap.h>
#include <redist/boost/include/boost/property_tree/ini_parser.hpp>
#include <redist/boost/include/boost/property_tree/ptree.hpp>

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
    const std::string LOG_MESSAGE = "msg";
    const std::string SPLIT_STRING = "#SPLIT#";
    const std::string OSQUERY_RESTART_CPU_LOG_MSG =
        "osqueryd worker (" + SPLIT_STRING + ") stopping: Maximum sustainable CPU utilization limit exceeded:";
    const std::string OSQUERY_RESTART_MEMORY_LOG_MSG =
        "osqueryd worker (" + SPLIT_STRING + ") stopping: Memory limits exceeded:";
    const std::string DATABASE_PURGE_LOG_MSG = "Osquery database watcher purge completed";
    static constexpr unsigned long MAX_SYSLOG_SIZE = 1024 * 1024 * 50; // 50Mb limit on file size
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

} // namespace plugin
