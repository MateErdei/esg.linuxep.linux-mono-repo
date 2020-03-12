/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Time.h"

#include <stdexcept>

using namespace datatypes;

std::string Time::epochToCentralTime(const std::time_t& rawtime)
{
    struct tm timeinfo{};
    struct tm* result = gmtime_r(&rawtime, &timeinfo);
    if (result == nullptr)
    {
        throw std::runtime_error("Failed to convert timestamp to UTC");
    }

    char timebuffer[16];
    int timesize = ::strftime(timebuffer, sizeof(timebuffer), "%Y%m%d %H%M%S", &timeinfo);
    if (timesize == 0)
    {
        throw std::runtime_error("Failed to format timestamp");
    }

    return timebuffer;
}

std::string Time::currentToCentralTime()
{
    std::time_t currentTime = std::time(nullptr);
    return epochToCentralTime(currentTime);
}