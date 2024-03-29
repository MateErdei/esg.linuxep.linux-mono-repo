/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TimeUtils.h"

#include <sys/sysinfo.h>

#include <cassert>
#include <chrono>
#include <iomanip>

namespace
{
    class TimeSource : public Common::UtilityImpl::ITime
    {
        std::time_t getCurrentTime() override { return std::time(nullptr); }
    };

    std::unique_ptr<Common::UtilityImpl::ITime>& staticTimeSource()
    {
        static std::unique_ptr<Common::UtilityImpl::ITime> timer{ new TimeSource{} };
        return timer;
    }

} // namespace

namespace Common::UtilityImpl
{
    std::string TimeUtils::MessageTimeStamp(
        const std::chrono::system_clock::time_point& time_point,
        Common::UtilityImpl::Granularity granularity) noexcept
    {
        auto tt{ std::chrono::system_clock::to_time_t(time_point) };
        std::tm gmtime{};
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

    std::string TimeUtils::fromTime(std::time_t time_, const char* format)
    {
        if (time_ == -1)
        {
            return "";
        }

        std::tm time_tm;
        (void)::localtime_r(&time_, &time_tm);
        return fromTime(time_tm, format);
    }

    std::time_t TimeUtils::getCurrTime()
    {
        return staticTimeSource()->getCurrentTime();
    }

    std::string TimeUtils::getBootTime()
    {
        return fromTime(getBootTimeAsTimet(), "%Y%m%d %H%M%S");
    }

    std::time_t TimeUtils::getBootTimeAsTimet()
    {
        struct sysinfo info;
        if (sysinfo(&info) != 0)
        {
            return 0; // error
        }
        auto curr = getCurrTime();
        return curr - info.uptime;
    }

    std::string TimeUtils::fromTime(std::tm time_tm, const char* format)
    {
        char formattedTime[16];
        size_t ret = strftime(formattedTime, 16, format, &time_tm);
        assert(ret != 0);
        static_cast<void>(ret);

        return formattedTime;
    }

    std::time_t TimeUtils::toTime(const std::string& s, const char* format)
    {
        std::tm timebuffer{};
        time_t now{};
        std::time(&now);
        (void)::localtime_r(&now, &timebuffer); // pre-fill local timezone info

        char* ret = strptime(s.c_str(), format, &timebuffer);
        if (ret == nullptr)
        {
            return 0;
        }
        return mktime(&timebuffer); // should be local time
    }

    std::time_t TimeUtils::toUTCTime(const std::string& s, const char* format)
    {
        std::tm timebuffer{};
        char* ret = strptime(s.c_str(), format, &timebuffer);
        if (ret == nullptr)
        {
            return 0;
        }
        timebuffer.tm_isdst = 0;
        // Using the GNU extension, maybe replace with C++ when switching to C++20?
        return timegm(&timebuffer); // should be UTC time
    }

    std::string TimeUtils::fromTime(std::time_t time_)
    {
        return fromTime(time_, "%Y%m%d %H%M%S");
    }

    std::string TimeUtils::fromTime(std::tm time_tm)
    {
        return fromTime(time_tm, "%Y%m%d %H%M%S");
    }

    std::time_t TimeUtils::toTime(const std::string& str)
    {
        return toTime(str, "%Y%m%d %H%M%S");
    }

    std::string TimeUtils::toEpochTime(const std::string& dateTime)
    {
        std::tm tm = {};
        std::stringstream ss(dateTime);
        ss >> std::get_time(&tm, "%Y%m%d %H%M%S");
        auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count());
    }

    long int TimeUtils::getMinutesSince(std::string timestamp)
    {
        tm tm = {};
        std::stringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

        std::chrono::system_clock::time_point then = std::chrono::system_clock::from_time_t(mktime(&tm));

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        return std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch() - then.time_since_epoch())
            .count();
    }

    // copied from SED
    static constexpr uint64_t WINDOWS_FILETIME_OFFSET = 0x019db1ded53e8000;
    static constexpr uint64_t WINDOWS_100NANO_PER_SECOND = 10000000;

    std::time_t TimeUtils::WindowsFileTimeToEpoch(int64_t ft)
    {
        return (ft - WINDOWS_FILETIME_OFFSET) / WINDOWS_100NANO_PER_SECOND;
    }

    int64_t TimeUtils::EpochToWindowsFileTime(std::time_t t)
    {
        return (static_cast<int64_t>(t) * WINDOWS_100NANO_PER_SECOND) + WINDOWS_FILETIME_OFFSET;
    }

    std::string FormattedTime::currentTime() const
    {
        return TimeUtils::fromTime(TimeUtils::getCurrTime(), "%Y%m%d %H%M%S");
    }
    std::string FormattedTime::bootTime() const
    {
        return TimeUtils::getBootTime();
    }

    std::string FormattedTime::currentEpochTimeInSeconds() const
    {
        return TimeUtils::fromTime(TimeUtils::getCurrTime(), "%s");
    }

    u_int64_t FormattedTime::currentEpochTimeInSecondsAsInteger() const
    {
        return std::stoul(TimeUtils::fromTime(TimeUtils::getCurrTime(), "%s"));
    }

    ScopedReplaceITime::ScopedReplaceITime(std::unique_ptr<ITime> mockTimer)
    {
        staticTimeSource().reset(mockTimer.release());
    }

    ScopedReplaceITime::~ScopedReplaceITime()
    {
        staticTimeSource().reset(new TimeSource{});
    }
} // namespace Common::UtilityImpl