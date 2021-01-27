/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatReporter.h"

namespace threat_scanner
{
    class ThreatReporter : public IThreatReporter
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
