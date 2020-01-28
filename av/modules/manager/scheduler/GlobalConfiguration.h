/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace manager::scheduler
{
    class GlobalConfiguration
    {
    public:
        std::vector<std::string> exclusions()
        {
            return m_exclusions;
        }
    private:
        std::vector<std::string> m_exclusions;
    };
}
