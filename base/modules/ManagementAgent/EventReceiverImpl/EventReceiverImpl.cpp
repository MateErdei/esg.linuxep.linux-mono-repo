/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventReceiverImpl.h"
#include "EventTask.h"

using ManagementAgent::EventReceiverImpl::EventReceiverImpl;

ManagementAgent::EventReceiverImpl::EventReceiverImpl::EventReceiverImpl(
        Common::TaskQueue::ITaskQueueSharedPtr taskQueue)
        : m_taskQueue(std::move(taskQueue))
{

}

void EventReceiverImpl::receivedSendEvent(const std::string& appId, const std::string& eventXml)
{
    // Write file to directory
    Common::TaskQueue::ITaskPtr task
            (
                    new EventTask(
                            appId,
                            eventXml
                            )
                    );

    m_taskQueue->queueTask(task);
}
