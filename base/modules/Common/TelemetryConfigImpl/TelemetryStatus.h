// Copyright 2024 Sophos Limited. All rights reserved.
#pragma once

#include "base/modules/Common/TelemetryConfigImpl/Constants.h"

#include <chrono>
#include <string>
#include <vector>

namespace Common::TelemetryConfigImpl
{
    using namespace std::chrono;

    class TelemetryStatus
    {
    public:
        TelemetryStatus();

        system_clock::time_point getTelemetryStartTime() const;
        void setTelemetryStartTime(system_clock::time_point startTime);

        int getTelemetryStartTimeInSecondsSinceEpoch() const;
        void setTelemetryStartTimeInSecondsSinceEpoch(int startTime);

        bool operator==(const TelemetryStatus& rhs) const;
        bool operator!=(const TelemetryStatus& rhs) const;

        bool isValid() const;

    private:
        int m_lastStartTime{};
    };
} // namespace TelemetryConfigImpl
