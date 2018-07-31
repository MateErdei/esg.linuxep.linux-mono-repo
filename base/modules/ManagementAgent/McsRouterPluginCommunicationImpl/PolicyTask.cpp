/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyTask.h"
#include "Logger.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>

void ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask::run()
{
    LOGSUPPORT("Process new policy from mcsrouter: " << m_filePath);
    std::string basename = Common::FileSystem::fileSystem()->basename(m_filePath);

    size_t pos = basename.find("-");

    if (pos == std::string::npos)
    {
        LOGWARN("Got an invalid file name for policy detection: "<<m_filePath);
        return;
    }

    std::string appId = basename.substr(0, pos);

    std::string payload;

    try
    {
        payload = Common::FileSystem::fileSystem()->readFile(m_filePath);
    }
    catch(Common::FileSystem::IFileSystemException &e)
    {
        LOGERROR("Failed to read " << m_filePath << " with error: " << e.what());
        throw;
    }

    int newPolicyResult = m_pluginManager.applyNewPolicy(appId, payload);
    LOGINFO("Policy " << m_filePath << " applied to " << newPolicyResult << " plugins");
}

ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask::PolicyTask(
        ManagementAgent::PluginCommunication::IPluginManager& pluginManager, std::string filePath)
    : m_pluginManager(pluginManager),
      m_filePath(std::move(filePath))
{
}
