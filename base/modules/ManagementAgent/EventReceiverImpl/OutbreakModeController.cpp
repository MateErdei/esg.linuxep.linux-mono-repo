// Copyright 2023 Sophos Limited. All rights reserved.

#include "OutbreakModeController.h"

#include <tuple>

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::recordEventAndDetermineIfItShouldBeDropped(
    const std::string& appId,
    const std::string& eventXml)
{
    std::ignore = appId;
    std::ignore = eventXml;
    return false;
}
