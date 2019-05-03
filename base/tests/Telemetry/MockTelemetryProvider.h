/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/HttpSender/IHttpSender.h>
#include <gmock/gmock.h>
#include <modules/Telemetry/TelemetryImpl/ITelemetryProvider.h>

#include <string>

using namespace ::testing;

class MockTelemetryProvider : public Telemetry::ITelemetryProvider
{
public:
    MOCK_METHOD0(getTelemetry, std::string());
    MOCK_METHOD0(getName, std::string());
};