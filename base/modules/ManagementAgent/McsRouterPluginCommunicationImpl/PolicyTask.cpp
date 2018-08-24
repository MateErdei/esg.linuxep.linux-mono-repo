/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyTask.h"
#include "Logger.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/RegexUtilities.h>


namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {

        void PolicyTask::run()
        {
            LOGSUPPORT("Process new policy from mcsrouter: " << m_filePath);

            std::string appId = PolicyTask::ExtractAppIdFromPolicyFile(m_filePath);

            if (appId.empty())
            {
                LOGWARN("Got an invalid file name for policy detection: "<<m_filePath);
                return;
            }

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

            int newPolicyResult = m_pluginManager.applyNewPolicy(appId, payload);
            LOGINFO("Policy " << m_filePath << " applied to " << newPolicyResult << " plugins");
        }

        PolicyTask::PolicyTask(
                ManagementAgent::PluginCommunication::IPluginManager& pluginManager, std::string filePath)
                : m_pluginManager(pluginManager)
                  , m_filePath(std::move(filePath))
        {
        }

/**
 * The policy file pattern currently implemented by mcsrouter: GenericAdapter::__processPolicy
 * is as follow: AppID[-PolicyType]_policy.xml
 */
        std::string PolicyTask::ExtractAppIdFromPolicyFile(
                const std::string& policyPath)
        {
            std::string policyFileName = Common::FileSystem::basename(policyPath);
            std::string PolicyPattern{R"sophos(([\w]+)(-[\w]+)?_policy.xml)sophos"};
            return Common::UtilityImpl::returnFirstMatch(PolicyPattern, policyFileName);

        }

    }

}