// Copyright 2022, Sophos Limited. All rights reserved.

// These tests cover the SafeStore wrapper and call into the real safestore library (no mocks) so these
// test are run in TAP in our robotframework suite. The tests interact heavily with the filesystem so shouldn't be
// unit tests but gtest is very well suited to the types of tests where we're calling into a library.
// See sspl-plugin-anti-virus/PostInstall/CMakeLists.txt for where this binary is archived ready for publishing.

// The tests can be executed just like normal unit tests for local dev.

#include "../common/LogInitializedTests.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"

#include <Common/UtilityImpl/Uuid.h>
#include <common/ApplicationPaths.h>
#include <gtest/gtest.h>

using namespace safestore::SafeStoreWrapper;

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
        ASSERT_EQ(initReturnCode, InitReturnCode::OK);
    }

    static void deleteDatabase()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        fileSystem->removeFilesInDirectory(m_safeStoreDbDir);
    }

    void TearDown() override
    {
        std::cout << "TearDown" << std::endl;
        auto filesystem = Common::FileSystem::fileSystem();
        for (const auto& file : m_filesToDeleteOnTeardown)
        {
            filesystem->removeFile(file, true);
        }
        deleteDatabase();
    }

    static inline const std::string m_testWorkingDir = "/tmp/SafeStoreWrapperTapTests";
    static inline const std::string m_safeStoreDbDir = Common::FileSystem::join(m_testWorkingDir, "safestore_db");
    static inline const std::string m_safeStoreDbName = "safestore.db";
    static inline const std::string m_safeStoreDbPw = "a test password";

    std::unique_ptr<ISafeStoreWrapper> m_safeStoreWrapper = std::make_unique<SafeStoreWrapperImpl>();
    std::set<std::string> m_filesToDeleteOnTeardown;
};

TEST_F(SafeStoreWrapperTapTests, initExistingDb)
{
    auto initReturnCode = m_safeStoreWrapper->initialise(m_safeStoreDbDir, m_safeStoreDbName, m_safeStoreDbPw);
    ASSERT_EQ(initReturnCode, InitReturnCode::OK);
}

TEST_F(SafeStoreWrapperTapTests, readDefaultConfigOptions)
{
    auto autoPurge = m_safeStoreWrapper->getConfigIntValue(ConfigOption::AUTO_PURGE);
    ASSERT_TRUE(autoPurge.has_value());
    ASSERT_EQ(autoPurge.value(), 1);

    auto maxObjSize = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_OBJECT_SIZE);
    ASSERT_TRUE(maxObjSize.has_value());
    ASSERT_EQ(maxObjSize.value(), 107374182400);

    auto maxObjInRegistrySubtree = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_REG_OBJECT_COUNT);
    ASSERT_FALSE(maxObjInRegistrySubtree.has_value());

    auto maxSafeStoreSize = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_SAFESTORE_SIZE);
    ASSERT_TRUE(maxSafeStoreSize.has_value());
    ASSERT_EQ(maxSafeStoreSize.value(), 214748364800);

    auto maxObjCount = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_STORED_OBJECT_COUNT);
    ASSERT_TRUE(maxObjCount.has_value());
    ASSERT_EQ(maxObjCount.value(), 2000);
}

TEST_F(SafeStoreWrapperTapTests, writeAndThenReadBackConfigOptions)
{
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(ConfigOption::AUTO_PURGE, false));
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(ConfigOption::MAX_OBJECT_SIZE, 100000000000));

    // This currently fails but it's windows only so doesn't need to work on Linux.
    // ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(ConfigOption::MAX_REG_OBJECT_COUNT, 100));

    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(ConfigOption::MAX_SAFESTORE_SIZE, 200000000000));
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(ConfigOption::MAX_STORED_OBJECT_COUNT, 5000));

    auto autoPurge = m_safeStoreWrapper->getConfigIntValue(ConfigOption::AUTO_PURGE);
    ASSERT_TRUE(autoPurge.has_value());
    ASSERT_EQ(autoPurge.value(), 0);

    auto maxObjSize = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_OBJECT_SIZE);
    ASSERT_TRUE(maxObjSize.has_value());
    ASSERT_EQ(maxObjSize.value(), 100000000000);

    auto maxSafeStoreSize = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_SAFESTORE_SIZE);
    ASSERT_TRUE(maxSafeStoreSize.has_value());
    ASSERT_EQ(maxSafeStoreSize.value(), 200000000000);

    auto maxObjCount = m_safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_STORED_OBJECT_COUNT);
    ASSERT_TRUE(maxObjCount.has_value());
    ASSERT_EQ(maxObjCount.value(), 5000);
}

TEST_F(SafeStoreWrapperTapTests, quarantineThreatAndLookupDetails)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto now = std::time(nullptr);

    // Add fake threat
    std::string fakeVirusFilePath1 = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath1, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath1);
    std::string threatId = "00000000-0000-0000-0000-000000000000";
    std::string threatName = "threat name1";
    auto objectHandle1 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath1),
            Common::FileSystem::basename(fakeVirusFilePath1),
            threatId,
            threatName,
            *objectHandle1),
        SaveFileReturnCode::OK);

    // Find all FILE threats in SafeStore
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    bool foundAnyResults = false;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        foundAnyResults = true;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), Common::FileSystem::basename(fakeVirusFilePath1));
        ASSERT_EQ(m_safeStoreWrapper->getObjectLocation(result), Common::FileSystem::dirName(fakeVirusFilePath1));
        ASSERT_TRUE(Common::UtilityImpl::Uuid::IsValid(m_safeStoreWrapper->getObjectId(result)));
        ASSERT_EQ(m_safeStoreWrapper->getObjectType(result), ObjectType::FILE);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);

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
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath1);

    std::string fakeVirusFilePath2 = "/tmp/fakevirus2";
    std::string threatName2 = "threat name2";
    fileSystem->writeFile(fakeVirusFilePath2, "a temp test file2");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath2);

    // Store 1
    auto objectHandle1 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath1),
            Common::FileSystem::basename(fakeVirusFilePath1),
            "00000000-0000-0000-0000-000000000000",
            threatName1,
            *objectHandle1),
        SaveFileReturnCode::OK);

    // Store 2
    auto objectHandle2 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath2),
            Common::FileSystem::basename(fakeVirusFilePath2),
            "11111111-1111-1111-1111-111111111111",
            threatName2,
            *objectHandle2),
        SaveFileReturnCode::OK);

    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;

    std::set<std::string> actualObjectNames;
    std::set<std::string> actualObjectLocations;
    std::set<std::string> actualThreatNames;
    std::set<std::string> expectedObjectNames = {
        Common::FileSystem::basename(fakeVirusFilePath1),
        Common::FileSystem::basename(fakeVirusFilePath2),
    };
    std::set<std::string> expectedObjectLocations = {
        Common::FileSystem::dirName(fakeVirusFilePath1),
        Common::FileSystem::dirName(fakeVirusFilePath2),
    };
    std::set<std::string> expectedThreatNames = { threatName1, threatName2 };

    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        actualObjectNames.insert(m_safeStoreWrapper->getObjectName(result));
        actualObjectLocations.insert(m_safeStoreWrapper->getObjectLocation(result));
        actualThreatNames.insert(m_safeStoreWrapper->getObjectThreatName(result));
    }
    ASSERT_EQ(expectedObjectNames, actualObjectNames);
    ASSERT_EQ(expectedObjectLocations, actualObjectLocations);
    ASSERT_EQ(expectedThreatNames, actualThreatNames);
}

TEST_F(SafeStoreWrapperTapTests, quarantineThreatAndAddCustomData)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    std::string sha256 = "f2c91a583cfd1371a3085187aa5b2841ada3b62f5d1cc6b08bc02703ded3507a";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name1";
    std::vector<uint8_t> someBytes { 1, 2 };
    auto objectHandle1 = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandle1),
        SaveFileReturnCode::OK);

    // Set custom data
    ASSERT_TRUE(m_safeStoreWrapper->setObjectCustomDataString(*objectHandle1, "SHA256", sha256));
    ASSERT_TRUE(m_safeStoreWrapper->setObjectCustomData(*objectHandle1, "2bytes", someBytes));

    // Find all FILE threats in SafeStore
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), Common::FileSystem::basename(fakeVirusFilePath));
        ASSERT_EQ(m_safeStoreWrapper->getObjectLocation(result), Common::FileSystem::dirName(fakeVirusFilePath));
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);

        // Validate custom data saved ok
        ASSERT_EQ(m_safeStoreWrapper->getObjectCustomDataString(result, "SHA256"), sha256);
        ASSERT_EQ(m_safeStoreWrapper->getObjectCustomData(result, "2bytes"), someBytes);

        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
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
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name1";
    auto objectHandle = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandle),
        SaveFileReturnCode::OK);

    // Finalise SafeStore object
    ASSERT_TRUE(m_safeStoreWrapper->finaliseObject(*objectHandle));

    // Set custom data
    ASSERT_TRUE(m_safeStoreWrapper->setObjectCustomDataString(*objectHandle, "SHA256", sha256));

    // Find all FILE threats in SafeStore
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), Common::FileSystem::basename(fakeVirusFilePath));
        ASSERT_EQ(m_safeStoreWrapper->getObjectLocation(result), Common::FileSystem::dirName(fakeVirusFilePath));
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);

        // Validate custom data saved ok
        ASSERT_EQ(m_safeStoreWrapper->getObjectCustomDataString(result, "SHA256"), sha256);

        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::QUARANTINED);
    }
    ASSERT_EQ(resultsFound, 1);
}

TEST_F(SafeStoreWrapperTapTests, getObjectHandleAndSetCustomDataUsingIt)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    std::string sha256 = "f2c91a583cfd1371a3085187aa5b2841ada3b62f5d1cc6b08bc02703ded3507a";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name1";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    auto objectId = m_safeStoreWrapper->getObjectId(*objectHandleFromSaveFile);

    std::shared_ptr<ObjectHandleHolder> objectHandleFromGetObjHandle = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_TRUE(m_safeStoreWrapper->getObjectHandle(objectId, objectHandleFromGetObjHandle));

    // Set custom data
    ASSERT_TRUE(m_safeStoreWrapper->setObjectCustomDataString(*objectHandleFromGetObjHandle, "SHA256", sha256));

    // Find all FILE threats in SafeStore
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectName(result), Common::FileSystem::basename(fakeVirusFilePath));
        ASSERT_EQ(m_safeStoreWrapper->getObjectLocation(result), Common::FileSystem::dirName(fakeVirusFilePath));
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);

        // Validate custom data saved ok
        ASSERT_EQ(m_safeStoreWrapper->getObjectCustomDataString(result, "SHA256"), sha256);

        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 1);
}

// restoreObjectById tests

TEST_F(SafeStoreWrapperTapTests, restoreObjectByIdAndVerifyFile)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto filePermissions = Common::FileSystem::filePermissions();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/restoreObjectById_fake_threat";
    std::string contents = "a temp test file";
    fileSystem->writeFile(fakeVirusFilePath, contents);
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();

    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));
    std::string groupBefore = filePermissions->getGroupName(fakeVirusFilePath);
    std::string userBefore = filePermissions->getUserName(fakeVirusFilePath);
    auto permissionsBefore = filePermissions->getFilePermissions(fakeVirusFilePath);

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    fileSystem->removeFile(fakeVirusFilePath);
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    auto objectId = m_safeStoreWrapper->getObjectId(*objectHandleFromSaveFile);
    ASSERT_TRUE(m_safeStoreWrapper->restoreObjectById(objectId));

    // Verify the file
    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));
    ASSERT_EQ(fileSystem->readFile(fakeVirusFilePath), contents);

    std::string groupAfter = filePermissions->getGroupName(fakeVirusFilePath);
    std::string userAfter = filePermissions->getUserName(fakeVirusFilePath);
    auto permissionsAfter = filePermissions->getFilePermissions(fakeVirusFilePath);

    ASSERT_EQ(groupBefore, groupAfter);
    ASSERT_EQ(userBefore, userAfter);
    ASSERT_EQ(permissionsBefore, permissionsAfter);
}

TEST_F(SafeStoreWrapperTapTests, restoreObjectByIdHandleMissingId)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/restoreObjectById_fake_threat";
    std::string contents = "a temp test file";
    fileSystem->writeFile(fakeVirusFilePath, contents);
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();

    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    fileSystem->removeFile(fakeVirusFilePath);
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    ASSERT_FALSE(m_safeStoreWrapper->restoreObjectById("11111111-1111-1111-1111-111111111111"));

    // Verify the file
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));
}

// restoreObjectByIdToLocation tests

TEST_F(SafeStoreWrapperTapTests, restoreObjectByIdToLocationAndVerifyFile)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto filePermissions = Common::FileSystem::filePermissions();

    // Add fake threat and make a dir to use to restore a file to
    std::string fakeVirusFilePath = "/tmp/restoreObjectByIdToLocationAndVerifyFile_fake_threat";
    std::string dirToRestoreTo = "/tmp/restoreObjectByIdToLocationAndVerifyFile";
    fileSystem->makedirs(dirToRestoreTo);
    std::string expectedRestoreLocation =
        "/tmp/restoreObjectByIdToLocationAndVerifyFile/restoreObjectByIdToLocationAndVerifyFile_fake_threat";
    std::string contents = "a temp test file";
    fileSystem->writeFile(fakeVirusFilePath, contents);
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    m_filesToDeleteOnTeardown.insert(expectedRestoreLocation);
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();

    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));
    std::string groupBefore = filePermissions->getGroupName(fakeVirusFilePath);
    std::string userBefore = filePermissions->getUserName(fakeVirusFilePath);
    auto permissionsBefore = filePermissions->getFilePermissions(fakeVirusFilePath);

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    fileSystem->removeFile(fakeVirusFilePath);
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    auto objectId = m_safeStoreWrapper->getObjectId(*objectHandleFromSaveFile);
    ASSERT_TRUE(m_safeStoreWrapper->restoreObjectByIdToLocation(objectId, dirToRestoreTo));

    // Check that the file has not been restored to the original location
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    // Verify the file
    ASSERT_TRUE(fileSystem->exists(expectedRestoreLocation));
    ASSERT_EQ(fileSystem->readFile(expectedRestoreLocation), contents);

    std::string groupAfter = filePermissions->getGroupName(expectedRestoreLocation);
    std::string userAfter = filePermissions->getUserName(expectedRestoreLocation);
    auto permissionsAfter = filePermissions->getFilePermissions(expectedRestoreLocation);

    ASSERT_EQ(groupBefore, groupAfter);
    ASSERT_EQ(userBefore, userAfter);
    ASSERT_EQ(permissionsBefore, permissionsAfter);
}

TEST_F(SafeStoreWrapperTapTests, restoreObjectByIdToLocationDoesNotOverwriteExistingFile)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto filePermissions = Common::FileSystem::filePermissions();

    // Add fake threat and make a dir to use to restore a file to
    std::string fakeVirusFilePath = "/tmp/restoreObjectByIdToLocationAndVerifyFile_fake_threat";
    std::string dirToRestoreTo = "/tmp/restoreObjectByIdToLocationAndVerifyFile";
    fileSystem->makedirs(dirToRestoreTo);
    std::string expectedRestoreLocation =
        "/tmp/restoreObjectByIdToLocationAndVerifyFile/restoreObjectByIdToLocationAndVerifyFile_fake_threat";
    std::string contents1 = "a temp test file";
    std::string contents2 = "some other content";
    fileSystem->writeFile(fakeVirusFilePath, contents1);

    // Create the target location file to test that we can't restore over the top of it
    fileSystem->writeFile(expectedRestoreLocation, contents2);
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    m_filesToDeleteOnTeardown.insert(expectedRestoreLocation);
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();

    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));
    std::string groupBefore = filePermissions->getGroupName(fakeVirusFilePath);
    std::string userBefore = filePermissions->getUserName(fakeVirusFilePath);
    auto permissionsBefore = filePermissions->getFilePermissions(fakeVirusFilePath);

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);
    fileSystem->removeFile(fakeVirusFilePath);
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    auto objectId = m_safeStoreWrapper->getObjectId(*objectHandleFromSaveFile);

    // Restore should fail because there is already a file in the target location
    ASSERT_FALSE(m_safeStoreWrapper->restoreObjectByIdToLocation(objectId, dirToRestoreTo));

    // Check that the file has not been restored to the original location
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    // Verify the placeholder file has not been overwritten
    ASSERT_TRUE(fileSystem->exists(expectedRestoreLocation));
    ASSERT_EQ(fileSystem->readFile(expectedRestoreLocation), contents2);

    std::string groupAfter = filePermissions->getGroupName(expectedRestoreLocation);
    std::string userAfter = filePermissions->getUserName(expectedRestoreLocation);
    auto permissionsAfter = filePermissions->getFilePermissions(expectedRestoreLocation);

    ASSERT_EQ(groupBefore, groupAfter);
    ASSERT_EQ(userBefore, userAfter);
    ASSERT_EQ(permissionsBefore, permissionsAfter);
}

// restoreObjectsByThreatId tests

TEST_F(SafeStoreWrapperTapTests, restoreObjectsByThreatIdAndVerifyFile)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto filePermissions = Common::FileSystem::filePermissions();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/restoreObjectsByThreatIdAndVerifyFile_fake_threat";
    std::string contents = "a temp test file";
    fileSystem->writeFile(fakeVirusFilePath, contents);
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatId = "00000000-0000-0000-0000-000000000000";
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();

    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));
    std::string groupBefore = filePermissions->getGroupName(fakeVirusFilePath);
    std::string userBefore = filePermissions->getUserName(fakeVirusFilePath);
    auto permissionsBefore = filePermissions->getFilePermissions(fakeVirusFilePath);

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    fileSystem->removeFile(fakeVirusFilePath);
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));

    ASSERT_TRUE(m_safeStoreWrapper->restoreObjectsByThreatId(threatId));

    // Verify the file
    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));
    ASSERT_EQ(fileSystem->readFile(fakeVirusFilePath), contents);

    std::string groupAfter = filePermissions->getGroupName(fakeVirusFilePath);
    std::string userAfter = filePermissions->getUserName(fakeVirusFilePath);
    auto permissionsAfter = filePermissions->getFilePermissions(fakeVirusFilePath);

    ASSERT_EQ(groupBefore, groupAfter);
    ASSERT_EQ(userBefore, userAfter);
    ASSERT_EQ(permissionsBefore, permissionsAfter);
}

TEST_F(SafeStoreWrapperTapTests, restoreObjectsByThreatIdHandlesMissingThreatId)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/restoreObjectsByThreatIdAndVerifyFile_fake_threat";
    std::string contents = "a temp test file";
    fileSystem->writeFile(fakeVirusFilePath, contents);
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();

    ASSERT_TRUE(fileSystem->exists(fakeVirusFilePath));

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    fileSystem->removeFile(fakeVirusFilePath);
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));
    ASSERT_FALSE(m_safeStoreWrapper->restoreObjectsByThreatId("11111111-1111-1111-1111-111111111111"));

    // Verify the file
    ASSERT_FALSE(fileSystem->exists(fakeVirusFilePath));
}

// deleteObjectById tests

TEST_F(SafeStoreWrapperTapTests, deleteObjectByIdAndcheckItIsNoLongerInDatabase)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name1";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    auto objectId = m_safeStoreWrapper->getObjectId(*objectHandleFromSaveFile);

    // Prove that the object is in the DB before we delete it
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 1);

    // Delete the object specified by ID, this will remove it from the DB
    ASSERT_TRUE(m_safeStoreWrapper->deleteObjectById(objectId));

    // Try and find the file in SafeStore DB, it should not exist.
    resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        FAIL() << "This object should have been removed from SafeStore DB: "
               << m_safeStoreWrapper->getObjectThreatName(result);
    }
}

TEST_F(SafeStoreWrapperTapTests, deleteObjectByIdHandlesMissingId)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name1";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    // Prove that the object is in the DB before we delete it
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 1);

    ASSERT_FALSE(m_safeStoreWrapper->deleteObjectById("11111111-1111-1111-1111-111111111111"));

    // Try and find the file in SafeStore DB
    resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
    }
    ASSERT_EQ(resultsFound, 1);
}

// deleteObjectsByThreatId tests

TEST_F(SafeStoreWrapperTapTests, deleteObjectsByThreatIdAndCheckItIsNoLongerInDatabase)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatId = "00000000-0000-0000-0000-000000000000";
    std::string threatName = "threat name1";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    // Prove that the object is in the DB before we delete it
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 1);

    // Delete the object specified by ID, this will remove it from the DB
    ASSERT_TRUE(m_safeStoreWrapper->deleteObjectsByThreatId(threatId));

    // Try and find the file in SafeStore DB
    resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        FAIL() << "This object should have been removed from SafeStore DB: "
               << m_safeStoreWrapper->getObjectThreatName(result);
    }
    ASSERT_EQ(resultsFound, 0);
}

TEST_F(SafeStoreWrapperTapTests, deleteObjectsByThreatIdDeletesAllFilesWithSameThreatId)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatId = "00000000-0000-0000-0000-000000000000";
    std::string threatName = "threat name";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    // Prove that the objects are in the DB before we delete it
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 2);

    // Delete the object specified by ID, this will remove it from the DB
    ASSERT_TRUE(m_safeStoreWrapper->deleteObjectsByThreatId(threatId));

    // Try and find the file in SafeStore DB
    resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        FAIL() << "This object should have been removed from SafeStore DB: "
               << m_safeStoreWrapper->getObjectThreatName(result);
    }
    ASSERT_EQ(resultsFound, 0);
}

TEST_F(SafeStoreWrapperTapTests, deleteObjectsByThreatIdHandlesMissingId)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatName = "threat name1";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            "00000000-0000-0000-0000-000000000000",
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    // Prove that the object is in the DB before we delete it
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    int resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
        ASSERT_EQ(m_safeStoreWrapper->getObjectStatus(result), ObjectStatus::STORED);
    }
    ASSERT_EQ(resultsFound, 1);

    // Delete the object specified by ID, this will remove it from the DB
    ASSERT_FALSE(m_safeStoreWrapper->deleteObjectsByThreatId("11111111-1111-1111-1111-111111111111"));

    // Try and find the file in SafeStore DB
    resultsFound = 0;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        ASSERT_EQ(m_safeStoreWrapper->getObjectThreatName(result), threatName);
    }
    ASSERT_EQ(resultsFound, 1);
}

TEST_F(SafeStoreWrapperTapTests, objectHandlesThatAreInvalidatedDueToRemovalDoNotThrowOrCrashWhenUsed)
{
    auto fileSystem = Common::FileSystem::fileSystem();

    // Add fake threat
    std::string fakeVirusFilePath = "/tmp/fakevirus1";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file1");
    m_filesToDeleteOnTeardown.insert(fakeVirusFilePath);
    std::string threatId = "00000000-0000-0000-0000-000000000000";
    std::string threatName = "threat name1";
    auto objectHandleFromSaveFile = m_safeStoreWrapper->createObjectHandleHolder();
    ASSERT_EQ(
        m_safeStoreWrapper->saveFile(
            Common::FileSystem::dirName(fakeVirusFilePath),
            Common::FileSystem::basename(fakeVirusFilePath),
            threatId,
            threatName,
            *objectHandleFromSaveFile),
        SaveFileReturnCode::OK);

    ASSERT_EQ(m_safeStoreWrapper->getObjectName(*objectHandleFromSaveFile), "fakevirus1");

    // Delete the object specified by ID, this will remove it from the DB
    ASSERT_TRUE(m_safeStoreWrapper->deleteObjectsByThreatId(threatId));

    // Threat should no longer be in the database
    int resultsFound = 0;
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    for (auto& result : m_safeStoreWrapper->find(filter))
    {
        ++resultsFound;
        FAIL() << "This object should have been removed from SafeStore DB: "
               << m_safeStoreWrapper->getObjectThreatName(result);
    }
    ASSERT_EQ(resultsFound, 0);

    // This handle is now invalid but calling a function on it should not throw or crash.
    ASSERT_NO_THROW(m_safeStoreWrapper->getObjectName(*objectHandleFromSaveFile));
}