/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CopyPlugin.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <grp.h>
#include <unistd.h>

using namespace wdctl::wdctlactions;

CopyPlugin::CopyPlugin(const Action::Arguments& args) : Action(args) {}

int CopyPlugin::run()
{
    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();

    std::string basePluginName = Common::FileSystem::basename(m_args.m_argument);

    if (basePluginName.length() > 25)
    { // Assumes that .json is included in the filename
        LOGFATAL("Plugin name is longer than the maximum 20 characters.");
        return 1;
    }

    Path destination = Common::FileSystem::join(pluginRegistry, Common::FileSystem::basename(m_args.m_argument));

    LOGDEBUG("Copying " << m_args.m_argument << " to " << destination);
    Common::FileSystem::fileSystem()->copyFile(m_args.m_argument, destination);

    try
    {
        Common::FileSystem::filePermissions()->chmod(destination, S_IRUSR | S_IWUSR | S_IRGRP);
        Common::FileSystem::filePermissions()->chown(destination, "root", "sophos-spl-group");
    }
    catch (Common::FileSystem::IFileSystemException& error)
    {
        LOGFATAL(error.what());
        return 1;
    }

    return 0;
}
