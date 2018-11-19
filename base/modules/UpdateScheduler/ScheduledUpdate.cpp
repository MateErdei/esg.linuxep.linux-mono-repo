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

    bool ScheduledUpdate::timeToUpdate() const
    {
        std::tm now = Common::UtilityImpl::TimeUtils::getLocalTime();
        return (now.tm_wday == m_scheduledTime.tm_wday) &&
               (now.tm_hour == m_scheduledTime.tm_hour) &&
               (abs(now.tm_min - m_scheduledTime.tm_min) < 2);
    }

    bool ScheduledUpdate::missedUpdate(const std::string& lastUpdate) const
    {
        std::tm lastUpdateTime;
        char* returnCode = strptime(lastUpdate.c_str(), "%Y%m%d %H:%M:%S", &lastUpdateTime);
        if (!returnCode)
        {
            return true;
        }
        std::time_t lastUpdateTimestamp = mktime(&lastUpdateTime);
        std::time_t mostRecentScheduledTime = calculateMostRecentScheduledTime();
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();

        // If the last update time is before the most recent scheduled time, we have missed an update.
        // Additionally, if the most recent scheduled time will be more than a week ago in ten minutes time then we are about to miss an update
        return (mostRecentScheduledTime > lastUpdateTimestamp ||
                mostRecentScheduledTime < (now - 7*24*60*60 + 10*60));
    }

    std::time_t ScheduledUpdate::calculateMostRecentScheduledTime() const
    {
        std::time_t nowTime = Common::UtilityImpl::TimeUtils::getCurrTime();
        std::tm now = *std::localtime(&nowTime);

        int carry = 0;
        std::tm then = m_scheduledTime;

        int minuteDifference = now.tm_min - then.tm_min;
        if (minuteDifference < 0)
        {
            minuteDifference += 60;
            carry = 1;
        }

        int hourDifference = (now.tm_hour - then.tm_hour) - carry;
        carry = 0;
        if (hourDifference < 0)
        {
            hourDifference += 24;
            carry = 1;
        }

        int dayDifference = (now.tm_wday - then.tm_wday) - carry;
        if (dayDifference < 0)
        {
            dayDifference += 7;
        }

        std::chrono::minutes delay(((dayDifference * 24 * 60) + (hourDifference * 60) + minuteDifference) * 60);
        return nowTime - delay.count();
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