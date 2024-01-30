// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/StatusCacheImpl/StatusCache.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
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
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystem));        
    }

    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }

    StrictMock<MockFileSystem>* m_mockFileSystem;
    std::string m_statusCachePath = "/tmp";

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    Tests::ScopedReplaceFileSystem m_replacer; 

};

TEST_F(TestStatusCache, testConstruction)
{
    EXPECT_NO_THROW(ManagementAgent::StatusCacheImpl::StatusCache cache);
}

TEST_F(TestStatusCache, canAddFirstStatus)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("F"), contents("A");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkSameStatusXmlForDifferentAppIdsIsAccepted)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId1("F"), appId2("F2"), contents("A");
    std::string fullPath1 = Common::FileSystem::join(m_statusCachePath, appId1) + ".xml";
    std::string fullPath2 = Common::FileSystem::join(m_statusCachePath, appId2) + ".xml";

    EXPECT_CALL(*m_mockFileSystem, exists(fullPath1)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath1, contents));
    bool v = cache.statusChanged(appId1, contents);
    EXPECT_TRUE(v);

    EXPECT_CALL(*m_mockFileSystem, exists(fullPath2)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath2, contents));
    v = cache.statusChanged(appId2, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkDifferentStatusXmlForSameAppIdIsAccepted)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId("F"), contents1("A"), contents2("A2");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents1));
    bool v = cache.statusChanged(appId, contents1);
    EXPECT_TRUE(v);

    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents2));
    v = cache.statusChanged(appId, contents2);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkSameStatusSameAppIdIsRejected)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId("F"), contents("A");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);

    v = cache.statusChanged(appId, contents);
    EXPECT_FALSE(v); // Don't send repeated statuses
}

TEST_F(TestStatusCache, checkEmptyStatusIsAccepted)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId("APPID");
    std::string contents; // Empty string
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, checkEmptyAppIDIsAccepted)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::string appId; // Empty string
    std::string contents("StatusXML");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId + ".xml");
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents));
    bool v = cache.statusChanged(appId, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, loadStatusCache_WithEmptyCacheDoesNotThrow)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;

    std::vector<std::string> fileNames;
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_NO_THROW(cache.loadCacheFromDisk());
}

TEST_F(TestStatusCache, loadStatusCache_WithOneCachedFileLoadsSuccessfully)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("APPID");
    std::string contents("StatusWithoutTimeStamp");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId + ".xml");
    std::vector<std::string> fileNames = { fullPath };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath)).WillOnce(Return(contents));
    EXPECT_NO_THROW(cache.loadCacheFromDisk());
    bool v = cache.statusChanged(appId, contents);
    EXPECT_FALSE(v);
}

TEST_F(TestStatusCache, loadStatusCache_WithMultipleCachedFileLoadsSuccessfully)
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
    EXPECT_NO_THROW(cache.loadCacheFromDisk());

    EXPECT_FALSE(cache.statusChanged(appId + "1", contents));
    EXPECT_FALSE(cache.statusChanged(appId + "2", contents));
    EXPECT_FALSE(cache.statusChanged(appId + "3", contents));
    EXPECT_FALSE(cache.statusChanged(appId + "4", contents));
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath5)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath5, contents));
    EXPECT_TRUE(cache.statusChanged(appId + "5", contents));
}

TEST_F(TestStatusCache, loadStatusCache_CanAppendNewAppIDStatusToCacheAfterFileLoadsSuccessfully)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId1("APPID"), appId2("APPID2"), contents("StatusWithoutTimeStamp");
    std::string fullPath1 = Common::FileSystem::join(m_statusCachePath, appId1) + ".xml";
    std::string fullPath2 = Common::FileSystem::join(m_statusCachePath, appId2) + ".xml";
    std::vector<std::string> fileNames = { fullPath1 };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath1)).WillOnce(Return(contents));
    EXPECT_NO_THROW(cache.loadCacheFromDisk());
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath2)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath2, contents));
    bool v = cache.statusChanged(appId2, contents);
    EXPECT_TRUE(v);
}

TEST_F(TestStatusCache, loadStatusCache_CanUpdateAppIDWithNewStatusToCacheAfterFileLoadsSuccessfully)
{
    ManagementAgent::StatusCacheImpl::StatusCache cache;
    std::string appId("APPID"), contents1("StatusWithoutTimeStamp1"), contents2("StatusWithoutTimeStamp2");
    std::string fullPath = Common::FileSystem::join(m_statusCachePath, appId) + ".xml";
    std::vector<std::string> fileNames = { fullPath };
    EXPECT_CALL(*m_mockFileSystem, listFiles(m_statusCachePath)).WillOnce(Return(fileNames));
    EXPECT_CALL(*m_mockFileSystem, readFile(fullPath)).WillOnce(Return(contents1));
    EXPECT_NO_THROW(cache.loadCacheFromDisk());
    EXPECT_CALL(*m_mockFileSystem, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, writeFile(fullPath, contents2));
    bool v = cache.statusChanged(appId, contents2);
    EXPECT_TRUE(v);
}