/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryObject.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Common/TelemetryHelperImpl/TelemetryValue.h>
#include <include/gtest/gtest.h>
#include <json.hpp>

namespace Common::Telemetry
{
    void to_json(nlohmann::json& j, const TelemetryValue& node);
    void from_json(const nlohmann::json& j, TelemetryValue& node);

    void to_json(nlohmann::json& j, const TelemetryObject& telemetryObject);
    void from_json(const nlohmann::json& j, TelemetryObject& telemetryObject);
}

using namespace Common::Telemetry;

class TelemetrySerialiserTestFixture : public ::testing::Test
{
public:
    TelemetrySerialiserTestFixture() :
        m_testValue1(1UL),
        m_testValue2(4294967200UL),
        m_testValue3(3.2),
        m_testValue4(-4L),
        m_testString("TestValue"),
        m_testBool(true)
    {
        TelemetryObject stringObj;
        stringObj.set(TelemetryValue(m_testString));
        TelemetryObject boolObj;
        boolObj.set(TelemetryValue(m_testBool));

        TelemetryObject nested;
        nested.set("nested1", TelemetryValue(m_testValue2));
        nested.set("nested2", TelemetryValue(m_testValue3));
        nested.set("nested3", TelemetryValue(m_testValue4));

        m_root.set("key1", TelemetryValue(m_testValue1));
        m_root.set("key2", nested);

        std::list<TelemetryObject> array{ stringObj, boolObj, nested };
        m_root.set("my array", array);
    }

    unsigned long m_testValue1;
    unsigned long m_testValue2;
    double m_testValue3;
    long m_testValue4;
    std::string m_testString;
    bool m_testBool;

    TelemetryObject m_root;
    std::string m_serialisedTelemetry =
        R"({"key1":1,"key2":{"nested1":4294967200,"nested2":3.2,"nested3":-4},"my array":["TestValue",true,{"nested1":4294967200,"nested2":3.2,"nested3":-4}]})";
};

class TelemetrySerialiserParameterisedTestFixture : public ::testing::TestWithParam<std::string>
{
};

INSTANTIATE_TEST_CASE_P(
    ParameterisedJsonTests, // NOLINT
    TelemetrySerialiserParameterisedTestFixture,
    ::testing::Values(
        R"({"a":"b"})",
        R"({"a":""})",
        R"({"key":[]})",
        R"([])",
        R"({"a":"1", "b":2, "c":false, "d":["string", 2, true, "string2", "string"]})",
        R"({"a":"1", "b":2, "c":false, "d":["string", 2, true, {"nested":{"array":[1,2,3]}}, "string"]})",
        R"({"OS":{"Name":"Ubuntu","Arch":"x64","Platform":"Linux","Version":"18.04"},"DiskSpace":{"SpaceInMB":150000,"Healthy":true}})"));

TEST_P(TelemetrySerialiserParameterisedTestFixture, SerialiseAndDeserialise) // NOLINT
{
    std::string testJson = GetParam();
    auto telemetryObject = TelemetrySerialiser::deserialise(testJson);
    std::string serialized = TelemetrySerialiser::serialise(telemetryObject);
    auto anotherObject = TelemetrySerialiser::deserialise(serialized);
    EXPECT_EQ(telemetryObject, anotherObject) << "Failed: " << testJson;
}

TEST_F(TelemetrySerialiserTestFixture, TelemetryObjectToJsonAndBackToTelemetryObject) // NOLINT
{
    // Serialise known object to json and verify fields.
    nlohmann::json j = m_root;

    // Check json object constructed correctly.
    ASSERT_EQ(m_testValue1, j["key1"].get<unsigned int>());
    ASSERT_EQ(m_testValue2, j["key2"]["nested1"].get<unsigned int>());
    ASSERT_EQ(m_testValue3, j["key2"]["nested2"].get<double>());
    ASSERT_EQ(m_testValue4, j["key2"]["nested3"].get<int>());
    ASSERT_EQ(m_testString, j["my array"][0].get<std::string>());
    ASSERT_EQ(m_testBool, j["my array"][1].get<bool>());
    ASSERT_EQ(m_testValue2, j["my array"][2]["nested1"].get<unsigned int>());
    ASSERT_EQ(m_testValue3, j["my array"][2]["nested2"].get<double>());
    ASSERT_EQ(m_testValue4, j["my array"][2]["nested3"].get<int>());

    // Create a new Telemetry Object from the json object and check it is equal in value to the original root.
    auto newRoot = j.get<TelemetryObject>();
    ASSERT_EQ(m_root, newRoot);
}

TEST_F(TelemetrySerialiserTestFixture, SerialiseToString) // NOLINT
{
    ASSERT_EQ(m_serialisedTelemetry, TelemetrySerialiser::serialise(m_root));
}

TEST_F(TelemetrySerialiserTestFixture, DeserialiseToTelemetryObject) // NOLINT
{
    ASSERT_EQ(m_root, TelemetrySerialiser::deserialise(m_serialisedTelemetry));
}

TEST_F(TelemetrySerialiserTestFixture, DeserialiseEmptyString) // NOLINT
{
    ASSERT_THROW(TelemetrySerialiser::deserialise(""), nlohmann::detail::parse_error); // NOLINT
}

TEST_F(TelemetrySerialiserTestFixture, DeserialiseInvalidJson) // NOLINT
{
    ASSERT_THROW(TelemetrySerialiser::deserialise("{thing:}"), nlohmann::detail::parse_error); // NOLINT
}
