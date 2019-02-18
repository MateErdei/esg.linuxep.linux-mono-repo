/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITask.h>
#include <ManagementAgent/StatusCache/IStatusCache.h>

#include <memory>

namespace ManagementAgent
{
    namespace StatusReceiverImpl
    {
        class StatusTask : public virtual Common::TaskQueue::ITask
        {
        public:
            StatusTask(
                const std::shared_ptr<ManagementAgent::StatusCache::IStatusCache>& statusCache,
                std::string appId,
                std::string statusXml,
                std::string statusXmlWithoutTimestamps,
                std::string tempDir,
                std::string statusDir);

            void run() override;

        private:
            std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> m_statusCache;
            std::string m_appId;
            std::string m_statusXml;
            std::string m_statusXmlWithoutTimestamps;
            std::string m_statusDir;
            std::string m_tempDir;
        };
    } // namespace StatusReceiverImpl
} // namespace ManagementAgent
