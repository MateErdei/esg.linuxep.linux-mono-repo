/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CopyPlugin.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

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

    return 0;
}
