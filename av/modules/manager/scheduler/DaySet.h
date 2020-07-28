/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/XmlUtilities/AttributesMap.h>

namespace manager::scheduler
{
    enum Day
    {
        INVALID = -1,
        SUNDAY = 0,
        MONDAY = 1,
        TUESDAY = 2,
        WEDNESDAY = 3,
        THURSDAY = 4,
        FRIDAY = 5,
        SATURDAY = 6
    };

    class DaySet
    {
    public:
        DaySet() = default;
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

        int getNextScanTimeDelta(struct tm now, bool forceTomorrow) const;

        [[nodiscard]] std::string str() const;

    private:
        std::vector<Day> m_days;
    };
}


