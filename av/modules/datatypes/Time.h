/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ctime>
#include <string>

namespace datatypes
{
    class Time
    {
    public:
        Time() = delete;

        static std::string epochToCentralTime(const std::time_t& rawtime);

        static std::string currentToCentralTime();
    };
}