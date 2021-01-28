/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "TimeStamp.h"

#include <iomanip>
#include <sstream>
#include <time.h>

namespace Utilities
{
    std::string MessageTimeStamp(const std::chrono::system_clock::time_point& time_point, Granularity granularity) noexcept
    {
        auto tt{ std::chrono::system_clock::to_time_t(time_point) };
        std::tm gmtime{};
        //gmtime_r(&gmtime, &tt);
        gmtime_r(&tt, &gmtime);
        std::ostringstream oss{};
        oss << std::put_time(&gmtime, "%FT%T");

        if (Granularity::milliseconds == granularity)
        {
            const auto tse = time_point.time_since_epoch();
            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;
            oss << "." << std::setfill('0') << std::setw(3) << milliseconds;
        }
        oss << "Z";
        return oss.str();
    }

    std::wstring FilenameTimeStamp(const std::chrono::system_clock::time_point& time_point, Granularity granularity)
    {
        auto tt{ std::chrono::system_clock::to_time_t(time_point) };
        std::tm gmtime{};
        gmtime_r(&tt, &gmtime);

        std::wostringstream oss{};
        oss.exceptions(std::ios::failbit | std::ios::badbit);
        oss << std::put_time(&gmtime, L"%Y%m%d_%H%M%S");

        if (Granularity::milliseconds == granularity)
        {
            const auto tse = time_point.time_since_epoch();
            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;
            oss << L"_" << std::setfill(L'0') << std::setw(3) << milliseconds;
        }
        return oss.str();
    }

    std::chrono::system_clock::time_point MakeTimePoint(int year, int month, int day, int hour, int minute, int second, int millisecond)
    {
        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;

        auto tt{ ::mktime(&tm) };

        return std::chrono::system_clock::from_time_t(tt) + std::chrono::milliseconds(millisecond);
    }
} // namespace Utilities