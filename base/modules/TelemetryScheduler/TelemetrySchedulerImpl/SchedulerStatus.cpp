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

    system_clock::time_point SchedulerStatus::getTelemetryScheduledTime() const
    {
        auto duration = system_clock::duration(seconds(m_telemetryScheduledTime));
        system_clock::time_point scheduledTime(duration);
        return scheduledTime;
    }

    void SchedulerStatus::setTelemetryScheduledTime(system_clock::time_point scheduledTime)
    {
        m_telemetryScheduledTime = duration_cast<seconds>(scheduledTime.time_since_epoch()).count();
    }

    int SchedulerStatus::getTelemetryScheduledTimeInSecondsSinceEpoch() const { return m_telemetryScheduledTime; }

    void SchedulerStatus::setTelemetryScheduledTimeInSecondsSinceEpoch(int scheduledTime)
    {
        m_telemetryScheduledTime = scheduledTime;
    }
} // namespace TelemetrySchedulerImpl