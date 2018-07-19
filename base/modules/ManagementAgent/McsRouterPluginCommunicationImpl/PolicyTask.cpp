/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyTask.h"
#include "Logger.h"
#include <Common/FileSystem/IFileSystem.h>

void ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask::run()
{

    std::string basename = Common::FileSystem::fileSystem()->basename(m_filePath);

    size_t pos = basename.find("-");

    if (pos == std::string::npos)
    {
        LOGWARN("Got an invalid file name for policy detection: "<<m_filePath);
        return;
    }

    std::string appId = basename.substr(0, pos);
    std::string payload = Common::FileSystem::fileSystem()->readFile(m_filePath);

    m_pluginManager.applyNewPolicy(appId, payload);
}

ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask::PolicyTask(
        ManagementAgent::PluginCommunication::IPluginManager& pluginManager, std::string filePath)
    : m_pluginManager(pluginManager),
      m_filePath(std::move(filePath))
{
}
