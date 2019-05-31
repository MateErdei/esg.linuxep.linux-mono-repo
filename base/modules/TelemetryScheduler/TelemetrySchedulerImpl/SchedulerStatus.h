/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Common/TelemetryExeConfigImpl/Constants.h"

#include <string>
#include <vector>

namespace TelemetrySchedulerImpl
{
    class SchedulerStatus
    {
    public:
        SchedulerStatus();

        size_t getTelemetryScheduledTime() const;
        void setTelemetryScheduledTime(size_t scheduledTime);

        bool operator==(const SchedulerStatus& rhs) const;
        bool operator!=(const SchedulerStatus& rhs) const;

        bool isValid() const;

    private:
        size_t m_telemetryScheduledTime{};
    };
} // namespace TelemetrySchedulerImpl