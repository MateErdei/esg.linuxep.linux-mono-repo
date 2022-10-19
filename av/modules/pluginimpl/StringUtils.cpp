// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "StringUtils.h"

#include "HealthStatus.h"
#include "Logger.h"
#include "ThreatDetected.capnp.h"

#include "datatypes/Time.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <boost/locale.hpp>
#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iomanip>

using json = nlohmann::json;
namespace fs = sophos_filesystem;

std::string pluginimpl::generateThreatDetectedXml(const scan_messages::ThreatDetected& detection)
{
    if (detection.filePath.empty())
    {
        LOGERROR("Missing path from threat report while generating xml: empty string");
    }

    std::string utf8Path = common::toUtf8(detection.filePath);
    common::escapeControlCharacters(utf8Path, true);

    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="@@DETECTION_TIME@@">
  <user userId="@@USER_ID@@"/>
  <alert id="@@ID@@" name="@@NAME@@" threatType="@@THREAT_TYPE@@" origin="@@ORIGIN@@" remote="@@REMOTE@@">
    <sha256>@@SHA256@@</sha256>
    <path>@@PATH@@</path>
  </alert>
</event>)sophos",
        { { "@@DETECTION_TIME@@",
            Common::UtilityImpl::TimeUtils::MessageTimeStamp(
                { std::chrono::system_clock::time_point(std::chrono::seconds(detection.detectionTime)) }) },
          { "@@USER_ID@@", detection.userID },
          { "@@ID@@", detection.threatId },
          { "@@NAME@@", detection.threatName },
          { "@@THREAT_TYPE@@", std::to_string(static_cast<int>(detection.threatType)) },
          { "@@ORIGIN@@", std::to_string(static_cast<int>(getOriginOf(detection.reportSource, detection.threatType))) },
          { "@@REMOTE@@", detection.isRemote ? "true" : "false" },
          { "@@SHA256@@", detection.sha256 },
          { "@@PATH@@", utf8Path } });

    return result;
}

std::string pluginimpl::generateOnAccessConfig(
    const std::string& isEnabled,
    const std::vector<std::string>& exclusionList,
    const std::string& excludeRemoteFiles)
{
    json config;
    json exclusions(exclusionList);

    config["exclusions"] = exclusions;

    if (isEnabled != "true")
    {
        config["enabled"] = "false";
    }
    else
    {
        config["enabled"] = "true";
    }

    if (excludeRemoteFiles != "true")
    {
        config["excludeRemoteFiles"] = "false";
    }
    else
    {
        config["excludeRemoteFiles"] = "true";
    }

    return config.dump();
}

std::string pluginimpl::generateThreatDetectedJson(const scan_messages::ThreatDetected& detection)
{
    json threatEvent;
    json details;
    json items;
    std::string path = common::toUtf8(detection.filePath);

    details["filePath"] = path;
    details["sha256FileHash"] = detection.sha256;

    items["1"]["path"] = path;
    items["1"]["primary"] = true;
    items["1"]["sha256"] = detection.sha256;
    items["1"]["type"] = 1; // FILE

    threatEvent["detectionName"]["short"] = detection.threatName;
    threatEvent["threatSource"] = 1; // SAV
    threatEvent["threatType"] = detection.threatType;
    threatEvent["time"] = detection.detectionTime;

    threatEvent["details"] = details;
    threatEvent["items"] = items;

    return threatEvent.dump();
}

long pluginimpl::getThreatStatus()
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string storedtelemetry = telemetry.serialise();
    json j = json::parse(storedtelemetry);

    if (j.find("threatHealth") != j.end())
    {
        try
        {
            long health = static_cast<long>(j["threatHealth"]);
            return health;
        }
        catch (std::exception& exception)
        {
            LOGWARN("Could not initialise threatHealth value from stored telemetry due to error: " << exception.what());
            return (Plugin::E_THREAT_HEALTH_STATUS_GOOD);
        }
    }
    else
    {
        LOGDEBUG("No stored threatHealth");
        return (Plugin::E_THREAT_HEALTH_STATUS_GOOD);
    }
}