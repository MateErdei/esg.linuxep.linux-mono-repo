/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryImpl/TelemetrySerialiser.h>
#include <Common/TelemetryImpl/TelemetryDictionary.h>
#include <Common/TelemetryImpl/TelemetryValue.h>

#include <include/gtest/gtest.h>
#include <json.hpp>
#include <iostream>
#include <memory>

const int TEST_INTEGER = 10;
const bool TEST_BOOL = true;
const std::string TEST_STRING = "Test String";  // NOLINT

TEST(TestTelemetryValueImpl, SerialiseValue) // NOLINT
{
    Common::Telemetry::TelemetryValue telemetryValue("key", TEST_STRING);
    nlohmann::json j = telemetryValue;

    std::cout << j.dump() << std::endl;
}

TEST(TestTelemetryValueImpl, SerialiseDictionaryNoKey) // NOLINT
{
    Common::Telemetry::TelemetryDictionary telemetryDictionary;
    auto nestedDict = std::make_shared<Common::Telemetry::TelemetryDictionary>("nested");
    nestedDict->set(std::make_shared<Common::Telemetry::TelemetryValue>("nestedKey1", "value1"));
    nestedDict->set(std::make_shared<Common::Telemetry::TelemetryValue>("nestedKey2", true));
    telemetryDictionary.set(nestedDict);
    telemetryDictionary.set(std::make_shared<Common::Telemetry::TelemetryValue>("key1", "value1"));
    telemetryDictionary.set(std::make_shared<Common::Telemetry::TelemetryValue>("key2", true));
    telemetryDictionary.set(std::make_shared<Common::Telemetry::TelemetryValue>("key3", 10));

    nlohmann::json j = telemetryDictionary;

    std::cout << j.dump() << std::endl;

    Common::Telemetry::TelemetryDictionary otherTelemetryDictionary;
    otherTelemetryDictionary = j.get<Common::Telemetry::TelemetryDictionary>();

    ASSERT_EQ(telemetryDictionary, otherTelemetryDictionary);
}