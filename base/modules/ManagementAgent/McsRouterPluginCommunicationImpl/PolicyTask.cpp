/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyTask.h"
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/UtilityImpl/PolicyFileUtilities.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/RegexUtilities.h>


namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {

        void PolicyTask::run()
        {
            distributePolicy(m_pluginManager, m_filePath);
        }

        PolicyTask::PolicyTask(
                ManagementAgent::PluginCommunication::IPluginManager& pluginManager, std::string filePath)
                : m_pluginManager(pluginManager)
                  , m_filePath(std::move(filePath))
        {
        }


        void PolicyTask::distributePolicy(PluginCommunication::IPluginManager& pluginManager, const std::string& filepath)
        {
            LOGSUPPORT("Process new policy from mcsrouter: " << filepath);

            std::string appId = UtilityImpl::extractAppIdFromPolicyFile(filepath);

            if (appId.empty())
            {
                LOGWARN("Got an invalid file name for policy detection: "<<filepath);
                return;
            }

            std::string payload;

            try
            {
                payload = Common::FileSystem::fileSystem()->readFile(filepath);
            }
            catch (Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("Failed to read " << filepath << " with error: " << e.what());
                throw;
            }

            int newPolicyResult = pluginManager.applyNewPolicy(appId, payload);
            LOGINFO("Policy " << filepath << " applied to " << newPolicyResult << " plugins");

        }


    }

}