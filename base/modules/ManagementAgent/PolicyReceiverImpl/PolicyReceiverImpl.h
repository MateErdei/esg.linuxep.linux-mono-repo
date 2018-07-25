/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_POLICYRECEIVERIMPL_H
#define EVEREST_BASE_POLICYRECEIVERIMPL_H



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
            explicit PolicyReceiverImpl(const std::string& mcsDir, std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue, PluginCommunication::IPluginManager& pluginManager);
            bool receivedGetPolicyRequest(const std::string& appId, const std::string& policyId) override;
        private:
            std::string m_mcsDir;
            std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQeue;
            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_policyDir;

        };

    }
}

#endif //EVEREST_BASE_POLICYRECEIVERIMPL_H
