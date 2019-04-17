///******************************************************************************************************
//
//Copyright 2019, Sophos Limited.  All rights reserved.
//
//******************************************************************************************************/
//
//#include <Common/TelemetryImpl/TelemetryDictionary.h>
//
//#include <include/gtest/gtest.h>
//
//TEST(TestTelemetryDictionaryImpl, Construction) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    ASSERT_EQ(NodeType::dict, telemetryDictionary.getType());
//}
//
//TEST(TestTelemetryDictionaryImpl, SetValue) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    ASSERT_EQ(NodeType::dict, telemetryDictionary.getType());
//
//    telemetryDictionary.set("key", std::make_shared<Common::Telemetry::TelemetryValue>("someString"));
//    auto node = telemetryDictionary.getNode("key");
//    auto value = std::dynamic_pointer_cast<Common::Telemetry::TelemetryValue>(node);
//    ASSERT_NE(nullptr, value);
//    ASSERT_EQ("someString", value->getString());
//}
//
//TEST(TestTelemetryDictionaryImpl, SetEmptyDictionary) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    ASSERT_EQ(NodeType::dict, telemetryDictionary.getType());
//
//    telemetryDictionary.set("key", std::make_shared<Common::Telemetry::TelemetryDictionary>());
//    auto node = telemetryDictionary.getNode("key");
//    auto dict = std::dynamic_pointer_cast<Common::Telemetry::TelemetryDictionary>(node);
//    ASSERT_NE(nullptr, dict);
//    ASSERT_TRUE(telemetryDictionary.keyExists("key"));
//}
//
//TEST(TestTelemetryDictionaryImpl, SetDictionary) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    ASSERT_EQ(NodeType::dict, telemetryDictionary.getType());
//
//    auto nestedDict = std::make_shared<Common::Telemetry::TelemetryDictionary>();
//    nestedDict->set("nestedKey1", std::make_shared<Common::Telemetry::TelemetryValue>("value1"));
//    nestedDict->set("nestedKey2", std::make_shared<Common::Telemetry::TelemetryValue>(true));
//
//    telemetryDictionary.set("key", nestedDict);
//    auto node = telemetryDictionary.getNode("key");
//    auto dict = std::dynamic_pointer_cast<Common::Telemetry::TelemetryDictionary>(node);
//    ASSERT_NE(nullptr, dict);
//    ASSERT_TRUE(telemetryDictionary.keyExists("key"));
//
//    auto retrievedNestedDict = std::dynamic_pointer_cast<Common::Telemetry::TelemetryDictionary>(telemetryDictionary.getNode("key"));
//    ASSERT_NE(nullptr, retrievedNestedDict);
//    ASSERT_TRUE(retrievedNestedDict->keyExists("nestedKey1"));
//    ASSERT_TRUE(retrievedNestedDict->keyExists("nestedKey2"));
//    ASSERT_NE(nullptr, retrievedNestedDict->getNode("nestedKey1"));
//    ASSERT_NE(nullptr, retrievedNestedDict->getNode("nestedKey2"));
//}
//
//TEST(TestTelemetryDictionaryImpl, ClearEmptyDictionary) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    ASSERT_NO_THROW(telemetryDictionary.clear());  // NOLINT
//}
//
//TEST(TestTelemetryDictionaryImpl, ClearDictionary) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    auto nestedDict = std::make_shared<Common::Telemetry::TelemetryDictionary>();
//    nestedDict->set("nestedKey1", std::make_shared<Common::Telemetry::TelemetryValue>("value1"));
//    nestedDict->set("nestedKey2", std::make_shared<Common::Telemetry::TelemetryValue>(true));
//    telemetryDictionary.set("key", nestedDict);
//
//    ASSERT_NO_THROW(telemetryDictionary.clear());  // NOLINT
//    ASSERT_FALSE(telemetryDictionary.keyExists("key"));
//}
//
//TEST(TestTelemetryDictionaryImpl, KeyExistsWithValue) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    telemetryDictionary.set("key", std::make_shared<Common::Telemetry::TelemetryValue>("someString"));
//    auto node = telemetryDictionary.getNode("key");
//
//    ASSERT_TRUE(telemetryDictionary.keyExists("key"));
//}
//
//TEST(TestTelemetryDictionaryImpl, KeyExistsWithDictionary) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    auto nestedDict = std::make_shared<Common::Telemetry::TelemetryDictionary>();
//    nestedDict->set("nestedKey1", std::make_shared<Common::Telemetry::TelemetryValue>("value1"));
//    nestedDict->set("nestedKey2", std::make_shared<Common::Telemetry::TelemetryValue>(true));
//    telemetryDictionary.set("key", nestedDict);
//
//    ASSERT_TRUE(telemetryDictionary.keyExists("key"));
//}
//
//TEST(TestTelemetryDictionaryImpl, KeyExistsNoMatchingKey) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    auto nestedDict = std::make_shared<Common::Telemetry::TelemetryDictionary>();
//    nestedDict->set("nestedKey1", std::make_shared<Common::Telemetry::TelemetryValue>("value1"));
//    nestedDict->set("nestedKey2", std::make_shared<Common::Telemetry::TelemetryValue>(true));
//    telemetryDictionary.set("key", nestedDict);
//
//    ASSERT_FALSE(telemetryDictionary.keyExists("bad key"));
//}
//
//TEST(TestTelemetryDictionaryImpl, GetExistingValueFromValueNode) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    telemetryDictionary.set("key", std::make_shared<Common::Telemetry::TelemetryValue>(2));
//    auto valueNode = std::dynamic_pointer_cast<Common::Telemetry::TelemetryValue>(telemetryDictionary.getNode("key"));
//    ASSERT_EQ(2, valueNode->getInteger());
//}
//
//TEST(TestTelemetryDictionaryImpl, GetExistingValueFromDictionaryNode) // NOLINT
//{
//    // Top level dictionary
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//
//    // Dictionary to insert into top level dictionary with a value node
//    auto dict = std::make_shared<Common::Telemetry::TelemetryDictionary>();
//    dict->set("value", std::make_shared<Common::Telemetry::TelemetryValue>(2));
//    telemetryDictionary.set("dictionary", dict);
//
//    auto dictNode = std::dynamic_pointer_cast<Common::Telemetry::TelemetryDictionary>(telemetryDictionary.getNode("dictionary"));
//    auto valueNode = std::dynamic_pointer_cast<Common::Telemetry::TelemetryValue>(dictNode->getNode("value"));
//    ASSERT_EQ(2, valueNode->getInteger());
//}
//
//TEST(TestTelemetryDictionaryImpl, ExpectThrowOnGetOfNonExistentValue) // NOLINT
//{
//    Common::Telemetry::TelemetryDictionary telemetryDictionary;
//    EXPECT_THROW(telemetryDictionary.getNode("does not exist"), std::out_of_range); // NOLINT
//}
//
