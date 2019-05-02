/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ctime>
#include <string>

namespace UpdateScheduler
{
    class ScheduledUpdate
    {
    public:
        struct WeekDayAndTimeForDelay
        {
            int weekDay;
            int hour;
            int minute;
            // seconds is not used when setting up delayed updates
        };
        ScheduledUpdate();

        // the offsetInMinutes must be a positive number as it simplifies the
        // calculation of the next time as the delayedUpdate never happens before the next scheduled time.
        bool timeToUpdate(int offsetInMinutes);

        bool missedUpdate(const std::string& lastUpdate);


        void confirmUpdatedTime();

        WeekDayAndTimeForDelay getScheduledTime() const;
        void setScheduledTime(const WeekDayAndTimeForDelay & time);

        bool getEnabled() const;
        void setEnabled(bool enabled);

        std::string nextUpdateTime();
    private:
        std::time_t calculateNextScheduledUpdateTime(const std::time_t& nowTime);

        bool m_enabled;
        std::tm m_scheduledTime;
        std::time_t m_nextScheduledUpdateTime;

        constexpr static int SecondsInMin = 60;
        constexpr static int SecondsInHour = 60 * SecondsInMin;
        constexpr static int SecondsInDay = 24 * SecondsInHour;
        constexpr static int SecondsInWeek = 7 * SecondsInDay;
    };
} // namespace UpdateScheduler
