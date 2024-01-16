// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "SchedulerStatus.h"

#include <chrono>

namespace TelemetrySchedulerImpl
{
    bool SchedulerStatus::operator==(const SchedulerStatus& rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }

        return m_telemetryScheduledTime == rhs.m_telemetryScheduledTime;
    }

    bool SchedulerStatus::operator!=(const SchedulerStatus& rhs) const
    {
        return !(rhs == *this);
    }

    bool SchedulerStatus::isValid() const
    {
        if (m_telemetryScheduledTime.has_value())
        {
            auto seconds = clock::to_time_t(m_telemetryScheduledTime.value());
            return seconds >= 0;
        }
        return false;
    }

    SchedulerStatus::time_point SchedulerStatus::getTelemetryScheduledTime() const
    {
        return m_telemetryScheduledTime.value_or(time_point{});
    }

    void SchedulerStatus::setTelemetryScheduledTime(system_clock::time_point scheduledTime)
    {
        m_telemetryScheduledTime = scheduledTime;
    }

    std::optional<SchedulerStatus::time_point> SchedulerStatus::getLastTelemetryStartTime() const
    {
        return lastTelemetryStartTime_;
    }

    void SchedulerStatus::setLastTelemetryStartTime(time_point time)
    {
        lastTelemetryStartTime_ = time;
    }

} // namespace TelemetrySchedulerImpl