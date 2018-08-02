/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include "CopyPlugin.h"
#include "Logger.h"

using namespace wdctl::wdctlimpl;

CopyPlugin::CopyPlugin(const wdctl::wdctlimpl::Arguments& args)
    : m_args(args)
{
}

int CopyPlugin::run()
{
    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();
    LOGDEBUG("Copying "<< m_args.m_argument << " to "<< pluginRegistry);

    Common::FileSystem::fileSystem()->copyFile(m_args.m_argument,pluginRegistry);

    return 0;
}
