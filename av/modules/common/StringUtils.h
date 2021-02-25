/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <log4cplus/logger.h>

namespace common
{
    void escapeControlCharacters(std::string& text, bool escapeXML=false);
    std::string sha256_hash(const std::string& str);
    std::string md5_hash(const std::string& str);
    std::string toUtf8(const std::string& str, bool appendConversion = true, bool throws = true);
    std::string fromLogLevelToString(const log4cplus::LogLevel& logLevel);
    std::string pluralize(int number, std::string singularString, std::string pluralString);
    std::string escapePathForLogging(const std::string& path, bool appendUtf8Conversion = false, bool Utf8ConversionThrows = false, bool escapeXmlCharacters = false);
    bool contains(const std::string& string, const std::string& value);
}