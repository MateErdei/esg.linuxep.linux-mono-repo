// Copyright 2023 Sophos Limited. All rights reserved.

#include "OutbreakModeController.h"

#include "ManagementAgent/LoggerImpl/Logger.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <ctime>
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
    return recordEventAndDetermineIfItShouldBeDropped(appId, eventXml, clock_t::now());
}

bool ManagementAgent::EventReceiverImpl::OutbreakModeController::recordEventAndDetermineIfItShouldBeDropped(
    const std::string& appId,
    const std::string& eventXml,
    time_point_t now)
{
    resetCountOnDayChange(now);

    if (!isCountableEvent(appId, eventXml))
    {
        return false;
    }

    if (!outbreakMode_)
    {
        detectionCount_++;
        if (detectionCount_ >= 100)
        {
            LOGWARN("Entering outbreak mode");
            outbreakMode_ = true;
        }
    }
    return false;
}

void ManagementAgent::EventReceiverImpl::OutbreakModeController::resetCountOnDayChange(
    ManagementAgent::EventReceiverImpl::OutbreakModeController::time_point_t now)
{
    auto nowTime = clock_t::to_time_t(now);
    struct tm nowTm{};
    localtime_r(&nowTime, &nowTm);
    // We need to compare year, month and day, to avoid aliasing
    // e.g. 99 alters 2023-01-17, and 2 more 2023-02-17, would go into outbreak mode
    // if we only recorded day of month, and didn't get any events in between
    // same issue with year: 2023-01-17 and 2024-01-17, and no events in between
    if (savedDay_ != nowTm.tm_mday || savedMonth_ != nowTm.tm_mon || savedYear_ != nowTm.tm_year)
    {
        // Reset count if the Day Of Month changes
        detectionCount_ = 0;
        savedDay_ = nowTm.tm_mday;
        savedMonth_ = nowTm.tm_mon;
        savedYear_ = nowTm.tm_year;
    }
}
