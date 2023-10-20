// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IThreatReporter.h"

#include <gmock/gmock.h>

namespace {
    class MockThreatReporter : public threat_scanner::IThreatReporter
    {
    public:
        MOCK_METHOD(void, sendThreatReport, (const scan_messages::ThreatDetected& threatDetected));
    };
}