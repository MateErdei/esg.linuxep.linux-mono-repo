/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PolicyReceiverImpl.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/UtilityImpl/PolicyFileUtilities.h>

using namespace ManagementAgent::McsRouterPluginCommunicationImpl;

namespace ManagementAgent
{
    namespace PolicyReceiverImpl
    {
        PolicyReceiverImpl::PolicyReceiverImpl(
            std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue,
            PluginCommunication::IPluginManager& pluginManager) :
            m_taskQeue(taskQueue),
            m_pluginManager(pluginManager)
        {
            m_policyDir = Common::ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath();
            m_internalPolicyDir = Common::ApplicationConfiguration::applicationPathManager().getInternalPolicyFilePath();
        }

        bool PolicyReceiverImpl::receivedGetPolicyRequest(const std::string& appId)
        {
            bool policyTaskAddedToQueue = false;

            std::vector<std::string> policyFiles;
            auto fs  = Common::FileSystem::fileSystem();
            try
            {
                policyFiles = fs->listFiles(m_policyDir);
            }
            catch (Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("Failed to load file list from : " << m_policyDir << ". With error, " << e.what());
                return false;
            }
            try
            {
                if (fs->isDirectory(m_internalPolicyDir))
                {
                    std::vector<std::string> internalPolicyFiles = fs->listFiles(m_internalPolicyDir);
                    policyFiles.insert(policyFiles.end(),internalPolicyFiles.begin(),internalPolicyFiles.end());
                }
            }
            catch (Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("Failed to load file list from : " << m_internalPolicyDir << ". With error, " << e.what());
                return false;
            }
            for (auto& policyFile : policyFiles)
            {
                LOGSUPPORT("Checking policyFile: " << policyFile << " for appid: " << appId);
                std::string appid_file = UtilityImpl::extractAppIdFromPolicyFile(policyFile);
                if (!appid_file.empty() && appid_file == appId)
                {
                    LOGSUPPORT("Queue policy file to be sent to plugins " << policyFile);
                    std::unique_ptr<Common::TaskQueue::ITask> task(new PolicyTask(m_pluginManager, policyFile));

                    m_taskQeue->queueTask(std::move(task));

                    policyTaskAddedToQueue = true;
                }
            }

            return policyTaskAddedToQueue;
        }

    } // namespace PolicyReceiverImpl
} // namespace ManagementAgent
