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

#include <openssl/sha.h>
#include <iomanip>

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
            case '\\': buffer.append("\\\\");          break;
            default:
                auto character = static_cast<unsigned>(text[pos]);
                if (character <= 31 || character == 127)
                {
                    std::ostringstream escaped;
                    escaped << '\\'
                            << std::oct
                            << std::setfill('0')
                            << std::setw(3)
                            << character
                            ;
                    buffer.append(escaped.str());
                }
                else
                {
                    buffer.push_back(text[pos]);
                }
                break;
        }
    }

    text.swap(buffer);
}

std::string sha256_hash(const std::string& str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(const auto& ch : hash)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)ch;
    }
    return ss.str();
}

std::string unixsocket::toUtf8(const std::string& str)
{
    try
    {
        return boost::locale::conv::to_utf<char>(str, "UTF-8", boost::locale::conv::stop);
    }
    catch(const boost::locale::conv::conversion_error& e)
    {
        LOGDEBUG("Could not convert from: " << "UTF-8");
    }

    std::vector<std::string> encodings{"EUC-JP", "SJIS", "Latin1"};
    for (const auto& encoding : encodings)
    {
        try
        {
            std::string encoding_info = " (" + encoding + ")";
            return boost::locale::conv::to_utf<char>(str, encoding, boost::locale::conv::stop).append(encoding_info);
        }
        catch(const boost::locale::conv::conversion_error& e)
        {
            LOGDEBUG("Could not convert from: " << encoding);
            continue;
        }
    }

    throw std::runtime_error(std::string("Failed to convert string to utf8: ") + str);
}

//TO DO: maybe move it in a XMLUtils file?
std::string unixsocket::generateThreatDetectedXml(const scan_messages::ServerThreatDetected& detection)
{
    std::string path = detection.getFilePath();
    if (path.size() == 0)
    {
        LOGERROR("Received threat report with empty path!");
    }

    std::string utf8Path = toUtf8(path);

    escapeControlCharacters(utf8Path);
    std::string fileName = fs::path(utf8Path).filename();
    std::string threatName =  detection.getThreatName();

    std::string threatIDinput = utf8Path + threatName;
    std::string threatID = "T" + sha256_hash(threatIDinput);

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