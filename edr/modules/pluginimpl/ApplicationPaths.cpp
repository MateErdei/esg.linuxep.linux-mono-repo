/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ApplicationPaths.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
namespace
{
    std::string fromRelative(const std::string& relative)
    {
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        return Common::FileSystem::join(installPath, "plugins/edr", relative);
    }
} // namespace

std::string Plugin::osqueryBinaryName()
{
    return "osqueryd";
}

std::string Plugin::osqueryPath()
{
    return Common::FileSystem::join(fromRelative("bin"), osqueryBinaryName());
}

std::string Plugin::pidFile()
{
    return fromRelative("var/edr.lock");
}

std::string Plugin::getEdrVersionIniFilePath()
{
    return fromRelative("VERSION.ini");
}
std::string Plugin::osqueryFlagsFilePath()
{
    return fromRelative("etc/osquery.flags");
}

std::string Plugin::osquerySocket()
{
    return fromRelative("var/osquery.sock");
}

std::string Plugin::osqueryConfigFilePath()
{
    return fromRelative("etc/osquery.conf");
}

//
//std::string Plugin::getDbosLogsDir()
//{
//    return fromRelative("dbos/data/logs");
//}
//
//std::string Plugin::getOsqueryWatcherLog()
//{
//    return Common::FileSystem::join(getDbosLogsDir(), "osquery.watcher.log");
//}
//
//std::string Plugin::getOsqueryOutputLog()
//{
//    return Common::FileSystem::join(getDbosLogsDir(), "osqueryd.output.log");
//}
//
//std::string Plugin::getRestartOSQueryLogLineCacheCPU()
//{
//    return fromRelative("var/TelemetryOSQueryRestartCPU.txt");
//}
//
//std::string Plugin::getRestartOSQueryLogLineCacheMemory()
//{
//    return fromRelative("var/TelemetryOSQueryRestartMemory.txt");
//}
//
//std::string Plugin::getPurgeDatabaseOSQueryLogLineCache()
//{
//    return fromRelative("var/TelemetryDBPurge.txt");
//}

