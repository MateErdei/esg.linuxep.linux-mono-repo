///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "EventReceiverImpl.h"
#include "EventTask.h"

using ManagementAgent::EventReceiverImpl::EventReceiverImpl;

ManagementAgent::EventReceiverImpl::EventReceiverImpl::EventReceiverImpl(std::string mcsDir,
                                                                         Common::TaskQueue::ITaskQueueSharedPtr taskQueue)
    : m_mcsDir(std::move(mcsDir)),
      m_taskQueue(std::move(taskQueue))
{

}

void EventReceiverImpl::receivedSendEvent(const std::string& appId, const std::string& eventXml)
{
    // Write file to directory
    Common::TaskQueue::ITaskPtr task
            (
                    new EventTask(
                            m_mcsDir,
                            appId,
                            eventXml
                            )
                    );

    m_taskQueue->queueTask(task);
}
