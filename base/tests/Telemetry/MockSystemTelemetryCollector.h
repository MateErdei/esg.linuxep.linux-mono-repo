// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include <gmock/gmock.h>
#include "Telemetry/TelemetryImpl/ISystemTelemetryCollector.h"

#include <string>

using namespace ::testing;

class MockSystemTelemetryCollector : public Telemetry::ISystemTelemetryCollector
{
public:
    MOCK_CONST_METHOD0(collectObjects, std::map<std::string, Telemetry::TelemetryItem>());
    MOCK_CONST_METHOD0(collectArraysOfObjects, std::map<std::string, std::vector<Telemetry::TelemetryItem>>());
};