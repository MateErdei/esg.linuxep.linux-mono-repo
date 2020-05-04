/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace common
{
    void escapeControlCharacters(std::string& text);
    std::string sha256_hash(const std::string& str);
    std::string toUtf8(const std::string& str, bool appendConversion = true);
}