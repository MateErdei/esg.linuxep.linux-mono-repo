/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"
#include "StringReplace.h"

namespace Example
{

    Telemetry &Telemetry::instance()
    {
        static Telemetry telemetry;
        return telemetry;
    }

    void Telemetry::updateWithReport(const ScanReport &scanReport)
    {
        time_t timespan = scanReport.getFinishTime()-scanReport.getStartTime();
        if (timespan != 0 )
        {
            double performance = scanReport.getTotalMemoryMB()/(timespan);
            double accumulatedPerformance = (m_info.AvgPerformance * m_info.NoScans + performance) / (m_info.NoScans + 1);
            m_info.AvgPerformance = accumulatedPerformance;
        }
        m_info.NoScans++;
        m_info.NoInfections += scanReport.getInfections().size();
        m_info.NoFilesScanned += scanReport.getNoFilesScanned();
    }

    std::string Telemetry::getJson() const
    {
        std::string jsonTemplate{R"sophos({
	"Number of Scans" : NOSCANSKEY,
	"Number of Files Scanned" : NOFILESKEY,
	"Number of Infections" : NOINFECTIONSKEY,
	"Average Performance" : AVGPERFKEY,
    "Average Performance Unit" : "MB/s"
})sophos"};
        KeyValueCollection keyvalues = {
                {"NOSCANSKEY", std::to_string(m_info.NoScans)},
                {"NOFILESKEY", std::to_string(m_info.NoFilesScanned)},
                {"NOINFECTIONSKEY", std::to_string(m_info.NoInfections)},
                {"AVGPERFKEY", std::to_string(m_info.AvgPerformance)}
        };
        return orderedStringReplace(jsonTemplate, keyvalues);
    }

    void Telemetry::clear()
    {
        m_info = TelemetryInfo();
    }
}