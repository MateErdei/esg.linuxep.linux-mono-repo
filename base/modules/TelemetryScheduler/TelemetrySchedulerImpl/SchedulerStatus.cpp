/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerStatus.h"

#include <chrono>

namespace TelemetrySchedulerImpl
{
    SchedulerStatus::SchedulerStatus() {}

    bool SchedulerStatus::operator==(const SchedulerStatus& rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }

        return m_telemetryScheduledTime == rhs.m_telemetryScheduledTime;
    }

    bool SchedulerStatus::operator!=(const SchedulerStatus& rhs) const { return !(rhs == *this); }

    bool SchedulerStatus::isValid() const { return m_telemetryScheduledTime >= 0; }

    int SchedulerStatus::getTelemetryScheduledTime() const { return m_telemetryScheduledTime; }

    void SchedulerStatus::setTelemetryScheduledTime(int scheduledTime) { m_telemetryScheduledTime = scheduledTime;
    }
} // namespace TelemetrySchedulerImpl