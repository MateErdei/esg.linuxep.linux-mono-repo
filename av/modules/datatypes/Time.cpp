// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Time.h"

#include "AVException.h"

using namespace datatypes;

std::string Time::epochToDateTimeString(const std::time_t& rawtime, const char* format)
{
    struct tm timeinfo{};
    struct tm* result = gmtime_r(&rawtime, &timeinfo);
    if (result == nullptr)
    {
        throw datatypes::AVException(LOCATION, "Failed to convert timestamp to UTC");
    }

    char timebuffer[20];
    int timesize = ::strftime(timebuffer, sizeof(timebuffer), format, &timeinfo);
    if (timesize == 0)
    {
        throw datatypes::AVException(LOCATION, "Failed to format timestamp");
    }

    return timebuffer;
}

std::string Time::currentToDateTimeString(const char* format)
{
    std::time_t currentTime = std::time(nullptr);
    return epochToDateTimeString(currentTime, format);
}