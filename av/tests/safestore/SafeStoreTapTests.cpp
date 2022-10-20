// Copyright 2022, Sophos Limited.  All rights reserved.

// These tests cover the SafeStore wrapper and call into the real safestore library (no mocks) so these
// test are run in TAP in our robotframework suite. The tests interact heavily with the filesystem so shouldn't be
// unit tests but gtest is very well suited to the types of tests where we're calling into a library.
// See sspl-plugin-anti-virus/PostInstall/CMakeLists.txt for where this binary is archived ready for publishing.

// The tests can be executed just like normal unit tests for local dev.

#include "../common/LogInitializedTests.h"
#include "safestore/SafeStoreWrapperImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"

#include <common/ApplicationPaths.h>
#include <gtest/gtest.h>

class SafeStoreWrapperTapTests : public LogInitializedTests
{
protected:
    static void SetUpTestSuite()
    {
        std::cout << "SetUpTestSuite" << std::endl;
        auto fileSystem = Common::FileSystem::fileSystem();
        fileSystem->makedirs(m_safeStoreDbDir);
    }

    static void TearDownTestSuite()
    {
        std::cout << "TearDownTestSuite" << std::endl;
    }

    void SetUp() override
    {
        std::cout << "SetUp" << std::endl;
        auto initReturnCode = m_safeStoreWrapper->initialise(m_safeStoreDbDir, m_safeStoreDbName, m_safeStoreDbPw);
        ASSERT_EQ(initReturnCode, safestore::InitReturnCode::OK);
    }

    static void deleteDatabase()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        fileSystem->removeFilesInDirectory(m_safeStoreDbDir);
    }

    void TearDown() override
    {
        std::cout << "TearDown" << std::endl;
        deleteDatabase();
    }

    static inline const std::string m_testWorkingDir = "/tmp/SafeStoreWrapperTapTests";
    static inline const std::string m_safeStoreDbDir = Common::FileSystem::join(m_testWorkingDir, "safestore_db");
    static inline const std::string m_safeStoreDbName = "safestore.db";
    static inline const std::string m_safeStoreDbPw = "a test password";
    std::unique_ptr<safestore::ISafeStoreWrapper> m_safeStoreWrapper =
        std::make_unique<safestore::SafeStoreWrapperImpl>();
};

TEST_F(SafeStoreWrapperTapTests, initExistingDb)
{
    auto initReturnCode = m_safeStoreWrapper->initialise(m_safeStoreDbDir, m_safeStoreDbName, m_safeStoreDbPw);
    ASSERT_EQ(initReturnCode, safestore::InitReturnCode::OK);
}

TEST_F(SafeStoreWrapperTapTests, readDefaultConfigOptions)
{
    auto autoPurge = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::AUTO_PURGE);
    ASSERT_TRUE(autoPurge.has_value());
    ASSERT_EQ(autoPurge.value(), 1);

    auto maxObjSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE);
    ASSERT_TRUE(maxObjSize.has_value());
    ASSERT_EQ(maxObjSize.value(), 107374182400);

    auto maxObjInRegistrySubtree = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT);
    ASSERT_FALSE(maxObjInRegistrySubtree.has_value());

    auto maxSafeStoreSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE);
    ASSERT_TRUE(maxSafeStoreSize.has_value());
    ASSERT_EQ(maxSafeStoreSize.value(), 214748364800);

    auto maxObjCount = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT);
    ASSERT_TRUE(maxObjCount.has_value());
    ASSERT_EQ(maxObjCount.value(), 2000);
}

TEST_F(SafeStoreWrapperTapTests, writeAndThenRreadBackConfigOptions)
{
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::AUTO_PURGE, false));
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE, 100000000000));

    // This currently fails - defect or we don't care as it's windows only?
    // ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT, 100));

    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE, 200000000000));
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT, 5000));

    auto autoPurge = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::AUTO_PURGE);
    ASSERT_TRUE(autoPurge.has_value());
    ASSERT_EQ(autoPurge.value(), 0);

    auto maxObjSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE);
    ASSERT_TRUE(maxObjSize.has_value());
    ASSERT_EQ(maxObjSize.value(), 100000000000);

    auto maxSafeStoreSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE);
    ASSERT_TRUE(maxSafeStoreSize.has_value());
    ASSERT_EQ(maxSafeStoreSize.value(), 200000000000);

    auto maxObjCount = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT);
    ASSERT_TRUE(maxObjCount.has_value());
    ASSERT_EQ(maxObjCount.value(), 5000);
}

TEST_F(SafeStoreWrapperTapTests, quarantineThreatAndLookupDetails)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto now = std::time(0);

    // Add fake threat
    std::string fakeVirusFilePath1 = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath1, "a temp test file1");
    std::string threatId = "dummy_threat_ID1";
    std::string threatName = "threat name1";
    auto objectHandle1 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath1),
            Common::FileSystem::basename(fakeVirusFilePath1),
            threatId,
            threatName,
            *objectHandle1),
        safestore::SaveFileReturnCode::OK);

    // Find all FILE threats in SafeStore
    safestore::SafeStoreFilter filter;
    filter.objectType = safestore::ObjectType::FILE;
    filter.activeFields = { safestore::FilterField::OBJECT_TYPE };
    bool foundAnyResults = false;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        foundAnyResults = true;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), "fakevirus1");
        ASSERT_EQ(m_safeStoreWrapper->getObjectId(result).size(), 16);
        ASSERT_EQ(m_safeStoreWrapper->getObjectType(result), safestore::ObjectType::FILE);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), safestore::ObjectStatus::STORED);

        // Basic check to ensure that the store time is, at least, sensible
        auto storeTime = m_safeStoreWrapper->getObjectStoreTime(result);
        ASSERT_GE(storeTime, now);
        ASSERT_LE(storeTime, now + 10);

        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatId(result), threatId);
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
    }

    ASSERT_TRUE(foundAnyResults);
}

TEST_F(SafeStoreWrapperTapTests, quarantineMultipleThreatsAndLookupDetails)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add some fake threats
    std::string fakeVirusFilePath1 = "/tmp/fakevirus1";
    std::string threatName1 = "threat name1";
    fileSystem->writeFile(fakeVirusFilePath1, "a temp test file1");

    std::string fakeVirusFilePath2 = "/tmp/fakevirus2";
    std::string threatName2 = "threat name2";
    fileSystem->writeFile(fakeVirusFilePath2, "a temp test file2");

    // Store 1
    auto objectHandle1 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath1),
            Common::FileSystem::basename(fakeVirusFilePath1),
            "dummy_threat_ID1",
            threatName1,
            *objectHandle1),
        safestore::SaveFileReturnCode::OK);

    // Store 2
    auto objectHandle2 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath2),
            Common::FileSystem::basename(fakeVirusFilePath2),
            "dummy_threat_ID2",
            threatName2,
            *objectHandle2),
        safestore::SaveFileReturnCode::OK);

    safestore::SafeStoreFilter filter;
    filter.objectType = safestore::ObjectType::FILE;
    filter.activeFields = { safestore::FilterField::OBJECT_TYPE };

    std::set<std::string> actualObjectNames;
    std::set<std::string> actualThreatNames;
    std::set<std::string> expectedObjectNames = {
        Common::FileSystem::basename(fakeVirusFilePath1),
        Common::FileSystem::basename(fakeVirusFilePath2),
    };
    std::set<std::string> expectedThreatNames = { threatName1, threatName2 };

    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        actualObjectNames.insert(m_safeStoreWrapper->getObjectName(result));
        actualThreatNames.insert(m_safeStoreWrapper->getObjectThreatName(result));
    }
    ASSERT_EQ(expectedObjectNames, actualObjectNames);
    ASSERT_EQ(expectedThreatNames, actualThreatNames);
}

TEST_F(SafeStoreWrapperTapTests, quarantineThreatAndAddCustomData)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    std::string sha256 = "f2c91a583cfd1371a3085187aa5b2841ada3b62f5d1cc6b08bc02703ded3507a";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    std::string threatId = "dummy_threat_ID1";
    std::string threatName = "threat name1";
    auto objectHandle1 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandle1),
        safestore::SaveFileReturnCode::OK);

    // Set custom data
    ASSERT_TRUE(m_safeStoreWrapper->setObjectCustomDataString(*objectHandle1, "SHA256", sha256));

    // Find all FILE threats in SafeStore
    safestore::SafeStoreFilter filter;
    filter.objectType = safestore::ObjectType::FILE;
    filter.activeFields = { safestore::FilterField::OBJECT_TYPE };
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), "fakevirus1");
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);

        // Validate custom data saved ok
        ASSERT_EQ(m_safeStoreWrapper->getObjectCustomDataString(result, "SHA256"), sha256);

        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), safestore::ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 1);
}

TEST_F(SafeStoreWrapperTapTests, quarantineAndFinaliseThreatAndStatusChangesToQuarantined)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    std::string sha256 = "f2c91a583cfd1371a3085187aa5b2841ada3b62f5d1cc6b08bc02703ded3507a";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    std::string threatId = "dummy_threat_ID1";
    std::string threatName = "threat name1";
    auto objectHandle = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandle),
        safestore::SaveFileReturnCode::OK);

    // Finalise SafeStore object
    ASSERT_TRUE(m_safeStoreWrapper->finaliseObject(*objectHandle));

    // Set custom data
    ASSERT_TRUE(m_safeStoreWrapper->setObjectCustomDataString(*objectHandle, "SHA256", sha256));

    // Find all FILE threats in SafeStore
    safestore::SafeStoreFilter filter;
    filter.objectType = safestore::ObjectType::FILE;
    filter.activeFields = { safestore::FilterField::OBJECT_TYPE };
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), "fakevirus1");
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);

        // Validate custom data saved ok
        ASSERT_EQ(m_safeStoreWrapper->getObjectCustomDataString(result, "SHA256"), sha256);

        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), safestore::ObjectStatus::QUARANTINED);
    }
    ASSERT_EQ(resultsFound, 1);
}