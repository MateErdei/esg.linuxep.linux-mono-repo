///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryValue.h>

#include <include/gtest/gtest.h>

const int TEST_INTEGER = 10;
const bool TEST_BOOL = true;
const char* TEST_CSTRING = "Test String";
const std::string TEST_STRING = TEST_CSTRING;  // NOLINT

using namespace Common::Telemetry;

// Construction
TEST(TestTelemetryValueImpl, Construction) // NOLINT
{
    TelemetryValue telemetryValue;
    ASSERT_EQ(TelemetryValue::ValueType::unset, telemetryValue.getValueType());
}

TEST(TestTelemetryValueImpl, ConstructionWithString) // NOLINT
{
    TelemetryValue telemetryValue(TEST_STRING);
    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, ConstructionWithCString) // NOLINT
{
    TelemetryValue telemetryValue(TEST_CSTRING);
    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, ConstructionWithInt) // NOLINT
{
    TelemetryValue telemetryValue(TEST_INTEGER);
    ASSERT_EQ(TelemetryValue::ValueType::integer_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_INTEGER, telemetryValue.getInteger());
}

TEST(TestTelemetryValueImpl, ConstructionWithBool) // NOLINT
{
    TelemetryValue telemetryValue(TEST_BOOL);
    ASSERT_EQ(TelemetryValue::ValueType::boolean_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_BOOL, telemetryValue.getBoolean());
}

// Integers
TEST(TestTelemetryValueImpl, IntegerValue) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_INTEGER);

    ASSERT_EQ(TelemetryValue::ValueType::integer_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_INTEGER, telemetryValue.getInteger());
}

TEST(TestTelemetryValueImpl, GetIntegerThrowsWhenSetToString) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);
    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    EXPECT_THROW(telemetryValue.getInteger(), std::invalid_argument);  // NOLINT
}

// Strings
TEST(TestTelemetryValueImpl, StringValue) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);

    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, CStringValue) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set("astring");

    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    ASSERT_EQ("astring", telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, GetStringThrowsWhenSetToInt) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_INTEGER);
    ASSERT_EQ(TelemetryValue::ValueType::integer_type, telemetryValue.getValueType());
    EXPECT_THROW(telemetryValue.getString(), std::invalid_argument);  // NOLINT
}

// Bools
TEST(TestTelemetryValueImpl, BoolValue) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_BOOL);
    ASSERT_EQ(TelemetryValue::ValueType::boolean_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_BOOL, telemetryValue.getBoolean());
}

TEST(TestTelemetryValueImpl, GetBoolThrowsWhenSetToString) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);
    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    EXPECT_THROW(telemetryValue.getBoolean(), std::invalid_argument);  // NOLINT
}

TEST(TestTelemetryValueImpl, GetBoolThrowsWhenNothingSet) // NOLINT
{
    TelemetryValue telemetryValue;
    EXPECT_THROW(telemetryValue.getBoolean(), std::invalid_argument);  // NOLINT
}
// Operators
TEST(TestTelemetryValueImpl, EqualityMatchingItems) // NOLINT
{
    TelemetryValue telemetryValue1(TEST_INTEGER);
    TelemetryValue telemetryValue2(TEST_INTEGER);
    ASSERT_TRUE(telemetryValue1 == telemetryValue2);
}

TEST(TestTelemetryValueImpl, EqualityDifferingItems) // NOLINT
{
    TelemetryValue telemetryValue1(TEST_INTEGER);
    TelemetryValue telemetryValue2(999);
    ASSERT_TRUE(telemetryValue1 != telemetryValue2);
}

