/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerConfig.h"

namespace TelemetrySchedulerImpl
{
    SchedulerConfig::SchedulerConfig() {}

    bool SchedulerConfig::operator==(const SchedulerConfig& rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }

        return m_telemetryScheduledTime == rhs.m_telemetryScheduledTime;
    }

    bool SchedulerConfig::operator!=(const SchedulerConfig& rhs) const { return !(rhs == *this); }

    bool SchedulerConfig::isValid() const
    {
        if (m_telemetryScheduledTime < 1 || m_telemetryScheduledTime < 86700) // workout limits
        {
            return false;
        }

        return true;
    }

    unsigned int SchedulerConfig::getTelemetryScheduledTime() const { return m_telemetryScheduledTime; }

    void SchedulerConfig::setTelemetryScheduledTime(unsigned int scheduledTime)
    {
        m_telemetryScheduledTime = scheduledTime;
    }
} // namespace TelemetrySchedulerImpl