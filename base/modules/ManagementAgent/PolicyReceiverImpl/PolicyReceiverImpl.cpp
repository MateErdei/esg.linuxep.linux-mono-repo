/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyReceiverImpl.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>



using namespace ManagementAgent::McsRouterPluginCommunicationImpl;

namespace ManagementAgent
{
    namespace PolicyReceiverImpl
    {
        PolicyReceiverImpl::PolicyReceiverImpl(std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue,
                                               PluginCommunication::IPluginManager& pluginManager)
                : m_taskQeue(taskQueue)
        , m_pluginManager(pluginManager)
        {
            m_policyDir = Common::ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath();
        }

        bool PolicyReceiverImpl::receivedGetPolicyRequest(const std::string& appId)
        {

            bool policyTaskAddedToQueue = false;

            std::vector<std::string> policyFiles;

            try
            {
                policyFiles = Common::FileSystem::fileSystem()->listFiles(m_policyDir);
            }
            catch (Common::FileSystem::IFileSystemException& e )
            {
                LOGERROR("Failed to load file list from : " << m_policyDir << ". With error, " << e.what());
                return false;
            }


            for(auto& policyFile : policyFiles)
            {
                LOGSUPPORT("Checking policyFile: " << policyFile << " for appid: " << appId);
                if (policyFile.find(appId) != std::string::npos)
                {

                    LOGSUPPORT("Queue policy file to be sent to plugins " << policyFile);
                    std::unique_ptr<Common::TaskQueue::ITask> task(new PolicyTask(m_pluginManager, policyFile));

                    m_taskQeue->queueTask(task);

                    policyTaskAddedToQueue = true;
                }
            }

            return policyTaskAddedToQueue;
        }


    }
}
