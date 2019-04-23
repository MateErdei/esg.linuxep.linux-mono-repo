///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryValue.h>

#include <include/gtest/gtest.h>

const int TEST_INTEGER = 10;
const unsigned int TEST_UNSIGNED_INTEGER = 11U;
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

TEST(TestTelemetryValueImpl, ConstructionWithUnsignedInt) // NOLINT
{
    TelemetryValue telemetryValue(TEST_UNSIGNED_INTEGER);
    ASSERT_EQ(TelemetryValue::ValueType::unsigned_integer_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_UNSIGNED_INTEGER, telemetryValue.getUnsignedInteger());
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
    EXPECT_THROW(telemetryValue.getInteger(), std::logic_error);  // NOLINT
}

// Integers
TEST(TestTelemetryValueImpl, UnsignedIntegerValue) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_UNSIGNED_INTEGER);

    ASSERT_EQ(TelemetryValue::ValueType::unsigned_integer_type, telemetryValue.getValueType());
    ASSERT_EQ(TEST_UNSIGNED_INTEGER, telemetryValue.getUnsignedInteger());
}

TEST(TestTelemetryValueImpl, GetUnsignedIntegerThrowsWhenSetToString) // NOLINT
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);
    ASSERT_EQ(TelemetryValue::ValueType::string_type, telemetryValue.getValueType());
    EXPECT_THROW(telemetryValue.getUnsignedInteger(), std::logic_error);  // NOLINT
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
    EXPECT_THROW(telemetryValue.getString(), std::logic_error);  // NOLINT
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
    EXPECT_THROW(telemetryValue.getBoolean(), std::logic_error);  // NOLINT
}

TEST(TestTelemetryValueImpl, GetBoolThrowsWhenNothingSet) // NOLINT
{
    TelemetryValue telemetryValue;
    EXPECT_THROW(telemetryValue.getBoolean(), std::logic_error);  // NOLINT
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

