/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DaySet.h"
#include "Logger.h"

#include <algorithm>

using namespace manager::scheduler;

/*
 *
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>{{day}}</day>
          </daySet>
 */

namespace
{
    Day convertFromString(const std::string& day)
    {
        if (day == "monday")
        {
            return MONDAY;
        }
        if (day == "tuesday")
        {
            return TUESDAY;
        }
        if (day == "wednesday")
        {
            return WEDNESDAY;
        }
        if (day == "thursday")
        {
            return THURSDAY;
        }
        if (day == "friday")
        {
            return FRIDAY;
        }
        if (day == "saturday")
        {
            return SATURDAY;
        }
        if (day == "sunday")
        {
            return SUNDAY;
        }
        LOGERROR("Invalid day from policy: "<< day);
        return INVALID;
    }

    std::string convertToString(const Day& day)
    {
        if (day == MONDAY)
        {
            return "monday";
        }
        if (day == TUESDAY)
        {
            return "tuesday";
        }
        if (day == WEDNESDAY)
        {
            return "wednesday";
        }
        if (day == THURSDAY)
        {
            return "thursday";
        }
        if (day == FRIDAY)
        {
            return "friday";
        }
        if (day == SATURDAY)
        {
            return "saturday";
        }
        if (day == SUNDAY)
        {
            return "sunday";
        }
        return "invalid";
    }
}

DaySet::DaySet(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id)
{
    for ( const auto& attr : savPolicy.lookupMultiple(id))
    {
        const std::string d = attr.contents();
        m_days.push_back(convertFromString(d));
    }
    sort();
}

void DaySet::sort()
{
    std::sort(m_days.begin(), m_days.end());
}

int DaySet::getNextDay(struct tm now, bool forceTomorrow) const
{
    for (const auto& day : m_days)
    {
        if (day < now.tm_wday)
        {
            continue;
        }
        else if (day == now.tm_wday)
        {
            if (!forceTomorrow)
            {
                return 0;
            }
        }
        else
        {
            return day - now.tm_wday;
        }
    }
    return 7 + m_days[0] - now.tm_wday;
}

std::string DaySet::str() const
{
    std::string returnString = "Days: ";
    for (const auto& day : m_days)
    {
        returnString += convertToString(day) + " ";
    }
    return returnString;
}