/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryObject.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Common/TelemetryHelperImpl/TelemetryValue.h>
#include <include/gtest/gtest.h>

using namespace Common::Telemetry;

class TelemetrySerialiserTestFixture : public ::testing::Test
{
public:
    TelemetrySerialiserTestFixture() :
        m_testValue1(1U),
        m_testValue2(4294967200U),
        m_testValue3(3U),
        m_testValue4(-4),
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

    unsigned int m_testValue1;
    unsigned int m_testValue2;
    unsigned int m_testValue3;
    int m_testValue4;
    std::string m_testString;
    bool m_testBool;

    TelemetryObject m_root;
    std::string m_serialisedTelemetry =
        R"({"key1":1,"key2":{"nested1":4294967200,"nested2":3,"nested3":-4},"my array":["TestValue",true,{"nested1":4294967200,"nested2":3,"nested3":-4}]})";
};


class TelemetrySerialiserParameterisedTestFixture : public ::testing::TestWithParam<std::string>
{
};

INSTANTIATE_TEST_CASE_P(ParameterisedJsonTests,  // NOLINT
                        TelemetrySerialiserParameterisedTestFixture,
                        ::testing::Values(
                            R"({"a":"b"})",
                            R"({"OS":{"Name":"Ubuntu","Arch":"x64","Platform":"Linux","Version":"18.04"},"DiskSpace":{"SpaceInMB":150000,"Healthy":true}})"));

TEST_P(TelemetrySerialiserParameterisedTestFixture, SerialiseAndDeserialise ) // NOLINT
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
    ASSERT_EQ(m_testValue3, j["key2"]["nested2"].get<unsigned int>());
    ASSERT_EQ(m_testValue4, j["key2"]["nested3"].get<int>());
    ASSERT_EQ(m_testString, j["my array"][0].get<std::string>());
    ASSERT_EQ(m_testBool, j["my array"][1].get<bool>());
    ASSERT_EQ(m_testValue2, j["my array"][2]["nested1"].get<unsigned int>());
    ASSERT_EQ(m_testValue3, j["my array"][2]["nested2"].get<unsigned int>());
    ASSERT_EQ(m_testValue4, j["my array"][2]["nested3"].get<int>());

    // Create a new Telemetry Object from the json object and check it is equal in value to the original root.
    auto newRoot = j.get<TelemetryObject>();
    ASSERT_EQ(m_root, newRoot);
}

TEST_F(TelemetrySerialiserTestFixture, SerialiseToString) // NOLINT
{
    TelemetrySerialiser serialiser;
    ASSERT_EQ(m_serialisedTelemetry, serialiser.serialise(m_root));
}

TEST_F(TelemetrySerialiserTestFixture, DeserialiseToTelemetryObject) // NOLINT
{
    ASSERT_EQ(m_root, TelemetrySerialiser::deserialise(m_serialisedTelemetry));
}

TEST_F(TelemetrySerialiserTestFixture, DeserialiseEmptyString) // NOLINT
{
    TelemetrySerialiser serialiser;
    ASSERT_THROW(serialiser.deserialise("{thing:}"), nlohmann::detail::parse_error); // NOLINT
}

TEST_F(TelemetrySerialiserTestFixture, DeserialiseInvalidJson) // NOLINT
{
    ASSERT_THROW(TelemetrySerialiser::deserialise("{thing:}"), nlohmann::detail::parse_error); // NOLINT
}
