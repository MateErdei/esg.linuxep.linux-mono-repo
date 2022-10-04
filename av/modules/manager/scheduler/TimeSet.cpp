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
    if (ist.fail())
    {
        m_isValid = false;
    }
    m_hour = time_buffer.tm_hour; // NOLINT(cppcoreguidelines-prefer-member-initializer)
    m_minute = time_buffer.tm_min; // NOLINT(cppcoreguidelines-prefer-member-initializer)
    m_second = time_buffer.tm_sec; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

std::string Time::str() const
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::to_string(m_hour) << ':'
    << std::setw(2) << std::setfill('0') << std::to_string(m_minute) << ':'
    << std::setw(2) << std::setfill('0') << std::to_string(m_second);
    return ss.str();
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
    return hour() > now.tm_hour ||
           (hour() == now.tm_hour && minute() > now.tm_min) ||
           (hour() == now.tm_hour && minute() == now.tm_min && second() > now.tm_sec);
}

TimeSet::TimeSet(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id)
{
    for ( const auto& attr : savPolicy.lookupMultiple(id))
    {
        m_times.emplace_back(attr.contents());
    }
    sort();
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

std::string TimeSet::str() const
{
    std::string returnString = "Times: ";
    for (const auto& time : m_times)
    {
        returnString += time.str() + " ";
    }
    return returnString;
}

bool TimeSet::isValid() const
{
    auto foundInvalid = std::find_if(m_times.begin(), m_times.end(), [](Time _t) { return !_t.isValid();});
    return foundInvalid != m_times.cend() ? false : true;
}