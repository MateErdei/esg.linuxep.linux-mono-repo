// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/TaskQueue/ITaskQueue.h"
#include "ManagementAgent/PluginCommunication/IThreatHealthReceiver.h"

namespace ManagementAgent::ThreatHealthReceiverImpl
{
    class ThreatHealthReceiverImpl : public virtual ManagementAgent::PluginCommunication::IThreatHealthReceiver
    {
    public:
        explicit ThreatHealthReceiverImpl(Common::TaskQueue::ITaskQueueSharedPtr taskQueue);

        bool receivedThreatHealth(
            const std::string& pluginName,
            const std::string& threatHealth,
            std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj) override;

    private:
        Common::TaskQueue::ITaskQueueSharedPtr m_taskQueue;
    };
} // namespace ManagementAgent::ThreatHealthReceiverImpl
