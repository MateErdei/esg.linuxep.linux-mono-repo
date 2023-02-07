// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventTask.h"

#include "EventUtils.h"

ManagementAgent::EventReceiverImpl::EventTask::EventTask(std::string appId, std::string eventXml,
                                                         IOutbreakModeControllerPtr outbreakModeController) :
    event_(std::move(appId), std::move(eventXml)),
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
