/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITaskQueue.h>
#include <ManagementAgent/PluginCommunication/IEventReceiver.h>

namespace ManagementAgent
{
    namespace EventReceiverImpl
    {
        class EventReceiverImpl : public virtual ManagementAgent::PluginCommunication::IEventReceiver
        {
        public:
            explicit EventReceiverImpl(Common::TaskQueue::ITaskQueueSharedPtr taskQueue);

            void receivedSendEvent(const std::string& appId, const std::string& eventXml) override;

        private:
            Common::TaskQueue::ITaskQueueSharedPtr m_taskQueue;
        };

    } // namespace EventReceiverImpl
} // namespace ManagementAgent
