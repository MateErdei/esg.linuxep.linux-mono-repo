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
        std::time_t getCurrentTime() override
        {
            return std::time(nullptr);
        }
    };

    std::unique_ptr<Common::UtilityImpl::ITime>& static_Timer()
    {
        static std::unique_ptr<Common::UtilityImpl::ITime> timer{new TimeSource{}};
        return timer;
    }


}

namespace Common
{
    namespace UtilityImpl
    {
        std::string TimeUtils::fromTime(std::time_t time_)
        {
            if (time_ == -1)
            {
                return "";
            }

            std::tm time_tm;
            (void) ::localtime_r(&time_, &time_tm);
            return fromTime(time_tm);
        }

        std::time_t TimeUtils::getCurrTime()
        {
            return static_Timer()->getCurrentTime();
        }

        std::string TimeUtils::getBootTime() { return fromTime(getBootTimeAsTimet()); }

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

        std::string TimeUtils::fromTime(std::tm time_tm)
        {
            char formattedTime[16];
            size_t ret = strftime(formattedTime, 16, "%Y%m%d %H%M%S", &time_tm);
            assert(ret != 0);
            static_cast<void>(ret);

            return formattedTime;
        }


        std::string FormattedTime::currentTime() const { return TimeUtils::fromTime(TimeUtils::getCurrTime()); }
        std::string FormattedTime::bootTime() const { return TimeUtils::getBootTime(); }

        ScopedReplaceITime::ScopedReplaceITime(std::unique_ptr<ITime> mockTimer)
        {
            static_Timer().reset(mockTimer.release());
        }

        ScopedReplaceITime::~ScopedReplaceITime()
        {
            static_Timer().reset(new TimeSource{});
        }
    } // namespace UtilityImpl
} // namespace Common