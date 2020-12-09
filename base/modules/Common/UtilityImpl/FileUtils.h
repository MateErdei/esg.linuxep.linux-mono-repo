/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace UtilityImpl
    {
        class FileUtils
        {
        public:
            static std::pair<std::string, std::string> extractValueFromFile(
                const std::string& filePath,
                const std::string& key);
        };
    } // namespace UtilityImpl
} // namespace Common
