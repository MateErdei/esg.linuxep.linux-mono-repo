/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StatusReceiverImpl.h"
#include "StatusTask.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

ManagementAgent::StatusReceiverImpl::StatusReceiverImpl::StatusReceiverImpl(
        Common::TaskQueue::ITaskQueueSharedPtr taskQueue)
    : m_taskQueue(std::move(taskQueue))
{
    m_tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
    m_statusDir = Common::ApplicationConfiguration::applicationPathManager().getMcsStatusFilePath();
}

void ManagementAgent::StatusReceiverImpl::StatusReceiverImpl::receivedChangeStatus(const std::string& appId,
                                                                                   const Common::PluginApi::StatusInfo& statusInfo)
{
    std::unique_ptr<Common::TaskQueue::ITask>
            task(
                    new StatusTask(
                            m_statusCache,
                            appId,
                            statusInfo.statusXml,
                            statusInfo.statusWithoutTimestampsXml,
                            m_tempDir,
                            m_statusDir
                            )
                    );

    // Add task to queue
    m_taskQueue->queueTask(task);
}
