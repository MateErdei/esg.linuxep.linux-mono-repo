/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ManagementAgent/StatusCache/IStatusCache.h"

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
                    Common::TaskQueue::ITaskQueueSharedPtr taskQueue, const std::shared_ptr<ManagementAgent::StatusCache::IStatusCache>& statusCache);

            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo) override;
        private:
            Common::TaskQueue::ITaskQueueSharedPtr m_taskQueue;
            std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> m_statusCache;
            std::string m_tempDir;
            std::string m_statusDir;
        };
    }
}



