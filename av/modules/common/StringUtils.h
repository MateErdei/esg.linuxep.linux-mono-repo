/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <log4cplus/logger.h>

namespace common
{
    void escapeControlCharacters(std::string& text);
    std::string sha256_hash(const std::string& str);
    std::string md5_hash(const std::string& str);
    std::string toUtf8(const std::string& str, bool appendConversion = true, bool throws = true);
    std::string fromLogLevelToString(const log4cplus::LogLevel& logLevel);
    std::string pluralize(int number, std::string singularString, std::string pluralString);
}