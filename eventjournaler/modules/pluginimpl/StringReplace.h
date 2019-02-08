/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace Plugin
{
    using KeyValueCollection = std::vector<std::pair<std::string, std::string>>;

    std::string orderedStringReplace(const std::string& pattern, const KeyValueCollection& keyvalues);
} // namespace Plugin