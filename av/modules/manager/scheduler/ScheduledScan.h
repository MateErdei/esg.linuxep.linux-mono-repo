/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "DaySet.h"
#include "TimeSet.h"

namespace manager::scheduler
{
    class ScheduledScan
    {
    public:
        ScheduledScan(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id);
        [[nodiscard]] std::string name() const
        {
            return m_name;
        }

        [[nodiscard]] const DaySet& days() const
        {
            return m_days;
        }

        [[nodiscard]] const TimeSet& times() const
        {
            return m_times;
        }

    private:
        std::string m_name;
        DaySet m_days;
        TimeSet m_times;
    };
}



