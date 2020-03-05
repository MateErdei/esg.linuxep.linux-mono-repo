/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/UtilityImpl/StringUtils.h>
#include "ThreatDetected.capnp.h"
#include "StringUtils.h"
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

void unixsocket::escapeControlCharacters(std::string& text)
{
    std::string buffer;
    buffer.reserve(text.size());
    for(size_t pos = 0; pos != text.size(); ++pos) {
        switch(text[pos]) {
            case '\1': buffer.append("\\1");           break;
            case '\2': buffer.append("\\2");           break;
            case '\3': buffer.append("\\3");           break;
            case '\4': buffer.append("\\4");           break;
            case '\5': buffer.append("\\5");           break;
            case '\6': buffer.append("\\6");           break;
            case '\a': buffer.append("\\a");           break;
            case '\b': buffer.append("\\b");           break;
            case '\t': buffer.append("\\t");           break;
            case '\n': buffer.append("\\n");           break;
            case '\v': buffer.append("\\v");           break;
            case '\f': buffer.append("\\f");           break;
            case '\r': buffer.append("\\r");           break;
            case '\\': buffer.append("\\");            break;
            default: buffer.push_back(text[pos]);         break;
        }
    }

    text.swap(buffer);
}

//TO DO: maybe move it in a XMLUtils file?
std::string unixsocket::generateThreatDetectedXml(const Sophos::ssplav::ThreatDetected::Reader& detection)
{
    std::string path = detection.getFilePath();
    std::string threatName =  detection.getThreatName();
    //TO DO: convert to unicode first before escaping characters
    escapeControlCharacters(path);
    std::locale loc("");

    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
            R"sophos(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
                     <notification xmlns="http://www.sophos.com/EE/Event"
                               description="Virus/spyware @@THREAT_NAME@@ has been detected in @@THREAT_PATH@@"
                               type="sophos.mgt.msg.event.threat"
                               timestamp="@@DETECTION_TIME@@">

                     <user userId="@@USER@@"
                               domain="local"/>
                     <threat  type="@@THREAT_TYPE@@"
                               name="@@THREAT_NAME@@"
                               scanType="@@SMT_SCAN_TYPE@@"
                               status="@@NOTIFICATION_STATUS@@"
                               id="@@THREAT_ID@@"
                               idSource="@@ID_SOURCE@@">

                               <item file="@@FILE_NAME@@"
                                      path="@@THREAT_PATH@@"/>
                               <action action="@@SMT_ACTION_CODES@@"/>
                     </threat>
                     </notification>
            )sophos",{
                    {"@@THREAT_NAME@@", threatName},
                    {"@@THREAT_PATH@@", path},
                    {"@@DETECTION_TIME@@", std::to_string(detection.getDetectionTime())},
                    {"@@USER@@", detection.getUserID()},
                    {"@@THREAT_TYPE@@",  std::to_string(detection.getThreatType())},
                    {"@@THREAT_NAME@@",threatName},
                    {"@@SMT_SCAN_TYPE@@",  std::to_string(detection.getScanType())},
                    {"@@NOTIFICATION_STATUS@@", std::to_string(detection.getNotificationStatus())},
                    // TO DO: at the moment we don't store THREAT_ID  and ID_SOURCE in the capnp object
                    // cause there is no way to retrieve this information
                    {"@@THREAT_ID@@", "1"},
                    {"@@ID_SOURCE@@", "1"},
                    {"@@FILE_NAME@@", fs::path(path).filename()},
                    {"@@THREAT_PATH@@", path},
                    {"@@SMT_ACTION_CODES@@", std::to_string(detection.getActionCode())}
            });

    return result;
}