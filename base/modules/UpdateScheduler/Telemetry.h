/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace UpdateScheduler
{
    struct TelemetryInfo
    {
        TelemetryInfo()
        {}
    };

    class Telemetry
    {
        Telemetry() = default;
        TelemetryInfo m_info;

    public:
        static Telemetry & instance();
        std::string getJson() const;
        void clear();
    };
}