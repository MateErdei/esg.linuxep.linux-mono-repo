// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include <gmock/gmock.h>
#include "Telemetry/TelemetryImpl/ITelemetryProvider.h"

#include <string>

using namespace ::testing;

class MockTelemetryProvider : public Telemetry::ITelemetryProvider
{
public:
    MOCK_METHOD0(getTelemetry, std::string());
    MOCK_METHOD0(getName, std::string());
};