/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CopyPlugin.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

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

    group* sophosSplGroup = getgrnam("sophos-spl-group");

    if (sophosSplGroup)
    {
        if (chown(destination.c_str(), 0, sophosSplGroup->gr_gid) != 0)
        {
            LOGERROR("chown failed to set group owner on file " << destination << " to sophos-spl-group");
            return 1;
        }
    }
    else
    {
        LOGERROR("Group sophos-spl-group does not exist");
        return 1;
    }

    if (chmod(destination.c_str(), S_IRUSR | S_IWUSR | S_IRGRP) != 0)
    {
        LOGERROR("chmod failed to set file permissions to 0640 on " << destination);
        return 1;
    }

    return 0;
}
