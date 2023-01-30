// Copyright 2023 Sophos Limited. All rights reserved.

#include "OutbreakModeController.h"

#include "ManagementAgent/LoggerImpl/Logger.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <tuple>

namespace
{
    bool isCountableEvent(
        const std::string& appId,
        const std::string& eventXml)
    {
        std::ignore = appId;

        return Common::UtilityImpl::StringUtils::startswith(eventXml, R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection")");
    }
}

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::recordEventAndDetermineIfItShouldBeDropped(
    const std::string& appId,
    const std::string& eventXml)
{
    if (!isCountableEvent(appId, eventXml))
    {
        return false;
    }

    detectionCount_++;
    if (detectionCount_ == 100)
    {
        LOGWARN("Entering outbreak mode");
    }
    return false;
}
