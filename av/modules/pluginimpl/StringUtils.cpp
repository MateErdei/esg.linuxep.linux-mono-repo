// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "StringUtils.h"

#include "HealthStatus.h"
#include "Logger.h"

#include "common/PluginUtils.h"
#include "datatypes/Time.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"

#include <boost/locale.hpp>
#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iomanip>

using json = nlohmann::json;
namespace fs = sophos_filesystem;

// XML defined at https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42233512399/EMP+event-core-detection
std::string pluginimpl::generateThreatDetectedXml(const scan_messages::ThreatDetected& detection)
{
    if (detection.filePath.empty())
    {
        LOGWARN("Missing path from threat report while generating xml: empty string");
    }

    std::string utf8Path = common::toUtf8(detection.filePath);
    common::escapeControlCharacters(utf8Path, true);

    // EMP specifies a length limit for paths
    if (utf8Path.size() > centralLimitedStringMaxSize)
    {
        LOGWARN("Threat path longer than " << centralLimitedStringMaxSize << " characters, truncating.");
        utf8Path.resize(centralLimitedStringMaxSize);
    }

    // TODO LINUXDAR-5929
    // from_time_t is marked as noexcept but can break that promise on very large but valid time_t values
    // Either we need to perform validation on construction of ThreatDetected or we need to create a new function to
    // replace MessageTimeStamp that takes time_t directly and avoids this problem
    auto time = std::chrono::system_clock::from_time_t(detection.detectionTime);
    std::string timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(time);

    std::string result = populateThreatReportXml(detection, utf8Path, timestamp);

    try
    {
        //verify that the xml generated is parsable
        Common::XmlUtilities::parseXml(result);
    }
    catch (const std::exception& e)
    {
        LOGWARN("Failed to generate valid threat report xml, replacing threat path and trying again");
        auto logPath = common::getPluginInstallPath() / "log/sophos_threat_detector/sophos_threat_detector.log";
        std::stringstream replacementPath;
        replacementPath << "See endpoint logs for threat file path at: " << logPath.c_str();
        result = populateThreatReportXml(detection, replacementPath.str(), timestamp);
    }

    return result;
}
std::string pluginimpl::populateThreatReportXml(
    const scan_messages::ThreatDetected& detection,
    const std::string& utf8Path,
    const std::string& timestamp)
{
    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="@@TS@@">
  <user userId="@@USER_ID@@"/>
  <alert id="@@ID@@" name="@@NAME@@" threatType="@@THREAT_TYPE@@" origin="@@ORIGIN@@" remote="@@REMOTE@@">
    <sha256>@@SHA256@@</sha256>
    <path>@@PATH@@</path>
  </alert>
</event>)sophos",
        { { "@@TS@@", timestamp },
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
    bool isEnabled,
    const std::vector<std::string>& exclusionList,
    bool excludeRemoteFiles)
{
    json config;
    json exclusions(exclusionList);

    config["exclusions"] = exclusions;
    config["enabled"] = isEnabled;
    config["excludeRemoteFiles"] = excludeRemoteFiles;

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
    bool overallSuccess = detection.notificationStatus == scan_messages::E_NOTIFCATION_STATUS::E_NOTIFICATION_STATUS_CLEANED_UP;
    threatEvent["quarantineSuccess"] = overallSuccess;
    if (detection.scanType == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS
        || detection.scanType == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_OPEN
        || detection.scanType == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_CLOSE)
    {
        threatEvent["avScanType"] = static_cast<int>(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS);
    }
    else
    {
        threatEvent["avScanType"] = static_cast<int>(detection.scanType);
    }
    if (threatEvent["avScanType"] == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS)
    {
        threatEvent["pid"] = detection.pid;
        threatEvent["processParentPath"] = detection.executablePath;
    }
    return threatEvent.dump();
}

// XML defined at https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42255827359/EMP+event-core-clean
std::string pluginimpl::generateCoreCleanEventXml(
    const scan_messages::ThreatDetected& detection,
    const common::CentralEnums::QuarantineResult& quarantineResult)
{
    if (detection.filePath.empty())
    {
        LOGWARN("Missing file path from threat report while generating Clean Event XML");
    }

    std::string utf8Path = common::toUtf8(detection.filePath);
    common::escapeControlCharacters(utf8Path, true);
    if (utf8Path.size() > centralLimitedStringMaxSize)
    {
        LOGWARN("Threat path longer than " << centralLimitedStringMaxSize << " characters, truncating.");
        utf8Path.resize(centralLimitedStringMaxSize);
    }

    auto timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(
        std::chrono::system_clock::from_time_t(detection.detectionTime));
    bool overallSuccess = quarantineResult == common::CentralEnums::QuarantineResult::SUCCESS;

    std::string result = createCleanEventXml(detection, quarantineResult, utf8Path, timestamp, overallSuccess);

    try
    {
        //verify that the xml generated is parsable
        Common::XmlUtilities::parseXml(result);
    }
    catch (const std::exception& e)
    {
        LOGWARN("Failed to generate valid clean report xml, replacing threat path and trying again");
        auto logPath = common::getPluginInstallPath() / "log/sophos_threat_detector/sophos_threat_detector.log";
        std::stringstream replacementPath;
        replacementPath << "See endpoint logs for threat file path at: " << logPath.c_str();
        result = createCleanEventXml(detection, quarantineResult, replacementPath.str(), timestamp, overallSuccess);
    }

    return result;
}
std::string pluginimpl::createCleanEventXml(
    const scan_messages::ThreatDetected& detection,
    const common::CentralEnums::QuarantineResult& quarantineResult,
    const std::string& utf8Path,
    const std::string& timestamp,
    bool overallSuccess)
{
    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="@@TS@@">
  <alert id="@@CORRELATION_ID@@" succeeded="@@SUCCESS_OVERALL@@" origin="@@ORIGIN@@">
    <items totalItems="@@TOTAL_ITEMS@@">
      <item type="file" result="@@SUCCESS_DETAILED@@">
        <descriptor>@@PATH@@</descriptor>
      </item>
    </items>
  </alert>
</event>)sophos",
        { { "@@TS@@", timestamp },
          { "@@CORRELATION_ID@@", detection.threatId },
          { "@@SUCCESS_OVERALL@@", std::to_string(overallSuccess) },
          { "@@ORIGIN@@", std::to_string(static_cast<int>(getOriginOf(detection.reportSource, detection.threatType))) },
          { "@@TOTAL_ITEMS@@",
            std::to_string(1) }, // hard coded until we deal with multiple threats per ThreatDetected object
          { "@@SUCCESS_DETAILED@@", std::to_string(static_cast<int>(quarantineResult)) },
          { "@@PATH@@", utf8Path } });
    return result;
}

// XML defined at https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42255827362/EMP+event-core-restore
std::string pluginimpl::generateCoreRestoreEventXml(const scan_messages::RestoreReport& restoreReport)
{
    auto timestamp =
        Common::UtilityImpl::TimeUtils::MessageTimeStamp(std::chrono::system_clock::from_time_t(restoreReport.time));

    const std::string escapedPath = common::escapePathForLogging(restoreReport.path, true, true, true);

    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.restore" ts="@@TS@@">
  <alert id="@@ID@@" succeeded="@@SUCCEEDED@@">
    <items totalItems="1">
      <item type="file">
        <descriptor>@@PATH@@</descriptor>
      </item>
    </items>
  </alert>
</event>)sophos",
        { { "@@TS@@", timestamp },
          { "@@ID@@", restoreReport.correlationId },
          { "@@SUCCEEDED@@", restoreReport.wasSuccessful ? "1" : "0" },
          { "@@PATH@@", escapedPath } });

    return result;
}