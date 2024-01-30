// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/TelemetryHelperImpl/TelemetryValue.h"
#include <gtest/gtest.h>

static const long TEST_INTEGER = 10L;
static const unsigned long TEST_UNSIGNED_INTEGER = 11UL;
static const double TEST_DOUBLE = 3.2;
static const bool TEST_BOOL = true;
static const char* TEST_CSTRING = "Test String";
static const std::string TEST_STRING = TEST_CSTRING;

using namespace Common::Telemetry;

// Construction
TEST(TestTelemetryValueImpl, Construction)
{
    TelemetryValue telemetryValue;
    ASSERT_EQ(TelemetryValue::Type::integer_type, telemetryValue.getType());
}

TEST(TestTelemetryValueImpl, ConstructionWithString)
{
    TelemetryValue telemetryValue(TEST_STRING);
    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, ConstructionWithCString)
{
    TelemetryValue telemetryValue(TEST_CSTRING);
    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, ConstructionWithInt)
{
    TelemetryValue telemetryValue(TEST_INTEGER);
    ASSERT_EQ(TelemetryValue::Type::integer_type, telemetryValue.getType());
    ASSERT_EQ(TEST_INTEGER, telemetryValue.getInteger());
}

TEST(TestTelemetryValueImpl, ConstructionWithUnsignedInt)
{
    TelemetryValue telemetryValue(TEST_UNSIGNED_INTEGER);
    ASSERT_EQ(TelemetryValue::Type::unsigned_integer_type, telemetryValue.getType());
    ASSERT_EQ(TEST_UNSIGNED_INTEGER, telemetryValue.getUnsignedInteger());
}

TEST(TestTelemetryValueImpl, ConstructionWithDouble)
{
    TelemetryValue telemetryValue(TEST_DOUBLE);
    ASSERT_EQ(TelemetryValue::Type::double_type, telemetryValue.getType());
    ASSERT_EQ(TEST_DOUBLE, telemetryValue.getDouble());
}

TEST(TestTelemetryValueImpl, ConstructionWithBool)
{
    TelemetryValue telemetryValue(TEST_BOOL);
    ASSERT_EQ(TelemetryValue::Type::boolean_type, telemetryValue.getType());
    ASSERT_EQ(TEST_BOOL, telemetryValue.getBoolean());
}

// Integers
TEST(TestTelemetryValueImpl, IntegerValue)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_INTEGER);

    ASSERT_EQ(TelemetryValue::Type::integer_type, telemetryValue.getType());
    ASSERT_EQ(TEST_INTEGER, telemetryValue.getInteger());
}

TEST(TestTelemetryValueImpl, GetIntegerThrowsWhenSetToString)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);
    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    EXPECT_THROW(telemetryValue.getInteger(), std::logic_error);
}

// Integers
TEST(TestTelemetryValueImpl, UnsignedIntegerValue)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_UNSIGNED_INTEGER);

    ASSERT_EQ(TelemetryValue::Type::unsigned_integer_type, telemetryValue.getType());
    ASSERT_EQ(TEST_UNSIGNED_INTEGER, telemetryValue.getUnsignedInteger());
}

TEST(TestTelemetryValueImpl, GetUnsignedIntegerThrowsWhenSetToString)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);
    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    EXPECT_THROW(telemetryValue.getUnsignedInteger(), std::logic_error);
}

// Strings
TEST(TestTelemetryValueImpl, StringValue)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);

    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, CStringValue)
{
    TelemetryValue telemetryValue;
    telemetryValue.set("astring");

    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    ASSERT_EQ("astring", telemetryValue.getString());
}

TEST(TestTelemetryValueImpl, GetStringThrowsWhenSetToInt)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_INTEGER);
    ASSERT_EQ(TelemetryValue::Type::integer_type, telemetryValue.getType());
    EXPECT_THROW(telemetryValue.getString(), std::logic_error);
}

// Bools
TEST(TestTelemetryValueImpl, BoolValue)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_BOOL);
    ASSERT_EQ(TelemetryValue::Type::boolean_type, telemetryValue.getType());
    ASSERT_EQ(TEST_BOOL, telemetryValue.getBoolean());
}

TEST(TestTelemetryValueImpl, GetBoolThrowsWhenSetToString)
{
    TelemetryValue telemetryValue;
    telemetryValue.set(TEST_STRING);
    ASSERT_EQ(TelemetryValue::Type::string_type, telemetryValue.getType());
    EXPECT_THROW(telemetryValue.getBoolean(), std::logic_error);
}

TEST(TestTelemetryValueImpl, GetBoolThrowsWhenNothingSet)
{
    TelemetryValue telemetryValue;
    EXPECT_THROW(telemetryValue.getBoolean(), std::logic_error);
}
// Operators
TEST(TestTelemetryValueImpl, EqualityMatchingItems)
{
    TelemetryValue telemetryValue1(TEST_INTEGER);
    TelemetryValue telemetryValue2(TEST_INTEGER);
    ASSERT_TRUE(telemetryValue1 == telemetryValue2);
}

TEST(TestTelemetryValueImpl, EqualityDifferingItems)
{
    TelemetryValue telemetryValue1(TEST_INTEGER);
    TelemetryValue telemetryValue2(999L);
    ASSERT_TRUE(telemetryValue1 != telemetryValue2);
}

TEST(TestTelemetryValueImpl, EqualitySignedUnsigned)
{
    TelemetryValue telemetryValue1(2L);
    TelemetryValue telemetryValue2(2UL);
    ASSERT_TRUE(telemetryValue1 == telemetryValue2);
}

TEST(TestTelemetryValueImpl, EqualitySignedUnsignedDifferent)
{
    TelemetryValue telemetryValue1(1L);
    TelemetryValue telemetryValue2(2UL);
    ASSERT_FALSE(telemetryValue1 == telemetryValue2);
}

TEST(TestTelemetryValueImpl, EqualitySignedUnsignedDifferentNegative)
{
    TelemetryValue telemetryValue1(-1L);
    TelemetryValue telemetryValue2(1UL);
    ASSERT_FALSE(telemetryValue1 == telemetryValue2);
}