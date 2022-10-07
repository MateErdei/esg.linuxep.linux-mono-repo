
#include "../common/LogInitializedTests.h"
#include "safestore/SafeStoreWrapperImpl.h"

#include <gtest/gtest.h>

class SafeStoreWrapperTests : public LogInitializedTests
{};

TEST_F(SafeStoreWrapperTests, threatIdFromStringTakesFirst16Bytes)
{
    auto ssThreatIdStruct = safestore::threatIdFromString("Tabcdefghijklmnopqrstuvwxyz");
    ASSERT_TRUE(ssThreatIdStruct.has_value());

    ASSERT_EQ((ssThreatIdStruct.value().Data1 & 0x000000ff), 'a');
    ASSERT_EQ((ssThreatIdStruct.value().Data1 & 0x0000ff00) >> 8, 'b');
    ASSERT_EQ((ssThreatIdStruct.value().Data1 & 0x00ff0000) >> 16, 'c');
    ASSERT_EQ((ssThreatIdStruct.value().Data1 & 0xff000000) >> 24, 'd');

    ASSERT_EQ((ssThreatIdStruct.value().Data2 & 0x00ff), 'e');
    ASSERT_EQ((ssThreatIdStruct.value().Data2 & 0xff00) >> 8, 'f');

    ASSERT_EQ((ssThreatIdStruct.value().Data3 & 0x00ff), 'g');
    ASSERT_EQ((ssThreatIdStruct.value().Data3 & 0xff00) >> 8, 'h');

    ASSERT_EQ(ssThreatIdStruct.value().Data4[0], 'i');
    ASSERT_EQ(ssThreatIdStruct.value().Data4[1], 'j');
    ASSERT_EQ(ssThreatIdStruct.value().Data4[2], 'k');
    ASSERT_EQ(ssThreatIdStruct.value().Data4[3], 'l');
}


TEST_F(SafeStoreWrapperTests, threatIdFromStringHandlesShortIds)
{
    auto ssThreatIdStruct = safestore::threatIdFromString("Tabcdefg");
    ASSERT_FALSE(ssThreatIdStruct.has_value());
}

TEST_F(SafeStoreWrapperTests, threatIdFromStringHandlesMalformedIds)
{
    auto ssThreatIdStruct = safestore::threatIdFromString("this is not a threat ID");
    ASSERT_FALSE(ssThreatIdStruct.has_value());
}

TEST_F(SafeStoreWrapperTests, stringFromThreatIdStruct)
{
    std::string idString = "abcdefghijklmnop"; // only 16bytes long.
    std::string threatIdPrefix = "T";
    std::string threatIdString = threatIdPrefix + idString;
    auto ssThreatIdStruct = safestore::threatIdFromString(threatIdString);
    std::string id = safestore::stringFromThreatId(ssThreatIdStruct.value());
    ASSERT_EQ(idString, id);
}