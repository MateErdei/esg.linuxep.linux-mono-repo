/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Common/TelemetryExeConfigImpl/Constants.h"

#include <string>
#include <vector>

namespace TelemetrySchedulerImpl
{
    class SchedulerConfig
    {
    public:
        SchedulerConfig();

        unsigned int getTelemetryScheduledTime() const;
        void setTelemetryScheduledTime(unsigned int scheduledTime);

        bool operator==(const SchedulerConfig& rhs) const;
        bool operator!=(const SchedulerConfig& rhs) const;

        bool isValid() const;

    private:
        unsigned int m_telemetryScheduledTime{};
    };
} // namespace TelemetrySchedulerImpl