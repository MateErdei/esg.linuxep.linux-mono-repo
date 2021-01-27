/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/IThreatReporter.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatReporter : public threat_scanner::IThreatReporter
    {
    public:
        void sendThreatReport(
            const std::string& threatPath,
            const std::string& threatName,
            int64_t scanType,
            const std::string& userID,
            std::time_t detectionTimeStamp) override;
    };
}
