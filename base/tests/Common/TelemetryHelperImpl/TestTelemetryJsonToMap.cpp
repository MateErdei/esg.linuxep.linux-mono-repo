///******************************************************************************************************
///
/// Copyright 2019, Sophos Limited.  All rights reserved.
///
///******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryJsonToMap.h>
#include <include/gtest/gtest.h>

using namespace Common::Telemetry;

TEST(TestTelemetryJsonToMap, InvalidJsonInputShouldThrow) // NOLINT
{
    std::string invalidJson("{this is not a valid json string");
    EXPECT_THROW(flatJsonToMap(invalidJson), std::runtime_error);
}

TEST(TestTelemetryJsonToMap, JsonInputIsReadAndNestedStructureAndArraysAreIgnored) // NOLINT
{
    std::string testJson = R"({"unsigned integer":1,"signed integer":-1, "float" : 3.142, "string":"ThisIsAString", "boolean": true, "NestedStructure":{"nested1":4294967200,"nested2":3.2,"nested3":-4},"Array":["TestValue",true,{"nested1":4294967200,"nested2":3.2,"nested3":-4}]})";
    auto map = flatJsonToMap(testJson);
    TelemetryValue unsignedInteger(1UL);
    TelemetryValue signedInteger(-1L);
    TelemetryValue doubleNum(3.142);
    TelemetryValue stringValue("ThisIsAString");
    TelemetryValue boolean(true);

    ASSERT_EQ(map.size(), 5);
    EXPECT_EQ( map["unsigned integer"], unsignedInteger);
    EXPECT_EQ( map["signed integer"], signedInteger);
    EXPECT_EQ( map["float"], doubleNum);
    EXPECT_EQ( map["string"], stringValue);
    EXPECT_EQ( map["boolean"], boolean);
}
