// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventTask.h"

#include "EventUtils.h"

ManagementAgent::EventReceiverImpl::EventTask::EventTask(
    ManagementAgent::EventReceiverImpl::Event event,
    ManagementAgent::EventReceiverImpl::IOutbreakModeControllerPtr outbreakModeController) :
    event_(std::move(event)),
    outbreakModeController_(std::move(outbreakModeController))
{
}


void ManagementAgent::EventReceiverImpl::EventTask::run()
{
    // Determine if event should be filtered by Outbreak Mode
    if (
        outbreakModeController_->processEvent(event_)
    )
    {
        // Drop the event
        return;
    }

    event_.send();
}