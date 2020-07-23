/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PolicyTask.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/RegexUtilities.h>
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/UtilityImpl/PolicyFileUtilities.h>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        void PolicyTask::run()
        {
            LOGSUPPORT("Process new policy from mcsrouter: " << m_filePath);

            std::string appId = UtilityImpl::extractAppIdFromPolicyFile(m_filePath);

            if (appId.empty())
            {
                LOGWARN("Got an invalid file name for policy detection: " << m_filePath);
                return;
            }

            int newPolicyResult = m_pluginManager.applyNewPolicy(appId, Common::FileSystem::basename(m_filePath));
            LOGINFO("Policy " << m_filePath << " applied to " << newPolicyResult << " plugins");
        }

        PolicyTask::PolicyTask(
            ManagementAgent::PluginCommunication::IPluginManager& pluginManager,
            std::string filePath) :
            m_pluginManager(pluginManager),
            m_filePath(std::move(filePath))
        {
        }

    } // namespace McsRouterPluginCommunicationImpl

} // namespace ManagementAgent