/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <gmock/gmock.h>
#include <modules/Telemetry/TelemetryImpl/ISystemTelemetryCollector.h>

#include <string>

using namespace ::testing;

class MockSystemTelemetryCollector : public Telemetry::ISystemTelemetryCollector
{
public:
    MOCK_CONST_METHOD0(
        collectObjects,
        std::map<std::string, std::vector<std::pair<std::string, std::variant<std::string, int>>>>());
    MOCK_CONST_METHOD0(
        collectArraysOfObjects,
        std::map<std::string, std::vector<std::vector<std::pair<std::string, std::variant<std::string, int>>>>>());
};