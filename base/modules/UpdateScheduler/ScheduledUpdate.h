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
        ScheduledUpdate();

        bool timeToUpdate(int offsetInMinutes);

        bool missedUpdate(const std::string& lastUpdate);

        void resetScheduledUpdateTimes();

        bool getEnabled() const;

        std::tm getScheduledTime() const;

        void setEnabled(bool enabled);

        void setScheduledTime(const std::tm& time);

    private:
        std::time_t calculateNextScheduledUpdateTime(const std::time_t& nowTime);

        std::time_t calculateLastScheduledUpdateTime(const std::time_t& nowTime);

        bool m_enabled;
        std::tm m_scheduledTime;
        std::time_t m_nextScheduledUpdateTime;
        std::time_t m_lastScheduledUpdateTime;

        constexpr static int SecondsInMin = 60;
        constexpr static int SecondsInHour = 60 * SecondsInMin;
        constexpr static int SecondsInDay = 24 * SecondsInHour;
        constexpr static int SecondsInWeek = 7 * SecondsInDay;
    };
}
