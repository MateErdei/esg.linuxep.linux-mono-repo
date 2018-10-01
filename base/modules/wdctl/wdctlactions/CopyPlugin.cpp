/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CopyPlugin.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>


using namespace wdctl::wdctlactions;

CopyPlugin::CopyPlugin(const Action::Arguments& args)
    : Action(args)
{
}

int CopyPlugin::run()
{
    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();

    Path destination = Common::FileSystem::join(
            pluginRegistry,
            Common::FileSystem::basename(m_args.m_argument)
            );

    LOGDEBUG("Copying "<< m_args.m_argument << " to "<< destination);
    Common::FileSystem::fileSystem()->copyFile(m_args.m_argument, destination);

    try
    {
        Common::FileSystem::fileSystem()->chownChmod(destination, "root", "sophos-spl-group", S_IRUSR | S_IWUSR | S_IRGRP);
    }
    catch (Common::FileSystem::IFileSystemException& error)
    {
        LOGFATAL( error.what());
        return 1;
    }

    return 0;
}
