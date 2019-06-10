/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Common/TelemetryExeConfigImpl/Constants.h"

#include <chrono>
#include <string>
#include <vector>

namespace TelemetrySchedulerImpl
{
    using namespace std::chrono;

    class SchedulerStatus
    {
    public:
        SchedulerStatus();

        system_clock::time_point getTelemetryScheduledTime() const;
        void setTelemetryScheduledTime(system_clock::time_point scheduledTime);

        int getTelemetryScheduledTimeInSecondsSinceEpoch() const;
        void setTelemetryScheduledTimeInSecondsSinceEpoch(int scheduledTime);

        bool operator==(const SchedulerStatus& rhs) const;
        bool operator!=(const SchedulerStatus& rhs) const;

        bool isValid() const;

    private:
        int m_telemetryScheduledTime{};
    };
} // namespace TelemetrySchedulerImpl