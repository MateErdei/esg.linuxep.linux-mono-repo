// Copyright 2023 Sophos Limited. All rights reserved.

#include "UpdateScheduler/SchedulerTaskQueue.h"
#include "UpdateSchedulerImpl/SchedulerPluginCallback.h"
#include "UpdateSchedulerImpl/cronModule/CronSchedulerThread.h"
#include "MockAsyncDownloaderRunner.h"
#include "tests/Common/Helpers/MockApiBaseServices.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "UpdateSchedulerImpl/UpdateSchedulerProcessor.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "tests/Common/FileSystemImpl/MockPidLockFileUtils.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "Common/UtilityImpl/UniformIntDistribution.h"

using namespace Common::UtilityImpl;
using namespace ::testing;
static const std::string updatePolicy{ R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
    <AUConfig platform="Linux">
        <sophos_address address="http://es-web.sophos.com/update"/>
        <primary_location>
            <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="blank" UserName="blank" UseSophos="true" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
        </primary_location>
        <secondary_location>
            <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </secondary_location>
        <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="40" DetectDialUp="false"/>
        <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
        <bootstrap Location="" UsePrimaryServerAddress="true"/>
        <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
        <cloud_subscriptions>
            <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
        </cloud_subscriptions>
        <delay_supplements enabled="true"/>
    </AUConfig>
    <Features>
        <Feature id="CORE"/>
    </Features>
    <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
    <customer id="9972e4cf-dba3-e4ab-19dc-77619acac988"/>
</AUConfigurations>
)sophos" };

std::string jsonString = R"({
                               "sophosURLs": [
                               "https://sophosupdate.sophos.com/latest/warehouse"
                               ],
                               "updateCache": [
                               "https://cache.sophos.com/latest/warehouse"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },
                               "manifestNames" : ["manifest.dat"],
                               "optionalManifestNames" : ["flags_manifest.dat"],
                               "primarySubscription": {
                                "rigidName" : "ServerProtectionLinux-Base",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },
                                "features": ["CORE"],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";
class TestUpdateSchedulerProcessor : public LogInitializedTests
{
};

TEST_F(TestUpdateSchedulerProcessor, teststartupAndShutdown)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();


    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));


    taskQueue->pushStop();

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}

ACTION_P(QueueStopTask, taskQueue)
{
    taskQueue->pushStop();
}

TEST_F(TestUpdateSchedulerProcessor, NoUpdateTriggeredIfpolicyIsSameAsExistingConfig)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();

    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
    
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(jsonString));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));

    auto Policy = UpdateScheduler::SchedulerTask::TaskType::Policy;
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, updatePolicy ,"ALC"});
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, "{}" ,"FLAGS"});
    taskQueue->pushStop();

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}

TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewFeature)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();
    EXPECT_CALL(*mockRunner.get(), isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mockRunner.get(), triggerSulDownloader()).WillOnce(QueueStopTask(taskQueue));


    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
    
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(jsonString));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, removeFile(HasSubstr("supplement_only.marker"),true));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));

    auto Policy = UpdateScheduler::SchedulerTask::TaskType::Policy;
    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(updatePolicy,"<Feature id=\"CORE\"/>\n","<Feature id=\"CORE\"/>\n<Feature id=\"EXTRA\"/>");
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, ALCPolicy ,"ALC"});
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}

TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewFeatureEvenWhenPaused)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();
    EXPECT_CALL(*mockRunner.get(), isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mockRunner.get(), triggerSulDownloader()).WillOnce(QueueStopTask(taskQueue));


    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
    
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
    std::string configJson = Common::UtilityImpl::StringUtils::replaceAll(jsonString,
                                                                          "<\"fixedVersion\" : \"\"",
                                                                          "\"fixedVersion\" : \"2022.1.0.40\"");
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(configJson));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, removeFile(HasSubstr("supplement_only.marker"),true));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));

    auto Policy = UpdateScheduler::SchedulerTask::TaskType::Policy;
    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(updatePolicy,"<Feature id=\"CORE\"/>\n","<Feature id=\"CORE\"/>\n<Feature id=\"EXTRA\"/>");
    ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        ALCPolicy,
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-Base\" Tag=\"RECOMMENDED\"/>",
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-Base\" Tag=\"RECOMMENDED\" FixedVersion=\"2022.1.0.40\"/>");
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, ALCPolicy ,"ALC"});
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}


TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewSubscriptionEvenWhenPaused)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();
    EXPECT_CALL(*mockRunner.get(), isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mockRunner.get(), triggerSulDownloader()).WillOnce(QueueStopTask(taskQueue));


    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
    
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
    std::string configJson = Common::UtilityImpl::StringUtils::replaceAll(jsonString,
                                                                          "<\"fixedVersion\" : \"\"",
                                                                          "\"fixedVersion\" : \"2022.1.0.40\"");
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(configJson));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, removeFile(HasSubstr("supplement_only.marker"),true));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));

    auto Policy = UpdateScheduler::SchedulerTask::TaskType::Policy;
    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-Base\" Tag=\"RECOMMENDED\"/>",
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-MDR\" Tag=\"RECOMMENDED\" FixedVersion=\"2022.1.0.40\"/>");
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, ALCPolicy ,"ALC"});
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}


TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewFeatureEvenWhenScheduled)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();
    EXPECT_CALL(*mockRunner.get(), isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mockRunner.get(), triggerSulDownloader()).WillOnce(QueueStopTask(taskQueue));


    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
    
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(jsonString));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, removeFile(HasSubstr("supplement_only.marker"),true));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));

    auto Policy = UpdateScheduler::SchedulerTask::TaskType::Policy;
    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(updatePolicy,"<Feature id=\"CORE\"/>\n","<Feature id=\"CORE\"/>\n<Feature id=\"EXTRA\"/>");
    ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        ALCPolicy,
        "<delay_supplements enabled=\"true\"/>",
        "<delay_updating Day=\"Wednesday\" Time=\"22:05:00\"/>");


    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, ALCPolicy ,"ALC"});
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}


TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewSubscriptionEvenWhenScheduled)
{
    auto taskQueue = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
    auto sharedPluginCallBack = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue);
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS"));


    Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

    auto cronThread = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
        taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
    auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();

    auto mockRunner = std::make_unique<MockAsyncDownloaderRunner>();
    EXPECT_CALL(*mockRunner.get(), isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mockRunner.get(), triggerSulDownloader()).WillOnce(QueueStopTask(taskQueue));


    auto* mockPidLockFileUtilsPtr = new NaggyMock<MockPidLockFileUtils>();
    EXPECT_CALL(*mockPidLockFileUtilsPtr, open(_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, flock(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, ftruncate(_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, write(_,_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockPidLockFileUtilsPtr, getpid()).WillRepeatedly(Return(2));
    std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
        std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
    Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));


    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
    
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(jsonString));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, removeFile(HasSubstr("supplement_only.marker"),true));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, listFiles(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue,
        std::move(mockBaseService),
        sharedPluginCallBack,
        std::move(cronThread),
        std::move(mockRunner));

    auto Policy = UpdateScheduler::SchedulerTask::TaskType::Policy;
    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-Base\" Tag=\"RECOMMENDED\"/>",
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-MDR\" Tag=\"RECOMMENDED\"/>");
    ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        ALCPolicy,
        "<delay_supplements enabled=\"true\"/>",
        "<delay_updating Day=\"Wednesday\" Time=\"22:05:00\"/>");
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, ALCPolicy ,"ALC"});
    taskQueue->push(UpdateScheduler::SchedulerTask{ Policy, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    sharedPluginCallBack->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted

}