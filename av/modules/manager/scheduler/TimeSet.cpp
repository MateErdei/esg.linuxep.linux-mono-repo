/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TimeSet.h"

#include <iomanip>
#include <algorithm>

using namespace manager::scheduler;

/*
 *
          <timeSet>
            <time>{{scheduledScanTime}}</time>
          </timeSet>
          <timeSet>
            <time>00:00:00</time>
          </timeSet>
          <timeSet>
            <time>13:00:00</time>
            <time>17:00:00</time>
          </timeSet>
 */

Time::Time(const std::string& time)
{
    std::stringstream ist(time);
    std::tm time_buffer{};
    ist >> std::get_time(&time_buffer, "%H:%M:%S");
    m_hour = time_buffer.tm_hour;
    m_minute = time_buffer.tm_min;
    m_second = time_buffer.tm_sec;
}

bool Time::operator<(const Time& rhs) const
{
    if (m_hour < rhs.m_hour)
    {
        return true;
    }
    if (m_hour > rhs.m_hour)
    {
        return false;
    }
    if (m_minute < rhs.m_minute)
    {
        return true;
    }
    if (m_minute > rhs.m_minute)
    {
        return false;
    }
    return m_second < rhs.m_second;
}

bool Time::stillDueToday(const struct tm& now) const
{
    return (hour() > now.tm_hour || (hour() == now.tm_hour && minute() > now.tm_min));
}

TimeSet::TimeSet(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id)
{
    for ( const auto& attr : savPolicy.lookupMultiple(id))
    {
        m_times.emplace_back(attr.contents());
    }
}

void TimeSet::sort()
{
    std::sort(m_times.begin(), m_times.end());
}

Time TimeSet::getNextTime(const struct tm& now, bool& forceTomorrow) const
{
    forceTomorrow = false;
    for (const auto& t : m_times)
    {
        if (t.stillDueToday(now))
        {
            return t;
        }
    }
    forceTomorrow = true;
    return m_times.at(0);
}
