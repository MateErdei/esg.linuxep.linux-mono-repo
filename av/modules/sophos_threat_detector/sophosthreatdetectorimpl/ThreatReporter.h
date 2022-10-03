// Copyright 2021-2022, Sophos Limited.  All rights reserved.

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

        void sendThreatReport(const scan_messages::ThreatDetected& threatDetected) override;

    private:
        sophos_filesystem::path m_threatReporterSocketPath;
    };
} // namespace sspl::sophosthreatdetectorimpl
