/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace datatypes
{
    class Time
    {
    public:
        Time() = delete;

        static std::string epochToCentralTime(const std::time_t& rawtime)
        {
            struct tm timeinfo{};
            struct tm* result = localtime_r(&rawtime, &timeinfo);
            if (result == nullptr)
            {
                throw std::runtime_error("Failed to get localtime");
            }

            char timebuffer[128];
            int timesize = ::strftime(timebuffer, sizeof(timebuffer), "%Y%m%d %H%M%S", &timeinfo);
            if (timesize == 0)
            {
                throw std::runtime_error("Failed to format timestamp");
            }

            return timebuffer;
        }

        static std::string currentToCentralTime()
        {
            std::time_t currentTime = std::time(nullptr);
            return epochToCentralTime(currentTime);
        }
    };
}