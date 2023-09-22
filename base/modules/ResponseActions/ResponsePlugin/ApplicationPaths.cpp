// Copyright 2023 Sophos Limited. All rights reserved.
#include "ApplicationPaths.h"

#include "Logger.h"
#include "ResponseActions/ResponsePlugin/config.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
namespace
{
    std::string fromRelative(const std::string& relative)
    {
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string name = RA_PLUGIN_NAME;
        return Common::FileSystem::join(installPath, "plugins/" + name, relative);
    }
} // namespace

std::string ResponsePlugin::getVersionIniFilePath()
{
    return fromRelative("VERSION.ini");
}
