/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Plugin
{
    struct TelemetryInfo
    {
        TelemetryInfo() : NoScans(0), AvgPerformance(0), NoFilesScanned(0), NoInfections(0) {}
        int NoScans;
        // MB/s
        double AvgPerformance;
        int NoFilesScanned;
        int NoInfections;
    };

    class Telemetry
    {
        Telemetry() = default;
        TelemetryInfo m_info;

    public:
        static Telemetry& instance();
        std::string getJson() const;
        void clear();
    };
} // namespace Plugin