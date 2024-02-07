// Copyright 2021-2024 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "DownloadReportTestBuilder.h"
#include "MockAsyncDownloaderRunner.h"
#include "MockCronSchedulerThread.h"

#include "Common/FileSystemImpl/PidLockFile.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "UpdateSchedulerImpl/UpdateSchedulerProcessor.h"
#include "tests/Common/FileSystemImpl/MockPidLockFileUtils.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockApiBaseServices.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/UtilityImpl/MockFormattedTime.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;
using namespace ::testing;

class TestUpdateSchedulerProcessorHelperMethods : public MemoryAppenderUsingTests
{
public:
    TestUpdateSchedulerProcessorHelperMethods()
        : MemoryAppenderUsingTests("updatescheduler")
    {}
};

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromJsonFile)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["featureA","featureB"])"));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> expectedFeatures = { "featureA", "featureB" };
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromJsonFileWithEmptyList)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return("[]"));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromEmptyJsonFile)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, noThrowWhenFeaturesAreReadFromMalformedJsonFile)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return("this is not json..."));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresDefaultToEmptyListWhenFileDoesNotExist)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, waitForTheFirstPolicyDoesNotReorderTasks)
{
    UpdateScheduler::SchedulerTaskQueue queueTask;
    auto notPolicy = UpdateScheduler::SchedulerTask::TaskType::UpdateNow;
    queueTask.push(UpdateScheduler::SchedulerTask{ notPolicy, "1" });
    queueTask.push(UpdateScheduler::SchedulerTask{ notPolicy, "2" });
    queueTask.push(UpdateScheduler::SchedulerTask{ UpdateScheduler::SchedulerTask::TaskType::Policy, "3", "APPID" });
    queueTask.push(UpdateScheduler::SchedulerTask{ notPolicy, "4" });
    queueTask.push(UpdateScheduler::SchedulerTask{ notPolicy, "5" });
    std::string policyContents = UpdateSchedulerImpl::UpdateSchedulerProcessor::waitForPolicy(queueTask, 5, "APPID");

    EXPECT_EQ("3", policyContents);

    UpdateScheduler::SchedulerTask task;
    EXPECT_TRUE(queueTask.pop(task, 1));
    EXPECT_EQ("1", task.content);
    EXPECT_TRUE(queueTask.pop(task, 1));
    EXPECT_EQ("2", task.content);
    EXPECT_TRUE(queueTask.pop(task, 1));
    EXPECT_EQ("4", task.content);
    EXPECT_TRUE(queueTask.pop(task, 1));
    EXPECT_EQ("5", task.content);
    EXPECT_FALSE(queueTask.pop(task, 1));
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, doesNotReprocessIfRecievesIdenticalPolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string policySnippet = R"sophos({
"livequery.network-tables.available" : true,
"endpoint.flag2.enabled" : false,
"endpoint.flag3.enabled" : false,
"av.onaccess.enabled" : true,
"safestore.enabled" : true
})sophos";

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillRepeatedly(Return(policySnippet));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::move(filesystemMock) };

    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockCronSchedulerThread = std::make_unique<MockCronSchedulerThread>();
    auto mockAsyncDownloaderRunner = std::make_unique<MockAsyncDownloaderRunner>();

    UpdateSchedulerImpl::UpdateSchedulerProcessor processor(taskQueue, std::move(mockBaseService), sharedPluginCallBack, std::move(mockCronSchedulerThread), std::move(mockAsyncDownloaderRunner));
    processor.processPolicy(policySnippet, "FLAGS");
    EXPECT_TRUE(appenderContains("DEBUG - Recieved first policy with app id FLAGS"));

    processor.processPolicy(policySnippet, "FLAGS");
    EXPECT_TRUE(appenderContains("DEBUG - Policy with app id FLAGS unchanged, will not be processed"));
}

// isSuldownloaderRunning tests

TEST_F(TestUpdateSchedulerProcessorHelperMethods, isSuldownloaderRunningReturnsTrueWhenLockFileIsLocked)
{
    auto lockPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();
    auto filesystemMock = new NaggyMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, exists(lockPath)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(-1)); // Cannot take lock
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));

    bool running = UpdateSchedulerImpl::isSuldownloaderRunning();
    EXPECT_TRUE(running);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, isSuldownloaderRunningReturnsFalseWhenLockFileLockCanBeTaken)
{
    auto lockPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();

    auto filesystemMock = new NaggyMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, exists(lockPath)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0)); // Took lock.
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_, _, _)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, unlink(_));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));

    bool running = UpdateSchedulerImpl::isSuldownloaderRunning();
    EXPECT_FALSE(running);
}
