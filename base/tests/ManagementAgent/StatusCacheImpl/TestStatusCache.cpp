/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

class TestStatusCache : public ::testing::Test
{
public:
    TestStatusCache() : m_mockFileSystem(nullptr) {}

    void SetUp() override
    {
        MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
        MockedApplicationPathManager& mock(*mockAppManager);
        ON_CALL(mock, getManagementAgentStatusCacheFilePath()).WillByDefault(Return(m_statusCachePath));
        Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));
        m_mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(m_mockFileSystem);
        Tests::replaceFileSystem(std::move(mockIFileSystemPtr));
    }

    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Tests::restoreFileSystem();
    }

    StrictMock<MockFileSystem>* m_mockFileSystem;
    std::string m_statusCachePath = "/tmp";

private:
    TestLogging::TestConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestStatusCache, testConstruction) // NOLINT
{
    EXPECT_NO_THROW(ManagementAgent::StatusCacheImpl::StatusCache cache;); // NOLINT
}

TEST_F(TestStatusCache, canAddFirstStatus) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("F"), contents("A");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkSameStatusXmlForDifferentAppIdsIsAccepted) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId1("F"), appId2("F2"), contents("A");
    std::string fullPath1 = Common::FileSystem::join(m_statusCachePath, appId1) + ".xml";
    std::string fullPath2 = Common::FileSystem::join(m_statusCachePath, appId2) + ".xml";

    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath1, contents));
    bool v = cache.statusChanged(appId1, contents);
    EXPECT_TRUE(v);

    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath2, contents));
    v = cache.statusChanged(appId2, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkDifferentStatusXmlForSameAppIdIsAccepted) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId("F"), contents1("A"), contents2("A2");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents1));
    bool v = cache.statusChanged(appId, contents1);
    EXPECT_TRUE(v);

    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents2));
    v = cache.statusChanged(appId, contents2);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkSameStatusSameAppIdIsRejected) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId("F"), contents("A");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);

    v = cache.statusChanged(appId, contents);
    EXPECT_FALSE(v); // Don't send repeated statuses
}

TEST_F(TestStatusCache, checkEmptyStatusIsAccepted) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId("APPID");
    std::string contents; // Empty string
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkEmptyAppIDIsAccepted) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId; // Empty string
    std::string contents("StatusXML");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId + ".xml");
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, loadStatusCache_WithEmptyCacheDoesNotThrow) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::vector<std::string> fileNames;
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_NO_THROW(cache.loadCacheFromDisk()); // NOLINT
}

TEST_F(TestStatusCache, loadStatusCache_WithOneCachedFileLoadsSuccessfully) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("APPID");
    std::string contents("StatusWithoutTimeStamp");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId + ".xml");
    std::vector<std::string> fileNames = { fullPath };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath)).WillOnce(Return(contents));
    EXPECT_NO_THROW(cache.loadCacheFromDisk()); // NOLINT
    bool v = cache.statusChanged(appId, contents);
    EXPECT_FALSE(v);
}

TEST_F(TestStatusCache, loadStatusCache_WithMultipleCachedFileLoadsSuccessfully) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("APPID"), contents("StatusWithoutTimeStamp");
    std::string fullPath1 = Common::FileSystem::join(m_statusCachePath, appId) + "1.xml";
    std::string fullPath2 = Common::FileSystem::join(m_statusCachePath, appId) + "2.xml";
    std::string fullPath3 = Common::FileSystem::join(m_statusCachePath, appId) + "3.xml";
    std::string fullPath4 = Common::FileSystem::join(m_statusCachePath, appId) + "4.xml";
    std::string fullPath5 = Common::FileSystem::join(m_statusCachePath, appId) + "5.xml";
    std::vector<std::string> fileNames = { fullPath1, fullPath2, fullPath3, fullPath4 };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath1)).WillOnce(Return(contents));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath2)).WillOnce(Return(contents));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath3)).WillOnce(Return(contents));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath4)).WillOnce(Return(contents));
    EXPECT_NO_THROW(cache.loadCacheFromDisk()); // NOLINT

    EXPECT_FALSE(cache.statusChanged(appId + "1", contents));
    EXPECT_FALSE(cache.statusChanged(appId + "2", contents));
    EXPECT_FALSE(cache.statusChanged(appId + "3", contents));
    EXPECT_FALSE(cache.statusChanged(appId + "4", contents));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath5, contents));
    EXPECT_TRUE(cache.statusChanged(appId + "5", contents));
}

TEST_F(TestStatusCache, loadStatusCache_CanAppendNewAppIDStatusToCacheAfterFileLoadsSuccessfully) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId1("APPID"), appId2("APPID2"), contents("StatusWithoutTimeStamp");
    std::string fullPath1 = Common::FileSystem::join(m_statusCachePath, appId1) + ".xml";
    std::string fullPath2 = Common::FileSystem::join(m_statusCachePath, appId2) + ".xml";
    std::vector<std::string> fileNames = { fullPath1 };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath1)).WillOnce(Return(contents));
    EXPECT_NO_THROW(cache.loadCacheFromDisk()); // NOLINT
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath2, contents));
    bool v = cache.statusChanged(appId2, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, loadStatusCache_CanUpdateAppIDWithNewStatusToCacheAfterFileLoadsSuccessfully) // NOLINT
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("APPID"), contents1("StatusWithoutTimeStamp1"), contents2("StatusWithoutTimeStamp2");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    std::vector<std::string> fileNames = { fullPath };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath)).WillOnce(Return(contents1));
    EXPECT_NO_THROW(cache.loadCacheFromDisk()); // NOLINT
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents2));
    bool v = cache.statusChanged(appId, contents2);
    EXPECT_TRUE(v);
}