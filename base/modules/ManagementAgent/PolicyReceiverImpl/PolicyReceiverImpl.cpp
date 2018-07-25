/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyReceiverImpl.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>



using namespace ManagementAgent::McsRouterPluginCommunicationImpl;

namespace ManagementAgent
{
    namespace PolicyReceiverImpl
    {
        PolicyReceiverImpl::PolicyReceiverImpl(const std::string& mcsDir,
                                               std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue,
                                               PluginCommunication::IPluginManager& pluginManager)
        : m_mcsDir(mcsDir)
        , m_taskQeue(taskQueue)
        , m_pluginManager(pluginManager)
        {
            m_policyDir = Common::FileSystem::fileSystem()->join(mcsDir, "policy");
        }

        bool PolicyReceiverImpl::receivedGetPolicyRequest(const std::string& appId, const std::string& policyId)
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

            std::string policyFileName = appId + "-" + policyId + "_policy.xml";

            for(auto& policyFile : policyFiles)
            {
                if(policyFile == policyFileName)
                {
                    std::string fullPolicyFilePath = Common::FileSystem::fileSystem()->join(m_policyDir, policyFile);

                    std::unique_ptr<Common::TaskQueue::ITask> task(new PolicyTask(m_pluginManager, fullPolicyFilePath));

                    m_taskQeue->queueTask(task);

                    policyTaskAddedToQueue = true;
                }
            }

            return policyTaskAddedToQueue;
        }


    }
}
