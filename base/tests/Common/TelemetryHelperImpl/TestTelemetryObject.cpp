///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryObject.h>

#include <include/gtest/gtest.h>

using namespace Common::Telemetry;

class TelemetryObjectTestFixture : public ::testing::Test
{
public:
    std::string m_testKey = "TestKey";
    std::string m_badKey = "BadKey";
    std::string m_testString = "Test Value";
    TelemetryValue m_testValue = TelemetryValue(m_testString);
    std::list<TelemetryObject> m_testArray {TelemetryObject(), TelemetryObject()};
    TelemetryObject m_root;
};

TEST_F(TelemetryObjectTestFixture, Construction)
{
    TelemetryObject telemetryObject;

    ASSERT_EQ(TelemetryObject::Type::object, telemetryObject.getType());
}

TEST_F(TelemetryObjectTestFixture, SetKeyValue) // NOLINT
{
    m_root.set(m_testValue);
    ASSERT_EQ(TelemetryObject::Type::value, m_root.getType());
    ASSERT_EQ(m_testString, m_root.getValue().getString());
}

TEST_F(TelemetryObjectTestFixture, SetKeyObject) // NOLINT
{
    m_root.set(m_testKey, m_testValue);
    ASSERT_EQ(TelemetryObject::Type::object, m_root.getType());
    ASSERT_EQ(m_testValue, m_root.getObject(m_testKey).getValue());
}

TEST_F(TelemetryObjectTestFixture, SetKeyList) // NOLINT
{
    m_root.set(m_testKey, m_testArray);
    ASSERT_EQ(TelemetryObject::Type::object, m_root.getType());
    ASSERT_EQ(m_testArray, m_root.getObject(m_testKey).getArray());
}

TEST_F(TelemetryObjectTestFixture, SetValue) // NOLINT
{
    m_root.set(m_testValue);
    ASSERT_EQ(TelemetryObject::Type::value, m_root.getType());
    ASSERT_EQ(m_testString, m_root.getValue().getString());
}

TEST_F(TelemetryObjectTestFixture, SetList) // NOLINT
{
    m_root.set(m_testArray);
    ASSERT_EQ(TelemetryObject::Type::array, m_root.getType());
    ASSERT_EQ(m_testArray, m_root.getArray());
}

TEST_F(TelemetryObjectTestFixture, GetNonExistentObject) // NOLINT
{
    ASSERT_THROW(m_root.getObject(m_badKey), std::out_of_range); // NOLINT
}

TEST_F(TelemetryObjectTestFixture, GetValue_NotValue) // NOLINT
{
    m_root.set(m_testArray);
    ASSERT_THROW(m_root.getValue(), std::invalid_argument); //NOLINT
}

TEST_F(TelemetryObjectTestFixture, GetValueReference) // NOLINT
{
    m_root.set(m_testValue);
    ASSERT_EQ(m_testString, m_root.getValue().getString());
}

TEST_F(TelemetryObjectTestFixture, GetValueConstReference) // NOLINT
{
    m_root.set(m_testValue);
    const auto& testObj = m_root;
    ASSERT_EQ(m_testString, testObj.getValue().getString());
}

TEST_F(TelemetryObjectTestFixture, GetArray_NotArray) // NOLINT
{
    m_root.set(m_testValue);
    ASSERT_THROW(m_root.getArray(), std::invalid_argument); //NOLINT
}

TEST_F(TelemetryObjectTestFixture, GetArrayReference) // NOLINT
{
    m_root.set(m_testArray);
    ASSERT_EQ(m_testArray, m_root.getArray());
}

TEST_F(TelemetryObjectTestFixture, GetArrayConstReference) // NOLINT
{
    m_root.set(m_testArray);
    const auto& testObj = m_root;
    ASSERT_EQ(m_testArray, testObj.getArray());
}

TEST_F(TelemetryObjectTestFixture, GetChildObjects_NotObject) // NOLINT
{
    m_root.set(m_testValue);
    ASSERT_THROW(m_root.getChildObjects(), std::invalid_argument); //NOLINT
}

TEST_F(TelemetryObjectTestFixture, GetChildObjectsReference) // NOLINT
{
    m_root.set("nested1", m_testValue);
    m_root.set("nested2", m_testArray);

    auto& children = m_root.getChildObjects();
    ASSERT_EQ(2U, children.size());
    ASSERT_EQ(m_testValue.getString(), children.at("nested1").getValue().getString());
    ASSERT_EQ(m_testArray, children.at("nested2").getArray());
}

TEST_F(TelemetryObjectTestFixture, GetChildObjectsConstReference) // NOLINT
{
    m_root.set("nested1", m_testValue);
    m_root.set("nested2", m_testArray);
    const auto& testObj = m_root;

    const auto& children = testObj.getChildObjects();
    ASSERT_EQ(2U, children.size());

    ASSERT_EQ(m_testValue.getString(), children.at("nested1").getValue().getString());
    ASSERT_EQ(m_testArray, children.at("nested2").getArray());
}

TEST_F(TelemetryObjectTestFixture, KeyExists) // NOLINT
{
    m_root.set(m_testKey, m_testArray);
    ASSERT_TRUE(m_root.keyExists(m_testKey));
}

TEST_F(TelemetryObjectTestFixture, KeyDoesntExist) // NOLINT
{
    m_root.set(m_testKey, m_testArray);
    ASSERT_FALSE(m_root.keyExists(m_badKey));
}

TEST_F(TelemetryObjectTestFixture, EqualityMatching) // NOLINT
{
}

TEST_F(TelemetryObjectTestFixture, EqualityDifferent) // NOLINT
{
}
