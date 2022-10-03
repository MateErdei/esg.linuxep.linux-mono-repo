// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

#include "scan_messages/ThreatDetected.h"

namespace threat_scanner
{
    class IThreatReporter
    {
    public:
        virtual ~IThreatReporter() = default;
        virtual void sendThreatReport(const scan_messages::ThreatDetected& threatDetected) = 0;
    };
    using IThreatReporterSharedPtr = std::shared_ptr<IThreatReporter>;
}
