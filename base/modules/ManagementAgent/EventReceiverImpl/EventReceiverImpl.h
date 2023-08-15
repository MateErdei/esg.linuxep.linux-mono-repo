// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IOutbreakModeController.h"

#include "Common/TaskQueue/ITaskQueue.h"
#include "ManagementAgent/PluginCommunication/IEventReceiver.h"

namespace ManagementAgent
{
    namespace EventReceiverImpl
    {
        class EventReceiverImpl : public virtual ManagementAgent::PluginCommunication::IEventReceiver
        {
        public:
            explicit EventReceiverImpl(Common::TaskQueue::ITaskQueueSharedPtr taskQueue);

            void receivedSendEvent(const std::string& appId, const std::string& eventXml) override;

            void handleAction(const std::string& actionXml) override;

            [[nodiscard]] bool outbreakMode() const override;

        private:
            Common::TaskQueue::ITaskQueueSharedPtr m_taskQueue;
            IOutbreakModeControllerPtr outbreakModeController_;
        };

    } // namespace EventReceiverImpl
} // namespace ManagementAgent
