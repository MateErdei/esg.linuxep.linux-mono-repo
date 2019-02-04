/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScheduledUpdate.h"
#include <Common/UtilityImpl/TimeUtils.h>
#include <cstdlib>
#include <chrono>

namespace UpdateScheduler
{
    ScheduledUpdate::ScheduledUpdate()
            :
            m_enabled(false)
            , m_scheduledTime()
            , m_nextScheduledUpdateTime(0)
            , m_lastScheduledUpdateTime(0)
    {}

    bool ScheduledUpdate::timeToUpdate(int offsetInMinutes)
    {
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
        time_t nextScheduledUpdateTime = calculateNextScheduledUpdateTime(now) + offsetInMinutes*SecondsInMin;
        time_t lastScheduledUpdateTime = calculateLastScheduledUpdateTime(now) + offsetInMinutes*SecondsInMin;
        time_t timeUntilNextScheduledUpdateTime = nextScheduledUpdateTime - now;
        time_t timeSinceLastScheduledUpdateTime = now - lastScheduledUpdateTime;

        return (timeUntilNextScheduledUpdateTime <= SecondsInMin || timeSinceLastScheduledUpdateTime <= SecondsInMin);
    }

    bool ScheduledUpdate::missedUpdate(const std::string& lastUpdate)
    {
        std::tm lastUpdateTime = {0};
        char* returnChar = strptime(lastUpdate.c_str(), "%Y%m%d %H:%M:%S", &lastUpdateTime);
        lastUpdateTime.tm_isdst = -1; // We don't know if this time is in DST or not
        if (!returnChar)
        {
            return true;
        }
        std::time_t lastUpdateTimestamp = mktime(&lastUpdateTime);
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();

        // If the last update time is before the most recent scheduled time, we have missed an update.
        // Additionally, if the next scheduled update time is in 10 minutes, we may miss an update.
        return (calculateLastScheduledUpdateTime(now) > lastUpdateTimestamp ||
                calculateNextScheduledUpdateTime(now) < (now + 10*SecondsInMin));
    }

    std::time_t ScheduledUpdate::calculateNextScheduledUpdateTime(const std::time_t& nowTime)
    {
        if (m_nextScheduledUpdateTime == 0)
        {
            std::tm now = *std::localtime(&nowTime);

            int dayDiff = m_scheduledTime.tm_wday - now.tm_wday;
            int hourDiff = m_scheduledTime.tm_hour - now.tm_hour;
            int minDiff = m_scheduledTime.tm_min - now.tm_min;

            time_t totalDiff = (dayDiff*SecondsInDay) + (hourDiff*SecondsInHour) + (minDiff*SecondsInMin) - now.tm_sec;

            // If totalDiff is negative it is in the past and a week should be added to get the next time
            if (totalDiff < 0)
            {
                totalDiff += SecondsInWeek;
            }
            m_nextScheduledUpdateTime = nowTime + totalDiff;
        }

        return m_nextScheduledUpdateTime;
    }

    std::time_t ScheduledUpdate::calculateLastScheduledUpdateTime(const std::time_t& nowTime)
    {
        if (m_lastScheduledUpdateTime == 0)
        {
            m_lastScheduledUpdateTime = calculateNextScheduledUpdateTime(nowTime) - SecondsInWeek;
        }
        return m_lastScheduledUpdateTime;
    }

    void ScheduledUpdate::resetScheduledUpdateTimes()
    {
        m_nextScheduledUpdateTime = 0;
        m_lastScheduledUpdateTime = 0;
    }

    bool ScheduledUpdate::getEnabled() const
    {
        return m_enabled;
    }

    std::tm ScheduledUpdate::getScheduledTime() const
    {
        return m_scheduledTime;
    }

    void ScheduledUpdate::setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    void ScheduledUpdate::setScheduledTime(const std::tm& time)
    {
        m_scheduledTime = time;
        resetScheduledUpdateTimes();
    }
}