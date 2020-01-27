/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/XmlUtilities/AttributesMap.h>

namespace manager::scheduler
{

    class Time
    {
    public:
        explicit Time(const std::string& time);
        int hour() { return m_hour;}
        int minute() { return m_minute;}
        bool operator<(const Time& rhs);
    private:
        int m_hour;
        int m_minute;
        int m_second;
    };

    class TimeSet
    {
    public:
        TimeSet(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id);
        [[nodiscard]] std::vector<Time> times() const
        {
            return m_times;
        }
        [[nodiscard]] size_t size() const
        {
            return m_times.size();
        }
        void sort();
    private:
        std::vector<Time> m_times;
    };
}



