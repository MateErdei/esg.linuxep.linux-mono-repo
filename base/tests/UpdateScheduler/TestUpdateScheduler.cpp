/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadReportTestBuilder.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <UpdateSchedulerImpl/UpdateSchedulerProcessor.cpp>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <tests/Common/FileSystemImpl/MockPidLockFileUtils.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/UtilityImpl/MockFormattedTime.h>

using namespace Common::UtilityImpl;
using namespace ::testing;

class TestUpdateSchedulerProcessorHelperMethods : public LogOffInitializedTests{};

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
        exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["featureA","featureB"])"));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {"featureA", "featureB"};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromJsonFileWithEmptyList) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
                readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return("[]"));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromEmptyJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
                readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, noThrowWhenFeaturesAreReadFromMalformedJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
                readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return("this is not json..."));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresDefaultToEmptyListWhenFileDoesNotExist) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = UpdateSchedulerImpl::readInstalledFeatures();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, waitForTheFirstPolicyDoesNotReorderTasks) // NOLINT
{
    UpdateScheduler::SchedulerTaskQueue queueTask;
    auto notPolicy = UpdateScheduler::SchedulerTask::TaskType::UpdateNow;
    queueTask.push(UpdateScheduler::SchedulerTask{notPolicy,"1"});
    queueTask.push(UpdateScheduler::SchedulerTask{notPolicy,"2"});
    queueTask.push(UpdateScheduler::SchedulerTask{UpdateScheduler::SchedulerTask::TaskType::Policy,"3", "APPID"});
    queueTask.push(UpdateScheduler::SchedulerTask{notPolicy,"4"});
    queueTask.push(UpdateScheduler::SchedulerTask{notPolicy,"5"});
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


// isSuldownloaderRunning tests

TEST_F(TestUpdateSchedulerProcessorHelperMethods, isSuldownloaderRunningReturnsFalseWhenLockFileDoesNotExist) // NOLINT
{
    auto lockPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();
    auto filesystemMock = new NaggyMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, exists(lockPath)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    bool running = UpdateSchedulerImpl::isSuldownloaderRunning();
    EXPECT_FALSE(running);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, isSuldownloaderRunningReturnsTrueWhenLockFileIsLocked) // NOLINT
{
    auto lockPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();
    auto filesystemMock = new NaggyMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, exists(lockPath)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(-1)); // Cannot take lock
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils = std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));

    bool running = UpdateSchedulerImpl::isSuldownloaderRunning();
    EXPECT_TRUE(running);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, isSuldownloaderRunningReturnsFalseWhenLockFileLockCanBeTaken) // NOLINT
{
    auto lockPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath();

    auto filesystemMock = new NaggyMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, exists(lockPath)).WillRepeatedly(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0)); // Took lock.
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, unlink(_));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils = std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));

    bool running = UpdateSchedulerImpl::isSuldownloaderRunning();
    EXPECT_FALSE(running);
}
