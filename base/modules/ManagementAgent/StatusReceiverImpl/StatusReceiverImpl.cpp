///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include "StatusReceiverImpl.h"
#include "StatusTask.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TaskQueue/ITask.h>

ManagementAgent::StatusReceiverImpl::StatusReceiverImpl::StatusReceiverImpl(const std::string& mcsDir,
                                                                            Common::TaskQueue::ITaskQueueSharedPtr taskQueue)
    : m_taskQueue(std::move(taskQueue))
{
    m_tempDir = Common::FileSystem::fileSystem()->join(mcsDir,"tmp");
    m_statusDir = Common::FileSystem::fileSystem()->join(mcsDir,"status");
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
