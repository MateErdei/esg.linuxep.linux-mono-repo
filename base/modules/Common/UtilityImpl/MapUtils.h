/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <map>

namespace Common
{
    namespace UtilityImpl
    {
        std::map<std::string, std::string> getMapFromFile(const std::string & filepath);
    }
}
