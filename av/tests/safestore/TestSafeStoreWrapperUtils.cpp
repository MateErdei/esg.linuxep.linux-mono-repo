// Copyright 2022, Sophos Limited.  All rights reserved.

// Tests for the various helper functions within the SafeStoreWrapper.
// Most of the wrapper code calls into the safestore library so all those tests cannot be done as unit tests, they
// are covered in SafeStoreTapTest.cpp which produces a gtest binary that is run at TAP test time by robot.

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

TEST_F(SafeStoreWrapperTests, stringFromSafeStoreId)
{
    std::string idString = "abcdefghijklmnop"; // only 16bytes long.
    auto ssThreatIdStruct = safestore::safeStoreIdFromString(idString);
    std::string id = safestore::stringFromSafeStoreId(ssThreatIdStruct.value());
    ASSERT_EQ(idString, id);
}

TEST_F(SafeStoreWrapperTests, stringFromSafeStoreIdDefaultDoesNotThrow)
{
    // The struct 'SafeStore_Id_t' in the safestore library does not default initialise any of its members so
    // for this test a default constructed one should not cause our code to error but will produce a garbage string.
    SafeStore_Id_t ssThreatIdStruct;
    std::string id;
    EXPECT_NO_THROW(id = safestore::stringFromSafeStoreId(ssThreatIdStruct));
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

TEST_F(SafeStoreWrapperTests, convertObjStatusToSafeStoreObjStatus)
{
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::ANY), SOS_ANY);
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::STORED), SOS_STORED);
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::QUARANTINED), SOS_QUARANTINED);
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::RESTORED), SOS_RESTORED);
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::RESTORED_AS), SOS_RESTORED_AS);
    ASSERT_EQ(
        safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::RESTORE_FAILED), SOS_RESTORE_FAILED);
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::UNDEFINED), SOS_UNDEFINED);
    ASSERT_EQ(safestore::convertObjStatusToSafeStoreObjStatus(safestore::ObjectStatus::LAST), SOS_LAST);
}

TEST_F(SafeStoreWrapperTests, convertObjTypeToSafeStoreObjType)
{
    ASSERT_EQ(safestore::convertObjTypeToSafeStoreObjType(safestore::ObjectType::ANY), SOT_ANY);
    ASSERT_EQ(safestore::convertObjTypeToSafeStoreObjType(safestore::ObjectType::FILE), SOT_FILE);
    ASSERT_EQ(safestore::convertObjTypeToSafeStoreObjType(safestore::ObjectType::LAST), SOT_LAST);
    ASSERT_EQ(safestore::convertObjTypeToSafeStoreObjType(safestore::ObjectType::REGVALUE), SOT_REGVALUE);
    ASSERT_EQ(safestore::convertObjTypeToSafeStoreObjType(safestore::ObjectType::REGKEY), SOT_REGKEY);
    ASSERT_EQ(safestore::convertObjTypeToSafeStoreObjType(safestore::ObjectType::UNKNOWN), SOT_UNKNOWN);
}

TEST_F(SafeStoreWrapperTests, convertToSafeStoreConfigId)
{
    ASSERT_EQ(
        safestore::convertToSafeStoreConfigId(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT),
        SC_MAX_STORED_OBJECT_COUNT);
    ASSERT_EQ(
        safestore::convertToSafeStoreConfigId(safestore::ConfigOption::MAX_SAFESTORE_SIZE), SC_MAX_SAFESTORE_SIZE);
    ASSERT_EQ(
        safestore::convertToSafeStoreConfigId(safestore::ConfigOption::MAX_REG_OBJECT_COUNT), SC_MAX_REG_OBJECT_COUNT);
    ASSERT_EQ(safestore::convertToSafeStoreConfigId(safestore::ConfigOption::MAX_OBJECT_SIZE), SC_MAX_OBJECT_SIZE);
    ASSERT_EQ(safestore::convertToSafeStoreConfigId(safestore::ConfigOption::AUTO_PURGE), SC_AUTO_PURGE);
}
