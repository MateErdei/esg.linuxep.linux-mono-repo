/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <regex>
#include <string>

namespace Common::UtilityImpl
{
    std::string returnFirstMatch(const std::string& stringpattern, const std::string& content);

    std::string returnFirstMatch(const std::regex& regexPattern, const std::string& content);
} // namespace Common::UtilityImpl
