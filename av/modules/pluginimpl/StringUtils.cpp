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
//    json threatEvent;
//    json threatEventDetails;

//    threatEvent["detection_name"]["asdasd"] = detection.getThreatName();
//    threatEvent["detection_thumbprint"] = detection.getThreatName(); // turn into sha?
//    threatEvent["threatPath"] = detection.getFilePath();
//    //std::string utf8Path = common::toUtf8(path);
//    threatEvent["threat_type"] = "Virus";
//    threatEvent["threat_source"] = "SAV";
//    threatEvent["time"] = detection.getDetectionTime();
//    std::string blah =  threatEvent.dump();

    json threatEvent;
    json details;
    json items;

    std::string path = common::toUtf8(detection.getFilePath());
    details["filePath"] = path;
    details["sha256FileHash"] = detection.getSha256();
//    threatEvent["detectionName"]["full"] = "";
    items["1"]["path"] = path;
    items["1"]["primary"] = true;
    items["1"]["sha256"] = detection.getSha256();
    items["1"]["type"] = 1; // FILE
    threatEvent["detectionName"]["short"] =  detection.getThreatName();
    threatEvent["threatSource"] = 1; // SAV
    threatEvent["time"] = detection.getDetectionTime();
    threatEvent["details"] = details;
    threatEvent["items"] = items;

    return threatEvent.dump();

    // rowid
    //   Ignore for this ticket - Should come from journal
// time
    //   Comes from SUSI
    // raw
    //   Ignore for this ticket - it's the entire JSON message
    // query_id
    //   ignore fo this ticket.
// detection_name
    //   Full if provided and not empty, or use short name, comes from AV already.
// detection_thumbprint
    //   SHA256 from SUSI output
// threat_source
    //   SAV - hardcoded in the AV plugin - Might need a to replace with look-up later on
// threat_type
    //   Virus - hardcoded in the AV plugin - Might need a to replace with look-up later on
// sid
    //   Empty string, or 0 should be ok. This does not apply to Linux scheduled scans
// monitor_mode
    //   Windows feature we do no have, set as disabled / default for now. Perhaps this should be 1 to indicate non-blocking though?
// primary_item
    //   Json blob for primary item. Create primary data blob as seen in trigger context by using SUSI fields.
// primary_item_type
    //   Hard coded to FILE
// primary_item_name
    //   Path of the File - need to work out what to do with non-utf8 paths. base64 encode if non-utf8? Have emailed Chloe.
// primary_item_spid
    //  Empty string - the file wasn't detected via on access so there's PID.





//    {
    //    "details": {
    //     n   "certificates": {},
    //     n   "fileMetadata": {},
    //     y   "filePath": "C:\\Users\\Administrator\\Desktop\\HighScore.exe",
    //     n  "fileSize": 660,
    //     n   "globalReputation": 62,
    //     n   "localReputation": -1,
    //     n   "localReputationConfigVersion": "e9f53340ca6598d87ba24a832311b9c61fb486d9bbf9b25c53b6c3d4fdd7578c",
    //     n   "localReputationSampleRate": 0,
    //     n   "mlScoreMain": 100,
    //     n   "mlScorePua": 100,
    //     n   "mlSuppressed": false,
    //     n   "remote": false,
    //     n   "reputationSource": "SXL4_LOOKUP",
    //     n   "savScanResult": "",
    //     n   "scanEvalReason": "PINNED_CACHE_BLOCK_NOTIFY",
    //     n  "sha1FileHash": "46763aa950d6f2a3615cc63226afb5180b5a229b",
    //     y   "sha256FileHash": "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9",
    //     n   "sting20Enabled": false
    //    },
    //    "detectionName": {
    //     y   "short": "ML/PE-A"
    //    },
    //    "flags": {
    //     n   "isSuppressible": false,
    //     n   "isUserVisible": true,
    //     n   "sendVdlTelemetry": true,
    //     n   "useClassicTelemetry": true,
    //     n   "useSavScanResults": true
    //    },
    //    "items": {
    //      y  "1": {
    //      n      "certificates": "{\"details\": {}, \"isSigned\": false, \"signerInfo\": []}",
    //      n      "cleanUp": true,
    //      n      "isPeFile": true,
    //      y      "path": "C:\\Users\\Administrator\\Desktop\\HighScore.exe",
    //      y      "primary": true,
    //      n      "remotePath": false,
    //      y      "sha256": "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9",
    //      y      "type": 1
    //        }
    //    },
    //    y "threatSource": 0,
    //    y "threatType": 1
    //}

}