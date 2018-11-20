/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ctime>
#include <string>

namespace UpdateScheduler
{
    class ScheduledUpdate
    {
    public:
        ScheduledUpdate();

        bool timeToUpdate(int offsetInMinutes) const;

        bool missedUpdate(const std::string& lastUpdate) const;

        std::time_t calculateNextScheduledUpdateTime(const std::time_t& nowTime) const;

        std::time_t calculateLastScheduledUpdateTime(const std::time_t& nowTime) const;

        bool getEnabled() const;

        std::tm getScheduledTime() const;

        void setEnabled(bool enabled);

        void setScheduledTime(const std::tm& time);

    private:
        bool m_enabled;
        std::tm m_scheduledTime;
    };
}
