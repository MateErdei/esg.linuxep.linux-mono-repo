/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StringUtils.h"

#include "Logger.h"

#include "ThreatDetected.capnp.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Time.h"

#include <boost/locale.hpp>

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
std::string unixsocket::generateThreatDetectedXml(const scan_messages::ServerThreatDetected& detection)
{
    std::string path = detection.getFilePath();
    if (path.size() == 0)
    {
        LOGERROR("Received threat report with empty path!");
    }

    std::locale localLocale("");
    std::locale conversionInformation = boost::locale::util::create_info(localLocale, localLocale.name());
    std::string utf8Path = boost::locale::conv::to_utf<char>(path, conversionInformation);

    escapeControlCharacters(utf8Path);
    std::string fileName = fs::path(utf8Path).filename();
    std::string threatName =  detection.getThreatName();

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
                    {"@@THREAT_PATH@@", path},
                    {"@@DETECTION_TIME@@", datatypes::Time::epochToCentralTime(detection.getDetectionTime())},
                    {"@@USER@@", detection.getUserID()},
                    {"@@THREAT_ID@@", "1"},
                    {"@@ID_SOURCE@@", "1"},
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