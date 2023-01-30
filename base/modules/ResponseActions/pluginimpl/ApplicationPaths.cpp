/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
namespace
{
    std::string fromRelative(const std::string& relative)
    {
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        // When forking from this repo remove the logerror and replace TemplatePlugin with the new plugin name
        LOGERROR("new plugin name has not been added");
        return Common::FileSystem::join(installPath, "plugins/TemplatePlugin", relative);
    }
} // namespace

std::string Plugin::getVersionIniFilePath()
{
    return fromRelative("VERSION.ini");
}



