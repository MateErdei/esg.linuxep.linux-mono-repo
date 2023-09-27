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
        return Common::FileSystem::join(installPath, "plugins/liveresponse", relative);
    }
} // namespace

std::string Plugin::getVersionIniFilePath()
{
    return fromRelative("VERSION.ini");
}

std::string Plugin::sessionsDirectoryPath()
{
    return fromRelative("var");
}

std::string Plugin::liveResponseTempPath()
{
    return fromRelative("tmp");
}

std::string Plugin::liveResponseExecutable()
{
    return fromRelative("bin/sophos-live-terminal");
}
