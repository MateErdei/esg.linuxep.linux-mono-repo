//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/ISusiWrapperFactory.h"

#include <gmock/gmock.h>

// gmock macro didn't seem to like taking a std::pair
using checkConfigReturnType = std::pair<bool, std::unique_ptr<common::ThreatDetector::SusiSettings>>;

class MockSusiWrapperFactory : public threat_scanner::ISusiWrapperFactory
{
public:
    MOCK_METHOD1(createSusiWrapper, threat_scanner::ISusiWrapperSharedPtr(const std::string& /*scannerConfig*/));

    MOCK_METHOD0(update, bool());
    MOCK_METHOD0(reload, ReloadResult());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(susiIsInitialized, bool());
    MOCK_METHOD(checkConfigReturnType, checkConfig, ());

};
