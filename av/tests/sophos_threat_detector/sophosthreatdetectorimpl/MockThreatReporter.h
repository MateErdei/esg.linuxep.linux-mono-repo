// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"

#include <gmock/gmock.h>

namespace {
    class MockThreatReporter : public threat_scanner::IThreatReporter
    {
    public:
        MOCK_METHOD(void, sendThreatReport, (const scan_messages::ThreatDetected& threatDetected));
    };
}