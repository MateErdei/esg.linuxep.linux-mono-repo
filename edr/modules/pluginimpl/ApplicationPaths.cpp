/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

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

std::string Plugin::lockFilePath()
{
    return fromRelative("var/edr.lock");
}

std::string Plugin::getVersionIniFilePath()
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

std::string Plugin::osqueryPidFile()
{
    return fromRelative("var/osquery.pid");
}

std::string Plugin::syslogPipe()
{
    return fromRelative("var/syslog_pipe");
}

std::string Plugin::osQueryLogDirectoryPath()
{
    return fromRelative("log");
}

std::string Plugin::osQueryResultsLogPath()
{
    return fromRelative("log/osqueryd.results.log");
}

std::string Plugin::osQueryDataBasePath()
{
    return fromRelative("var/osquery.db");
}

std::string Plugin::edrConfigFilePath()
{
    return fromRelative("etc/plugin.conf");
}
std::string Plugin::systemctlPath()
{
    return "/bin/systemctl";
}
std::string Plugin::servicePath()
{
    return "/sbin/service";
}

std::string Plugin::osQueryExtensionsPath() {
    return fromRelative("extensions");
}

std::string Plugin::livequeryExecutable()
{
    return fromRelative("bin/sophos_livequery");
}
