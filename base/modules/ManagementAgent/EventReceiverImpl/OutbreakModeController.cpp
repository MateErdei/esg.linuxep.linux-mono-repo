// Copyright 2023 Sophos Limited. All rights reserved.

#include "OutbreakModeController.h"

#include <ManagementAgent/LoggerImpl/Logger.h>

#include <tuple>

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::recordEventAndDetermineIfItShouldBeDropped(
    const std::string& appId,
    const std::string& eventXml)
{
    std::ignore = appId;
    std::ignore = eventXml;
    detectionCount_++;
    if (detectionCount_ == 100)
    {
        LOGWARN("Entering outbreak mode");
    }
    return false;
}
