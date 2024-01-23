// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventReceiverImpl.h"

#include "EventTask.h"
#include "OutbreakModeController.h"

#include "ManagementAgent/LoggerImpl/Logger.h"

using ManagementAgent::EventReceiverImpl::EventReceiverImpl;

ManagementAgent::EventReceiverImpl::EventReceiverImpl::EventReceiverImpl(
    Common::TaskQueue::ITaskQueueSharedPtr taskQueue) :
    m_taskQueue(std::move(taskQueue)), outbreakModeController_(std::make_shared<OutbreakModeController>())
{
}

void EventReceiverImpl::receivedSendEvent(const std::string& appId, const std::string& eventXml)
{
    // Determine if event should be filtered by Outbreak Mode
    if (outbreakModeController_->processEvent({appId, eventXml}))
    {
        // Drop the event
        LOGDEBUG("Dropping event as we are in outbreak mode: " << appId << ": " << eventXml);
        return;
    }

    Common::TaskQueue::ITaskPtr task(new EventTask({ appId, eventXml }));
    m_taskQueue->queueTask(std::move(task));
}

void ManagementAgent::EventReceiverImpl::EventReceiverImpl::handleAction(const std::string& actionXml)
{
    outbreakModeController_->processAction(actionXml);
}

bool ManagementAgent::EventReceiverImpl::EventReceiverImpl::outbreakMode() const
{
    return outbreakModeController_->outbreakMode();
}
