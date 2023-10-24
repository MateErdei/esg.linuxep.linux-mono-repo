// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "EdrCommon/ApplicationPaths.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"

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

std::string Plugin::osqueryConfigDirectoryPath()
{
    return fromRelative("etc/osquery.conf.d");
}

std::string Plugin::osqueryXDRConfigFilePath()
{
    return fromRelative("etc/osquery.conf.d/sophos-scheduled-query-pack.conf");
}

std::string Plugin::osqueryMTRConfigFilePath()
{
    return fromRelative("etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf");
}

std::string Plugin::osqueryXDRConfigStagingFilePath()
{
    return fromRelative("etc/query_packs/sophos-scheduled-query-pack.conf");
}

std::string Plugin::osqueryMTRConfigStagingFilePath()
{
    return fromRelative("etc/query_packs/sophos-scheduled-query-pack.mtr.conf");
}

std::string Plugin::osqueryNextXDRConfigStagingFilePath()
{
    return fromRelative("etc/query_packs/sophos-scheduled-query-pack-next.conf");
}

std::string Plugin::osqueryNextMTRConfigStagingFilePath()
{
    return fromRelative("etc/query_packs/sophos-scheduled-query-pack-next.mtr.conf");
}

std::string Plugin::osqueryXDRResultSenderIntermediaryFilePath()
{
    return fromRelative("var/xdr_intermediary");
}

std::string Plugin::osqueryCustomConfigFilePath()
{
    return fromRelative("etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf");
}

std::string Plugin::osqueryXDROutputDatafeedFilePath()
{
    return Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
        "base/mcs/datafeed");
}

std::string Plugin::osqueryPidFile()
{
    return fromRelative("var/osquery.pid");
}

std::string Plugin::syslogPipe()
{
    return Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
        "shared/syslog_pipe");
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

std::string Plugin::osQueryExtensionsDirectory()
{
    return fromRelative("extensions");
}

std::string Plugin::osQueryExtensionsPath()
{
    return Common::FileSystem::join(osQueryExtensionsDirectory(), "extensions.load");
}

std::string Plugin::livequeryExecutable()
{
    return fromRelative("bin/sophos_livequery");
}

std::string Plugin::osQueryLensesPath()
{
    return fromRelative("osquery/lenses");
}

std::string Plugin::livequeryResponsePath()
{
    std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    return Common::FileSystem::join(installPath, "base/mcs/response");
}

std::string Plugin::etcDir()
{
    return fromRelative("etc");
}

std::string Plugin::varDir()
{
    return fromRelative("var");
}

std::string Plugin::getJRLPath()
{
    return Common::FileSystem::join(varDir(), "jrl");
}

std::string Plugin::mtrFlagsFile()
{
    return Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
        "plugins/mtr/dbos/data/osquery.flags");
}

std::string Plugin::edrBinaryPath()
{
    return fromRelative("bin/edr");
}


