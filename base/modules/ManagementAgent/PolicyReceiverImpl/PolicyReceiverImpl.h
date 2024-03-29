// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/TaskQueue/ITaskQueue.h"
#include "ManagementAgent/PluginCommunication/IPluginManager.h"
#include "ManagementAgent/PluginCommunication/IPolicyReceiver.h"

namespace ManagementAgent::PolicyReceiverImpl
{
    class PolicyReceiverImpl : public virtual PluginCommunication::IPolicyReceiver
    {
    public:
        explicit PolicyReceiverImpl(
            std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue,
            PluginCommunication::IPluginManager& pluginManager);

        bool receivedGetPolicyRequest(const std::string& appId, const std::string& pluginName) override;

    private:
        std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQeue;
        PluginCommunication::IPluginManager& m_pluginManager;
        std::string m_policyDir;
        std::string m_internalPolicyDir;
    };
} // namespace ManagementAgent::PolicyReceiverImpl
