/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryImpl/TelemetrySerialiser.h>
#include <Common/TelemetryImpl/TelemetryObject.h>
#include <Common/TelemetryImpl/TelemetryValue.h>

#include <include/gtest/gtest.h>
#include <json.hpp>
#include <iostream>
#include <memory>
#include <map>

const int TEST_INTEGER = 10;
const bool TEST_BOOL = true;
const std::string TEST_STRING = "Test String";  // NOLINT

//TEST(TestTelemetryValueImpl, SerialiseValue) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue("key", TEST_STRING);
//    nlohmann::json j = telemetryValue;
//
//    std::cout << j.dump() << std::endl;
//
//}
namespace Common::Telemetry
{
    TEST(TestTelemetryValueImpl, SerialiseDictionaryNoKey) // NOLINT
    {
        TelemetryObject root;
        TelemetryValue val1;
        val1.set(1);
        TelemetryValue val2;
        val2.set(2);
        TelemetryValue val3;
        val3.set(3);
        TelemetryValue val4;
        val4.set(4);
        TelemetryValue val5;
        val5.set(5);
        root.set("key1", val1);
        TelemetryObject nested1;
        nested1.set("nested1", val2);
        nested1.set("nested2", val3);
        nested1.set("nested3", val4);
        root.set("key2", nested1);

        TelemetryObject stringObj;
        TelemetryValue stringVal("string");
        stringObj.set(stringVal);

        TelemetryObject boolObj;
        TelemetryValue boolVal(true);
        boolObj.set(boolVal);

        std::list<TelemetryObject> array{stringObj, boolObj, nested1};
        root.set("my array", array);

        nlohmann::json j = root;

        ASSERT_EQ(1, j["key1"].get<int>());
        ASSERT_EQ(2, j["key2"]["nested1"].get<int>());
        ASSERT_EQ(3, j["key2"]["nested2"].get<int>());
        ASSERT_EQ(4, j["key2"]["nested3"].get<int>());

        ASSERT_EQ("string", j["my array"][0].get<std::string>());
        ASSERT_EQ(true, j["my array"][1].get<bool>());
        ASSERT_EQ(2, j["my array"][2]["nested1"].get<int>());
        ASSERT_EQ(3, j["my array"][2]["nested2"].get<int>());
        ASSERT_EQ(4, j["my array"][2]["nested3"].get<int>());

        auto newRoot = j.get<TelemetryObject>();
        ASSERT_EQ(root, newRoot);

    }

    TEST(TestTelemetryValueImpl, SerialiseToString) // NOLINT
    {
        TelemetryObject obj;
        TelemetryValue value;
        value.set("test");
        obj.set("key", value);
        TelemetrySerialiser serialiser;
        ASSERT_EQ(R"({"key":"test"})", serialiser.serialise(obj));
    }

    TEST(TestTelemetryValueImpl, DeserialiseToTelemetryObject) // NOLINT
    {
        TelemetryObject obj;
        TelemetryValue value;
        value.set("test");
        obj.set("key", value);
        std::string jsonString = R"({"key":"test"})";
        TelemetrySerialiser serialiser;
        auto deserialised = serialiser.deserialise(jsonString);
        ASSERT_EQ(obj, deserialised);
    }
}
