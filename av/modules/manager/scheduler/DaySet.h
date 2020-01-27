/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/XmlUtilities/AttributesMap.h>

namespace manager::scheduler
{
    enum Day
    {
        INVALID,
        MONDAY,
        TUESDAY,
        WEDNESDAY,
        THURSDAY,
        FRIDAY,
        SATURDAY,
        SUNDAY
    };

    class DaySet
    {
    public:
        DaySet(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id);
        [[nodiscard]] std::vector<Day> days() const
        {
            return m_days;
        }
        [[nodiscard]] size_t size() const
        {
            return m_days.size();
        }
        void sort();
    private:
        std::vector<Day> m_days;
    };
}


