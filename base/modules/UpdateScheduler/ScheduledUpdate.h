/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ctime>
#include <string>

class ScheduledUpdate
{
public:
    ScheduledUpdate();
    bool timeToUpdate() const;
    bool missedUpdate(const std::string& lastUpdate) const;
    std::time_t calculateMostRecentScheduledTime() const;
    bool getEnabled() const;
    std::tm getScheduledTime() const;
    void setEnabled(bool enabled);
    void setScheduledTime(const std::tm& time);
private:
    bool m_enabled;
    std::tm m_scheduledTime;
};
