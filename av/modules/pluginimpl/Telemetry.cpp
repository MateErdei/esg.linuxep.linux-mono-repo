/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "StringReplace.h"

namespace Plugin
{
    Telemetry& Telemetry::instance()
    {
        static Telemetry telemetry;
        return telemetry;
    }

    std::string Telemetry::getJson() const
    {
        std::string jsonTemplate{ R"sophos({
	"Number of Scans" : NOSCANSKEY,
	"Number of Files Scanned" : NOFILESKEY,
	"Number of Infections" : NOINFECTIONSKEY,
	"Average Performance" : AVGPERFKEY,
    "Average Performance Unit" : "MB/s",
    "Version Number" : VERSION
})sophos" };
        KeyValueCollection keyvalues = { { "NOSCANSKEY", std::to_string(m_info.NoScans) },
                                         { "NOFILESKEY", std::to_string(m_info.NoFilesScanned) },
                                         { "NOINFECTIONSKEY", std::to_string(m_info.NoInfections) },
                                         { "AVGPERFKEY", std::to_string(m_info.AvgPerformance) },
                                         { "VERSION", m_info.Version }, };

        return orderedStringReplace(jsonTemplate, keyvalues);
    }

    void Telemetry::clear() { m_info = TelemetryInfo(); }
} // namespace Plugin