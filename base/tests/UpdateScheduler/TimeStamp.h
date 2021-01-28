/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include <chrono>
#include <string>

namespace Utilities
{
    enum class Granularity
    {
        milliseconds,
        seconds
    };
    // A time stamp for use in messages (e.g. entries in a log file).
    // The format is ISO 8601 compliant and includes millisecond resolution.
    // No exceptions are thrown since a likely use is for logging where handling an exception would be awkward.
    // The time stamp is composed of ASCII characters so may be used in UTF-8 files.
    std::string MessageTimeStamp(const std::chrono::system_clock::time_point& time_point, Granularity granularity = Granularity::milliseconds) noexcept;

    // A time stamp for file names (e.g. when rotating log files or dropping event files into a queue folder).
    // The format is not ISO 8601 compliant since it includes millisecond resolution without using a decimal point which would be awkward for file names.
    // Chronological order follows lexicographical order.
    // Characters that are not allowed in Windows filename (e.g. ':') are excluded from the output.
    // An exception may be thrown in the case of an error.
    // The time stamp is composed of ASCII characters encoded as a string of wchar_t so may be used where UCS-2 or UTF-16 is required.
    std::wstring FilenameTimeStamp(const std::chrono::system_clock::time_point& time_point, Granularity granularity = Granularity::milliseconds);

    std::chrono::system_clock::time_point MakeTimePoint(int year, int month, int day, int hour, int minute, int second, int millisecond);
} // namespace Utilities