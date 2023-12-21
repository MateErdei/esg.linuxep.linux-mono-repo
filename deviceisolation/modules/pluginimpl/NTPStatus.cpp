// Copyright 2023 Sophos Limited. All rights reserved.
#include "NTPStatus.h"

#include <utility>

#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include "pluginimpl/config.h"

using namespace Plugin;

std::string Plugin::NTPStatus::xml() const
{
    auto now = NTPStatus::clock_t::now();
    return generateXml(revId_, adminIsolationEnabled_, now);
}

std::string Plugin::NTPStatus::xmlWithoutTimestamp() const
{
    return generateXml(revId_, adminIsolationEnabled_, "");
}

Plugin::NTPStatus::NTPStatus(std::string revId, bool adminIsolationEnabled)
    : revId_(std::move(revId))
    , adminIsolationEnabled_(adminIsolationEnabled)
{
}

std::string Plugin::NTPStatus::generateXml(const std::string& revId, bool adminIsolationEnabled,
                                           const Plugin::NTPStatus::timepoint_t& timepoint)
{
    auto timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(timepoint, Common::UtilityImpl::Granularity::seconds);
    return generateXml(revId, adminIsolationEnabled, timestamp);
}

std::string Plugin::NTPStatus::generateXml(const std::string& revId,
                                           bool adminIsolationEnabled,
                                           const std::string& timestamp)
{
    const static std::string TEMPLATE=R"SOPHOS(<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<status xmlns:csc='com.sophos\msys\csc' xmlns='com.sophos\mansys\status' type='NetworkThreatProtection'>
    <csc:CompRes policyType='24' Res='{{CompResult}}' RevID='{{revId}}'/>
    <meta protocolVersion='1.0' errorCode='0' timestamp='{{timestamp}}' softwareVersion='{{version}}'/>
    <isolation self="{{self}}" admin="{{admin}}" />
</status>
    )SOPHOS";
    auto CompResult = "Same";
    if (revId.empty())
    {
        CompResult = "NoRef";
    }
    return Common::UtilityImpl::StringUtils::orderedStringReplace(TEMPLATE,
                                                                  {
                                                                          { "{{CompResult}}", CompResult },
                                                                          { "{{revId}}", revId },
                                                                          { "{{timestamp}}", timestamp },
                                                                          { "{{version}}", PLUGIN_VERSION },
                                                                          { "{{self}}", "false" },
                                                                          { "{{admin}}", adminIsolationEnabled?"true":"false" },
                                                                  });
}
