/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StopAction.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>

using namespace wdctl::wdctlactions;

StopAction::StopAction(const wdctl::wdctlarguments::Arguments& args)
        : ZMQAction(args)
{
}

int StopAction::run()
{

    LOGINFO("Attempting to stop "<<m_args.m_argument);

    auto response = doOperationToWatchdog({"STOP",m_args.m_argument});

    if (isSuccessful(response))
    {
        return 0;
    }

    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();
    std::string registryFile = m_args.m_argument + ".json";

    Path destination = Common::FileSystem::join(
            pluginRegistry,
            Common::FileSystem::basename(registryFile)
    );

    if (Common::FileSystem::fileSystem()->isFile(m_args.m_argument))
    {
        LOGERROR("Failed to stop "<< m_args.m_argument<<": "<<response.at(0));
        return 1;
    }
    else
    {
        LOGINFO("Plugin "<<m_args.m_argument << " not in registry");
        return 0;
    }




}
