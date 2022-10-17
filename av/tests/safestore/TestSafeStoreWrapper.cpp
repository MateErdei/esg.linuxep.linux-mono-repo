// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../common/LogInitializedTests.h"
#include "safestore/SafeStoreWrapperImpl.h"

#include <gtest/gtest.h>

class SafeStoreWrapperTests : public LogInitializedTests
{
};

TEST_F(SafeStoreWrapperTests, threatIdFromStringTakesFirst16Bytes)
{
    auto ssThreatIdStruct = safestore::safeStoreIdFromString("abcdefghijklmnop"); // 16 bytes
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
    auto ssThreatIdStruct = safestore::safeStoreIdFromString("abcdefg");
    ASSERT_FALSE(ssThreatIdStruct.has_value());
}

TEST_F(SafeStoreWrapperTests, threatIdFromStringHandlesMalformedIds)
{
    auto ssThreatIdStruct = safestore::safeStoreIdFromString("this is not a threat ID because it's too long");
    ASSERT_FALSE(ssThreatIdStruct.has_value());
}

TEST_F(SafeStoreWrapperTests, stringFromThreatIdStruct)
{
    std::string idString = "abcdefghijklmnop"; // only 16bytes long.
    auto ssThreatIdStruct = safestore::safeStoreIdFromString(idString);
    std::string id = safestore::stringFromSafeStoreId(ssThreatIdStruct.value());
    ASSERT_EQ(idString, id);
}

TEST_F(SafeStoreWrapperTests, bytesFromSafeStoreId)
{
    std::string idString = "abcdefghijklmnop";
    auto ssThreatIdStruct = safestore::safeStoreIdFromString(idString);
    auto bytes = safestore::bytesFromSafeStoreId(ssThreatIdStruct.value());
    ASSERT_EQ(bytes.size(), idString.length());
    for (size_t i = 0; i < idString.length(); ++i)
    {
        ASSERT_EQ(bytes[i], 'a' + i);
    }
}