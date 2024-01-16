// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/TelemetryConfigImpl/Constants.h"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace TelemetrySchedulerImpl
{
    using namespace std::chrono;

    class SchedulerStatus
    {
    public:
        using clock = std::chrono::system_clock;
        using time_point = clock::time_point;

        [[nodiscard]] std::optional<time_point> getLastTelemetryStartTime() const;
        void setLastTelemetryStartTime(time_point time);

        [[nodiscard]] time_point getTelemetryScheduledTime() const;
        void setTelemetryScheduledTime(time_point scheduledTime);

        bool operator==(const SchedulerStatus& rhs) const;
        bool operator!=(const SchedulerStatus& rhs) const;

        [[nodiscard]] bool isValid() const;

    private:
        [[nodiscard]] int getTelemetryScheduledTimeInSecondsSinceEpoch() const;
        void setTelemetryScheduledTimeInSecondsSinceEpoch(int scheduledTime);

        std::optional<time_point> lastTelemetryStartTime_{};
        std::optional<time_point> m_telemetryScheduledTime{};
    };
} // namespace TelemetrySchedulerImpl