/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StringUtils.h"

#include "Logger.h"

#include "ThreatDetected.capnp.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Time.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <boost/locale.hpp>
#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iomanip>

using json = nlohmann::json;
namespace fs = sophos_filesystem;

std::string pluginimpl::generateThreatDetectedXml(const scan_messages::ServerThreatDetected& detection)
{
    std::string path = detection.getFilePath();
    if (path.size() == 0)
    {
        LOGERROR("Missing path from threat report while generating xml: empty string");
    }

    std::string utf8Path = common::toUtf8(path);

    common::escapeControlCharacters(utf8Path, true);
    std::string fileName = fs::path(utf8Path).filename();
    std::string threatName =  detection.getThreatName();

    std::string threatIDinput = utf8Path + threatName;
    std::string threatID = "T" + common::sha256_hash(threatIDinput);

    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
            R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found '@@THREAT_NAME@@' in '@@THREAT_PATH@@'" timestamp="@@DETECTION_TIME@@" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="@@USER@@"/>
  <threat id="@@THREAT_ID@@" idSource="@@ID_SOURCE@@" name="@@THREAT_NAME@@" scanType="@@SMT_SCAN_TYPE@@" status="@@NOTIFICATION_STATUS@@" type="@@THREAT_TYPE@@">
    <item file="@@FILE_NAME@@" path="@@THREAT_PATH@@"/>
    <action action="@@SMT_ACTION_CODES@@"/>
  </threat>
</notification>)sophos",{
                    {"@@THREAT_NAME@@", threatName},
                    {"@@THREAT_PATH@@", utf8Path},
                    {"@@DETECTION_TIME@@", datatypes::Time::epochToCentralTime(detection.getDetectionTime())},
                    {"@@USER@@", detection.getUserID()},
                    {"@@THREAT_ID@@", threatID},
                    {"@@ID_SOURCE@@", "Tsha256(path,name)"},
                    {"@@THREAT_NAME@@",threatName},
                    {"@@SMT_SCAN_TYPE@@",  std::to_string(detection.getScanType())},
                    {"@@NOTIFICATION_STATUS@@", std::to_string(detection.getNotificationStatus())},
                    {"@@THREAT_TYPE@@",  std::to_string(detection.getThreatType())},
                    {"@@FILE_NAME@@", fileName},
                    {"@@THREAT_PATH@@", fs::path(utf8Path).remove_filename()},
                    {"@@SMT_ACTION_CODES@@", std::to_string(detection.getActionCode())}
            });

    return result;
}

std::string pluginimpl::generateThreatDetectedJson(const scan_messages::ServerThreatDetected& detection)
{
    json threatEvent;
    threatEvent["detection_name"] = detection.getThreatName();
    threatEvent["detection_thumbprint"] = detection.getThreatName(); // turn into sha?
    threatEvent["threatPath"] = detection.getFilePath();
    threatEvent["threat_type"] = "Virus";
    threatEvent["threat_source"] = "SAV";
    threatEvent["time"] = detection.getDetectionTime();
    return threatEvent.dump();
}