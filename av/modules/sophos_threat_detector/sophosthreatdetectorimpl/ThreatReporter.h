/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/IThreatReporter.h"

#include "datatypes/sophos_filesystem.h"

namespace sspl::sophosthreatdetectorimpl
{

    class ThreatReporter : public threat_scanner::IThreatReporter
    {
    public:
        using path = sophos_filesystem::path;
        explicit ThreatReporter(path threatReporterSocketPath);

        void sendThreatReport(
            const std::string& threatPath,
            const std::string& threatName,
            const std::string& sha256,
            int64_t scanType,
            const std::string& userID,
            std::time_t detectionTimeStamp) override;
    private:
        sophos_filesystem::path m_threatReporterSocketPath;
    };
}
