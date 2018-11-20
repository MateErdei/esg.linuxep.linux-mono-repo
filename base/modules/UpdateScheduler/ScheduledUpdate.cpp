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
    {}

    bool ScheduledUpdate::timeToUpdate(int offsetInMinutes) const
    {
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
        time_t nextScheduledUpdateTime = calculateNextScheduledUpdateTime(now) + offsetInMinutes*60;
        time_t lastScheduledUpdateTime = calculateLastScheduledUpdateTime(now) + offsetInMinutes*60;
        time_t timeUntilNextScheduledUpdateTime = nextScheduledUpdateTime - now;
        time_t timeSinceLastScheduledUpdateTime = now - lastScheduledUpdateTime;

        return (timeUntilNextScheduledUpdateTime <= 60 || timeSinceLastScheduledUpdateTime <= 60);
    }

    bool ScheduledUpdate::missedUpdate(const std::string& lastUpdate) const
    {
        std::tm lastUpdateTime;
        char* returnChar = strptime(lastUpdate.c_str(), "%Y%m%d %H:%M:%S", &lastUpdateTime);
        if (!returnChar)
        {
            return true;
        }
        std::time_t lastUpdateTimestamp = mktime(&lastUpdateTime);
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();

        // If the last update time is before the most recent scheduled time, we have missed an update.
        // Additionally, if the next scheduled update time is in 10 minutes, we may miss an update.
        return (calculateLastScheduledUpdateTime(now) > lastUpdateTimestamp ||
                calculateNextScheduledUpdateTime(now) < (now + 10*60));
    }

    std::time_t ScheduledUpdate::calculateNextScheduledUpdateTime(const std::time_t& nowTime) const
    {
        std::tm now = *std::localtime(&nowTime);

        int dayDiff = m_scheduledTime.tm_wday - now.tm_wday;
        int hourDiff = m_scheduledTime.tm_hour - now.tm_hour;
        int minDiff = m_scheduledTime.tm_min - now.tm_min;

        time_t totalDiff = (dayDiff*24*60*60) + (hourDiff*60*60) + (minDiff*60) - now.tm_sec;

        // If totalDiff is negative it is in the past and a week should be added to get the next time
        if (totalDiff < 0)
        {
            totalDiff += 7*24*60*60;
        }

        return nowTime + totalDiff;
    }

    std::time_t ScheduledUpdate::calculateLastScheduledUpdateTime(const std::time_t& nowTime) const
    {
        return calculateNextScheduledUpdateTime(nowTime) - 7*24*60*60;
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
    }
}