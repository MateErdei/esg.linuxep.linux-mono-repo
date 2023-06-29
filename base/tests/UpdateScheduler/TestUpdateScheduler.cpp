/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#define TEST_PUBLIC public

#include "DownloadReportTestBuilder.h"
#include "MockAsyncDownloaderRunner.h"
#include "MockCronSchedulerThread.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <UpdateSchedulerImpl/UpdateSchedulerProcessor.cpp>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <tests/Common/FileSystemImpl/MockPidLockFileUtils.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MemoryAppender.h>
#include <tests/Common/Helpers/MockApiBaseServices.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/UtilityImpl/MockFormattedTime.h>

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

    const std::string policySnippet = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="54m5ung" UserName="QA940267" UseSophos="true" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="2" ProxyUserPassword="CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=" ProxyUserName="TestUser" ProxyPortNumber="8080" ProxyAddress="uk-abn-wpan-1.green.sophos" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="40" DetectDialUp="false"/>
    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="CORE"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="9972e4cf-dba3-e4ab-19dc-77619acac988"/>
</AUConfigurations>
)sophos";

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_, _, _));
    EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(std::vector<std::string> {}));

    Tests::replaceFileSystem(std::move(filesystemMock));

    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockCronSchedulerThread = std::make_unique<MockCronSchedulerThread>();

    auto mockAsyncDownloaderRunner = std::make_unique<MockAsyncDownloaderRunner>();
    EXPECT_CALL(*mockAsyncDownloaderRunner, isRunning()).WillOnce(Return(true));

    UpdateSchedulerImpl::UpdateSchedulerProcessor processor(taskQueue, std::move(mockBaseService), sharedPluginCallBack, std::move(mockCronSchedulerThread), std::move(mockAsyncDownloaderRunner));
    processor.processPolicy(policySnippet, "ALC");
    EXPECT_TRUE(appenderContains("DEBUG - Recieved first policy with app id ALC"));

    EXPECT_TRUE(appenderContains("DEBUG - Return from waitForPolicy"));
    processor.processPolicy(policySnippet, "ALC");
    EXPECT_TRUE(appenderContains("DEBUG - Policy with app id ALC unchanged, will not be processed"));
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
