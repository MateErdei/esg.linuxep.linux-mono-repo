/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "StatusCache.h"

#include <ManagementAgent/PluginCommunication/IStatusReceiver.h>
#include <Common/TaskQueue/ITaskQueue.h>

namespace ManagementAgent
{
    namespace StatusReceiverImpl
    {
        class StatusReceiverImpl : public virtual ManagementAgent::PluginCommunication::IStatusReceiver
        {
        public:
            explicit StatusReceiverImpl(
                    Common::TaskQueue::ITaskQueueSharedPtr taskQueue, StatusCache& statusCache);

            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo) override;
        private:
            Common::TaskQueue::ITaskQueueSharedPtr m_taskQueue;
            StatusCache m_statusCache;
            std::string m_tempDir;
            std::string m_statusDir;
        };
    }
}



