/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITaskQueue.h>
#include <ManagementAgent/PluginCommunication/IThreatHealthReceiver.h>

namespace ManagementAgent
{
    namespace ThreatHealthReceiverImpl
    {
        class ThreatHealthReceiverImpl : public virtual ManagementAgent::PluginCommunication::IThreatHealthReceiver
        {
        public:
            explicit ThreatHealthReceiverImpl(Common::TaskQueue::ITaskQueueSharedPtr taskQueue);

            void receivedThreatHealth(const std::string& pluginName, const std::string& threatHealth, std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj) override;

        private:
            Common::TaskQueue::ITaskQueueSharedPtr m_taskQueue;
        };

    } // namespace EventReceiverImpl
} // namespace ManagementAgent
