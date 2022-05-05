/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemUtils.h"

namespace OSUtilitiesImpl
{
    std::string SystemUtils::getEnvironmentVariable(const std::string& key) const
    {
        auto val = std::getenv(key.c_str());
        if(val != nullptr)
        {
            return std::string(val);
        }
        return "";
    }

}

