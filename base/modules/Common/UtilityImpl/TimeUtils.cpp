/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TimeUtils.h"

#include <sys/sysinfo.h>
#include <cassert>

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

            char formattedTime[16];
            size_t ret = strftime(formattedTime, 16, "%Y%m%d %H%M%S", std::localtime(&time_));
            assert(ret != 0);
            static_cast<void>(ret);

            return formattedTime;
        }

        std::time_t TimeUtils::getCurrTime() { return std::time(nullptr); }

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

        std::string FormattedTime::currentTime() const { return TimeUtils::fromTime(TimeUtils::getCurrTime()); }
        std::string FormattedTime::bootTime() const { return TimeUtils::getBootTime(); }
    } // namespace UtilityImpl
} // namespace Common