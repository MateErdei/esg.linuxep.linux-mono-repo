/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <redist/boost/include/boost/property_tree/ini_parser.hpp>
#include <redist/boost/include/boost/property_tree/ptree.hpp>

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

} // namespace plugin
