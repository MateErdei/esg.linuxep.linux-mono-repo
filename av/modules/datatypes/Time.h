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

        static std::string epochToDateTimeString(const std::time_t& rawtime, const char* format= "%Y%m%d %H%M%S");

        static std::string currentToDateTimeString(const char* format="%Y%m%d %H%M%S");
    };
}