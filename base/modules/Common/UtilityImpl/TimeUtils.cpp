/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TimeUtils.h"

#include <sys/sysinfo.h>

#include <cassert>

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

namespace Common
{
    namespace UtilityImpl
    {
        std::string TimeUtils::fromTime(std::time_t time_, const char *format)
        {
            if (time_ == -1)
            {
                return "";
            }

            std::tm time_tm;
            (void)::localtime_r(&time_, &time_tm);
            return fromTime(time_tm, format);
        }

        std::time_t TimeUtils::getCurrTime() { return staticTimeSource()->getCurrentTime(); }

        std::string TimeUtils::getBootTime() { return fromTime(getBootTimeAsTimet(), "%Y%m%d %H%M%S"); }

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

        std::string TimeUtils::fromTime(std::tm time_tm, const char *format)
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

        std::string FormattedTime::currentTime() const { return TimeUtils::fromTime(TimeUtils::getCurrTime(), "%Y%m%d %H%M%S"); }
        std::string FormattedTime::bootTime() const { return TimeUtils::getBootTime(); }

        std::string FormattedTime::currentEpochTimeInSeconds() const {
            return TimeUtils::fromTime(TimeUtils::getCurrTime(), "%s");
        }

        ScopedReplaceITime::ScopedReplaceITime(std::unique_ptr<ITime> mockTimer)
        {
            staticTimeSource().reset(mockTimer.release());
        }

        ScopedReplaceITime::~ScopedReplaceITime() { staticTimeSource().reset(new TimeSource{}); }
    } // namespace UtilityImpl
} // namespace Common