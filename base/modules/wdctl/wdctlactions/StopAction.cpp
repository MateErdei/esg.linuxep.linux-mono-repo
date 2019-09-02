/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StopAction.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <watchdog/watchdogimpl/Watchdog.h>

#include <thread>

using namespace wdctl::wdctlactions;

StopAction::StopAction(const wdctl::wdctlarguments::Arguments& args) : ZMQAction(args) {}

int StopAction::run()
{
    LOGINFO("Attempting to stop " << m_args.m_argument);

    auto response = doOperationToWatchdog({ "STOP", m_args.m_argument });

    if (isSuccessful(response))
    {
        for (int i = 0; i < 10; i++)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            StopAction::IsRunningStatus isRunningStatus = checkIsRunning();
            if (isRunningStatus == StopAction::IsRunningStatus::IsNotRunning)
            {
                return 0;
            }
        }
        LOGINFO("Watchdog did not confirm the plugin is not running after 10 seconds: " << m_args.m_argument);
        return 3;
    }

    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();
    std::string registryFile = m_args.m_argument + ".json";

    Path destination = Common::FileSystem::join(pluginRegistry, Common::FileSystem::basename(registryFile));

    if (Common::FileSystem::fileSystem()->isFile(destination))
    {
        LOGERROR("Failed to stop " << m_args.m_argument << ": " << response.at(0));
        return 1;
    }
    else
    {
        LOGERROR("Plugin \"" << m_args.m_argument << "\" not in registry");
        return 2;
    }
}

StopAction::IsRunningStatus StopAction::checkIsRunning()
{
    auto response = doOperationToWatchdog({ "ISRUNNING", m_args.m_argument });
    if (response.empty())
    {
        return StopAction::IsRunningStatus::Undefined;
    }
    if (response.at(0) == watchdog::watchdogimpl::watdhdogReturnsNotRunning)
    {
        return StopAction::IsRunningStatus::IsNotRunning;
    }
    if (response.at(0) == watchdog::watchdogimpl::watchdogReturnsOk)
    {
        return StopAction::IsRunningStatus::IsRunning;
    }
    return StopAction::IsRunningStatus::Undefined;
}
