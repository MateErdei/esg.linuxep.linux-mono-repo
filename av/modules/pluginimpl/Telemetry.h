/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Plugin
{
    struct TelemetryInfo
    {
        TelemetryInfo()
            : NoScans(0)
            , AvgPerformance(0)
            , NoFilesScanned(0)
            , NoInfections(0)
            , Version("0.0.0") {}

        int NoScans;
        // MB/s
        double AvgPerformance;
        int NoFilesScanned;
        int NoInfections;
        std::string Version;
    };

    class Telemetry
    {
        Telemetry() = default;
        TelemetryInfo m_info;

    public:
        static Telemetry& instance();

        void setVersion(const std::string& version)
        {
            m_info.Version = version;
        }

        std::string getJson() const;
        void clear();
    };
} // namespace Plugin