/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <cstdint>
#include <ctime>
#include <string>

namespace threat_scanner
{
    class IThreatReporter
    {
    public:
        virtual ~IThreatReporter() = default;
        virtual void sendThreatReport(
            const std::string& threatPath,
            const std::string& threatName,
            const std::string& sha256,
            int64_t scanType,
            const std::string& userID,
            std::time_t detectionTimeStamp) = 0;
    };
    using IThreatReporterSharedPtr = std::shared_ptr<IThreatReporter>;
}
