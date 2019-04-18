///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

//#include <Common/TelemetryImpl/TelemetryValue.h>
//
//#include <include/gtest/gtest.h>
//
//const int TEST_INTEGER = 10;
//const bool TEST_BOOL = true;
//const std::string TEST_STRING = "Test String";  // NOLINT
//
//// Construction
//TEST(TestTelemetryValueImpl, Construction) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    ASSERT_EQ(NodeType::value, telemetryValue.getType());
//}
//
//TEST(TestTelemetryValueImpl, ConstructionWithString) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue("key", TEST_STRING);
//    ASSERT_EQ(NodeType::value, telemetryValue.getType());
//    ASSERT_EQ(Common::Telemetry::ValueType::string_type, telemetryValue.getValueType());
//    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
//}
//
//TEST(TestTelemetryValueImpl, ConstructionWithInt) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue("key", TEST_INTEGER);
//    ASSERT_EQ(NodeType::value, telemetryValue.getType());
//    ASSERT_EQ(Common::Telemetry::ValueType::integer_type, telemetryValue.getValueType());
//    ASSERT_EQ(TEST_INTEGER, telemetryValue.getInteger());
//}
//
//TEST(TestTelemetryValueImpl, ConstructionWithBool) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue("key", TEST_BOOL);
//    ASSERT_EQ(NodeType::value, telemetryValue.getType());
//    ASSERT_EQ(Common::Telemetry::ValueType::boolean_type, telemetryValue.getValueType());
//    ASSERT_EQ(TEST_BOOL, telemetryValue.getBoolean());
//}
//
//// Integers
//TEST(TestTelemetryValueImpl, IntegerValue) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set(TEST_INTEGER);
//
//    ASSERT_EQ(Common::Telemetry::ValueType::integer_type, telemetryValue.getValueType());
//    ASSERT_EQ(TEST_INTEGER, telemetryValue.getInteger());
//}
//
//TEST(TestTelemetryValueImpl, GetIntegerThrowsWhenSetToString) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set(TEST_STRING);
//    ASSERT_EQ(Common::Telemetry::ValueType::string_type, telemetryValue.getValueType());
//    EXPECT_THROW(telemetryValue.getInteger(), std::bad_variant_access);  // NOLINT
//}
//
//// Strings
//TEST(TestTelemetryValueImpl, StringValue) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set(TEST_STRING);
//
//    ASSERT_EQ(Common::Telemetry::ValueType::string_type, telemetryValue.getValueType());
//    ASSERT_EQ(TEST_STRING, telemetryValue.getString());
//}
//
//TEST(TestTelemetryValueImpl, CStringValue) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set("astring");
//
//    ASSERT_EQ(Common::Telemetry::ValueType::string_type, telemetryValue.getValueType());
//    ASSERT_EQ("astring", telemetryValue.getString());
//}
//
//TEST(TestTelemetryValueImpl, GetStringThrowsWhenSetToInt) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set(TEST_INTEGER);
//    ASSERT_EQ(Common::Telemetry::ValueType::integer_type, telemetryValue.getValueType());
//    EXPECT_THROW(telemetryValue.getString(), std::bad_variant_access);  // NOLINT
//}
//
//// Bools
//TEST(TestTelemetryValueImpl, BoolValue) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set(TEST_BOOL);
//    ASSERT_EQ(Common::Telemetry::ValueType::boolean_type, telemetryValue.getValueType());
//    ASSERT_EQ(TEST_BOOL, telemetryValue.getBoolean());
//}
//
//TEST(TestTelemetryValueImpl, GetBoolThrowsWhenSetToString) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    telemetryValue.set(TEST_STRING);
//    ASSERT_EQ(Common::Telemetry::ValueType::string_type, telemetryValue.getValueType());
//    EXPECT_THROW(telemetryValue.getBoolean(), std::bad_variant_access);  // NOLINT
//}
//
//TEST(TestTelemetryValueImpl, GetBoolThrowsWhenNothingSet) // NOLINT
//{
//    Common::Telemetry::TelemetryValue telemetryValue;
//    EXPECT_THROW(telemetryValue.getBoolean(), std::bad_variant_access);  // NOLINT
//}
