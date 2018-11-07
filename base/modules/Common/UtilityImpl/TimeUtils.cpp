/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TimeUtils.h"
#include <sys/sysinfo.h>

namespace  Common
{
    namespace UtilityImpl
    {

        std::string TimeUtils::fromTime(std::time_t time_)
        {
            if ( time_ == -1 )
            {
                return "";
            }

            char formattedTime[16];
            strftime(formattedTime, 16, "%Y%m%d %H%M%S", std::localtime(&time_));

            return formattedTime;

        }

        std::time_t TimeUtils::getCurrTime()
        {
            return std::time(nullptr);
        }

        std::tm TimeUtils::getLocalTime()
        {
            time_t nowTime = getCurrTime();
            return *std::localtime(&nowTime);
        }

        /*
        delayTimestampPair TimeUtils::calculateTimestampFromWeekdayAndTime(const std::string& weekday, const std::string& time)
        {
            std::tm then;
            std::string weekdayAndTime = weekday + "," + time;

            // "Monday,17:43:00"
            strptime(weekdayAndTime.c_str(), "%a,%H:%M:%S", &then);

            time_t currentTime = getCurrTime();
            std::tm now = *std::localtime(&currentTime);

            int carry = 0;

            int minuteDifference = then.tm_min - now.tm_min;
            if (minuteDifference < 0)
            {
                minuteDifference += 60;
                carry = 1;
            }

            int hourDifference = (then.tm_hour - now.tm_hour) - carry;
            carry = 0;
            if (hourDifference < 0)
            {
                hourDifference += 24;
                carry = 1;
            }

            int dayDifference = (then.tm_wday - now.tm_wday) - carry;
            if (dayDifference < 0)
            {
                dayDifference += 7;
            }

            std::chrono::minutes delay(((dayDifference * 24 * 60) + (hourDifference * 60) + minuteDifference) * 60);
            std::pair<std::chrono::minutes, time_t> delayAndTimestamp{delay, currentTime + delay.count()};

            return delayAndTimestamp;
        }*/

        std::string TimeUtils::getBootTime()
        {
            return  fromTime( getBootTimeAsTimet());
        }

        std::time_t TimeUtils::getBootTimeAsTimet()
        {
            struct sysinfo info;
            if( sysinfo( &info) != 0)
            {
                return 0; // error
            }
            auto curr = getCurrTime();
            return curr - info.uptime;
        }


        std::string FormattedTime::currentTime() const
        {
            return TimeUtils::fromTime( TimeUtils::getCurrTime());
        }

        std::string FormattedTime::bootTime() const
        {
            return TimeUtils::getBootTime();
        }
    }
}