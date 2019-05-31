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

        int getTelemetryScheduledTime() const;
        void setTelemetryScheduledTime(int scheduledTime);

        bool operator==(const SchedulerStatus& rhs) const;
        bool operator!=(const SchedulerStatus& rhs) const;

        bool isValid() const;

    private:
        int m_telemetryScheduledTime{};
    };
} // namespace TelemetrySchedulerImpl