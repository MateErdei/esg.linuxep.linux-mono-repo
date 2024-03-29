// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "RemoveAction.h"

#include "Logger.h"
#include "StopAction.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"

using namespace wdctl::wdctlactions;

RemoveAction::RemoveAction(const Action::Arguments& args) : ZMQAction(args) {}

int RemoveAction::run()
{
    LOGINFO("Attempting to remove " << m_args.m_argument);
    std::unique_ptr<wdctl::wdctlactions::Action> action;
    action.reset(new wdctl::wdctlactions::StopAction(m_args));
    action->run();

    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();

    Path destination =
        Common::FileSystem::join(pluginRegistry, Common::FileSystem::basename(m_args.m_argument) + ".json");

    int result = 0;

    try
    {
        Common::FileSystem::fileSystem()->removeFile(destination);
        LOGINFO("Remove the plugin registry");
    }
    catch (const Common::FileSystem::IFileSystemException& e)
    {
        LOGERROR("Unable to delete " << destination);
        result = 1;
    }

    auto response = doOperationToWatchdog({ "REMOVE", m_args.m_argument });

    if (isSuccessfulOrWatchdogIsNotRunning(response))
    {
        return result;
    }

    LOGERROR("Failed to remove " << m_args.m_argument << ": " << response.at(0));
    return 2;
}
