/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScanReport.h"

namespace Example
{
    struct TelemetryInfo
    {
        TelemetryInfo(): NoScans(0), AvgPerformance(0), NoFilesScanned(0), NoInfections(0)
        {}
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
        static Telemetry & instance();
        void updateWithReport(const ScanReport & scanReport);
        std::string getJson() const;
        void clear();
    };
}