// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationPaths.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"

namespace
{
    std::string fromRelative(const std::string& relative)
    {
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        return Common::FileSystem::join(installPath, "plugins/deviceisolation", relative);
    }
} // namespace

std::string Plugin::getVersionIniFilePath()
{
    return fromRelative("VERSION.ini");
}
