// Copyright 2023 Sophos Limited. All rights reserved.

#include "MockAsyncDownloaderRunner.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/UniformIntDistribution.h"
#include "FileSystemImpl/PidLockFile.h"
#include "UpdateScheduler/SchedulerTaskQueue.h"
#include "UpdateSchedulerImpl/SchedulerPluginCallback.h"
#include "UpdateSchedulerImpl/UpdateSchedulerProcessor.h"
#include "UpdateSchedulerImpl/UpdateSchedulerTelemetryConsts.h"
#include "UpdateSchedulerImpl/cronModule/CronSchedulerThread.h"
#include "tests/Common/FileSystemImpl/MockPidLockFileUtils.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockApiBaseServices.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <json.hpp>
#include <utility>

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

ACTION_P(QueueStopTask, taskQueue)
{
    taskQueue->pushStop();
}

class TestUpdateSchedulerProcessor : public LogInitializedTests
{
public:
    void SetUp() override
    {
        taskQueue_ = std::make_shared<UpdateScheduler::SchedulerTaskQueue>();
        sharedPluginCallBack_ = std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(taskQueue_);
        cronThread_ = std::make_unique<UpdateSchedulerImpl::cronModule::CronSchedulerThread>(
            taskQueue_,
            std::chrono::seconds(distribution_.next()),
            std::chrono::minutes(60)
            );

        baseServiceMock_ = std::make_unique<StrictMock<MockApiBaseServices>>();
        EXPECT_NE(baseServiceMock_, nullptr);
        EXPECT_CALL(*baseServiceMock_, requestPolicies("ALC")).WillRepeatedly(Return());
        EXPECT_CALL(*baseServiceMock_, requestPolicies("FLAGS"));

        downloaderRunnerMock_ = std::make_unique<MockAsyncDownloaderRunner>();
        fileSystemMock_ = std::make_unique<StrictMock<MockFileSystem>>();

        pidLockFileUtilsMock_ = std::make_unique<NaggyMock<MockPidLockFileUtils>>();
        EXPECT_CALL(*pidLockFileUtilsMock_, open(_,_,_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*pidLockFileUtilsMock_, flock(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*pidLockFileUtilsMock_, ftruncate(_,_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*pidLockFileUtilsMock_, write(_,_,_)).WillRepeatedly(Return(1));
        EXPECT_CALL(*pidLockFileUtilsMock_, getpid()).WillRepeatedly(Return(2));
        Common::FileSystemImpl::replacePidLockUtils(std::move(pidLockFileUtilsMock_));
    }
    ~TestUpdateSchedulerProcessor() override
    {
        sharedPluginCallBack_->setRunning(false); // Needs to be set to false before UpdateSchedulerProcessor is deleted
        Common::FileSystemImpl::restorePidLockUtils();
    }

    void addRunnerExpectCalls() const
    {
        EXPECT_CALL(*downloaderRunnerMock_.get(), isRunning()).WillOnce(Return(false));
        EXPECT_CALL(*downloaderRunnerMock_.get(), triggerSulDownloader()).WillOnce(QueueStopTask(taskQueue_));
    }

    void addFileSystemExpectCalls(const std::string& updateConfigJsonString, bool supplementOnlyMarker=true) const
    {
        EXPECT_CALL(*fileSystemMock_, isFile(HasSubstr("mcs.config"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*fileSystemMock_, exists(HasSubstr("/opt/sophos-spl/base/update/var/updatescheduler/installed_features.json"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*fileSystemMock_, exists(HasSubstr("flags-mcs.json"))).WillRepeatedly(Return(true));
        EXPECT_CALL(*fileSystemMock_, isFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return(true));
        EXPECT_CALL(*fileSystemMock_, readFile(HasSubstr("machine_id.txt"))).WillRepeatedly(Return("id"));
        EXPECT_CALL(*fileSystemMock_, isFile(HasSubstr("update_config.json"))).WillOnce(Return(false)).WillRepeatedly(Return(true));
        EXPECT_CALL(*fileSystemMock_, readFile(HasSubstr("update_config.json"))).WillRepeatedly(Return(updateConfigJsonString));
        EXPECT_CALL(*fileSystemMock_, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*fileSystemMock_, isFile(HasSubstr("upgrade_marker_file"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*fileSystemMock_, isFile(HasSubstr("update_report.json"))).WillRepeatedly(Return(false));
        if (supplementOnlyMarker)
        {
            EXPECT_CALL(*fileSystemMock_, removeFile(HasSubstr("supplement_only.marker"),true));
        }
        EXPECT_CALL(*fileSystemMock_, listFiles(_)).WillRepeatedly(Return(std::vector<std::string>{}));
        EXPECT_CALL(*fileSystemMock_, writeFileAtomically(_,_,_)).WillRepeatedly(Return());
    }

    UpdateScheduler::SchedulerTask::TaskType policy_ = UpdateScheduler::SchedulerTask::TaskType::Policy;
    Common::UtilityImpl::UniformIntDistribution distribution_ = Common::UtilityImpl::UniformIntDistribution(300, 600);

    std::unique_ptr<MockApiBaseServices> baseServiceMock_;
    std::unique_ptr<MockAsyncDownloaderRunner> downloaderRunnerMock_;
    std::unique_ptr<MockFileSystem> fileSystemMock_;
    std::unique_ptr<MockPidLockFileUtils> pidLockFileUtilsMock_;

    std::unique_ptr<UpdateSchedulerImpl::cronModule::CronSchedulerThread> cronThread_;
    std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> taskQueue_;
    std::shared_ptr<UpdateSchedulerImpl::SchedulerPluginCallback> sharedPluginCallBack_;
};

TEST_F(TestUpdateSchedulerProcessor, teststartupAndShutdown)
{
    auto updateScheduler = UpdateSchedulerImpl::UpdateSchedulerProcessor(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_)
    );
    taskQueue_->pushStop();

    EXPECT_NO_THROW(updateScheduler.mainLoop());
}

TEST_F(TestUpdateSchedulerProcessor, NoUpdateTriggeredIfpolicyIsSameAsExistingConfig)
{
    addFileSystemExpectCalls(jsonString, false);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, updatePolicy ,"ALC"});
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, "{}" ,"FLAGS"});
    taskQueue_->pushStop();

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    scopedReplaceFileSystem.reset();
}

TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewFeature)
{
    addRunnerExpectCalls();
    addFileSystemExpectCalls(jsonString);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        "<Feature id=\"CORE\"/>\n",
        "<Feature id=\"CORE\"/>\n<Feature id=\"EXTRA\"/>"
        );

    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, ALCPolicy ,"ALC"});
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    scopedReplaceFileSystem.reset();
}

TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewFeatureEvenWhenPaused)
{
    addRunnerExpectCalls();
    std::string configJson = Common::UtilityImpl::StringUtils::replaceAll(
        jsonString,
        R"(<"fixedVersion" : "")",
        R"("fixedVersion" : "2022.1.0.40")"
        );
    addFileSystemExpectCalls(configJson);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        "<Feature id=\"CORE\"/>\n",
        "<Feature id=\"CORE\"/>\n<Feature id=\"EXTRA\"/>"
        );
    ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        ALCPolicy,
        R"(<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>)",
        R"(<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" FixedVersion="2022.1.0.40"/>)");
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, ALCPolicy ,"ALC"});
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    scopedReplaceFileSystem.reset();
}

TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewSubscriptionEvenWhenPaused)
{
    addRunnerExpectCalls();
    std::string configJson = Common::UtilityImpl::StringUtils::replaceAll(
        jsonString,
        R"(<"fixedVersion" : "")",
        R"("fixedVersion" : "2022.1.0.40")"
        );
    addFileSystemExpectCalls(configJson);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        R"(<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>)",
        R"(<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/><subscription Id="Base" RigidName="ServerProtectionLinux-MDR" Tag="RECOMMENDED" FixedVersion="2022.1.0.40"/>)"
        );
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, ALCPolicy ,"ALC"});
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    scopedReplaceFileSystem.reset();
}


TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewFeatureEvenWhenScheduled)
{
    addRunnerExpectCalls();
    addFileSystemExpectCalls(jsonString);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        "<Feature id=\"CORE\"/>\n",
        "<Feature id=\"CORE\"/>\n<Feature id=\"EXTRA\"/>"
        );
    ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        ALCPolicy,
        R"(<delay_supplements enabled="true"/>)",
        R"(<delay_updating Day="Wednesday" Time="22:05:00"/>)"
        );

    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, ALCPolicy ,"ALC"});
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    scopedReplaceFileSystem.reset();
}


TEST_F(TestUpdateSchedulerProcessor, UpdateTriggeredIfPolicyHasNewSubscriptionEvenWhenScheduled)
{
    addRunnerExpectCalls();
    addFileSystemExpectCalls(jsonString);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-Base\" Tag=\"RECOMMENDED\"/>",
        "<subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-Base\" Tag=\"RECOMMENDED\"/><subscription Id=\"Base\" RigidName=\"ServerProtectionLinux-MDR\" Tag=\"RECOMMENDED\"/>");
    ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        ALCPolicy,
        R"(<delay_supplements enabled="true"/>)",
        R"(<delay_updating Day="Wednesday" Time="22:05:00"/>)"
        );
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, ALCPolicy ,"ALC"});
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, "{}" ,"FLAGS"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());
    scopedReplaceFileSystem.reset();
}

TEST_F(TestUpdateSchedulerProcessor, TelemetryWithScheduledUpdatingEnabled)
{
    addRunnerExpectCalls();
    addFileSystemExpectCalls(jsonString);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    std::string ALCPolicy = Common::UtilityImpl::StringUtils::replaceAll(
        updatePolicy,
        R"(<delay_supplements enabled="true"/>)",
        R"(<delay_updating Day="Wednesday" Time="17:00:00"/>)"
        );
    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, ALCPolicy ,"ALC"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());

    auto telemetry = nlohmann::json::parse(Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset());
    EXPECT_EQ(telemetry[UpdateSchedulerImpl::Telemetry::scheduledUpdatingEnabled], true);
    EXPECT_EQ(telemetry[UpdateSchedulerImpl::Telemetry::scheduledUpdatingDay], "Wednesday");
    EXPECT_EQ(telemetry[UpdateSchedulerImpl::Telemetry::scheduledUpdatingTime], "17:00");

    scopedReplaceFileSystem.reset();
}

TEST_F(TestUpdateSchedulerProcessor, TelemetryWithoutScheduledUpdatingEnabled)
{
    addRunnerExpectCalls();
    addFileSystemExpectCalls(jsonString);
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(fileSystemMock_));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        taskQueue_,
        std::move(baseServiceMock_),
        sharedPluginCallBack_,
        std::move(cronThread_),
        std::move(downloaderRunnerMock_));

    taskQueue_->push(UpdateScheduler::SchedulerTask{ policy_, updatePolicy ,"ALC"});

    EXPECT_NO_THROW(updateScheduler.mainLoop());

    auto telemetry = nlohmann::json::parse(Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset());
    EXPECT_EQ(telemetry[UpdateSchedulerImpl::Telemetry::scheduledUpdatingEnabled], false);
    EXPECT_EQ(telemetry[UpdateSchedulerImpl::Telemetry::scheduledUpdatingDay], nullptr);
    EXPECT_EQ(telemetry[UpdateSchedulerImpl::Telemetry::scheduledUpdatingTime], nullptr);

    scopedReplaceFileSystem.reset();
}