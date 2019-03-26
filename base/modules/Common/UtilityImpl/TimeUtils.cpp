/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TimeUtils.h"

#include <sys/sysinfo.h>

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
            strftime(formattedTime, 16, "%Y%m%d %H%M%S", std::localtime(&time_));

            return formattedTime;
        }

        std::string TimeUtils::dateFromTime(std::time_t time_)
        {
            if (time_ == -1)
            {
                return "";
            }

            char formattedTime[9];
            strftime(formattedTime, 9, "%Y%m%d", std::localtime(&time_));

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
        std::string FormattedTime::currentDate() const { return TimeUtils::dateFromTime(TimeUtils::getCurrTime()); }

        std::string FormattedTime::bootTime() const { return TimeUtils::getBootTime(); }
    } // namespace UtilityImpl
} // namespace Common