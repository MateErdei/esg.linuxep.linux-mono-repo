/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScheduledUpdate.h"

#include <Common/UtilityImpl/TimeUtils.h>

#include <chrono>
#include <cstdlib>

namespace UpdateScheduler
{
    ScheduledUpdate::ScheduledUpdate() :
        m_enabled(false),
        m_scheduledTime(),
        m_nextScheduledUpdateTime(0)
    {
    }

    bool ScheduledUpdate::timeToUpdate(int offsetInMinutes)
    {
        if ( offsetInMinutes< 0 )
        {
            throw std::logic_error( "Do not use negative offset as it makes calculation of the next scheduled time complicated as the update can happens - before the delayed scheduled time. ");
        }
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();

        time_t nextScheduledUpdateTime = m_nextScheduledUpdateTime + offsetInMinutes * SecondsInMin;

        return now > nextScheduledUpdateTime ;
    }

    bool ScheduledUpdate::missedUpdate(const std::string& lastUpdate)
    {
        if ( m_nextScheduledUpdateTime == 0)
        {
            return false;
        }
        std::tm lastUpdateTime = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr }; // initialise to zero to avoid warnings

        char* returnChar = strptime(lastUpdate.c_str(), "%Y%m%d %H:%M:%S", &lastUpdateTime);
        lastUpdateTime.tm_isdst = -1; // We don't know if this time is in DST or not
        if (!returnChar)
        {
            return true;
        }
        std::time_t lastUpdateTimestamp = mktime(&lastUpdateTime);

        // if lastUpdateTime is before a week (plus an hour ) from the next update time it must have missed an update.
        return ( lastUpdateTimestamp + SecondsInWeek + SecondsInHour < m_nextScheduledUpdateTime );
    }

    std::time_t ScheduledUpdate::calculateNextScheduledUpdateTime(const std::time_t& nowTime)
    {
        if (m_nextScheduledUpdateTime == 0)
        {
            std::tm now = *std::localtime(&nowTime);

            int dayDiff = m_scheduledTime.tm_wday - now.tm_wday;
            int hourDiff = m_scheduledTime.tm_hour - now.tm_hour;
            int minDiff = m_scheduledTime.tm_min - now.tm_min;

            time_t totalDiff =
                (dayDiff * SecondsInDay) + (hourDiff * SecondsInHour) + (minDiff * SecondsInMin) - now.tm_sec;
            // If totalDiff is negative it is in the past and a week should be added to get the next time
            if (totalDiff < 0)
            {
                totalDiff += SecondsInWeek;
            }
            m_nextScheduledUpdateTime = nowTime + totalDiff;
        }

        return m_nextScheduledUpdateTime;
    }

    bool ScheduledUpdate::getEnabled() const { return m_enabled; }

    ScheduledUpdate::WeekDayAndTimeForDelay ScheduledUpdate::getScheduledTime() const
    {
        WeekDayAndTimeForDelay weekDayAndTimeForDelay{
            .weekDay = m_scheduledTime.tm_wday,
            .hour = m_scheduledTime.tm_hour,
            .minute = m_scheduledTime.tm_min};
        return weekDayAndTimeForDelay;
    }

    void ScheduledUpdate::setEnabled(bool enabled) { m_enabled = enabled; }

    void ScheduledUpdate::setScheduledTime(const ScheduledUpdate::WeekDayAndTimeForDelay& time)
    {
        std::tm time_tm{};
        time_tm.tm_wday = time.weekDay;
        time_tm.tm_hour = time.hour;
        time_tm.tm_min = time.minute;
        m_scheduledTime = time_tm;
        m_nextScheduledUpdateTime = 0;
        calculateNextScheduledUpdateTime(Common::UtilityImpl::TimeUtils::getCurrTime());
    }

    std::string ScheduledUpdate::nextUpdateTime()
    {
        if ( !getEnabled())
        {
            return std::string{};
        }
        if( m_nextScheduledUpdateTime == 0)
        {
            (void) timeToUpdate(0);
        }
        return Common::UtilityImpl::TimeUtils::fromTime(m_nextScheduledUpdateTime);
    }

    void ScheduledUpdate::confirmUpdatedTime()
    {
        m_nextScheduledUpdateTime = 0;
        calculateNextScheduledUpdateTime( Common::UtilityImpl::TimeUtils::getCurrTime() + 3600 );
    }

    void ScheduledUpdate::resetTimer()
    {
        setScheduledTime( getScheduledTime());
    }

} // namespace UpdateScheduler