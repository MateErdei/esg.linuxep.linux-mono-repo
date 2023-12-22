/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <chrono>
#include <ctime>
#include <memory>
#include <string>

namespace Common::UtilityImpl
{
    /**
     * To enable tests that need to provide some fixed timestamp.
     */
    class IFormattedTime
    {
    public:
        virtual ~IFormattedTime() = default;
        virtual std::string currentTime() const = 0;
        virtual std::string currentEpochTimeInSeconds() const = 0;
        virtual u_int64_t currentEpochTimeInSecondsAsInteger() const = 0;
        virtual std::string bootTime() const = 0;
    };

    class ITime
    {
    public:
        virtual ~ITime() = default;
        virtual std::time_t getCurrentTime() = 0;
    };

    /**
     * This class is to be used in test only. It allows testers to redirect: TimeUtils::getCurrTime to
     * the provided mockTimer->getCurrentTime.
     * The object created will setup the time replacement and it will restore it on its destruction.
     * As such, this class is not meant to be copied or moved.
     */
    class ScopedReplaceITime
    {
    public:
        ScopedReplaceITime(std::unique_ptr<ITime> mockTimer);
        ~ScopedReplaceITime();
        ScopedReplaceITime(const ScopedReplaceITime&) = delete;
        ScopedReplaceITime& operator=(const ScopedReplaceITime) = delete;
        // not necessary to state the move operators as the constructor will not create them anyway.
    };
    enum class Granularity
    {
        milliseconds,
        seconds
    };

    class TimeUtils
    {
    public:
        using clock_t = std::chrono::system_clock;
        using timepoint_t = clock_t::time_point;
        /**
         *
         * A time stamp for use in messages (e.g. entries in a log file).
         * The format is ISO 8601 compliant and includes millisecond resolution.
         * No exceptions are thrown since a likely use is for logging where handling an exception would be awkward.
         * The time stamp is composed of ASCII characters so may be used in UTF-8 files.
         */
        static std::string MessageTimeStamp(
            const timepoint_t& time_point,
            Common::UtilityImpl::Granularity granularity = Granularity::milliseconds) noexcept;

        /*
         * Get current unix timestamp, in seconds e.g. 1687350741
         */
        static std::time_t getCurrTime();
        static std::string getBootTime();
        static std::time_t getBootTimeAsTimet();
        static long int getMinutesSince(std::string timestamp);

        /**
         * Return timestamp formatted as required by Timestamp Event
         * https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
         *
         * YYYYMMDD HHMMSS
         * @return timestamp formatted as required above.
         */
        static std::string fromTime(std::time_t, const char* format);
        static std::string fromTime(std::tm, const char* format);
        static std::string fromTime(std::time_t);
        static std::string fromTime(std::tm);

        static std::time_t toTime(const std::string&);
        static std::time_t toTime(const std::string&, const char* format);
        static std::time_t toUTCTime(const std::string&, const char* format);
        static std::string toEpochTime(const std::string& dateTime);

        // Windows FILETIME contains a 64-bit value representing the number of 100-nanosecond intervals since January 1,
        // 1601 (UTC) https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
        static std::time_t WindowsFileTimeToEpoch(int64_t ft);
        static int64_t EpochToWindowsFileTime(std::time_t t);
    };

    class FormattedTime : public virtual IFormattedTime
    {
    public:
        std::string currentTime() const override;
        std::string currentEpochTimeInSeconds() const override;
        u_int64_t currentEpochTimeInSecondsAsInteger() const override;
        std::string bootTime() const override;
    };
} // namespace Common::UtilityImpl
