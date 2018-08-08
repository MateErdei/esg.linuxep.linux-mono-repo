/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "RemoveAction.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <Common/FileSystem/IFileSystemException.h>

using namespace wdctl::wdctlactions;

RemoveAction::RemoveAction(const Action::Arguments &args)
        : ZMQAction(args)
{
}

int RemoveAction::run()
{
    std::string pluginRegistry = Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath();

    Path destination = Common::FileSystem::join(
            pluginRegistry,
            Common::FileSystem::basename(m_args.m_argument)+".json"
    );

    int result = 0;

    try
    {
        Common::FileSystem::fileSystem()->removeFile(destination);
    }
    catch (const Common::FileSystem::IFileSystemException& e)
    {
        LOGERROR("Unable to delete "<<destination);
        result = 1;
    }

    Common::ZeroMQWrapper::ISocketRequesterPtr socket = m_context->getRequester();
    socket->connect(Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress());

    socket->write({"REMOVE",m_args.m_argument});

    auto response = socket->read();

    if (response.size() == 1 && response.at(0) == "OK")
    {
        return result;
    }

    LOGERROR("Failed to remove "<< m_args.m_argument<<": "<<response.at(0));
    return 2;
}
