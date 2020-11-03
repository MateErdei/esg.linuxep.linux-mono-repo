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
            static std::string extractValueFromFile(const std::string& filePath, const std::string& key);

        };
    }
}


