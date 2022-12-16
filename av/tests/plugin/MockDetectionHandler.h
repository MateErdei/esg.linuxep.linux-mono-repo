// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "pluginimpl/IDetectionHandler.h"

#include <gmock/gmock.h>

class MockDetectionHandler : public IDetectionHandler
{
public:
    MOCK_METHOD(void, finaliseDetection, (scan_messages::ThreatDetected & detection), (override));
    MOCK_METHOD(void, markAsQuarantining, (scan_messages::ThreatDetected & detection), (override));
};
