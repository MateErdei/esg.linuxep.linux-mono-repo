/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ActionTask.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <ManagementAgent/LoggerImpl/Logger.h>

ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::ActionTask(
    ManagementAgent::PluginCommunication::IPluginManager& pluginManager,
    const std::string& filePath) :
    m_pluginManager(pluginManager),
    m_filePath(std::move(filePath))
{
}

void ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::run()
{
    LOGSUPPORT("Process new action from mcsrouter: " << m_filePath);
    std::string basename = Common::FileSystem::basename(m_filePath);

    size_t pos = basename.find("_action_");

    if (pos == std::string::npos)
    {
        LOGWARN("Got an invalid file name for action detection: " << m_filePath);
        return;
    }
    //TODO: IMPLEMENT THE REQUEST WITH CORRELATION HERE
    std::string appId = basename.substr(0, pos);
    std::string payload;

    try
    {
        payload = Common::FileSystem::fileSystem()->readFile(m_filePath);
    }
    catch (Common::FileSystem::IFileSystemException& e)
    {
        LOGERROR("Failed to read " << m_filePath << " with error: " << e.what());
        throw;
    }

    int pluginsNotified = m_pluginManager.queueAction(appId, payload, "");
    LOGINFO("Action " << m_filePath << " sent to " << pluginsNotified << " plugins");

    Common::FileSystem::fileSystem()->removeFile(m_filePath);
}
