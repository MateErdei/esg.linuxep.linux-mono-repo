// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventReceiverImpl.h"

#include "EventTask.h"

using ManagementAgent::EventReceiverImpl::EventReceiverImpl;

ManagementAgent::EventReceiverImpl::EventReceiverImpl::EventReceiverImpl(
    Common::TaskQueue::ITaskQueueSharedPtr taskQueue) :
    m_taskQueue(std::move(taskQueue)),
    outbreakModeController_(std::make_shared<OutbreakModeController>())
{
}

void EventReceiverImpl::receivedSendEvent(const std::string& appId, const std::string& eventXml)
{
    Common::TaskQueue::ITaskPtr task(new EventTask({appId, eventXml}, outbreakModeController_));
    m_taskQueue->queueTask(std::move(task));
}

void ManagementAgent::EventReceiverImpl::EventReceiverImpl::handleAction(const std::string& actionXml)
{
    outbreakModeController_->processAction(actionXml);
}
