// Copyright 2022, Sophos Limited. All rights reserved.

// Tests for the various helper functions within the SafeStoreWrapper.
// Most of the wrapper code calls into the safestore library so all those tests cannot be done as unit tests, they
// are covered in SafeStoreTapTest.cpp which produces a gtest binary that is run at TAP test time by robot.

#include "../common/LogInitializedTests.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include <gtest/gtest.h>

using namespace safestore::SafeStoreWrapper;

class SafeStoreWrapperTests : public LogInitializedTests
{
};

TEST_F(SafeStoreWrapperTests, threatIdFromUuidString)
{
    auto ssThreatIdStruct = safeStoreIdFromUuidString("00010203-0405-0607-0809-0a0b0c0d0e0f");
    ASSERT_TRUE(ssThreatIdStruct.has_value());

    EXPECT_EQ(ssThreatIdStruct.value().Data1, 0x00010203);
    EXPECT_EQ(ssThreatIdStruct.value().Data2, 0x0405);
    EXPECT_EQ(ssThreatIdStruct.value().Data3, 0x0607);
    for (size_t i = 0; i < 8; ++i)
    {
        EXPECT_EQ(ssThreatIdStruct.value().Data4[i], 8 + i);
    }
}

TEST_F(SafeStoreWrapperTests, threatIdFromUuidStringFailsOnInvalidUuid)
{
    EXPECT_FALSE(safeStoreIdFromUuidString("00010203-0405-0607-0809").has_value());
}

TEST_F(SafeStoreWrapperTests, uuidStringFromSafeStoreId)
{
    SafeStore_Id_t safeStoreId { 0x00010203, 0x0405, 0x0607, { 8, 9, 10, 11, 12, 13, 14, 15 } };
    EXPECT_EQ(uuidStringFromSafeStoreId(safeStoreId), "00010203-0405-0607-0809-0a0b0c0d0e0f");
}

TEST_F(SafeStoreWrapperTests, uuidFromSafeStoreIdDefaultDoesNotThrow)
{
    // The struct 'SafeStore_Id_t' in the safestore library does not default initialise any of its members so
    // for this test a default constructed one should not cause our code to error but will produce a garbage string.
    SafeStore_Id_t ssThreatIdStruct;
    EXPECT_NO_THROW(uuidStringFromSafeStoreId(ssThreatIdStruct));
}

TEST_F(SafeStoreWrapperTests, convertObjStatusToSafeStoreObjStatus)
{
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::ANY), SOS_ANY);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::STORED), SOS_STORED);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::QUARANTINED), SOS_QUARANTINED);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::RESTORED), SOS_RESTORED);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::RESTORED_AS), SOS_RESTORED_AS);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::RESTORE_FAILED), SOS_RESTORE_FAILED);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::UNDEFINED), SOS_UNDEFINED);
    ASSERT_EQ(convertObjStatusToSafeStoreObjStatus(ObjectStatus::LAST), SOS_LAST);
}

TEST_F(SafeStoreWrapperTests, convertObjTypeToSafeStoreObjType)
{
    ASSERT_EQ(convertObjTypeToSafeStoreObjType(ObjectType::ANY), SOT_ANY);
    ASSERT_EQ(convertObjTypeToSafeStoreObjType(ObjectType::FILE), SOT_FILE);
    ASSERT_EQ(convertObjTypeToSafeStoreObjType(ObjectType::LAST), SOT_LAST);
    ASSERT_EQ(convertObjTypeToSafeStoreObjType(ObjectType::REGVALUE), SOT_REGVALUE);
    ASSERT_EQ(convertObjTypeToSafeStoreObjType(ObjectType::REGKEY), SOT_REGKEY);
    ASSERT_EQ(convertObjTypeToSafeStoreObjType(ObjectType::UNKNOWN), SOT_UNKNOWN);
}

TEST_F(SafeStoreWrapperTests, convertToSafeStoreConfigId)
{
    ASSERT_EQ(convertToSafeStoreConfigId(ConfigOption::MAX_STORED_OBJECT_COUNT), SC_MAX_STORED_OBJECT_COUNT);
    ASSERT_EQ(convertToSafeStoreConfigId(ConfigOption::MAX_SAFESTORE_SIZE), SC_MAX_SAFESTORE_SIZE);
    ASSERT_EQ(convertToSafeStoreConfigId(ConfigOption::MAX_REG_OBJECT_COUNT), SC_MAX_REG_OBJECT_COUNT);
    ASSERT_EQ(convertToSafeStoreConfigId(ConfigOption::MAX_OBJECT_SIZE), SC_MAX_OBJECT_SIZE);
    ASSERT_EQ(convertToSafeStoreConfigId(ConfigOption::AUTO_PURGE), SC_AUTO_PURGE);
}
