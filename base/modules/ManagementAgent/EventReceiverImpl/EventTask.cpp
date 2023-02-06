// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventTask.h"

#include "EventUtils.h"

ManagementAgent::EventReceiverImpl::EventTask::EventTask(std::string appId, std::string eventXml,
                                                         IOutbreakModeControllerPtr outbreakModeController) :
    m_appId(std::move(appId)),
    m_eventXml(std::move(eventXml)),
    outbreakModeController_(std::move(outbreakModeController))
{
}

void ManagementAgent::EventReceiverImpl::EventTask::run()
{
    // Determine if event should be filtered by Outbreak Mode
    if (
        outbreakModeController_->processEvent(m_appId, m_eventXml)
    )
    {
        // Drop the event
        return;
    }

    ManagementAgent::EventReceiverImpl::sendEvent(m_appId, m_eventXml);
}
