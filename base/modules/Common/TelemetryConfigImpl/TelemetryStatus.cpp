// Copyright 2024 Sophos Limited. All rights reserved.

#include "TelemetryStatus.h"

#include <chrono>

namespace Common::TelemetryConfigImpl
{
    TelemetryStatus::TelemetryStatus() {}

    bool TelemetryStatus::operator==(const TelemetryStatus& rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }

        return m_lastStartTime == rhs.m_lastStartTime;
    }

    bool TelemetryStatus::operator!=(const TelemetryStatus& rh) const
    {
        return !(rh == *this);
    }

    bool TelemetryStatus::isValid() const
    {
        return m_lastStartTime >= 0;
    }

    system_clock::time_point TelemetryStatus::getTelemetryStartTime() const
    {
        auto duration = system_clock::duration(seconds(m_lastStartTime));
        system_clock::time_point scheduledTime(duration);
        return scheduledTime;
    }

    void TelemetryStatus::setTelemetryStartTime(system_clock::time_point scheduledTime)
    {
        m_lastStartTime = duration_cast<seconds>(scheduledTime.time_since_epoch()).count();
    }

    int TelemetryStatus::getTelemetryStartTimeInSecondsSinceEpoch() const
    {
        return m_lastStartTime;
    }

    void TelemetryStatus::setTelemetryStartTimeInSecondsSinceEpoch(int scheduledTime)
    {
        m_lastStartTime = scheduledTime;
    }
} // namespace TelemetryConfigImpl