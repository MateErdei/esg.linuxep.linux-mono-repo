/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace UtilityImpl
    {
        class SystemExecutableUtils
        {
        public:
            static std::string getSystemExecutablePath(const std::string& executableName);
        };

    }
}