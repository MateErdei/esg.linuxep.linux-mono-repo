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
        [[nodiscard]] std::string str() const;
        [[nodiscard]] int hour() const { return m_hour;}
        [[nodiscard]] int minute() const { return m_minute;}
        [[nodiscard]] int second() const { return m_second;}
        bool operator<(const Time& rhs) const;
        [[nodiscard]] bool stillDueToday(const struct tm& now) const;
    private:
        int m_hour;
        int m_minute;
        int m_second;
    };

    class TimeSet
    {
    public:
        TimeSet() = default;
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

        Time getNextTime(const struct tm& now, bool& forceTomorrow) const;

        [[nodiscard]] std::string str() const;

    private:
        std::vector<Time> m_times;
    };
}



