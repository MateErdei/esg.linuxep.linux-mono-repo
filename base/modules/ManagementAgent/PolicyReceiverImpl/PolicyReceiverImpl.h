/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once



#include <ManagementAgent/PluginCommunication/IPolicyReceiver.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <Common/TaskQueue/ITaskQueue.h>

namespace ManagementAgent
{
    namespace PolicyReceiverImpl
    {
    class PolicyReceiverImpl : public virtual PluginCommunication::IPolicyReceiver
        {
        public:
        explicit PolicyReceiverImpl(std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue,
                                    PluginCommunication::IPluginManager& pluginManager);

        bool receivedGetPolicyRequest(const std::string& appId) override;
        private:
            std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQeue;
            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_policyDir;
        };

    }
}


