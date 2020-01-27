/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DaySet.h"
#include "Logger.h"

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
}

DaySet::DaySet(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id)
{
    for ( const auto& attr : savPolicy.lookupMultiple(id))
    {
        const std::string d = attr.contents();
        m_days.push_back(convertFromString(d));
    }
}
